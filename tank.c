/***************************************************************************
*                              Tank Versus UFO
*
*   File    : tank.c
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

struct tank_info_t
{
    uint8_t x;                  /* leftmost tank coordinate */
    direction_t direction;      /* direction of next tank move */
    pos_t shot_pos;             /* x & y coordinate of tank shot */
    bool shot_hit;              /* true if the ufo was just hit (+ displayed) */
    uint8_t on_fire;            /* 0 when not on fire, otherwise flame count */
    uint8_t number_died;        /* number of tanks that died (ufo score) */
    WINDOW* win;                /* ncurses window for the tank and its shot */
    int rows;                   /* window rows */
    int cols;                   /* window columns */
    sound_data_t *sound_data;
};

/* cchar_t for unicode charaters used in this file */
static const cchar_t TANK_SHOT_CHAR = {WA_NORMAL, L"▪", 0};
static const cchar_t BOX_CHAR = {WA_NORMAL, L"█", 0};

tank_info_t *tank_initialize(WINDOW *window, sound_data_t *sound_data)
{
    tank_info_t *tank;
    tank = (tank_info_t *)malloc(sizeof(struct tank_info_t));

    if (NULL == tank)
    {
        return NULL;    /* allocation failed */
    }

    /* start with tank on left and no shot */
    tank->x = 0;
    tank->shot_pos.x = -1;
    tank->shot_pos.y = -1;
    tank->shot_hit = false;
    tank->on_fire = 0;
    tank->number_died = 0;
    tank->win = window;
    getmaxyx(window, tank->rows, tank->cols);
    tank->sound_data = sound_data;

    return tank;
}


/* move the tank or flames of hit tank and record death */
void tank_move(tank_info_t *tank)
{
    WINDOW *win;

    win = tank->win;

    if (10 == tank->on_fire)
    {
        /* done with fire, restart on left */
        tank->on_fire = 0;
        tank->direction = DIR_NONE;
        mvwaddstr(win, TANK_GUN_ROW, tank->x + 3, " ");
        mvwaddstr(win, TANK_TURRET_ROW, tank->x + 1, "    ");
        mvwaddstr(win, TANK_TREAD_ROW, tank->x, "      ");
        tank->x = 0;
        tank->number_died += 1;
        select_sound(tank->sound_data, SOUND_OFF);
    }

    if (tank->on_fire)
    {
        tank->on_fire += 1;
        mvwaddstr(win, TANK_GUN_ROW, tank->x + 3, " ");

        wattron(win, COLOR_PAIR(3));       /* fire color */

        if (tank->on_fire % 2)
        {
            mvwaddstr(win, TANK_TURRET_ROW, tank->x + 1, "◣◣◣◣");
        }
        else
        {
            mvwaddstr(win, TANK_TURRET_ROW, tank->x + 1, "◢◢◢◢");
        }

        wattroff(win, COLOR_PAIR(3));
        mvwaddstr(win, TANK_TREAD_ROW, tank->x, "▕OOOO▏");
        wrefresh(win);
        return;
    }

    if (DIR_NONE == tank->direction)
    {
        /* redraw without moving */
        mvwaddstr(win, TANK_GUN_ROW, tank->x + 3, "▖");
        mvwaddstr(win, TANK_TURRET_ROW, tank->x + 1, "▁██▁");
        mvwaddstr(win, TANK_TREAD_ROW, tank->x, "▕OOOO▏");
    }
    else if (DIR_LEFT == tank->direction)
    {
        if (tank->x == 0)
        {
            /* can't go furter left */
            return;
        }

        /* move to the left, add a trailing space to erase the old */
        tank->x -= 1;
        mvwaddstr(win, TANK_GUN_ROW, tank->x + 3, "▖ ");
        mvwaddstr(win, TANK_TURRET_ROW, tank->x + 1, "▁██▁ ");
        mvwaddstr(win, TANK_TREAD_ROW, tank->x, "▕OOOO▏ ");
    }
    else if (DIR_RIGHT == tank->direction)
    {
        if (tank->x == tank->cols - 6)
        {
            /* can't go furter right */
            return;
        }

        /* move to the right, add a leading space to erase the old */
        mvwaddstr(win, TANK_GUN_ROW, tank->x + 3, " ▖");
        mvwaddstr(win, TANK_TURRET_ROW, tank->x + 1, " ▁██▁");
        mvwaddstr(win, TANK_TREAD_ROW, tank->x, " ▕OOOO▏");
        tank->x += 1;
    }

    wrefresh(win);
    return;
}


