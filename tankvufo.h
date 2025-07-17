/***************************************************************************
*                              Tank Versus UFO
*
*   File    : tankvufo.h
*   Purpose : Common definitions used by Tank Versus UFO
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
#ifndef  __TANKVUFO_H
#define  __TANKVUFO_H

/* forward declarations */
class tank_t;
class ufo_t;
typedef struct sound_data_t sound_data_t;

/* movement directions used by tank, ufo, and ufo shots */
typedef enum
{
    DIR_NONE,
    DIR_LEFT,
    DIR_RIGHT,
    DIR_FALLING_LEFT,
    DIR_FALLING_RIGHT,
    DIR_LANDED
} direction_t;

typedef struct
{
    int8_t x;
    int8_t y;
} pos_t;

namespace RowsAndCols
{
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

class TankVUfo
{
    public:
        TankVUfo(void);
        ~TankVUfo(void);

        /* vic-20 window methods */
        bool MakeV20Win(int rows, int cols, int begin_x, int begin_y);
        void InitializeV20Win(void);
        void PrintScore(void);
        void DrawGround(void);

        /* volume window methods */
        bool MakeVolWin(int rows, int cols, int begin_x, int begin_y);
        void DrawVolumeLevelBox(void);
        void ShowVolumeLevel(const float volume);

        bool InitializeVehicles(sound_data_t *sound_data);

        /* move and update objects */
        void MoveTank(void);
        void MoveUfo(void);
        void UpdateTankShot(void);
        void UpdateUfoShot(void);

        int HandleKeyPress(void);
        void Refresh(void) { wrefresh(v20Win); }

    private:
        WINDOW *v20Win;
        int v20Rows;
        int v20Cols;

        WINDOW *volWin;
        int volRows;
        int volCols;

        tank_t *tank;
        ufo_t *ufo;

        void CheckTankShot(void);
        void CheckUfoShot(void);
};

#endif /* ndef  __TANKVUFO_H */
