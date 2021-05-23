/***************************************************************************
*                              Tank Versus UFO
*
*   File    : ufo.c
*   Purpose : UFO movement functions
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
#include <time.h>

#include "ufo.h"

/* cchar_t for unicode charaters used in this file */
static const cchar_t GROUND_CHAR = {WA_NORMAL, L"▔", 0};
static const cchar_t UFO_SHOT_CHAR = {WA_NORMAL, L"●", 0};

void initialize_ufo(sound_data_t *sound_data, ufo_info_t *ufo)
{
    /* start without a ufo and no shot */
    ufo->x = 0;
    ufo->y = 0;
    ufo->direction = DIR_NONE;
    ufo->ufo_hit_ground = 0;
    ufo->shot_x = 0;
    ufo->shot_y = 0;
    ufo->shot_direction = DIR_NONE;
    ufo->shot_hit_ground = 0;
    ufo->sound_data = sound_data;

    srand((unsigned int)time(NULL));    /* seed the random number generator */
}


uint8_t move_ufo(WINDOW* win, ufo_info_t *ufo)
{
    uint8_t score;
    score = 0;

    switch (ufo->direction)
    {
        case DIR_NONE:
            if (DIR_NONE != ufo->shot_direction)
            {
                /* a shot is still falling, hold off */
                break;
            }

            /* no ufo or shot make a ufo */
            ufo->y = UFO_TOP + (rand() % (UFO_BOTTOM - UFO_TOP));

            if (rand() % 2)
            {
                /* start on left */
                ufo->x = 0;
                ufo->direction = DIR_RIGHT;
            }
            else
            {
                /* start on right */
                ufo->x = V20_COLS - 4;
                ufo->direction = DIR_LEFT;
            }

            make_ufo_shot(ufo);
            mvwaddstr(win, ufo->y, ufo->x, "<*>");
            break;

        case DIR_RIGHT:
            /* ufo is moving right */
            if ((UFO_BOTTOM == ufo->y) && (V20_COLS - 3 == ufo->x))
            {
                /* we're at the bottom , done with this one */
                mvwaddstr(win, ufo->y, ufo->x, "   ");
                ufo->x = 0;
                ufo->y = 0;
                ufo->direction = DIR_NONE;
            }
            else if (V20_COLS == ufo->x)
            {
                /* ufo is at the edge, remove old ufo */
                mvwaddstr(win, ufo->y, ufo->x, "   ");

                /* go down one row */
                ufo->y++;
                ufo->x = 0;
                mvwaddstr(win, ufo->y, ufo->x, "<*>");
            }
            else
            {
                /* normal right move */
                mvwaddstr(win, ufo->y, ufo->x, " <*>");
                ufo->x++;
            }

            make_ufo_shot(ufo);
            break;

        case DIR_LEFT:
            /* ufo is moving left */
            if (2 == ufo->x)
            {
                /* ufo is at the left edge, remove old ufo */
                mvwaddstr(win, ufo->y, ufo->x, "   ");

                /* go up a row */
                ufo->y--;

                if (UFO_TOP > ufo->y)
                {
                    /* we're at the top, done with this one */
                    ufo->x = 0;
                    ufo->y = 0;
                    ufo->direction = DIR_NONE;
                }
                else
                {
                    /* wrap around */
                    ufo->x = V20_COLS - 2;
                    mvwaddstr(win, ufo->y, ufo->x, "<*>");
                }
            }
            else
            {
                /* normal left move */
                ufo->x--;
                mvwaddstr(win, ufo->y, ufo->x, "<*> ");
            }

            make_ufo_shot(ufo);
            break;

        case DIR_FALLING_RIGHT:
            /* ufo is falling right, remove old ufo */
            mvwaddstr(win, ufo->y, ufo->x, "   ");

            if (V20_COLS == ufo->x)
            {
                /* go down one row and start at left */
                ufo->x = 0;
                ufo->y++;
                mvwaddstr(win, ufo->y, ufo->x, "<*>");
            }
            else
            {
                /* normal falling right move */
                ufo->x++;
                ufo->y++;
                mvwaddstr(win, ufo->y, ufo->x, "<*>");
            }

            if (TANK_TREAD_ROW == ufo->y)
            {
                /* we're at the bottom, done with this one */
                ufo->direction = DIR_LANDED;
                ufo->ufo_hit_ground = 0;
                select_sound(ufo->sound_data, SOUND_ON_FIRE);
            }
            else
            {
                next_ufo_sound(ufo->sound_data);
            }
            break;

        case DIR_FALLING_LEFT:
            /* ufo is falling left, remove old ufo */
            mvwaddstr(win, ufo->y, ufo->x, "   ");

            if (2 == ufo->x)
            {
                /* ufo is at the left edge, wrap around */
                ufo->x = V20_COLS - 2;
                ufo->y++;
                mvwaddstr(win, ufo->y, ufo->x, "<*>");
            }
            else
            {
                /* normal left move */
                ufo->x--;
                ufo->y++;
                mvwaddstr(win, ufo->y, ufo->x, "<*>");
            }

            if (TANK_TREAD_ROW == ufo->y)
            {
                /* we're at the bottom, done with this one */
                ufo->direction = DIR_LANDED;
                ufo->ufo_hit_ground = 0;
                select_sound(ufo->sound_data, SOUND_ON_FIRE);
            }
            else
            {
                next_ufo_sound(ufo->sound_data);
            }
            break;

        case DIR_LANDED:
            if (10 == ufo->ufo_hit_ground)
            {
                ufo->ufo_hit_ground = 0;
                ufo->direction = DIR_NONE;
                mvwaddstr(win, ufo->y - 1, ufo->x, "   ");
                mvwaddstr(win, ufo->y, ufo->x, "   ");

                /* redraw the ground */
                mvwhline_set(win, V20_ROWS - 1, 0, &GROUND_CHAR, V20_COLS);

                /* stop the fire sound */
                select_sound(ufo->sound_data, SOUND_OFF);

                score = 1;          /* credit tank with kill */
            }
            else
            {
                ufo->ufo_hit_ground += 1;
                wattron(win, COLOR_PAIR(3));       /* fire color */

                if (ufo->ufo_hit_ground % 2)
                {
                    mvwaddstr(win, ufo->y - 1, ufo->x, "◣◣◣");
                }
                else
                {
                    mvwaddstr(win, ufo->y - 1, ufo->x, "◢◢◢");
                }

                wattroff(win, COLOR_PAIR(3));
            }
            break;

        default:
            break;      /* this shouldn't happen */
    }

    wrefresh(win);
    return score;
}


