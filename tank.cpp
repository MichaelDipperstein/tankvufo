/***************************************************************************
*                              Tank Versus UFO
*
*   File    : tank.cpp
*   Purpose : Tank movement functions
*   Author  : Michael Dipperstein
*   Date    : May 21, 2021
*
****************************************************************************
*
* Tank Versus UFO: A tribute to the Tank-V-UFO, a Commodore VIC-20 Game
*                  by Duane Later
*
* Copyright (C) 2021 by
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
#include <stdlib.h>

#include "tank.h"

/* cchar_t for unicode charaters used in this file */
static const cchar_t TANK_SHOT_CHAR = {WA_NORMAL, L"▪", 0};
static const cchar_t BOX_CHAR = {WA_NORMAL, L"█", 0};

Tank::Tank(WINDOW *window, const int minY, sound_data_t *sd)
{
    /* start with tank on left and no shot */
    x = 0;
    EndShot();
    minShotY = minY;
    shotHit = false;
    onFire = 0;
    numberDied = 0;
    win = window;
    getmaxyx(window, rows, cols);
    soundData = sd;
}


/* move the tank or flames of hit tank and record death */
void Tank::Move(void)
{
    if (10 == onFire)
    {
        /* done with fire, restart on left */
        onFire = 0;
        direction = Tvu::DIR_NONE;
        mvwaddstr(win, Tvu::TANK_GUN_ROW, x + 3, " ");
        mvwaddstr(win, Tvu::TANK_TURRET_ROW, x + 1, "    ");
        mvwaddstr(win, Tvu::TANK_TREAD_ROW, x, "      ");
        x = 0;
        numberDied += 1;
        select_sound(soundData, SOUND_OFF);
    }

    if (onFire)
    {
        onFire += 1;
        mvwaddstr(win, Tvu::TANK_GUN_ROW, x + 3, " ");

        wattron(win, COLOR_PAIR(3));       /* fire color */

        if (onFire % 2)
        {
            mvwaddstr(win, Tvu::TANK_TURRET_ROW, x + 1, "◣◣◣◣");
        }
        else
        {
            mvwaddstr(win, Tvu::TANK_TURRET_ROW, x + 1, "◢◢◢◢");
        }

        wattroff(win, COLOR_PAIR(3));
        mvwaddstr(win, Tvu::TANK_TREAD_ROW, x, "▕OOOO▏");
        wrefresh(win);
        return;
    }

    if (Tvu::DIR_NONE == direction)
    {
        /* redraw without moving */
        mvwaddstr(win, Tvu::TANK_GUN_ROW, x + 3, "▖");
        mvwaddstr(win, Tvu::TANK_TURRET_ROW, x + 1, "▁██▁");
        mvwaddstr(win, Tvu::TANK_TREAD_ROW, x, "▕OOOO▏");
    }
    else if (Tvu::DIR_LEFT == direction)
    {
        if (x == 0)
        {
            /* can't go furter left */
            return;
        }

        /* move to the left, add a trailing space to erase the old */
        x -= 1;
        mvwaddstr(win, Tvu::TANK_GUN_ROW, x + 3, "▖ ");
        mvwaddstr(win, Tvu::TANK_TURRET_ROW, x + 1, "▁██▁ ");
        mvwaddstr(win, Tvu::TANK_TREAD_ROW, x, "▕OOOO▏ ");
    }
    else if (Tvu::DIR_RIGHT == direction)
    {
        if (x == cols - 6)
        {
            /* can't go furter right */
            return;
        }

        /* move to the right, add a leading space to erase the old */
        mvwaddstr(win, Tvu::TANK_GUN_ROW, x + 3, " ▖");
        mvwaddstr(win, Tvu::TANK_TURRET_ROW, x + 1, " ▁██▁");
        mvwaddstr(win, Tvu::TANK_TREAD_ROW, x, " ▕OOOO▏");
        x += 1;
    }

    wrefresh(win);
    return;
}


void Tank::SetDirection(const Tvu::Direction dir)
{
    direction = dir;
}


uint8_t Tank::GetPos(void)
{
    return x;
}


uint8_t Tank::GetTanksKilled(void)
{
    return numberDied;
}


void Tank::MoveShot(void)
{
    if ((shotPos.y < 0) || (shotHit))
    {
        return;     /* there's no shot */
    }

    if (shotPos.y != Tvu::TANK_SHOT_START_ROW)
    {
        /* erase old shot if it hasn't been overwritten */
        cchar_t c;
        mvwin_wch(win, shotPos.y, shotPos.x, &c);

        if (TANK_SHOT_CHAR.chars[0] == c.chars[0])
        {
            mvwaddch(win, shotPos.y, shotPos.x, ' ');

            /* move shot up */
            shotPos.y--;
        }
        else if (shotPos.y == (Tvu::TANK_SHOT_START_ROW - 1))
        {
            /* delete the muzzle flash */
            mvwaddch(win, shotPos.y + 1, shotPos.x, ' ');
        }

        if (shotPos.y >= minShotY)
        {
            /* draw new shot */
            mvwadd_wch(win, shotPos.y, shotPos.x, &TANK_SHOT_CHAR);
        }
        else
        {
            /* done with shot */
            EndShot();

            /* stop shot sound */
            select_sound(soundData, SOUND_OFF);
        }
    }
    else
    {
        /* muzzle flash */
        wattron(win, COLOR_PAIR(3));       /* fire color */
        mvwadd_wch(win, shotPos.y, shotPos.x, &BOX_CHAR);
        wattroff(win, COLOR_PAIR(3));
        shotPos.y--;
    }

    wrefresh(win);
}


bool Tank::WasShotFired(void) const
{
    return (shotPos.y != -1);
}


void Tank::Shoot(void)
{
    shotPos.x = x + 3;
    shotPos.y = Tvu::TANK_SHOT_START_ROW;
}


void Tank::EndShot(void)
{
    shotPos.x = -1;
    shotPos.y = -1;
}


bool Tank::UpdateShotHit(const Tvu::Pos ufoPos)
{
    bool justHit;

    justHit = false;

    if (true == shotHit)
    {
        /* clear explosion shot */
        mvwaddch(win, shotPos.y - 1, shotPos.x, ' ');
        mvwaddstr(win, shotPos.y, shotPos.x - 1, "   ");
        mvwaddch(win, shotPos.y + 1, shotPos.x, ' ');

        EndShot();
        shotHit = false;
    }
    else if (shotPos.y == ufoPos.y)
    {
        /* same row */
        int dx;

        dx = shotPos.x - ufoPos.x;

        if ((dx >= 0) && (dx <= 2))
        {
            /* hit */
            shotHit = true;
            justHit = true;

            wattron(win, COLOR_PAIR(3));       /* fire color */
            mvwaddstr(win, shotPos.y - 1, shotPos.x, "█");
            mvwaddstr(win, shotPos.y, shotPos.x - 1, "███");
            mvwaddstr(win, shotPos.y + 1, shotPos.x, "█");
            wattroff(win, COLOR_PAIR(3));
        }
    }

    return justHit;
}


bool Tank::IsOnFire(void) const
{
    return onFire != 0;     /* onFire is a counter. 0 is not on fire */
}


void Tank::SetOnFire(const bool of)
{
    /* onFire is a counter. 0 is not on fire */
    if (of)
    {
        onFire = 1;
    }
    else
    {
        onFire = 0;
    }
}


sound_data_t* Tank::GetSoundData(void) const
{
    return soundData;
}
