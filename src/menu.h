#ifndef MENU_H
#define MENU_H

void menu_init(void);
void menu_update(uint16_t delta);
void menu_reset(void);
void menu_render(void);
void menu_handle_input(uint8_t input, bool pressed);

#endif // MENU_H