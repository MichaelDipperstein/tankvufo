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
#ifndef  __UFO_H
#define  __UFO_H

#include "sounds.h"
#include "tvu_defs.h"

class Ufo
{
    public:
        Ufo(WINDOW *window, int upperLim, int lowerLim, sound_data_t *sd);

        /* ufo shot movement and information */
        void Move(void);
        Tvu::Pos GetPos(void) const;
        uint8_t GetUfosKilled(void) const;

        /* start falling direction and sound */
        sound_error_t SetFalling(void);

        /* ufo shot movement and information */
        void MoveShot(void);
        Tvu::Pos GetShotPos(void) const;
        void ClearShot(bool erase);
        bool IsShotFalling(void) const;
        bool IsShotExploding(void) const;
        int UpdateShotPhase(void);

    private:
        Tvu::Pos pos;               /* column and row containing the ufo */
        int upperLimit;             /* top of ufo travel */
        int lowerLimit;             /* bottom of ufo travel */
        Tvu::Direction direction;   /* direction that the UFO is moving */
        uint8_t ufoHitGround;       /* 0 when not on fire, otherwise flame count */
        Tvu::Pos shotPos;           /* x and y coordinate of ufo shot */
        Tvu::Direction shotDirection; /* direction the ufo shot is moving */
        uint8_t shotHitGround;      /* 0 if false, otherwise phase of explosion */
        uint8_t numberDied;         /* number of ufos that died (tank score) */
        WINDOW *win;                /* ncurses window for the ufo and its shot */
        int rows;                   /* window rows */
        int cols;                   /* window columns */
        sound_data_t *soundData;

        void UfoShotDecision(void);
};

#endif /* ndef  __UFO_H */
