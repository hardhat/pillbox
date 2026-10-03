#ifndef SOUND_H
#define SOUND_H

#include <zvb_sound.h>

#define SOUND_STARTUP 1
#define SOUND_LAUNCH 2
#define SOUND_EXPLODE 3
#define SOUND_INVALID 4
#define SOUND_GAME_OVER 5
#define SOUND_MOVE_CURSOR 6
#define SOUND_FANFARE 7

#define FREQ_C1 33
#define FREQ_CS1 35
#define FREQ_D1 37
#define FREQ_DS1 39
#define FREQ_E1 41
#define FREQ_F1 44
#define FREQ_FS1 46
#define FREQ_G1 49
#define FREQ_GS1 52
#define FREQ_A1 55
#define FREQ_AS1 58
#define FREQ_B1 62
#define FREQ_C2 65
#define FREQ_CS2 69
#define FREQ_D2 73
#define FREQ_DS2 78
#define FREQ_E2 82
#define FREQ_F2 87
#define FREQ_FS2 93
#define FREQ_G2 98
#define FREQ_GS2 104
#define FREQ_A2 110
#define FREQ_AS2 117
#define FREQ_B2 123
#define FREQ_C3 130
#define FREQ_CS3 138
#define FREQ_D3 146
#define FREQ_DS3 155
#define FREQ_E3 165
#define FREQ_F3 175
#define FREQ_FS3 185
#define FREQ_G3 196
#define FREQ_GS3 208
#define FREQ_A3 220
#define FREQ_AS3 233
#define FREQ_B3 247
#define FREQ_C4 261
#define FREQ_CS4 277
#define FREQ_D4 293
#define FREQ_DS4 311
#define FREQ_E4 329
#define FREQ_F4 349
#define FREQ_FS4 370
#define FREQ_G4 392
#define FREQ_GS4 415
#define FREQ_A4 440
#define FREQ_AS4 466
#define FREQ_B4 494
#define FREQ_C5 523
#define FREQ_CS5 554
#define FREQ_D5 587
#define FREQ_DS5 622
#define FREQ_E5 659
#define FREQ_F5 698
#define FREQ_FS5 740
#define FREQ_G5 784
#define FREQ_GS5 831
#define FREQ_A5 880
#define FREQ_AS5 932
#define FREQ_B5 988
#define FREQ_C6 1047
#define FREQ_CS6 1109
#define FREQ_D6 1175
#define FREQ_DS6 1245
#define FREQ_E6 1319
#define FREQ_F6 1397
#define FREQ_FS6 1480
#define FREQ_G6 1568
#define FREQ_GS6 1661
#define FREQ_A6 1760
#define FREQ_AS6 1865
#define FREQ_B6 1976

void sound_init(void);
void sound_play(uint8_t sound_id);
void sound_update(uint16_t delta_time);
void sound_stop(uint8_t sound_id);
void sound_term(void);

#endif // SOUND_H
