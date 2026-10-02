#include <stdlib.h>
#include <string.h>

#include<zvb_hardware.h>
#include<zvb_gfx.h>
#include<zvb_sprite.h>

#include "game.h"

#define MAP_WIDTH 40        // In tiles
#define MAP_HEIGHT 30       // In tiles

uint16_t seed;

struct Pillbox
{
    uint8_t x,y; // in tiles
} pillbox[2];

uint8_t elevation[MAP_WIDTH];

// Map 0 is the ground, clouds and white text on blue background
uint8_t map0[MAP_WIDTH*MAP_HEIGHT];
// Map 1 is the features like trees, rocks, bushes, and after explosions, holes
uint8_t map1[MAP_WIDTH*MAP_HEIGHT];

// These 2x2 tiles are used to connect a continious terrain seamlessly
enum TerrainType {
    TERRAIN_ACROSS, // horizontal terrain, sky at top
    TERRAIN_LEFT_HIGH_RIGHT_LOW,    // middle of the left top tile to middle of right bottom tile, sky at top
    TERRAIN_LEFT_LOW_RIGHT_HIGH,    // middle of the left bottom tile to middle of right top tile, sky at top
    TERRAIN_VALLEY, // High left and right, but low in the middle, sky at top
    TERRAIN_PEAK, // Low left and right, but high in the middle, sky at top
    TERRAIN_ACROSS_GROUND,  // horizontal terrain, ground at top
    TERRAIN_LEFT_LOW_RIGHT_HIGH_GROUND, // low left and high right, with terrain above
    TERRAIN_LEFT_HIGH_RIGHT_LOW_GROUND, // high left and low right, with terrain above
    TERRAIN_TYPE_COUNT
};

const struct Terrain2x2 {
    uint8_t tiles[2][2]; // 2x2 terrain tile pattern
} terrain2x2[TERRAIN_TYPE_COUNT] = {
    { //TERRAIN_ACROSS
        {{TILE_GND_PATH_LG_2x2, TILE_GND_PATH_LG_2x2 + 1},
        {TILE_GND_PATH_LG_2x2 + 0x10, TILE_GND_PATH_LG_2x2 + 0x11}}
    },
    { //TERRAIN_LEFT_LOW_RIGHT_HIGH
        {{0x66, TILE_EMPTY},
        {0x76, 0x67}}
    },
    { //TERRAIN_LEFT_HIGH_RIGHT_LOW
        {{TILE_EMPTY, 0x69},
        {0x68, 0x79}}
    },
    { //TERRAIN_VALLEY
        {{TILE_GND_VALLEY_TOP_2x1, TILE_GND_VALLEY_TOP_2x1 + 1},
        {TILE_GND_VALLEY_BOTTOM_MD_SKY_2x1, TILE_GND_VALLEY_BOTTOM_MD_SKY_2x1 + 1}}
    },
    { //TERRAIN_PEAK
        {{TILE_GND_ML_MR_LG, TILE_GND_ML_MR_LG + 1},
        {TILE_GND_ML_MR_LG + 0x10, TILE_GND_ML_MR_LG + 0x11}}
    },
    { //TERRAIN_ACROSS_GROUND
        {{0x82, 0x83},
        {0x7A, 0x7B}}
    },
    { //TERRAIN_LEFT_LOW_RIGHT_HIGH_GROUND
        {{0x76, 0x77},
        {0x82, 0x83}}
    },
    { //TERRAIN_LEFT_HIGH_RIGHT_LOW_GROUND
        {{0x78, 0x79},
        {0x82, 0x83}}
    }
};

void game_init(void)
{
    show_map_xy("PRESS SPACE TO START", 21, 1, 10, 25);

    for(int i=0; i<TERRAIN_TYPE_COUNT; i++)
    {
        const uint8_t tiles[4] = {
            terrain2x2[i].tiles[0][0], terrain2x2[i].tiles[0][1], 
            terrain2x2[i].tiles[1][0], terrain2x2[i].tiles[1][1]
        };
        // Show the terrain type for debugging or visualization purposes
        show_map_xy(tiles, 2, 2, 20-(TERRAIN_TYPE_COUNT*3)/2+3*i, 1);
    }
}

