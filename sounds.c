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
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <math.h>
#include "sounds.h"

#ifndef M_PI
#define M_PI                (3.14159265)
#endif

#define SAMPLE_RATE         (44100)

/* This routine will be called by the PortAudio engine when audio is needed.
** It may called at interrupt level on some machines so don't do anything
** that could mess up the system like calling malloc() or free().
*/
static int sine_callback(const void *inputBuffer,
    void *outputBuffer,
    unsigned long framesPerBuffer,
    const PaStreamCallbackTimeInfo* timeInfo,
    PaStreamCallbackFlags statusFlags,
    void *userData)
{
    sound_data_t *data = (sound_data_t*)userData;
    float *out = (float*)outputBuffer;
    unsigned long i;

    /* Prevent unused variable warnings. */
    (void)timeInfo;         /* doesn't work with PulseAudio */
    (void)statusFlags;
    (void)inputBuffer;

    if (FREQ_OFF == data->freq)
    {
        return paComplete;
    }

    for(i=0; i < framesPerBuffer; i++)
    {
        /* left channel output then right channel output */
        *out = data->table[data->phase];
        out++;
        *out = data->table[data->phase];
        out++;

        /* shift to next phase of wave based on frequency */
        data->phase += data->freq;

        if(data->phase >= data->tableSize)
        {
            data->phase -= data->tableSize;
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


int initialize_sine_wave(sound_data_t *data, int freq, float volume)
{
    int i;

    data->tableSize = SAMPLE_RATE / freq;
    data->table = malloc(data->tableSize * sizeof(float));

    if (NULL == data->table)
    {
        return errno;
    }

    /* create sinusoidal wave table for the specified frequency */
    for(i = 0; i < data->tableSize; i++)
    {
        data->table[i] = volume *
            (float)sin(((double)i/(double)data->tableSize) * M_PI * 2.0);
    }

    data->phase = 0;
    data->freq = FREQ_LOW;
    data->stream = NULL;

    return 0;
}


sound_error_t create_sine_stream(sound_data_t *data)
{
    sound_error_t err;

    err = Pa_OpenDefaultStream(&(data->stream), 0, 2, paFloat32, SAMPLE_RATE,
        paFramesPerBufferUnspecified, sine_callback, data);

    return err;
}


sound_error_t restart_stream(sound_data_t *data)
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

    result = Pa_StartStream(data->stream);
    return result;
}


sound_error_t close_stream(sound_data_t *data)
{
    free(data->table);
    return Pa_CloseStream(data->stream);
}


void frequecy_off(sound_data_t *data)
{
    data->freq = FREQ_OFF;
}


void frequecy_low(sound_data_t *data)
{
    data->freq = FREQ_LOW;
}


void frequecy_high(sound_data_t *data)
{
    data->freq = FREQ_HIGH;
}


void frequecy_toggle(sound_data_t *data)
{
    if (FREQ_LOW == data->freq)
    {
        data->freq = FREQ_HIGH;
    }
    else if (FREQ_HIGH == data->freq)
    {
        data->freq = FREQ_LOW;
    }
}
