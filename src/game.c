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

    TERRAIN_L3R3_E = 12, // Pairs well with TERRAIN_L3R0
    TERRAIN_L3R3_O = 13, // Pairs well with TERRAIN_L0R3

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

void game_init(void)
{
    show_map_xy("PRESS SPACE TO START", 21, 1, 10, 25);

    for(int i=0; i<TERRAIN_TYPE_COUNT; i++)
    {
        const uint8_t tiles[2] = {
            terrain1x2[i].tiles[0], terrain1x2[i].tiles[1]
        };
        show_map_xy(tiles, 1, 2, (MAP_WIDTH - TERRAIN_TYPE_COUNT) / 2 + i, 1);
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

    for(int x = 0; x < MAP_WIDTH; x++)
    {
        int surface = elevation[x];
        int incoming_slope = x > 0 ? surface - elevation[x - 1] : 0;
        int outgoing_slope = x + 1 < MAP_WIDTH ?
                             elevation[x + 1] - surface : 0;
        enum TerrainType surface_type;

        if(outgoing_slope > 0)
            surface_type = TERRAIN_L1R3;
        else if(outgoing_slope < 0)
            surface_type = TERRAIN_L3R1;
        else
            surface_type = x & 1 ? TERRAIN_L1R1_O : TERRAIN_L1R1_E;
        debug_logf("x=%d, surface=%d, incoming_slope=%d, outgoing_slope=%d, surface_type=%d",
                   x, surface, incoming_slope, outgoing_slope, surface_type);

        // Label terrain for debug purposes
        map0[x + (surface-3) * MAP_WIDTH] = '0'+(x/10);
        map0[x + (surface-2) * MAP_WIDTH] = '0'+(x%10);
        map0[x + (surface-5) * MAP_WIDTH] = '0' + surface_type;

        int top_y = surface > 0 ? surface - 1 : 0;
        if(top_y + 1 >= MAP_HEIGHT)
            top_y = MAP_HEIGHT - 2;
        map0[top_y * MAP_WIDTH + x] = terrain1x2[surface_type].tiles[0];
        map0[(top_y + 1) * MAP_WIDTH + x] = terrain1x2[surface_type].tiles[1];

        enum TerrainType ground_type = surface_type +
                          (TERRAIN_L1R1_E_G - TERRAIN_L1R1_E);

        for(int y = top_y + 2; y < MAP_HEIGHT; y += 2)
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
