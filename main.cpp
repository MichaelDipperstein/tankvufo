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

int main(void)
{
    /* setup the ncurses field-of-play */
    TankVUfo *tvu;
    int winX, winY;
    bool result;

    tvu = new TankVUfo();

    if (nullptr == tvu)
    {
        perror("creating tank vs ufo object");
        return 1;
    }

    /* vic-20 sized window for the game field */
    winX = (COLS - Tvu::V20_COLS) / 2;
    winY = (LINES - Tvu::V20_ROWS) / 2;

    result = tvu->MakeV20Win(Tvu::V20_ROWS, Tvu::V20_COLS, winY, winX);

    if (false == result)
    {
        delete tvu;
        perror("creating VIC-20 window");
        return 1;
    }

    /* window for volume meter */
    winY = (LINES - Tvu::VOL_ROWS) / 2;
    winX += Tvu::V20_COLS + Tvu::VOL_COLS;
    result = tvu->MakeVolWin(Tvu::VOL_ROWS, Tvu::VOL_COLS,
        winY, winX);

    if (false == result)
    {
        delete tvu;
        perror("creating volume control window");
        return 1;
    }

    refresh();      /* Refresh the whole screen to show the windows */

    tvu->InitializeV20Win();

    /* now draw the volume level box */
    tvu->DrawVolumeLevelBox();
    tvu->ShowVolumeLevel(TankVUfo::VOLUME);

    /* create and initialize the tank and ufo */
    result = tvu->InitializeVehicles();

    if (false == result)
    {
        delete tvu;
        perror("allocating tank or ufo");
        return 1;
    }

    tvu->PrintScore();                     /* 0 - 0 score */
    tvu->Refresh();

    /* timer and poll variables */
    int fdTimer;
    struct itimerspec timeout;
    struct pollfd fdPoll;

    /* create a timer fd that expires every 200ms */
    fdTimer = timerfd_create(CLOCK_MONOTONIC,0);

    if (fdTimer <= 0)
    {
        delete tvu;
        perror("creating timerfd");
        return 1;
    }

    /* make the timer timeout in 200ms and repeat */
    timeout.it_value.tv_sec = 0;
    timeout.it_value.tv_nsec = 200 * 1000000;
    timeout.it_interval.tv_sec = 0;
    timeout.it_interval.tv_nsec = 200 * 1000000;

    if (timerfd_settime(fdTimer, 0, &timeout, 0) != 0)
    {
        delete tvu;
        perror("setting timerfd");
        return 1;
    }

    /* set pollfd for timer read event */
    memset(&fdPoll, 0, sizeof(fdPoll));
    fdPoll.fd = fdTimer;
    fdPoll.events = POLLIN;
    fdPoll.revents = 0;

    /*
     * This is the event loop that makes the game work.
     * The timerfd expires every 200ms and starts the loop.
     * If HandleKeyPress sees a 'q' or a 'Q' the loop will be
     * exited causing the game to end.
     */
    while (poll(&fdPoll, 1, -1) > 0)
    {
        if (POLLIN == fdPoll.revents)
        {
            /* read the timer fd */
            uint64_t elapsed;

            read(fdTimer, &elapsed, sizeof(elapsed));
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

    delete tvu;
    return 0;
}
