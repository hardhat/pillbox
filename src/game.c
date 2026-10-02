#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#ifndef __SDCC_VERSION_MAJOR
#define __at(addr) (char *)(addr)
#define __naked
#define __sfr
#define va_list char * //struct {int dummy; }
#define va_start(ap, last)
#define va_end(ap)
#endif

#include<zvb_hardware.h>
#include<zvb_gfx.h>
#include<zvb_sprite.h>

#include "game.h"

#define SCREEN_WIDTH 40
#define SCREEN_HEIGHT 30
#define MAP_WIDTH 80        // In tiles
#define MAP_HEIGHT 30       // In tiles
#define TILE_SIZE_PIXELS 16
#define HALF_TILE_PIXELS (TILE_SIZE_PIXELS / 2)

uint16_t seed;
int16_t x_scroll;
int16_t x_scroll_target;
int16_t wind; // positive is wind blowing to the right, negative is wind blowing to the left, in km/h

struct Pillbox
{
    uint8_t x,y; // in tiles
    uint8_t angle,power; // Angle in degrees, power in percent (max 100)
} pillbox[2];

// 1 pixel = 100 mm and the game runs at 60 frames per second.
// Sub-pixel values are Q16.16 fixed point (1/65536 pixel units).
struct Rocket
{
    uint8_t angle,power; // Angle in degrees, power in percent (max 100)
    int16_t x,y; // Whole pixel position
    uint16_t fx,fy; // Fractional pixel position, in 1/65536 pixel
    int32_t vx,vy; // Velocity in 1/65536 pixel per frame
    int32_t ax,ay; // Acceleration in 1/65536 pixel per frame squared
    bool active; // Whether the rocket is currently active (1) or not (0)
    uint8_t sprite_index;
} rocket;

#define ROCKET_SPEED_PER_PERCENT 4369 // 40 m/s at 100% power
#define GRAVITY_Q16 1786              // 9.81 m/s^2
#define WIND_KMH_Q16 3034             // 1 km/h
#define DRAG_SHIFT 10                 // Drag is (air velocity - rocket velocity) / 1024 per frame

// sin(degrees) in Q15 for 0..90 degrees
static const uint16_t sin_q15[91] = {
    0, 572, 1144, 1715, 2286, 2856, 3425, 3993, 4560, 5126,
    5690, 6252, 6813, 7371, 7927, 8481, 9032, 9580, 10126, 10668,
    11207, 11743, 12275, 12803, 13328, 13848, 14365, 14876, 15384, 15886,
    16384, 16877, 17364, 17847, 18324, 18795, 19261, 19720, 20174, 20622,
    21063, 21498, 21926, 22348, 22763, 23170, 23571, 23965, 24351, 24730,
    25102, 25466, 25822, 26170, 26510, 26842, 27166, 27482, 27789, 28088,
    28378, 28660, 28932, 29197, 29452, 29698, 29935, 30163, 30382, 30592,
    30792, 30983, 31164, 31336, 31499, 31651, 31795, 31928, 32052, 32166,
    32270, 32365, 32449, 32524, 32588, 32643, 32688, 32723, 32748, 32763,
    32768
};

uint16_t elevation_pixels[MAP_WIDTH + 1];

// Map 0 is the ground, clouds and white text on blue background
uint8_t map0[MAP_WIDTH*MAP_HEIGHT];
// Map 1 is the features like trees, rocks, bushes, and after explosions, holes
uint8_t map1[MAP_WIDTH*MAP_HEIGHT];

// These 1x2 tiles are used to connect a continious terrain seamlessly
// Each terrain is measured on the left and right in half tiles, so since the terrain is 1x2, 
// they are measured in half tiles on the left and right sides denoted: 
// L[0-4]R[0-4] where 0 is the top, 2 is the middle, and 4 is the bottom. (half-tile measurements)
enum TerrainType {
    TERRAIN_L1R1_E = 0, // horizontal terrain, sky at top for even columns
    TERRAIN_L1R1_O = 1, // horizontal terrain, sky at top for odd columns
    
