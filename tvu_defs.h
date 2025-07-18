/***************************************************************************
*                              Tank Versus UFO
*
*   File    : tvu_defs.h
*   Purpose : definitions common to the game
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
#ifndef  __TVU_DEFS_H
#define  __TVU_DEFS_H

/* forward declarations */
class tank_t;
class ufo_t;
typedef struct sound_data_t sound_data_t;

namespace Tvu
{
    /* movement directions used by tank, ufo, and ufo shots */
    typedef enum
    {
        DIR_NONE,
        DIR_LEFT,
        DIR_RIGHT,
        DIR_FALLING_LEFT,
        DIR_FALLING_RIGHT,
        DIR_LANDED
    } Direction;

    typedef struct
    {
        int8_t x;
        int8_t y;
    } Pos;

    /* vic-20 screen dimensions */
    constexpr int V20_COLS = 22;
    constexpr int V20_ROWS = 23;

    /* score is written to this row */
    constexpr int SCORE_ROW = 2;

    /* rows containing tank animation */
    constexpr int TANK_TREAD_ROW = V20_ROWS - 2;
    constexpr int TANK_TURRET_ROW = TANK_TREAD_ROW - 1;
    constexpr int TANK_GUN_ROW = TANK_TURRET_ROW - 1;
    constexpr int TANK_SHOT_START_ROW = TANK_GUN_ROW - 1;

    /* lowest and highest rows of ufo travel */
    constexpr int UFO_BOTTOM = TANK_SHOT_START_ROW - 2;
    constexpr int UFO_TOP = SCORE_ROW + 2;

    /* volume control window dimensions and positions */
    constexpr int VOL_COLS = 10;
    constexpr int VOL_ROWS = 13;
}

#endif /* ndef  __TVU_DEFS_H */
