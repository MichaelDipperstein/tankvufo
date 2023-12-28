/***************************************************************************
*                              Tank Versus UFO
*
*   File    : tank.h
*   Purpose : Tank related definitions used by Tank Versus UFO
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
#ifndef  __TANK_H
#define  __TANK_H

#include "sounds.h"
#include "tankvufo.h"

class tank_t
{
    public:
        tank_t(WINDOW *window, sound_data_t *sd);

        /* movement and position */
        void move(void);
        void set_direction(const direction_t dir);
        uint8_t get_pos(void);
        uint8_t get_tanks_killed(void);

        /* tank shot movement and position */
        void move_shot(void);
        bool was_shot_fired(void) const;
        pos_t get_shot_pos(void) const;
        void set_shot_pos(const int8_t x, const int8_t y);
        bool did_shot_hit(void) const;
        void set_shot_hit(const bool hit);

        /* flaming status */
        bool is_on_fire(void) const;
        void set_on_fire(const bool of);

        sound_data_t* get_sound_data(void) const;

    private:
        uint8_t x;              /* leftmost tank coordinate */
        direction_t direction;  /* direction of next tank move */
        pos_t shot_pos;         /* x & y coordinate of tank shot */
        bool shot_hit;          /* true if the ufo was just hit (+ displayed) */
        uint8_t on_fire;        /* 0 when not on fire, otherwise flame count */
        uint8_t number_died;    /* number of tanks that died (ufo score) */
        WINDOW* win;            /* ncurses window for the tank and its shot */
        int rows;               /* window rows */
        int cols;               /* window columns */
        sound_data_t *sound_data;
};

#endif /* ndef  __TANKVUFO_H */
