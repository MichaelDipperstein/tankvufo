/***************************************************************************
*                              Tank Versus UFO
*
*   File    : main.cpp
*   Purpose : main() for A tribute to the Tank-V-UFO, a Commodore VIC-20
*             game by Duane Later
*   Author  : Michael Dipperstein
*   Date    : July 17, 2025
*
****************************************************************************
*
* Tank Versus UFO: A tribute to the Tank-V-UFO, a Commodore VIC-20 Game
*                  by Duane Later
*
* Copyright (C) 2020, 2021, 2025 by
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
#include <ncurses.h>
#include <cstdlib>
#include <cstring>
#include <sys/timerfd.h>
#include <sys/poll.h>
#include <unistd.h>
#include <cerrno>

#include "tankvufo.h"
#include "sounds.h"

constexpr float VOLUME = 0.5;    /* base volume for sounds */

int main(void)
{
    TankVUfo *tvu;
    int win_x, win_y;
    bool result;

    /* setup the ncurses field-of-play */
    tvu = new TankVUfo;

    if (nullptr == tvu)
    {
        perror("creating tank vs ufo object");
        return 1;
    }

    /* vic-20 sized window for the game field */
    win_x = (COLS - RowsAndCols::V20_COLS) / 2;
    win_y = (LINES - RowsAndCols::V20_ROWS) / 2;

    result = tvu->MakeV20Win(RowsAndCols::V20_ROWS,
        RowsAndCols::V20_COLS, win_y, win_x);

    if (false == result)
    {
        delete tvu;
        perror("creating VIC-20 window");
        return 1;
    }

    /* window for volume meter */
    win_y = (LINES - RowsAndCols::VOL_ROWS) / 2;
    win_x += RowsAndCols::V20_COLS + RowsAndCols::VOL_COLS;
    result = tvu->MakeVolWin(RowsAndCols::VOL_ROWS, RowsAndCols::VOL_COLS,
        win_y, win_x);

    if (false == result)
    {
        delete tvu;
        perror("creating volume control window");
        return 1;
    }

    refresh();              /* Refresh the whole screen to show the windows */

    tvu->InitializeV20Win();

    /* now draw the volume level box */
    tvu->DrawVolumeLevelBox();
    tvu->ShowVolumeLevel(VOLUME);

    /* initialize all of the sound stuff */
    sound_data_t sound_data;
    sound_error_t sound_error;

    sound_error = initialize_sounds();

    if (0 != sound_error)
    {
        handle_error(sound_error);
        delete tvu;
        return sound_error;
    }

    sound_error = create_sound_stream(&sound_data, VOLUME);

    if (0 != sound_error)
    {
        handle_error(sound_error);
        delete tvu;
        return sound_error;
    }

    /* create and initialize the tank and ufo */
    result = tvu->InitializeVehicles(&sound_data);

    if (false == result)
    {
        close_sound_stream(&sound_data);
        end_sounds();
        delete tvu;
        perror("allocating tank or ufo");
        return 1;
    }

    tvu->PrintScore();                     /* 0 - 0 score */
    tvu->Refresh();

    /* timer and poll variables */
    int tfd;
    struct itimerspec timeout;
    struct pollfd pfd;

    /* create a timer fd that expires every 200ms */
    tfd = timerfd_create(CLOCK_MONOTONIC,0);

    if (tfd <= 0)
    {
        close_sound_stream(&sound_data);
        end_sounds();
        delete tvu;
        perror("creating timerfd");
        return 1;
    }

    /* make the timer timeout in 200ms and repeat */
    timeout.it_value.tv_sec = 0;
    timeout.it_value.tv_nsec = 200 * 1000000;
    timeout.it_interval.tv_sec = 0;
    timeout.it_interval.tv_nsec = 200 * 1000000;

    if (timerfd_settime(tfd, 0, &timeout, 0) != 0)
    {
        close_sound_stream(&sound_data);
        end_sounds();
        delete tvu;
        perror("setting timerfd");
        return 1;
    }

    /* set pollfd for timer read event */
    memset(&pfd, 0, sizeof(pfd));
    pfd.fd = tfd;
    pfd.events = POLLIN;
    pfd.revents = 0;

    /*
     * This is the event loop that makes the game work.
     * The timerfd expires every 200ms and starts the loop.
     * If HandleKeyPress sees a 'q' or a 'Q' the loop will be
     * exited causing the game to end.
     */
    while (poll(&pfd, 1, -1) > 0)
    {
        if (POLLIN == pfd.revents)
        {
            /* read the timer fd */
            uint64_t elapsed;

            read(tfd, &elapsed, sizeof(elapsed));
        }
        else
        {
            /* something went wrong */
            break;
        }

        if (tvu->HandleKeyPress() < 0)
        {
            /* we got a quit key */
            break;
        }

        tvu->MoveTank();
        tvu->MoveUfo();

        tvu->UpdateTankShot();
        tvu->UpdateUfoShot();

        tvu->PrintScore();
    }

    close_sound_stream(&sound_data);
    end_sounds();
    delete tvu;
    return 0;
}
