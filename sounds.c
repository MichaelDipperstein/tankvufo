/***************************************************************************
*                              Tank Versus UFO
*
*   File    : sounds.h
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
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <math.h>
#include "sounds.h"
#include "sound_data.h"

#ifndef M_PI
#define M_PI                (3.14159265)
#endif

/* This routine will be called by the PortAudio engine when audio is needed.
** It may called at interrupt level on some machines so don't do anything
** that could mess up the system like calling malloc() or free().
*/
static int sound_callback(const void *inputBuffer,
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


void handle_error(sound_error_t error)
{
    Pa_Terminate();
    fprintf(stderr, "Error [%d]: %s\n", error, Pa_GetErrorText(error));
}


sound_error_t initialize_sounds(void)
{
    int newStdErr;
    int oldStdErr;
    PaError error;

    /* hide ALSA error during initialization (stderr -> /dev/null) */
    fflush(stderr);
    oldStdErr = dup(2);
    newStdErr = open("/dev/null", O_WRONLY);
    dup2(newStdErr, 2);
    close(newStdErr);

    error = Pa_Initialize();

    /* restore stderr */
    fflush(stderr);
    dup2(oldStdErr, 2);
    close(oldStdErr);

    return error;
}


void end_sounds(void)
{
    Pa_Terminate();
}


int create_sound_stream(sound_data_t *data, float volume)
{
    sound_error_t err;

    data->volume = volume;
    data->phase = 0;
    data->sound = SOUND_OFF;

    err = Pa_OpenDefaultStream(&(data->stream), 0, 2, paFloat32, SAMPLE_RATE,
        paFramesPerBufferUnspecified, sound_callback, data);

    return err;
}


sound_error_t restart_sound_stream(sound_data_t *data)
{
    PaError result;

    result = Pa_IsStreamStopped(data->stream);

    if (result < 0)
    {
        return result;
    }

    if (0 == result)
    {
        /* stream was not stopped, stop it */
        result = Pa_StopStream(data->stream);

        if(paNoError != result)
        {
            return result;
        }
    }

    data->phase = 0;
    result = Pa_StartStream(data->stream);

    return result;
}


sound_error_t close_sound_stream(sound_data_t *data)
{
    return Pa_CloseStream(data->stream);
}


void select_sound(sound_data_t *data, sound_t sound)
{
    if (sound == data->sound)
    {
        /* already selected */
        return;
    }

    data->phase = 0;
    data->sound = sound;

    if (SOUND_OFF == sound)
    {
        Pa_AbortStream(data->stream);
    }
}


void next_ufo_sound(sound_data_t *data)
{
    /* explosion or toggle between frequencies */
    if ((SOUND_OFF == data->sound) || (SOUND_TANK_SHOT == data->sound))
    {
        /* start with the explosion */
        select_sound(data, SOUND_EXPLODE);
    }
    else if (SOUND_LOW_FREQ == data->sound)
    {
        /* switch to high frequency falling sound */
        select_sound(data, SOUND_HIGH_FREQ);
    }
    else if (SOUND_HIGH_FREQ == data->sound)
    {
        /* switch to low frequency falling sound */
        select_sound(data, SOUND_LOW_FREQ);
    }
}


float increment_volume(sound_data_t *data)
{
    float new_volume;

    new_volume = data->volume + 0.1;

    if (new_volume > 1.0)
    {
        data->volume = 1.0;
    }
    else
    {
        data->volume = new_volume;
    }

    return data->volume;
}


float decrement_volume(sound_data_t *data)
{
    float new_volume;

    new_volume = data->volume - 0.1;

    if (new_volume < 0.0)
    {
        data->volume = 0.0;
    }
    else
    {
        data->volume = new_volume;
    }

    return data->volume;
}

