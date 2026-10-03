#include "sound.h"

#include <stdbool.h>
#include <stddef.h>

typedef struct Note {
    uint16_t divider;
    uint16_t duration;
    sound_volume_t volume;
} Note;

typedef struct Song {
    sound_waveform_t waveform;
    const Note* notes;
    uint8_t length;
} Song;

#define NOTE(frequency, duration, volume) \
    {SOUND_FREQ_TO_DIV(frequency), duration, volume}
#define REST(duration) {0, duration, VOL_0}
#define FADE_NOTE(frequency) \
    NOTE(frequency, 64, VOL_100), \
    NOTE(frequency, 64, VOL_75), \
    NOTE(frequency, 64, VOL_50), \
    NOTE(frequency, 64, VOL_25)
#define EIGHTH_NOTE(frequency) FADE_NOTE(frequency), REST(32)
#define QUARTER_NOTE(frequency) \
    NOTE(frequency, 128, VOL_100), \
    NOTE(frequency, 128, VOL_75), \
    NOTE(frequency, 128, VOL_50), \
    NOTE(frequency, 160, VOL_25), \
    REST(32)
#define NOTE_COUNT(notes) (sizeof(notes) / sizeof((notes)[0]))

static const Note startup_notes[] = {
    // Melody, measures 6-9 in 4/4: eighth = 288 ms, quarter = 576 ms.
    REST(576),
    QUARTER_NOTE(FREQ_E5),
    EIGHTH_NOTE(FREQ_D5),
    EIGHTH_NOTE(FREQ_C5),
    EIGHTH_NOTE(FREQ_C5),
    EIGHTH_NOTE(FREQ_G5),

    REST(576),
    QUARTER_NOTE(FREQ_E5),
    EIGHTH_NOTE(FREQ_D5),
    EIGHTH_NOTE(FREQ_C5),
    EIGHTH_NOTE(FREQ_C5),
    EIGHTH_NOTE(FREQ_A5),

    REST(576),
    QUARTER_NOTE(FREQ_E5),
    EIGHTH_NOTE(FREQ_D5),
    EIGHTH_NOTE(FREQ_C5),
    EIGHTH_NOTE(FREQ_C5),
    EIGHTH_NOTE(FREQ_F5),

    REST(576),
    QUARTER_NOTE(FREQ_F5),
    EIGHTH_NOTE(FREQ_F5),
    QUARTER_NOTE(FREQ_F5),
    QUARTER_NOTE(FREQ_F5),
    REST(864),
};

static const Note launch_notes[] = {
    NOTE(FREQ_D2, 64, VOL_75),
    NOTE(FREQ_C2, 128, VOL_100),
};

static const Note explode_notes[] = {
    NOTE(FREQ_A2, 128, VOL_100),
    NOTE(FREQ_A2, 128, VOL_75),
    NOTE(FREQ_A2, 128, VOL_50),
    NOTE(FREQ_A2, 128, VOL_25),
};

static const Note fanfare_notes[] = {
    // Two rising brass calls followed by a high answering flourish (1424 ms).
    NOTE(FREQ_G4, 80, VOL_75),
    REST(16),
    NOTE(FREQ_C5, 80, VOL_100),
    REST(16),
    NOTE(FREQ_E5, 80, VOL_100),
    REST(16),
    NOTE(FREQ_G5, 144, VOL_100),
    REST(32),

    NOTE(FREQ_C5, 80, VOL_75),
    REST(16),
    NOTE(FREQ_E5, 80, VOL_100),
    REST(16),
    NOTE(FREQ_G5, 80, VOL_100),
    REST(16),
    NOTE(FREQ_C6, 144, VOL_100),
    REST(32),

    NOTE(FREQ_G5, 96, VOL_75),
    REST(16),
    NOTE(FREQ_C6, 96, VOL_100),
    REST(16),
    NOTE(FREQ_E6, 96, VOL_100),
    REST(16),
    NOTE(FREQ_G6, 160, VOL_100),
};

static const Note game_over_notes[] = {
    NOTE(FREQ_G4, 180, VOL_100),
    NOTE(FREQ_E4, 180, VOL_75),
    NOTE(FREQ_C4, 220, VOL_75),
    NOTE(FREQ_G3, 400, VOL_50),
};

static const Note invalid_notes[] = {
    NOTE(FREQ_F4, 16, VOL_75),
    NOTE(FREQ_F4, 16, VOL_50),
    NOTE(FREQ_C4, 16, VOL_25),
};

static const Note move_cursor_notes[] = {
    //NOTE(FREQ_C5, 16, VOL_75),
    //NOTE(FREQ_C5, 16, VOL_100),
    NOTE(FREQ_C5, 16, VOL_75),
    NOTE(FREQ_C5, 16, VOL_50),
    NOTE(FREQ_C5, 16, VOL_25),
};

static const Song songs[] = {
    {WAV_SQUARE, NULL, 0},
    {WAV_TRIANGLE, startup_notes, NOTE_COUNT(startup_notes)},
    {WAV_SQUARE | DUTY_CYCLE_50_0, launch_notes, NOTE_COUNT(launch_notes)},
    {WAV_NOISE, explode_notes, NOTE_COUNT(explode_notes)},
    {WAV_SAWTOOTH, invalid_notes, NOTE_COUNT(invalid_notes)},
    {WAV_TRIANGLE, game_over_notes, NOTE_COUNT(game_over_notes)},
    {WAV_TRIANGLE, move_cursor_notes, NOTE_COUNT(move_cursor_notes)},
    {WAV_TRIANGLE, fanfare_notes, NOTE_COUNT(fanfare_notes)},
};

static uint8_t snd_playing_id;
static uint8_t snd_pos;
static uint16_t snd_timer;
static bool snd_playing;

static void sound_silence(void)
{
    if(snd_playing && songs[snd_playing_id].waveform == WAV_NOISE)
        zvb_sound_set_voices(VOICE0, 0, WAV_SQUARE | DUTY_CYCLE_50_0);
    snd_playing = false;
    snd_playing_id = 0;
    snd_pos = 0;
    snd_timer = 0;
    zvb_sound_set_hold(VOICE0, 1);
}

static void sound_start_note(void)
{
    const Song* current_song = &songs[snd_playing_id];
    const Note* current_note = &current_song->notes[snd_pos];

    zvb_sound_set_voices_vol(VOICE0, current_note->volume);
    zvb_sound_set_voices(VOICE0, current_note->divider, current_song->waveform);
    zvb_sound_set_hold(VOICE0, 0);
    snd_timer = current_note->duration;
}

void sound_init(void)
{
    zvb_sound_initialize(1);
    zvb_sound_set_channels(VOICE0, VOICE0);
    zvb_sound_set_volume(VOL_100);
    sound_silence();
}

void sound_play(uint8_t sound_id)
{
    if(sound_id == 0 || sound_id >= NOTE_COUNT(songs)) return;

    snd_playing_id = sound_id;
    snd_pos = 0;
    snd_playing = true;
    sound_start_note();
}

void sound_update(uint16_t delta_time)
{
    while(snd_playing && delta_time >= snd_timer) {
        delta_time -= snd_timer;
        snd_pos++;
        if(snd_pos >= songs[snd_playing_id].length) {
            sound_silence();
        } else {
            sound_start_note();
        }
    }

    if(snd_playing) snd_timer -= delta_time;
}

void sound_stop(uint8_t sound_id)
{
    if(snd_playing && sound_id == snd_playing_id) sound_silence();
}

void sound_term(void)
{
    sound_silence();
    zvb_sound_set_volume(VOL_0);
}