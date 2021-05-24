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
typedef struct ufo_info_t ufo_info_t;


ufo_info_t *ufo_initialize(WINDOW *window, sound_data_t *sound_data);

/* ufo shot movement and information */
void ufo_move(ufo_info_t *ufo);
pos_t ufo_get_pos(const ufo_info_t *ufo);
uint8_t ufo_get_tank_score(const ufo_info_t *ufo);

/* start falling direction and sound */
sound_error_t ufo_set_falling(ufo_info_t *ufo);

/* ufo shot movement and information */
void ufo_shot_decision(ufo_info_t *ufo);
void ufo_move_shot(ufo_info_t *ufo);
pos_t ufo_get_shot_pos(const ufo_info_t *ufo);
void ufo_clear_shot(ufo_info_t *ufo, bool erase);
bool ufo_shot_is_falling(const ufo_info_t *ufo);
bool ufo_shot_is_exploding(const ufo_info_t *ufo);
int ufo_shot_hit_ground(ufo_info_t *ufo);

#endif /* ndef  __UFO_H */
