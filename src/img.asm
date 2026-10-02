; Copyright (c) 2026 Dale Wick
; SPDX-License-Identifier: MIT
; See LICENSE for the full license text.

; Export the symbols

  .module img
  .area _TEXT

	.globl _tileset_palette
    .globl _tileset_palette_len
	.globl _tileset_tiles
    .globl _tileset_tiles_len

_tileset_palette:
    .incbin "img/pillboxtiles.ztp"
_tileset_palette_len:
    .dw .-_tileset_palette
_tileset_tiles:
    .dw _tileset_tiles_data
_tileset_tiles_data:
    .incbin "img/pillboxtiles.zts"
_tileset_tiles_len:
    .dw .-_tileset_tiles_data

