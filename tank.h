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

#ifdef __cplusplus
extern "C"
{
#endif

#include "sounds.h"
#include "tankvufo.h"

/* struct containing tank related data */
typedef struct tank_info_t tank_info_t;

tank_info_t *tank_initialize(WINDOW *window, sound_data_t *sound_data);

/* movement and position */
void tank_move(tank_info_t *tank);
void tank_set_direction(tank_info_t *tank, const direction_t direction);
uint8_t tank_get_pos(const tank_info_t *tank);
uint8_t tank_get_ufo_score(const tank_info_t *tank);

/* tank shot movement and position */
void tank_shot_move(tank_info_t *tank);
bool tank_took_shot(const tank_info_t *tank);
pos_t tank_get_shot_pos(const tank_info_t *tank);
void tank_set_shot_pos(tank_info_t *tank, const int8_t x, const int8_t y);
bool tank_shot_hit(const tank_info_t *tank);
void tank_set_shot_hit(tank_info_t *tank, const bool hit);

/* flaming status */
bool tank_is_on_fire(const tank_info_t *tank);
void tank_set_on_fire(tank_info_t *tank, const bool on_fire);

sound_data_t* tank_sound_data(const tank_info_t *tank);

#ifdef __cplusplus
}
#endif

#endif /* ndef  __TANKVUFO_H */
