// Copyright (c) 2026 Dale Wick
// SPDX-License-Identifier: MIT
// See LICENSE for the full license text.

#ifndef IMG_H
#define IMG_H

#include <stdint.h>

extern const uint16_t tileset_palette[64];
extern const uint16_t tileset_palette_len;
#define TILESET_PALETTE_BASE 0
extern const uint8_t *tileset_tiles;
#define TILESET_TILES_BASE 0
extern const uint16_t tileset_tiles_len;
extern const uint8_t *background_tilemap;
extern const uint8_t *foreground_tilemap;
extern const uint8_t *sprite_tilemap;

#endif // IMG_H