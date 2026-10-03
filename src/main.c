// Copyright (c) 2026 Dale Wick
// SPDX-License-Identifier: MIT
// See LICENSE.md for the full license text.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdbool.h>

#include<zos_sys.h>
#include<zos_time.h>
#include<zos_vfs.h>
#include<zos_keyboard.h>
#include<zos_video.h>
#include<zvb_hardware.h>
#include<zvb_gfx.h>
#include<zvb_sprite.h>
#include<zvb_sound.h>

#ifndef __SDCC_VERSION_MAJOR
#define __at(addr)
#define __naked
#define __sfr
#define va_list char * //struct {int dummy; }
#define va_start(ap, last)
#define va_end(ap)
#endif

#include "main.h"
#include "game.h"
#include "menu.h"
#include "img.h"
#include "sound.h"
#include "../img/title.h"

gfx_context ctx;
uint8_t sprite_count=0;
uint8_t last_sprite_count=0;
gfx_sprite sprites[128];

enum GameState current_state;
bool done;

zos_dev_t ser;

void debug_log(const char *message)
{
    size_t size=strlen(message);

    write(ser, message, &size);
    size=2;
    write(ser, "\r\n", &size);
}

void debug_logf(const char *format, ...)
{
    char buffer[256];
    va_list args;
    va_start(args, format);
    vsprintf(buffer, format, args);
    va_end(args);
    debug_log(buffer);
}

void initialize_graphics() {
    uint8_t buffer[256];
    
    ser = open("#SER0",O_WRONLY);
    if (ser < 0) {
        printf("Failed to open serial port\n");
	    exit(1);
    }
    debug_log("Initializing...");

    // Initialize the graphics context
    // Set to tiled 320x240 mode
    gfx_initialize(ZVB_CTRL_VID_MODE_GFX_640_8BIT, &ctx);

    char mapline[80];
    memset(mapline,0,sizeof(mapline));
    for(int layer=0;layer<2;layer++) {
        for(int y=0;y<80;y++)
        {
            gfx_tilemap_load(&ctx, mapline, sizeof(mapline), layer, 0, y);
        }
    }
    gfx_tileset_add_color_tile(&ctx,254,254);
    gfx_tileset_add_color_tile(&ctx,255,255);
    //for(int x=0;x<20;x++) gfx_tilemap_place(&ctx,254,0,x,7);
    //debug_log("Clear sprites");
    clear_sprites();
    //for(int x=0;x<5;x++) gfx_tilemap_place(&ctx,255,0,x,7);
    
    gfx_tileset_options options0 = {TILESET_COMP_LZ, TILESET_TILES_BASE*256, TILESET_PALETTE_BASE, 0};
    gfx_palette_load(&ctx, tileset_palette, tileset_palette_len, TILESET_PALETTE_BASE);
    const uint16_t solid_color[2]={(uint16_t)RGB888_TO_RGB565(0xF,0xF,0x00),(uint16_t)RGB888_TO_RGB565(0xE0,0xE0,0x00)}; // Yellow and dark yellow
    gfx_palette_load(&ctx,solid_color,4,254);
    //const uint16_t black[1]={RGB888_TO_RGB565(0x30,0x30,0x30)};
    //gfx_palette_load(&ctx,black,2,0);

    // Generate a 2x2 grid of 8x8 dots to blow up the 40x16 pixel title into a 20x8 tilemap
    const char circle[8] = {0x18, 0x3C, 0x7E, 0xFF, 0xFF, 0x7E, 0x3C, 0x18};
    memset(buffer, 0, sizeof(buffer));
    for(uint8_t k=0;k<16;k++) {
        for(uint8_t bit=0;bit<4;bit++) {
            uint8_t color = (k & (1 << bit)) ? 0xFF : 0xFE; // Use 0xFF for white, 0xFE for light gray
            uint8_t offset = ((bit & 1)?8:0) + ((bit & 2)?128:0);

            for(uint8_t i=0;i<8;i++) {
                uint8_t mask=128;
                for(uint8_t j=0;j<8;j++) {
                    buffer[offset+(i<<4)+j] = (circle[i] & mask) ? color : 0x00;
                    mask >>= 1;
                }
            }
        }
        gfx_tileset_options options1 = {TILESET_COMP_NONE, (192+k)*256, 0, 0};
        gfx_tileset_load(&ctx, buffer, 256, &options1);
    }

    // Generate title from title.zts
    memset(buffer, 0, sizeof(buffer));
    for(int y=0;y<16;y++) {
        for(int x=0;x<40;x++) {
            // Tile bits are one-hot per quadrant (bit0=TL,bit1=TR,bit2=BL,bit3=BR), not a plain index
            buffer[80+(y>>1)*20+(x>>1)] |= 192 | (header_data[y*40+x] ? (1 << ((x&1)+(y&1)*2)) : 0);
        }
    }
    show_map_xy(buffer,20,12,10,4);

    //for(int x=5;x<10;x++) gfx_tilemap_place(&ctx,255,0,x,7);
    //debug_log("Loading tiles");
    gfx_tileset_load(&ctx, tileset_tiles, tileset_tiles_len, &options0);

    //for(int x=10;x<15;x++) gfx_tilemap_place(&ctx,255,0,x,7);
    debug_log("Initalized.");
    //for(int x=15;x<20;x++) gfx_tilemap_place(&ctx,255,0,x,7);
}