void tank_set_direction(tank_info_t *tank, const direction_t direction)
{
    tank->direction = direction;
}


uint8_t tank_get_pos(const tank_info_t *tank)
{
    return tank->x;
}


uint8_t tank_get_ufo_score(const tank_info_t *tank)
{
    return tank->number_died;
}


void tank_shot_move(tank_info_t *tank)
{
    WINDOW* win;
    win = tank->win;

    if ((tank->shot_pos.y < 0) || (tank->shot_hit))
    {
        return;     /* there's no shot */
    }

    if (tank->shot_pos.y != TANK_SHOT_START_ROW)
    {
        /* erase old shot if it hasn't been overwritten */
        cchar_t c;
        mvwin_wch(win, tank->shot_pos.y, tank->shot_pos.x, &c);

        if (TANK_SHOT_CHAR.chars[0] == c.chars[0])
        {
            mvwaddch(win, tank->shot_pos.y, tank->shot_pos.x, ' ');

            /* move shot up */
            tank->shot_pos.y--;
        }
        else if (tank->shot_pos.y == (TANK_SHOT_START_ROW - 1))
        {
            /* delete the muzzle flash */
            mvwaddch(win, tank->shot_pos.y + 1, tank->shot_pos.x, ' ');
        }

        if (tank->shot_pos.y != SCORE_ROW)
        {
            /* draw new shot */
            mvwadd_wch(win, tank->shot_pos.y, tank->shot_pos.x, &TANK_SHOT_CHAR);
        }
        else
        {
            /* done with shot */
            tank->shot_pos.x = -1;
            tank->shot_pos.y = -1;

            /* stop shot sound */
            select_sound(tank->sound_data, SOUND_OFF);
        }
    }
    else
    {
        /* muzzle flash */
        wattron(win, COLOR_PAIR(3));       /* fire color */
        mvwadd_wch(win, tank->shot_pos.y, tank->shot_pos.x, &BOX_CHAR);
        wattroff(win, COLOR_PAIR(3));
        tank->shot_pos.y--;
    }

    wrefresh(win);
}


bool tank_took_shot(const tank_info_t *tank)
{
    return (tank->shot_pos.y != -1);
}


pos_t tank_get_shot_pos(const tank_info_t *tank)
{
    return tank->shot_pos;
}


void tank_set_shot_pos(tank_info_t *tank, const int8_t x, const int8_t y)
{
    tank->shot_pos.x = x;
    tank->shot_pos.y = y;
}


bool tank_shot_hit(const tank_info_t *tank)
{
    return tank->shot_hit;
}


void tank_set_shot_hit(tank_info_t *tank, const bool hit)
{
    tank->shot_hit = hit;
}


bool tank_is_on_fire(const tank_info_t *tank)
{
    /* on_fire is a counter 0 == not on fire */
    return (tank->on_fire != 0);
}


void tank_set_on_fire(tank_info_t *tank, const bool on_fire)
{
    /* on_fire is a counter 0 == not on fire */
    if (on_fire)
    {
        tank->on_fire = 1;
    }
    else
    {
        tank->on_fire = 0;
    }
}


sound_data_t* tank_sound_data(const tank_info_t *tank)
{
    return tank->sound_data;
}
