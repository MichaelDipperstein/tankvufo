#ifndef  __SOUNDS_H
#define  __SOUNDS_H
/***************************************************************************
*                             Tank Versuses UFO
*
*   File    : sounds.h
*   Purpose : PortAudio based sound library for Tank Versuses UFO
*   Author  : Michael Dipperstein
*   Date    : February 7, 2021
*
****************************************************************************
*
* Tank Versuses UFO: A tribute to the Tank-V-UFO, a Commodore VIC-20 Game
*                    by Duane Later
*
* Copyright (C) 2020, 2021 by
*       Michael Dipperstein (mdipperstein@gmail.com)
*
* This file is part of Tank Versuses UFO.
*
* Trim is free software; you can redistribute it and/or modify it under
* the terms of the GNU General Public License as published by the Free
* Software Foundation; either version 3 of the License, or (at your
* option) any later version.
*
* Trim is distributed in the hope that it will be useful, but WITHOUT ANY
* ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
* FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
* for more details.
*
* You should have received a copy of the GNU General Public License along
* with this program.  If not, see <http://www.gnu.org/licenses/>.
*
****************************************************************************/
#include <portaudio.h>

typedef enum
{
    FREQ_OFF = 0,       /* off */
    FREQ_LOW = 1,       /* normal frequency */
    FREQ_HIGH = 2       /* 2x frequency */
} freq_t;


typedef struct
{
    float *table;
    int tableSize;
    int phase;
    freq_t freq;        /* normal, 2x frequency, or off */
    PaStream *stream;
} sound_data_t;

typedef PaError sound_error_t;

void handle_error(sound_error_t error);

sound_error_t initialize_sounds(void);
void end_sounds(void);

sound_error_t initialize_sine_wave(sound_data_t *data, int freq, float volume);

int create_sine_stream(sound_data_t *data);
sound_error_t restart_stream(sound_data_t *data);
sound_error_t close_stream(sound_data_t *data);


/* frequency controls for dual frequency sine wave */
void frequecy_off(sound_data_t *data);
void frequecy_low(sound_data_t *data);
void frequecy_high(sound_data_t *data);
void frequecy_toggle(sound_data_t *data);

#endif /* ndef  __SOUNDS_H */