    TERRAIN_L1R2 = 2,
    TERRAIN_L2R3 = 3,
    
    TERRAIN_L3R2 = 4,
    TERRAIN_L2R1 = 5,

    TERRAIN_L1R3 = 6,
    TERRAIN_L3R1 = 7,

    TERRAIN_L1R0 = 8,
    TERRAIN_L0R1 = 9,

    TERRAIN_L3R0 = 10,
    TERRAIN_L0R3 = 11,

    TERRAIN_L3R3_E = 12, // Pairs well with TERRAIN_L3R0, match with TERRAIN_L1R1_E_G
    TERRAIN_L3R3_O = 13, // Pairs well with TERRAIN_L0R3, match with TERRAIN_L1R1_O_G

    // Ground variants for versions with no sky, expecting ground above
    TERRAIN_L1R1_E_G = 14,  // horizontal terrain, ground at top for even columns
    TERRAIN_L1R1_O_G = 15,   // horizontal terrain, ground at top for odd columns

    TERRAIN_L1R2_G = 16, // horizontal terrain, ground at top
    TERRAIN_L2R3_G = 17,
    TERRAIN_L3R2_G = 18,
    TERRAIN_L2R1_G = 19,
    TERRAIN_L1R3_G = 20,
    TERRAIN_L3R1_G = 21,
    TERRAIN_L1R0_G = 22,
    TERRAIN_L0R1_G = 23,
    TERRAIN_L3R0_G = 24,
    TERRAIN_L0R3_G = 25,
    
    TERRAIN_TYPE_COUNT
};

const struct Terrain1x2 {
    uint8_t tiles[2]; // 2x2 terrain tile pattern
} terrain1x2[TERRAIN_TYPE_COUNT] = {
    { //TERRAIN_L1R1_E = 0
        {TILE_GND_PATH_LG_2x2, TILE_GND_PATH_LG_2x2 + 0x10}
    },
    { //TERRAIN_L1R1_O = 1
        {TILE_GND_PATH_LG_2x2 + 0x1, TILE_GND_PATH_LG_2x2 + 0x11}
    },
    { //TERRAIN_L1R2 = 2
        {0x66, 0x76}
    },
    { //TERRAIN_L2R3 = 3
        {TILE_EMPTY, 0x67}
    },
    { //TERRAIN_L3R2 = 4
        {TILE_EMPTY, 0x68}
    },
    { //TERRAIN_L2R1 = 5
        {0x69, 0x79}
    },
    { //TERRAIN_L1R3 = 6
        {0x64, 0x84}
    },
    { //TERRAIN_L3R1 = 7
        {0x65, 0x85}
    },
    { //TERRAIN_L1R0 = 8
        {0x62, 0x72}
    },
    { //TERRAIN_L0R1 = 9
        {0x63, 0x73}
    },
    { // TERRAIN_L3R0 = 10
        {0x0D, 0x1D}
    },
    { //TERRAIN_L0R3 = 11
        {0x0E, 0x1E}
    },
    { //TERRAIN_L3R3_E = 12 pairs well with TERRAIN_L3R0
        {0x0C, 0x1C}
    },
    { //TERRAIN_L3R3_O = 13
        {0x0F, 0x1F}
    },

    { //TERRAIN_L1R1_E_G = 14
        {0x82, 0x7A},
    },
    { // TERRAIN_L1R1_O_G = 15
        {0x83, 0x7B}
    },
    { //TERRAIN_L1R2_G = 16
        {0x76, 0x82},
    },
    { //TERRAIN_L2R3_G = 17
        {0x77, 0x83}
    },
    { //TERRAIN_L3R2_G = 18
        {0x78, 0x82},
    },
    { //TERRAIN_L2R1_G = 19
        {0x79, 0x83},
    },
    { //TERRAIN_L1R3_G = 20
        {0x77, 0x82},
    },
    // TODO: check tile numbers from here to the bottom
    { //TERRAIN_L3R1_G = 21
        {0x78, 0x83},
    },
    { //TERRAIN_L1R0_G = 22
        {0x76, 0x82},
    },
    { //TERRAIN_L0R1_G = 23
        {0x79, 0x83},
    },
    { //TERRAIN_L3R0_G = 24
        {0x76, 0x82},
    },
    { //TERRAIN_L0R3_G = 25
        {0x79, 0x83},
    }

};

