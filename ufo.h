/***************************************************************************
*                              Tank Versus UFO
*
*   File    : ufo.h
*   Purpose : UFO related definitions used by Tank Versus UFO
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
#ifndef  __UFO_H
#define  __UFO_H

#include "tankvufo.h"
#include "sounds.h"

/* struct containing ufo related data */
typedef struct
{
    int x;                      /* leftmost ufo coordinate */
    int y;                      /* row containing the ufo */
    direction_t direction;      /* direction that the UFO is moving */
    uint8_t ufo_hit_ground;     /* 0 when not on fire, otherwise flame count */
    int shot_x;                 /* x coordinate of ufo shot */
    int shot_y;                 /* y coordinate of ufo shot */
    direction_t shot_direction; /* direction the ufo shot is moving */
    uint8_t shot_hit_ground;    /* 0 if false, otherwise phase of explosion */
    sound_data_t *sound_data;
} ufo_info_t;

void initialize_ufo(sound_data_t *sound_data, ufo_info_t *ufo);
uint8_t move_ufo(WINDOW* win, ufo_info_t *ufo);
void make_ufo_shot(ufo_info_t *ufo);
void move_ufo_shot(WINDOW* win, ufo_info_t *ufo);
int ufo_shot_hit_ground(WINDOW* win, ufo_info_t *ufo);

#endif /* ndef  __UFO_H */
