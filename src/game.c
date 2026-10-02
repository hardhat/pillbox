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

void game_init(void)
{
    show_map_xy("PRESS SPACE TO START", 21, 1, 10, 25);
}

void generate_terrain(void)
{
    memset(map0, TILE_SKY, MAP_WIDTH*MAP_HEIGHT);
    memset(map1, TILE_EMPTY, MAP_WIDTH*MAP_HEIGHT);

    int anchor_x[5] = {0, MAP_WIDTH / 5, MAP_WIDTH / 2,
                        (MAP_WIDTH * 4) / 5, MAP_WIDTH - 1};
    int anchor_y[5];
    anchor_y[0] = MAP_HEIGHT * 2 / 3 + rand() % 5 - 2;
    anchor_y[1] = anchor_y[0] + rand() % 5 - 2;
    anchor_y[2] = MAP_HEIGHT / 2 + rand() % (MAP_HEIGHT / 3 + 1);
    anchor_y[4] = MAP_HEIGHT * 2 / 3 + rand() % 5 - 2;
    anchor_y[3] = anchor_y[4] + rand() % 5 - 2;

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
        for(int y = elevation[x]; y < MAP_HEIGHT; y++)
        {
            map0[y * MAP_WIDTH + x] = TILE_GND_ACROSS_MD_2x1 + (x & 1);
        }

        if(x < MAP_WIDTH - 1)
        {
            int slope = elevation[x + 1] - elevation[x];
            if(slope > 0)
                map0[elevation[x] * MAP_WIDTH + x] = TILE_GND_TL_MR_SM_SKY;
            else if(slope < 0)
                map0[elevation[x] * MAP_WIDTH + x] = TILE_GND_ML_TR_SM_SKY;
        }
    }

    const uint8_t cloud_tiles[3] = {
        TILE_CLOUD_2x1, TILE_CLOUD_SM_2x1, TILE_CLOUD_LG_2x1
    };
    for(int cloud = 0; cloud < 7; cloud++)
    {
        int x = rand() % (MAP_WIDTH - 1);
        int surface = elevation[x] < elevation[x + 1] ?
                      elevation[x] : elevation[x + 1];
        int max_y = surface - 3;
        if(max_y > 14)
            max_y = 14;
        if(max_y < 2)
            continue;

        int y = 2 + rand() % (max_y - 1);
        int index = y * MAP_WIDTH + x;
        if(map0[index] == TILE_SKY && map0[index + 1] == TILE_SKY)
        {
            uint8_t tile = cloud_tiles[rand() % 3];
            map0[index] = tile;
            map0[index + 1] = tile + 1;
        }
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