static const int8_t terrain_left_half[TERRAIN_L1R1_E_G] = {
    1, 1, 1, 2, 3, 2, 1, 3, 1, 0, 3, 0, 3, 3
};
static const int8_t terrain_right_half[TERRAIN_L1R1_E_G] = {
    1, 1, 2, 3, 2, 1, 3, 1, 0, 1, 0, 3, 3, 3
};

static int find_surface_type(uint16_t left_pixels, uint16_t right_pixels,
                             int column, int *top_row)
{
    int left_half = left_pixels / HALF_TILE_PIXELS;
    int right_half = right_pixels / HALF_TILE_PIXELS;
    int selected_type = -1;
    int selected_top = 0;
    int matches = 0;

    for(int type = 0; type < TERRAIN_L1R1_E_G; type++)
    {
        if((type == TERRAIN_L1R1_E || type == TERRAIN_L1R1_O ||
            type == TERRAIN_L3R3_E || type == TERRAIN_L3R3_O) &&
           (type & 1) != (column & 1))
            continue;

        int left_offset = left_half - terrain_left_half[type];
        int right_offset = right_half - terrain_right_half[type];
        if(left_offset == right_offset && left_offset >= 0 &&
           (left_offset & 1) == 0)
        {
            int candidate_top = left_offset / 2;
            if(candidate_top + 1 < MAP_HEIGHT)
            {
                matches++;
                if(rand() % matches == 0)
                {
                    selected_type = type;
                    selected_top = candidate_top;
                }
            }
        }
    }
    if(selected_type >= 0)
        *top_row = selected_top;
    return selected_type;
}

static void perturb_terrain(void)
{
    int last_perturbation = -3;

    for(int x = 2; x < MAP_WIDTH - 1; x++)
    {
        if(x - last_perturbation < 3 ||
           elevation_pixels[x - 1] != elevation_pixels[x] ||
           elevation_pixels[x] != elevation_pixels[x + 1] ||
           rand() % 4 != 0)
            continue;

        int perturbation = rand() % 5 == 0 ? -3 * HALF_TILE_PIXELS :
                          (rand() & 1 ? HALF_TILE_PIXELS : -HALF_TILE_PIXELS);
        elevation_pixels[x] += perturbation;
        last_perturbation = x;
    }
}

void game_init(void)
{
    show_map_xy("PRESS ENTER TO START", 21, 1, 10, 25);

    for(int i=0; i<TERRAIN_TYPE_COUNT; i++)
    {
        const uint8_t tiles[2] = {
            terrain1x2[i].tiles[0], terrain1x2[i].tiles[1]
        };
        show_map_xy(tiles, 1, 2, (SCREEN_WIDTH - TERRAIN_TYPE_COUNT) / 2 + i, 1);
    }
}

