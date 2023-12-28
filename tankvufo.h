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
#ifndef  __TANKVUFO_H
#define  __TANKVUFO_H

/* forward declarations */
class tank_t;
typedef struct ufo_info_t ufo_info_t;
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

/* constants defined in tankvufo.c */

/* score is written to this row */
extern const int SCORE_ROW;

/* rows containing tank animation */
extern const int TANK_TREAD_ROW;
extern const int TANK_TURRET_ROW;
extern const int TANK_GUN_ROW;
extern const int TANK_SHOT_START_ROW;

/* lowest and highest rows of ufo travel */
extern const int UFO_BOTTOM;
extern const int UFO_TOP;

class tank_v_ufo_t
{
    public:
        tank_v_ufo_t(void);
        ~tank_v_ufo_t(void);

        /* vic-20 window methods */
        bool make_v20_win(int rows, int cols, int begin_x, int begin_y);
        void initialize_v20_win(void);
        void print_score(void);
        void draw_ground(void);

        /* volume window methods */
        bool make_vol_win(int rows, int cols, int begin_x, int begin_y);
        void draw_volume_level_box(void);
        void show_volume_level(const float volume);

        bool initialize_vehicles(sound_data_t *sound_data);

        /* move and update objects */
        void move_tank(void);
        void move_ufo(void);
        void update_tank_shot(void);
        void update_ufo_shot(void);

        int handle_keypress(void);
        void refresh(void) { wrefresh(v20_win); }

    private:
        WINDOW *v20_win;
        int v20_rows;
        int v20_cols;

        WINDOW *vol_win;
        int vol_rows;
        int vol_cols;

        tank_t *tank;
        ufo_info_t *ufo;

        /* cchar_t for unicode charaters used in this file */
        constexpr static cchar_t GROUND_CHAR = {WA_NORMAL, L"▔", 0};
        constexpr static cchar_t BOX_CHAR = {WA_NORMAL, L"█", 0};

        void check_tank_shot(void);
        void check_ufo_shot(void);
};

#endif /* ndef  __TANKVUFO_H */
