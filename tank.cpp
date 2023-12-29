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
#include <ncurses.h>
#include <stdlib.h>

#include "tank.h"

/* cchar_t for unicode charaters used in this file */
static const cchar_t TANK_SHOT_CHAR = {WA_NORMAL, L"▪", 0};
static const cchar_t BOX_CHAR = {WA_NORMAL, L"█", 0};

tank_t::tank_t(WINDOW *window, const int min_y, sound_data_t *sd)
{
    /* start with tank on left and no shot */
    x = 0;
    shot_pos.x = -1;
    shot_pos.y = -1;
    min_shot_y = min_y;
    shot_hit = false;
    on_fire = 0;
    number_died = 0;
    win = window;
    getmaxyx(window, rows, cols);
    sound_data = sd;
}


/* move the tank or flames of hit tank and record death */
void tank_t::move(void)
{
    if (10 == on_fire)
    {
        /* done with fire, restart on left */
        on_fire = 0;
        direction = DIR_NONE;
        mvwaddstr(win, TANK_GUN_ROW, x + 3, " ");
        mvwaddstr(win, TANK_TURRET_ROW, x + 1, "    ");
        mvwaddstr(win, TANK_TREAD_ROW, x, "      ");
        x = 0;
        number_died += 1;
        select_sound(sound_data, SOUND_OFF);
    }

    if (on_fire)
    {
        on_fire += 1;
        mvwaddstr(win, TANK_GUN_ROW, x + 3, " ");

        wattron(win, COLOR_PAIR(3));       /* fire color */

        if (on_fire % 2)
        {
            mvwaddstr(win, TANK_TURRET_ROW, x + 1, "◣◣◣◣");
        }
        else
        {
            mvwaddstr(win, TANK_TURRET_ROW, x + 1, "◢◢◢◢");
        }

        wattroff(win, COLOR_PAIR(3));
        mvwaddstr(win, TANK_TREAD_ROW, x, "▕OOOO▏");
        wrefresh(win);
        return;
    }

    if (DIR_NONE == direction)
    {
        /* redraw without moving */
        mvwaddstr(win, TANK_GUN_ROW, x + 3, "▖");
        mvwaddstr(win, TANK_TURRET_ROW, x + 1, "▁██▁");
        mvwaddstr(win, TANK_TREAD_ROW, x, "▕OOOO▏");
    }
    else if (DIR_LEFT == direction)
    {
        if (x == 0)
        {
            /* can't go furter left */
            return;
        }

        /* move to the left, add a trailing space to erase the old */
        x -= 1;
        mvwaddstr(win, TANK_GUN_ROW, x + 3, "▖ ");
        mvwaddstr(win, TANK_TURRET_ROW, x + 1, "▁██▁ ");
        mvwaddstr(win, TANK_TREAD_ROW, x, "▕OOOO▏ ");
    }
    else if (DIR_RIGHT == direction)
    {
        if (x == cols - 6)
        {
            /* can't go furter right */
            return;
        }

        /* move to the right, add a leading space to erase the old */
        mvwaddstr(win, TANK_GUN_ROW, x + 3, " ▖");
        mvwaddstr(win, TANK_TURRET_ROW, x + 1, " ▁██▁");
        mvwaddstr(win, TANK_TREAD_ROW, x, " ▕OOOO▏");
        x += 1;
    }

    wrefresh(win);
    return;
}


void tank_t::set_direction(const direction_t dir)
{
    direction = dir;
}


uint8_t tank_t::get_pos(void)
{
    return x;
}


uint8_t tank_t::get_tanks_killed(void)
{
    return number_died;
}


void tank_t::move_shot(void)
{
    if ((shot_pos.y < 0) || (shot_hit))
    {
        return;     /* there's no shot */
    }

    if (shot_pos.y != TANK_SHOT_START_ROW)
    {
        /* erase old shot if it hasn't been overwritten */
        cchar_t c;
        mvwin_wch(win, shot_pos.y, shot_pos.x, &c);

        if (TANK_SHOT_CHAR.chars[0] == c.chars[0])
        {
            mvwaddch(win, shot_pos.y, shot_pos.x, ' ');

            /* move shot up */
            shot_pos.y--;
        }
        else if (shot_pos.y == (TANK_SHOT_START_ROW - 1))
        {
            /* delete the muzzle flash */
            mvwaddch(win, shot_pos.y + 1, shot_pos.x, ' ');
        }

        if (shot_pos.y >= min_shot_y)
        {
            /* draw new shot */
            mvwadd_wch(win, shot_pos.y, shot_pos.x, &TANK_SHOT_CHAR);
        }
        else
        {
            /* done with shot */
            shot_pos.x = -1;
            shot_pos.y = -1;

            /* stop shot sound */
            select_sound(sound_data, SOUND_OFF);
        }
    }
    else
    {
        /* muzzle flash */
        wattron(win, COLOR_PAIR(3));       /* fire color */
        mvwadd_wch(win, shot_pos.y, shot_pos.x, &BOX_CHAR);
        wattroff(win, COLOR_PAIR(3));
        shot_pos.y--;
    }

    wrefresh(win);
}


bool tank_t::was_shot_fired(void) const
{
    return (shot_pos.y != -1);
}


pos_t tank_t::get_shot_pos(void) const
{
    return shot_pos;
}


void tank_t::set_shot_pos(const int8_t x, const int8_t y)
{
    shot_pos.x = x;
    shot_pos.y = y;
}


bool tank_t::did_shot_hit(void) const
{
    return shot_hit;
}


void tank_t::set_shot_hit(const bool hit)
{
    shot_hit = hit;
}


bool tank_t::is_on_fire(void) const
{
    /* on_fire is a counter 0 == not on fire */
    return on_fire != 0;
}


void tank_t::set_on_fire(const bool of)
{
    /* on_fire is a counter 0 == not on fire */
    if (of)
    {
        on_fire = 1;
    }
    else
    {
        on_fire = 0;
    }
}


sound_data_t* tank_t::get_sound_data(void) const
{
    return sound_data;
}