void generate_terrain(void)
{
    memset(map0, TILE_SKY, MAP_WIDTH*MAP_HEIGHT);
    memset(map1, TILE_EMPTY, MAP_WIDTH*MAP_HEIGHT);

    enum LandmarkType {
        LANDMARK_VALLEY,
        LANDMARK_MOUNTAIN,
        LANDMARK_CLIFF
    };
    int landmarks[5] = {
        LANDMARK_VALLEY, LANDMARK_VALLEY,
        LANDMARK_MOUNTAIN, LANDMARK_MOUNTAIN, LANDMARK_CLIFF
    };
    int baseline_pixels = (MAP_HEIGHT / 2) * TILE_SIZE_PIXELS +
                          HALF_TILE_PIXELS;
    int current_baseline = baseline_pixels;
    int total_feature_width = 0;
    int gap_index = 0;
    int feature_x = 0;

    for(int i = 4; i > 0; i--)
    {
        int swap = rand() % (i + 1);
        int landmark = landmarks[i];
        landmarks[i] = landmarks[swap];
        landmarks[swap] = landmark;
    }

    for(int i = 0; i < 5; i++)
    {
        int height_pixels = landmarks[i] == LANDMARK_CLIFF ?
                            2 * TILE_SIZE_PIXELS : 5 * TILE_SIZE_PIXELS;
        int ramp_columns = height_pixels / (2 * HALF_TILE_PIXELS);
        total_feature_width += landmarks[i] == LANDMARK_CLIFF ?
                               ramp_columns : 2 * ramp_columns;
    }

    int gap = (MAP_WIDTH - total_feature_width) / 6;
    int extra_gaps = (MAP_WIDTH - total_feature_width) % 6;
    for(int x = 0; x <= MAP_WIDTH; x++)
        elevation_pixels[x] = baseline_pixels;

    for(int i = 0; i < 5; i++)
    {
        feature_x += gap + (gap_index < extra_gaps);
        gap_index++;

        int height_pixels = landmarks[i] == LANDMARK_CLIFF ?
                            2 * TILE_SIZE_PIXELS : 5 * TILE_SIZE_PIXELS;
        int ramp_columns = height_pixels / (2 * HALF_TILE_PIXELS);
        int direction = landmarks[i] == LANDMARK_MOUNTAIN ? -1 : 1;
        int is_cliff = landmarks[i] == LANDMARK_CLIFF;

        for(int step = 1; step <= ramp_columns; step++)
        {
            elevation_pixels[feature_x + step] = current_baseline +
                direction * step * 2 * HALF_TILE_PIXELS;
        }

        if(is_cliff)
        {
            current_baseline += height_pixels;
            for(int x = feature_x + ramp_columns; x <= MAP_WIDTH; x++)
                elevation_pixels[x] = current_baseline;
            feature_x += ramp_columns;
        }
        else
        {
            int peak_or_floor = current_baseline + direction * height_pixels;
            for(int step = 1; step <= ramp_columns; step++)
            {
                elevation_pixels[feature_x + ramp_columns + step] =
                    peak_or_floor - direction * step * 2 * HALF_TILE_PIXELS;
            }
            feature_x += 2 * ramp_columns;
        }

    }
    feature_x += gap + (gap_index < extra_gaps);

    perturb_terrain();

    for(int x = 0; x < MAP_WIDTH; x++)
    {
        int top_row;
        int surface_type = find_surface_type(elevation_pixels[x],
                                             elevation_pixels[x + 1],
                                             x,
                                             &top_row);
        if(surface_type < 0)
        {
            debug_logf("No terrain tile for x=%d, edges=%u,%u", x,
                       elevation_pixels[x], elevation_pixels[x + 1]);
            continue;
        }
        debug_logf("x=%d, edges_px=%u,%u, surface_type=%d", x,
                   elevation_pixels[x], elevation_pixels[x + 1], surface_type);
        map0[top_row * MAP_WIDTH + x] = terrain1x2[surface_type].tiles[0];
        map0[(top_row + 1) * MAP_WIDTH + x] =
            terrain1x2[surface_type].tiles[1];

        int ground_type = surface_type <= TERRAIN_L0R3 ?
            surface_type + (TERRAIN_L1R1_E_G - TERRAIN_L1R1_E) :
            (x & 1 ? TERRAIN_L1R1_O_G : TERRAIN_L1R1_E_G);
        for(int y = top_row + 2; y < MAP_HEIGHT; y += 2)
        {
            map0[y * MAP_WIDTH + x] = terrain1x2[ground_type].tiles[0];
            if(y + 1 < MAP_HEIGHT)
                map0[(y + 1) * MAP_WIDTH + x] =
                    terrain1x2[ground_type].tiles[1];
        }
    }

    const uint8_t cloud_tiles[3] = {
        TILE_CLOUD_2x1, TILE_CLOUD_SM_2x1, TILE_CLOUD_LG_2x1
    };
    for(int cloud = 0; cloud < 7; cloud++)
    {
        int x = (rand() % (MAP_WIDTH / 2)) * 2;
        int surface_pixels = elevation_pixels[x] < elevation_pixels[x + 1] ?
                     elevation_pixels[x] : elevation_pixels[x + 1];
        int surface_row = surface_pixels / TILE_SIZE_PIXELS;
        int max_y = surface_row - 3;
        if(max_y > 14)
            max_y = 14;
        if(max_y < 2)
            continue;

        int y = 2 + 2 * (rand() % 7);
        if(y > max_y)
            continue;

        int index = y * MAP_WIDTH + x;
        if(map0[index] != TILE_SKY || map0[index + 1] != TILE_SKY ||
           map0[index + MAP_WIDTH] != TILE_SKY ||
           map0[index + MAP_WIDTH + 1] != TILE_SKY)
            continue;

                uint8_t tile = cloud_tiles[rand() % 3];
                map0[index] = tile;
                map0[index + 1] = tile + 1;
    }

    // Next add trees above the terrain
    for(int x = 0; x < MAP_WIDTH; x++)
    {
        if(rand() % 20 == 0) // 5% chance of a tree
        {
            int surface_row = (elevation_pixels[x] + TILE_SIZE_PIXELS - 1) /
                              TILE_SIZE_PIXELS;
            int y = surface_row - 1;
            if(y > 0)
            {
                uint8_t tile = rand() % 2 == 0 ? TILE_TREE1_1x2 : TILE_TREE2_1x2;
                map1[y * MAP_WIDTH + x] = tile + 0x10;
                if(y > 1)
                    map1[(y - 1) * MAP_WIDTH + x] = tile;
            }
        }
    }

}