void generate_terrain(void)
{
    memset(map0, TILE_SKY, MAP_WIDTH*MAP_HEIGHT);
    memset(map1, TILE_EMPTY, MAP_WIDTH*MAP_HEIGHT);

    int anchor_x[5] = {0, MAP_WIDTH / 5, MAP_WIDTH / 2,
                        (MAP_WIDTH * 4) / 5, MAP_WIDTH - 1};
    int anchor_y[5];
    anchor_y[0] = MAP_HEIGHT * 2 / 3 + rand() % 5 - 2;
    anchor_y[2] = MAP_HEIGHT / 2 + rand() % (MAP_HEIGHT / 3 + 1);
    anchor_y[4] = MAP_HEIGHT * 2 / 3 + rand() % 5 - 2;
    anchor_y[1] = anchor_y[0] +
                  (anchor_y[2] - anchor_y[0]) * anchor_x[1] / anchor_x[2];
    anchor_y[3] = anchor_y[2] +
                  (anchor_y[4] - anchor_y[2]) *
                  (anchor_x[3] - anchor_x[2]) /
                  (anchor_x[4] - anchor_x[2]);

    for(int segment = 0; segment < 4; segment++)
    {
        int start_x = anchor_x[segment];
        int end_x = anchor_x[segment + 1];
        int start_y = anchor_y[segment];
        int end_y = anchor_y[segment + 1];
        for(int x = start_x; x <= end_x; x++)
        {
            elevation[x] = start_y + (end_y - start_y) * (x - start_x) /
                           (end_x - start_x);
        }
    }

    for(int x = 0; x < MAP_WIDTH; x += 2)
    {
        int left_surface = elevation[x];
        int right_surface = elevation[x + 1];
        int min_surface = left_surface < right_surface ?
                          left_surface : right_surface;
        int max_surface = left_surface > right_surface ?
                          left_surface : right_surface;
        int left_slope = x >= 1 ? left_surface - elevation[x - 1] : 0;
        int right_slope = x + 2 < MAP_WIDTH ?
                  elevation[x + 2] - right_surface : 0;
        enum TerrainType surface_type;

        map0[x + (min_surface-3) * MAP_WIDTH] = '0'+(x/10);
        map0[x + (min_surface-2) * MAP_WIDTH] = '0'+(x%10);

        if(left_slope > 0 && right_slope < 0)
            surface_type = TERRAIN_VALLEY;
        else if(left_slope < 0 && right_slope > 0)
            surface_type = TERRAIN_PEAK;
        else if(left_surface > right_surface)
            surface_type = TERRAIN_LEFT_LOW_RIGHT_HIGH;
        else if(left_surface < right_surface)
            surface_type = TERRAIN_LEFT_HIGH_RIGHT_LOW;
        else if(right_slope > 0) // || left_slope < 0)
            surface_type = TERRAIN_LEFT_LOW_RIGHT_HIGH;
        else if(right_slope < 0) // || left_slope > 0)
            surface_type = TERRAIN_LEFT_HIGH_RIGHT_LOW;
        else
            surface_type = TERRAIN_ACROSS;
        debug_logf("x=%d, left_surface=%d, right_surface=%d, min_surface=%d, max_surface=%d, left_slope=%d, right_slope=%d, surface_type=%d",
                   x, left_surface, right_surface, min_surface, max_surface, left_slope, right_slope, surface_type);

        for(int y = 0; y < MAP_HEIGHT; y += 2)
        {
            if(y + 1 < min_surface)
                continue;

            enum TerrainType terrain_type = surface_type;
            int sloped_surface = surface_type == TERRAIN_LEFT_LOW_RIGHT_HIGH ||
                                 surface_type == TERRAIN_LEFT_HIGH_RIGHT_LOW;
            if(y > max_surface || (sloped_surface && y == max_surface))
            {
                switch(surface_type)
                {
                    case TERRAIN_LEFT_LOW_RIGHT_HIGH:
                        terrain_type = TERRAIN_LEFT_LOW_RIGHT_HIGH_GROUND;
                        break;
                    case TERRAIN_LEFT_HIGH_RIGHT_LOW:
                        terrain_type = TERRAIN_LEFT_HIGH_RIGHT_LOW_GROUND;
                        break;
                    default:
                        terrain_type = TERRAIN_ACROSS_GROUND;
                        break;
                }
            }

            for(int row = 0; row < 2; row++)
            {
                for(int column = 0; column < 2; column++)
                {
                    map0[(y + row) * MAP_WIDTH + x + column] =
                        terrain2x2[terrain_type].tiles[row][column];
                }
            }
        }
    }

    const uint8_t cloud_tiles[3] = {
        TILE_CLOUD_2x1, TILE_CLOUD_SM_2x1, TILE_CLOUD_LG_2x1
    };
    for(int cloud = 0; cloud < 7; cloud++)
    {
        int x = (rand() % (MAP_WIDTH / 2)) * 2;
        int surface = elevation[x] < elevation[x + 1] ?
                      elevation[x] : elevation[x + 1];
        int max_y = surface - 3;
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
            int y = elevation[x] - 1;
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
        int y = elevation[x];
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

void game_update(uint16_t delta)
{
    (void)delta;
    seed++;    

}

void game_reset(void)
{
    srand(seed);
    generate_terrain();
    place_pillboxes();
    show_map(map0, MAP_WIDTH, MAP_HEIGHT);
    show_map1(map1, MAP_WIDTH, MAP_HEIGHT);
}

void game_render(void)
{

}

void game_handle_input(uint8_t input, bool pressed)
{
    if(input == INPUT_A && pressed)
        game_reset();
}
