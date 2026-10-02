// Copyright (c) 2026 Dale Wick
// SPDX-License-Identifier: MIT
// See LICENSE.md for the full license text.

#ifndef MAIN_H
#define MAIN_H

#include <stdint.h>
#include <stdbool.h>

enum GameState {
    STATE_MENU,
    STATE_GAME,
    STATE_EXIT
};

enum INPUT
{
    INPUT_UP,
    INPUT_DOWN,
    INPUT_LEFT,
    INPUT_RIGHT,
    INPUT_A,
    INPUT_B,
    INPUT_X,
    INPUT_Y,
    INPUT_START,
    INPUT_SELECT,
    INPUT_L,
    INPUT_R,
    MAX_INPUT
};

#define TILE_EMPTY 0x00
#define TILE_SKY 0x00
#define TILE_RED 0x01
#define TILE_PILLBOX 0x02 // Pilboxes are 2x2 and there are 7 designs
#define TILE_SM_BUSH1 0x10
#define TILE_SM_BUSH2 0x11
#define TILE_ASCII_SPACE 0x20
#define TILE_CLOUD_2x1 0x21
#define TILE_CLOUD_SM_2x1 0x23
#define TILE_CLOUD_LG_2x1 0x25
#define TILE_ROCK_MD_2x1 0x27 // 2x1
#define TILE_ROCK_SM_2x1 0x29 // 2x1
#define TILE_ROCK_LG_2x1 0x82 // 2x1
#define TILE_NUMBER 0x30    // ASCII 0 to 9
#define TILE_DUST_MD_2x1 0x3A
#define TILE_DUST_SM1 0x3C
#define TILE_DUST_SM2 0x3D
#define TILE_HOLE1_MD_2x1 0x3E
#define TILE_ALPHABET 0x41
#define TILE_GRASS_2x1 0x5B
#define TILE_SHELL_UP 0x5D
#define TILE_SHELL_UP_RIGHT 0x5E
#define TILE_SHELL_RIGHT 0x5F
#define TILE_BUSH_LG_2x2 0x60

#define TILE_TREE1_1x2 0x6C
#define TILE_TREE2_1x2 0x6D
#define TILE_DUST_LG1_2x2 0x6E

#define TILE_GND_ML_MR_LG 0x62 // Ground from top mid left to top mid right, large (2x2)
#define TILE_GND_VALLEY_TOP_2x1 0x64
#define TILE_GND_VALLEY_BOTTOM_MD_2x1 0x74
#define TILE_GND_VALLEY_BOTTOM_MD_SKY_2x1 0x84
#define TILE_GND_ACROSS_MD_2x1 0x82
#define TILE_GND_PATH_LG_2x2 0x6A

#define TILE_BUMP_2x1 0x80
#define TILE_HOLE_LG_2x2 0x86
#define TILE_EXPLOSION1_2x2 0x88
#define TILE_EXPLOSION2_2x2 0x8A
#define TILE_EXPLOSION3_2x2 0x8C
#define TILE_DUST_LG2_2x2 0x8E
#define TILE_HOLE2_MD_2x1 0x90


#define SPRITE_FLAG_NONE 0x00
#define SPRITE_FLAG_PRIORITY 0x02
#define SPRITE_FLAG_FLIP_Y 0x04
#define SPRITE_FLAG_FLIP_X 0x08

extern enum GameState current_state;

void set_game_state(enum GameState new_state);
/// Reset the off-screen sprites table to empty
void reset_sprite(void);
/// Render all sprites to the screen from the off-screen sprites table
void render_sprites(void);
/// Clear all sprites from the screen and off-screen sprites table
void clear_sprites(void);
/// Add a sprite to the screen with the given parameters
/// @param x The x-coordinate of the sprite
/// @param y The y-coordinate of the sprite
/// @param tile The tile index of the sprite
/// @param flags The flags for the sprite (e.g., priority, flip)
/// @return The index of the added sprite, or 255 if the sprite table is full
uint8_t add_sprite(uint16_t x, uint8_t y, uint8_t tile, uint16_t flags);
/// Show map on layer 0
void show_map(uint8_t *map,uint8_t width,uint8_t height);
/// Show map on layer 1
void show_map1(uint8_t *map,uint8_t width,uint8_t height);
/// @brief Show number on the screen at the specified tilemap coordinates
/// @param  number The number to display
/// @param  x The x-coordinate on the tilemap
/// @param  y The y-coordinate on the tilemap
void show_number(uint16_t number, uint8_t x, uint8_t y);
/// @brief Show a portion of the map on the screen at the specified tilemap coordinates
/// @param map The map data to display
/// @param width The width of the map in tiles
/// @param height The height of the map in tiles
/// @param x The x-coordinate on the tilemap
/// @param y The y-coordinate on the tilemap
void show_map_xy(uint8_t *map,uint8_t width,uint8_t height,uint8_t x,uint8_t y);
/// @brief Show a portion of the map on the screen at the specified tilemap coordinates on layer 1
/// @param map The map data to display
/// @param width The width of the map in tiles
/// @param height The height of the map in tiles
/// @param x The x-coordinate on the tilemap
/// @param y The y-coordinate on the tilemap
void show_map_xy1(uint8_t *map,uint8_t width,uint8_t height,uint8_t x,uint8_t y);
/// @brief Log a message to the debug output
void debug_log(const char *message);
/// @brief  Log a formatted message to the debug output
/// @param format The format string (printf-style)
/// @param ... The values to format
void debug_logf(const char *format, ...);

#endif // MAIN_H