void place_pillboxes(void)
{
    // Place both player pillboxes randomly on the terrain and update pillbox[i]
    for(int i = 0; i < 2; i++)
    {
        int x = rand() % (MAP_WIDTH/4-1);
        if(i == 1) x += 3 * MAP_WIDTH / 4;
        int y = (elevation_pixels[x] + TILE_SIZE_PIXELS - 1) /
            TILE_SIZE_PIXELS;
        if(y > 0)
        {
            uint8_t tile = TILE_PILLBOX+(i*6);
            map1[y * MAP_WIDTH + x] = tile+0x10;
            map1[y * MAP_WIDTH + x + 1] = tile+0x11; // Place the second part of the 2x1 pillbox tile
            map1[(y - 1) * MAP_WIDTH + x] = tile;
            map1[(y - 1) * MAP_WIDTH + x + 1] = tile+1; // Place the second part of the 2x1 pillbox tile
            
            pillbox[i].x = x;
            pillbox[i].y = y;
        }
    }

}

void launch_rocket(uint8_t angle, uint8_t power)
{
    if(angle > 90)
        angle = 90;
    if(power > 100)
        power = 100;

    rocket.active = true;
    rocket.angle = angle;
    rocket.power = power;
    rocket.x = pillbox[0].x * TILE_SIZE_PIXELS + TILE_SIZE_PIXELS;
    rocket.y = (pillbox[0].y - 1) * TILE_SIZE_PIXELS;
    rocket.fx = 0;
    rocket.fy = 0;

    // power * sin is at most 100 * 32768 >> 5, so the speed scaling stays within 32 bits
    int32_t horizontal = ((int32_t)power * sin_q15[90 - angle]) >> 5;
    int32_t vertical = ((int32_t)power * sin_q15[angle]) >> 5;
    rocket.vx = (horizontal * ROCKET_SPEED_PER_PERCENT) >> 10;
    rocket.vy = -((vertical * ROCKET_SPEED_PER_PERCENT) >> 10); // Screen y grows downward
    rocket.ax = 0;
    rocket.ay = 0;
    rocket.sprite_index = 0;
}