void show_map(uint8_t *map,uint8_t width,uint8_t height)
{
    uint8_t layer=0;
    for(int y=0;y<height;y++)
    {
        gfx_tilemap_load(&ctx, &map[width*y], width, layer, 0, y);
    }
}
void show_map1(uint8_t *map,uint8_t width,uint8_t height)
{
    uint8_t layer=1;
    for(int y=0;y<height;y++)
    {
        gfx_tilemap_load(&ctx, &map[width*y], width, layer, 0, y);
    }
}

void show_map_xy(uint8_t *map,uint8_t width,uint8_t height,uint8_t x,uint8_t y)
{
    uint8_t layer=0;
    for(int j=0;j<height;j++)
    {
        gfx_tilemap_load(&ctx, &map[width*j], width, layer, x, y+j);
    }
}

void show_map_xy1(uint8_t *map,uint8_t width,uint8_t height,uint8_t x,uint8_t y)
{
    uint8_t layer=1;
    for(int j=0;j<height;j++)
    {
        gfx_tilemap_load(&ctx, &map[width*j], width, layer, x, y+j);
    }
}

void show_number(uint16_t number, uint8_t x, uint8_t y)
{
    char buffer[6];
    sprintf(buffer, "%u", number);
    for(int i=0;buffer[i];i++)
    {
        gfx_tilemap_place(&ctx, buffer[i], 0, x+i, y);
    }
}

void set_game_state(enum GameState new_state) {
    current_state = new_state;
    // Initialize the game and menu
    switch(current_state) {
        case STATE_GAME:
            game_init();
            break;
        case STATE_MENU:
            menu_init();
            break;
    }
}

void reset_sprite(void)
{
    last_sprite_count = sprite_count;
    sprite_count=0;
}

uint8_t add_sprite(uint16_t x, uint16_t y, uint8_t tile, uint16_t flags)
{
    if(sprite_count >= 128) return 255;
    sprites[sprite_count].x = x+16; // Note sprites are displayed anchored the bottom right corner
    sprites[sprite_count].y = y+16;
    sprites[sprite_count].tile = tile;
    sprites[sprite_count].flags = flags;
    sprite_count++;
    return sprite_count-1;
}

uint8_t update_sprite(uint8_t index, uint16_t x, uint16_t y, uint8_t tile, uint16_t flags)
{
    if(index >= 128) return 255;
    sprites[index].x = x+16; // Note sprites are displayed anchored the bottom right corner
    sprites[index].y = y+16;
    sprites[index].tile = tile;
    sprites[index].flags = flags;
    return index;
}

void render_sprites(void)
{
    uint8_t count = sprite_count;
    if(count < last_sprite_count) {
        // Clear only the stale tail left over from the previous, larger frame
        memset(&sprites[sprite_count], 0, (last_sprite_count - sprite_count) * sizeof(gfx_sprite));
        count = last_sprite_count;
    }
    gfx_sprite_render_array(&ctx, 0, sprites, count);
}