void make_ufo_shot(ufo_info_t *ufo)
{
    if ((DIR_NONE != ufo->shot_direction) || (ufo->shot_hit_ground))
    {
        /* there's already a shot */
        return;
    }

    /* don't take a shot that will go over the edge */
    if (DIR_LEFT == ufo->direction)
    {
        /* going left */
        if (ufo->x + ufo->y < 23)
        {
            return;
        }
    }
    else if (DIR_RIGHT == ufo->direction)
    {
        /* going right */
        if (ufo->x - ufo->y > -1)
        {
            return;
        }
    }
    else
    {
        /* we shouldn't be here */
        return;
    }

    /* 1 in 3 chance of non-shooter to shoot */
    if (0 == (rand() % 3))
    {
        /* prime the position for move_ufo_shot()  */
        if (DIR_RIGHT == ufo->direction)
        {
            /* shot will head right */
            ufo->shot_direction = DIR_FALLING_RIGHT;
            ufo->shot_x = ufo->x;
        }
        else
        {
            /* shot will head left */
            ufo->shot_direction = DIR_FALLING_LEFT;
            ufo->shot_x = ufo->x + 2;
        }

        ufo->shot_y = ufo->y;           /* UFO row */
    }
}


void move_ufo_shot(WINDOW* win, ufo_info_t *ufo)
{
    cchar_t c;

    /* erase old shot if it hasn't been overwritten */
    mvwin_wch(win, ufo->shot_y, ufo->shot_x, &c);

    if (UFO_SHOT_CHAR.chars[0] == c.chars[0])
    {
        mvwaddch(win, ufo->shot_y, ufo->shot_x, ' ');
    }

    if (TANK_TREAD_ROW == ufo->shot_y)
    {
        /* done with shot */
        ufo->shot_direction = DIR_NONE;
        ufo->shot_hit_ground = 1;
        wrefresh(win);
        return;
    }

    /* update shot position */
    ufo->shot_y++;

    if (DIR_FALLING_RIGHT == ufo->shot_direction)
    {
        /* shot is headed right */
        ufo->shot_x++;
    }
    else if (DIR_FALLING_LEFT == ufo->shot_direction)
    {
        /* shot is headed left */
        ufo->shot_x--;
    }

    /* draw the new shot */
    mvwadd_wch(win, ufo->shot_y, ufo->shot_x, &UFO_SHOT_CHAR);
    wrefresh(win);
}


int ufo_shot_hit_ground(WINDOW* win, ufo_info_t *ufo)
{
    int clean_up = 0;

    switch (ufo->shot_hit_ground)
    {
        case 1:
            /* just lines */
            ufo->shot_hit_ground++;
            mvwaddstr(win, TANK_TREAD_ROW, ufo->shot_x - 2, "╲ │ ╱");
            break;

        case 2:
            /* full explosion */
            ufo->shot_hit_ground++;
            mvwaddstr(win, TANK_TURRET_ROW, ufo->shot_x - 3, "•• • ••");
            mvwaddstr(win, TANK_TREAD_ROW, ufo->shot_x - 2, "╲ │ ╱");
            break;

        case 3:
            /* dots */
            ufo->shot_hit_ground++;
            mvwaddstr(win, TANK_TURRET_ROW, ufo->shot_x - 3, "•• • ••");
            mvwaddstr(win, TANK_TREAD_ROW, ufo->shot_x - 2, "     ");
            break;

        case 4:
            /* clean-up */
            ufo->shot_hit_ground = 0;
            mvwaddstr(win, TANK_TURRET_ROW, ufo->shot_x - 3, "       ");
            mvwaddstr(win, TANK_TREAD_ROW, ufo->shot_x - 2, "     ");
            ufo->shot_x = -1;
            ufo->shot_y = -1;
            ufo->shot_direction = DIR_NONE;

            /* indicate need to redraw ground and tank */
            clean_up = 1;
            break;

        default:
            break;
    }

    wrefresh(win);
    return clean_up;
}