static int16_t ground_pixels(int16_t x)
{
    int16_t column = x / TILE_SIZE_PIXELS;
    int16_t offset = x % TILE_SIZE_PIXELS;
    int16_t left = elevation_pixels[column];
    int16_t right = elevation_pixels[column + 1];

    return left + (right - left) * offset / TILE_SIZE_PIXELS;
}

void update_rocket(void)
{
    if(!rocket.active)
        return;

    // Drag acts on velocity relative to the moving air, so wind pushes the rocket
    int32_t wind_vx = (int32_t)wind * WIND_KMH_Q16;
    rocket.ax = (wind_vx - rocket.vx) >> DRAG_SHIFT;
    rocket.ay = GRAVITY_Q16 - (rocket.vy >> DRAG_SHIFT);

    rocket.vx += rocket.ax;
    rocket.vy += rocket.ay;

    int32_t sum = (int32_t)rocket.fx + rocket.vx;
    rocket.x += (int16_t)(sum >> 16);
    rocket.fx = (uint16_t)(sum & 0xFFFF);

    sum = (int32_t)rocket.fy + rocket.vy;
    rocket.y += (int16_t)(sum >> 16);
    rocket.fy = (uint16_t)(sum & 0xFFFF);

    if(rocket.x < 0 || rocket.x >= MAP_WIDTH * TILE_SIZE_PIXELS ||
       rocket.y >= MAP_HEIGHT * TILE_SIZE_PIXELS)
        rocket.active = false;
    else if(rocket.vy > 0 && rocket.y >= ground_pixels(rocket.x))
        rocket.active = false;
}

void sprite_print(int16_t x, int16_t y, const char *str)
{
    for(int i = 0; str[i] != '\0'; i++) {
        if(str[i] == ' ')
            continue;
        add_sprite(x + i * TILE_SIZE_PIXELS, y, str[i],0);
    }
}

void game_update(uint16_t delta)
{
    (void)delta;
    seed++;

    if( x_scroll_target != x_scroll)
    {
        int16_t diff = x_scroll_target - x_scroll;
        uint16_t abs_diff = diff > 0 ? diff : -diff;
        int16_t increment=6;
        if(abs_diff<increment)
            increment=abs_diff;
        if(x_scroll < x_scroll_target)
            x_scroll+=increment;
        else if(x_scroll > x_scroll_target)
            x_scroll-=increment;
    }

    char buffer[16];
    reset_sprite();
    sprintf(buffer, "WIND %c%3d", wind < 0 ? '-' : '+', wind < 0 ? -wind : wind);
    sprite_print(15*TILE_SIZE_PIXELS,0*TILE_SIZE_PIXELS, buffer);
    sprite_print(2*TILE_SIZE_PIXELS,3*TILE_SIZE_PIXELS, "POWER ");
    sprintf(buffer, "%3d", pillbox[0].power);
    sprite_print(8*TILE_SIZE_PIXELS,3*TILE_SIZE_PIXELS, buffer);
    sprite_print(2*TILE_SIZE_PIXELS,4*TILE_SIZE_PIXELS, "ANGLE ");
    sprintf(buffer, "%3d", pillbox[0].angle);
    sprite_print(8*TILE_SIZE_PIXELS,4*TILE_SIZE_PIXELS, buffer);

    update_rocket();
}

