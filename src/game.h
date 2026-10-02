#ifndef GAME_H
#define GAME_H

#include "main.h"

void game_init(void);
void game_update(uint16_t delta);
void game_reset(void);
void game_render(void);
void game_handle_input(uint8_t input, bool pressed);

#endif // GAME_H