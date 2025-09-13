/***************************************************************************
*                              Tank Versus UFO
*
*   File    : sounds.h
*   Purpose : PortAudio based sound library for Tank Versuses UFO
*   Author  : Michael Dipperstein
*   Date    : February 7, 2021
*
****************************************************************************
*
* Tank Versus UFO: A tribute to the Tank-V-UFO, a Commodore VIC-20 Game
*                  by Duane Later
*
* Copyright (C) 2020, 2021 by
*       Michael Dipperstein (mdipperstein@gmail.com)
*
* This file is part of Tank Versus UFO.
*
* Tank Versus UFO is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by the
* Free Software Foundation; either version 3 of the License, or (at your
* option) any later version.
*
* Tank Versus UFO is distributed in the hope that it will be fun, but
* WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
* or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
* for more details.
*
* You should have received a copy of the GNU General Public License along
* with this program.  If not, see <http://www.gnu.org/licenses/>.
*
****************************************************************************/
#ifndef  __SOUNDS_H
#define  __SOUNDS_H

#include <portaudio.h>
static const int SAMPLE_RATE = 44100;

typedef enum
{
    SOUND_OFF,          /* off */
    SOUND_LOW_FREQ,     /* normal ufo falling frequency */
    SOUND_HIGH_FREQ,    /* 2x ufo falling frequency */
    SOUND_TANK_SHOT,    /* tank shot flying */
    SOUND_ON_FIRE,      /* tank or ufo on fire */
    SOUND_EXPLODE       /* tank shot exploding when it hits ufo */
} sound_t;


typedef struct sound_data_t
{
    float volume;
    int phase;
    sound_t sound;       /* normal, 2x frequency, tank shot, or off */
    PaStream *stream;
} sound_data_t;


typedef PaError sound_error_t;


class Sounds
{
    public:
        Sounds(void);
        ~Sounds(void);

        void HandleError(void);

        void CreateSoundStream(float volume);
        void RestartSoundStream(void);
        void CloseSoundStream(void);

        void SelectSound(sound_t sound);
        void NextUfoSound(void);

        float IncrementVolume(void);
        float DecrementVolume(void);

        sound_error_t GetError(void);

    private:
        sound_error_t lastError;
        sound_data_t soundData;
};

#endif /* ndef  __SOUNDS_H */
