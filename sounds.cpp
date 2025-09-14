/***************************************************************************
*                              Tank Versus UFO
*
*   File    : sounds.cpp
*   Purpose : PortAudio based sound library for Tank Versus UFO
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
#include <cstdio>
#include <cstdlib>
#include <cerrno>
#include <unistd.h>
#include <fcntl.h>
#include "sounds.h"
#include "sound_data.h"

/* This routine will be called by the PortAudio engine when audio is needed.
** It may called at interrupt level on some machines so don't do anything
** that could mess up the system like calling malloc() or free().
*/
static int SoundCallback(const void *inputBuffer,
    void *outputBuffer,
    unsigned long framesPerBuffer,
    const PaStreamCallbackTimeInfo* timeInfo,
    PaStreamCallbackFlags statusFlags,
    void *userData)
{
    sound_data_t *data = (sound_data_t*)userData;
    float *out = (float*)outputBuffer;
    unsigned long i;
    int data_length;

    /* Prevent unused variable warnings. */
    (void)timeInfo;         /* doesn't work with PulseAudio */
    (void)statusFlags;
    (void)inputBuffer;

    if (SOUND_OFF == data->sound)
    {
        data->phase = 0;
        return paComplete;
    }

    /* handle explode first, because it might become high frequency sound */
    if (SOUND_EXPLODE == data->sound)
    {
        /* this is the sound for a tank shot */
        data_length = sizeof(explosion) / sizeof(explosion[0]);

        for(i = 0; i < framesPerBuffer; i++)
        {
            /* left channel output then right channel output */
            *out = data->volume * explosion[data->phase];
            out++;
            *out = data->volume * explosion[data->phase];
            out++;

            data->phase++;

            if(data->phase >= data_length)
            {
                /* switch to falling sound */
                data->phase = 0;
                data->sound = SOUND_HIGH_FREQ;
                framesPerBuffer -= (i + 1);
                break;
            }
        }
    }

    if ((SOUND_LOW_FREQ == data->sound) ||  (SOUND_HIGH_FREQ == data->sound))
    {
        /* this is the sound for a falling UFO */
        data_length = sizeof(ufo_falling) / sizeof(ufo_falling[0]);

        for(i = 0; i < framesPerBuffer; i++)
        {
            /* left channel output then right channel output */
            *out = data->volume * ufo_falling[data->phase];
            out++;
            *out = data->volume * ufo_falling[data->phase];
            out++;

            /* shift to next phase of wave based on frequency */
            if (SOUND_HIGH_FREQ == data->sound)
            {
                data->phase += 2;
            }
            else
            {
                data->phase++;
            }

            if(data->phase >= data_length)
            {
                data->phase -= data_length;
            }
        }
    }
    else if (SOUND_TANK_SHOT == data->sound)
    {
        /* this is the sound for a tank shot */
        data_length = sizeof(shot_sound) / sizeof(shot_sound[0]);

        for(i = 0; i < framesPerBuffer; i++)
        {
            /* left channel output then right channel output */
            *out = data->volume * shot_sound[data->phase];
            out++;
            *out = data->volume * shot_sound[data->phase];
            out++;

            data->phase++;

            if(data->phase >= data_length)
            {
                data->phase = 0;
                data->sound = SOUND_OFF;
                return paComplete;
            }
        }
    }
    else if (SOUND_ON_FIRE == data->sound)
    {
        /* this is the sound of a tank or ufo on fire */
        data_length = sizeof(on_fire) / sizeof(on_fire[0]);

        for(i = 0; i < framesPerBuffer; i++)
        {
            /* left channel output then right channel output */
            *out = data->volume * on_fire[data->phase];
            out++;
            *out = data->volume * on_fire[data->phase];
            out++;

            data->phase++;

            if(data->phase >= data_length)
            {
                data->phase = 0;
                data->sound = SOUND_OFF;
                return paComplete;
            }
        }
    }

    return paContinue;
}


Sounds::Sounds(void)
{
    int newStdErr;
    int oldStdErr;

    /* hide ALSA error during initialization (stderr -> /dev/null) */
    fflush(stderr);
    oldStdErr = dup(2);
    newStdErr = open("/dev/null", O_WRONLY);
    dup2(newStdErr, 2);
    close(newStdErr);

    lastError = Pa_Initialize();

    /* restore stderr */
    fflush(stderr);
    dup2(oldStdErr, 2);
    close(oldStdErr);
}


Sounds::~Sounds(void)
{
    Pa_CloseStream(soundData.stream);
    Pa_Terminate();
}


void Sounds::HandleError(void)
{
    fprintf(stderr, "Error [%d]: %s\n", lastError, Pa_GetErrorText(lastError));
    Pa_CloseStream(soundData.stream);
    Pa_Terminate();
}


void Sounds::CreateSoundStream(float volume)
{
    soundData.volume = volume;
    soundData.phase = 0;
    soundData.sound = SOUND_OFF;

    lastError = Pa_OpenDefaultStream(&(soundData.stream), 0, 2, paFloat32,
        SAMPLE_RATE, paFramesPerBufferUnspecified, SoundCallback,
        &soundData);
}


void Sounds::RestartSoundStream(void)
{
    lastError = Pa_IsStreamStopped(soundData.stream);

    if (lastError >= 0)
    {
        /* not an error */
        if (paNoError == lastError)
        {
            /* stream was not stopped, stop it */
            lastError = Pa_StopStream(soundData.stream);
        }

        if (lastError >= 0)
        {
            /* restart the sound stream */
        }

        soundData.phase = 0;
        lastError = Pa_StartStream(soundData.stream);
    }
}


void Sounds::CloseSoundStream(void)
{
    lastError = Pa_CloseStream(soundData.stream);
}


void Sounds::SelectSound(sound_t sound)
{
    if (sound == soundData.sound)
    {
        /* already selected */
        return;
    }

    soundData.phase = 0;
    soundData.sound = sound;

    if (SOUND_OFF == sound)
    {
        lastError = Pa_AbortStream(soundData.stream);
    }
}


void Sounds::NextUfoSound(void)
{
    /* explosion or toggle between frequencies */
    if ((SOUND_OFF == soundData.sound) || (SOUND_TANK_SHOT == soundData.sound))
    {
        /* start with the explosion */
        SelectSound(SOUND_EXPLODE);
    }
    else if (SOUND_LOW_FREQ == soundData.sound)
    {
        /* switch to high frequency falling sound */
        SelectSound(SOUND_HIGH_FREQ);
    }
    else if (SOUND_HIGH_FREQ == soundData.sound)
    {
        /* switch to low frequency falling sound */
        SelectSound(SOUND_LOW_FREQ);
    }
}


float Sounds::IncrementVolume(void)
{
    float new_volume;

    new_volume = soundData.volume + 0.1;

    if (new_volume > 1.0)
    {
        soundData.volume = 1.0;
    }
    else
    {
        soundData.volume = new_volume;
    }

    return soundData.volume;
}


float Sounds::DecrementVolume(void)
{
    float new_volume;

    new_volume = soundData.volume - 0.1;

    if (new_volume < 0.0)
    {
        soundData.volume = 0.0;
    }
    else
    {
        soundData.volume = new_volume;
    }

    return soundData.volume;
}


sound_error_t Sounds::GetError(void)
{
    return lastError;
}