void clear_sprites(void)
{
    memset(sprites, 0, sizeof(sprites));
    sprite_count=128;
    render_sprites();
    sprite_count=0;
}

uint8_t handle_input(uint8_t key)
{
    switch(key)
    {
        case KB_ESC:
            done = true;
            return MAX_INPUT;
        case KB_KEY_W:
        case KB_UP_ARROW:
            return INPUT_UP;
        case KB_KEY_S:
        case KB_DOWN_ARROW:
            return INPUT_DOWN;
        case KB_KEY_A:
        case KB_LEFT_ARROW:
            return INPUT_LEFT;
        case KB_KEY_D:
        case KB_RIGHT_ARROW:
            return INPUT_RIGHT;
        case KB_KEY_V:
        case KB_KEY_SPACE:
            return INPUT_A;
        case KB_KEY_BACKSPACE:
        case KB_KEY_B:
            return INPUT_B;
        case KB_KEY_COMMA:
        case KB_KEY_X:
            return INPUT_X;
        case KB_KEY_PERIOD:
        case KB_KEY_Y:
            return INPUT_Y;
        case KB_KEY_ENTER:
            return INPUT_START;
        case KB_KEY_QUOTE:
        case KB_RIGHT_SHIFT:
            return INPUT_SELECT;
        case KB_KEY_LEFT_BRACKET:
        case KB_KEY_Q:
            return INPUT_L;
        case KB_KEY_RIGHT_BRACKET:
        case KB_KEY_E:
            return INPUT_R;
        default:
            return MAX_INPUT;
    }
}

void send_input(uint8_t input, bool down)
{
    switch(current_state) {
        case STATE_GAME:
            game_handle_input(input, down);
            break;
        case STATE_MENU:
            menu_handle_input(input, down);
            break;
    }
 }

void process_input(void)
{
    unsigned char keys[32];
    int size;
    bool pressed = true;

    do {
        size=32;
        read(DEV_STDIN, &keys, &size);
        for(int i=0;i<size;i++) {
            char key = keys[i];
           //debug_logf("Processing input key %02x.", key);
           if(key == KB_RELEASED) {
                pressed = false;
            } else {
                uint8_t input = handle_input(key);
                if(input >= MAX_INPUT) {
                    pressed = true;
                    continue;
                }
                send_input(input, pressed);
                pressed=true;
            }
        }
    } while(size>0);
#if 0
    // Scan for game controller
    uint16_t value = controller_read();
    uint16_t changed = value ^ controller_state;
    if(changed) debug_logf("Ctrl: %04x, %04x", value, changed);
    controller_state = value;
    // Edge triggered
    for(uint8_t i=0;i<12;i++) {
        if(changed & (1<<i)) {
            uint8_t input = map_controller[i];
            bool down = (value & (1<<i)) == 0;
            send_input(input, down);
        }
    }
#endif
}


int main(int argc, char *argv[]) {
    (void)argc;
    (void)argv;

    printf("Starting game...\n");
    initialize_graphics();
    
    set_game_state(STATE_GAME);

    /* Initialize the keyboard by setting it to raw and non-blocking */
    void* arg = (void*) (KB_READ_NON_BLOCK | KB_MODE_RAW);
    ioctl(DEV_STDIN, KB_CMD_SET_MODE, arg);

    sound_init();
    // Main loop
    while (!done) {
        // Handle input
        process_input();
        gfx_wait_end_vblank(&ctx);

        sound_update(16);
        // Update game and menu state
        switch(current_state) {
            case STATE_GAME:
                game_update(16);
                break;
            case STATE_MENU:
                menu_update(16);
                break;
        }

        gfx_wait_vblank(&ctx);
        // Render game and menu
        switch(current_state) {
            case STATE_GAME:
                game_render();
                break;
            case STATE_MENU:
                menu_render();
                break;
        }
    }

    debug_log("Quitting.");

    // Clear out sprites
    memset(sprites, 0, sizeof(sprites));
    render_sprites();
    zvb_sound_reset();
    ioctl(DEV_STDOUT, CMD_RESET_SCREEN, NULL);
    printf("Exiting...\n");

    printf("Goodbye!\n");
    close(ser);

    exit(0);

    return 0;
}