void game_reset(void)
{
    srand(seed);
    generate_terrain();
    place_pillboxes();
    pillbox[0].angle = 45;
    pillbox[0].power = 30;
    show_map(map0, MAP_WIDTH, MAP_HEIGHT);
    show_map1(map1, MAP_WIDTH, MAP_HEIGHT);
    rocket.active = false;
    rocket.x = 0;
    rocket.y = 0;
    rocket.fx = 0;
    rocket.fy = 0;
    rocket.vx = 0;
    rocket.vy = 0;
    rocket.ax = 0;
    rocket.ay = 0;
    rocket.sprite_index = 0;
    clear_sprites();
    wind = (rand() % 61) - 30; // Wind can be between -30 and +30 km/h
    x_scroll = 0;
    x_scroll_target = 0;
    zvb_ctrl_l0_scr_x_low = x_scroll & 0xFF;
    zvb_ctrl_l0_scr_x_high = (x_scroll >> 8) & 0xFF;
    zvb_ctrl_l1_scr_x_low = x_scroll & 0xFF;
    zvb_ctrl_l1_scr_x_high = (x_scroll >> 8) & 0xFF;
}

void game_render(void)
{
    zvb_ctrl_l0_scr_x_low = x_scroll & 0xFF;
    zvb_ctrl_l0_scr_x_high = (x_scroll >> 8) & 0xFF;
    zvb_ctrl_l1_scr_x_low = x_scroll & 0xFF;
    zvb_ctrl_l1_scr_x_high = (x_scroll >> 8) & 0xFF;

    if(rocket.active)
    {
        // Pick the sprite from the heading in 45 degree sectors; the art points up, up-right and right
        int32_t speed_x = rocket.vx < 0 ? -rocket.vx : rocket.vx;
        int32_t speed_y = rocket.vy < 0 ? -rocket.vy : rocket.vy;
        uint8_t tile = TILE_SHELL_UP_RIGHT;
        uint8_t flags = 0;
        if(speed_y * 5 < speed_x * 2)
            tile = TILE_SHELL_RIGHT;
        else if(speed_x * 5 < speed_y * 2)
            tile = TILE_SHELL_UP;
        if(tile != TILE_SHELL_UP && rocket.vx < 0)
            flags |= SPRITE_FLAG_FLIP_X;
        if(tile != TILE_SHELL_RIGHT && rocket.vy > 0)
            flags |= SPRITE_FLAG_FLIP_Y;


        // Render the rocket sprite at its current position
       // if(rocket.sprite_index == 0)
            rocket.sprite_index = add_sprite(rocket.x +TILE_SIZE_PIXELS - x_scroll, rocket.y + TILE_SIZE_PIXELS, tile, flags);
        //else
          //  update_sprite(rocket.sprite_index, rocket.x +TILE_SIZE_PIXELS - x_scroll, rocket.y + TILE_SIZE_PIXELS, tile, flags);
    }
    render_sprites();
}

void game_handle_input(uint8_t input, bool pressed)
{
    if(input == INPUT_START && pressed)
        game_reset();
    if(input == INPUT_L && pressed)
    {
        if(x_scroll_target >= 8)
            x_scroll_target-=8;
    }
    if(input == INPUT_R && pressed)
    {
        if(x_scroll_target <= (MAP_WIDTH-SCREEN_WIDTH)*TILE_SIZE_PIXELS-8)
            x_scroll_target+=8;
    }
    if(input == INPUT_UP && pressed)
    {
        if(pillbox[0].angle < 90)
            pillbox[0].angle+=5;
    } else if(input == INPUT_DOWN && pressed)
    {
        if(pillbox[0].angle > 0)
            pillbox[0].angle-=5;
    } else if(input == INPUT_LEFT && pressed)
    {
        if(pillbox[0].power > 0)
            pillbox[0].power-=5;
    } else if(input == INPUT_RIGHT && pressed)
    {
        if(pillbox[0].power < 100)
            pillbox[0].power+=5;
    } else if(input == INPUT_A && pressed)
    {
        if(!rocket.active)
            launch_rocket(pillbox[0].angle, pillbox[0].power);
    }
}
