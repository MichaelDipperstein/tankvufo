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

/* struct containing ufo related data */
struct ufo_info_t
{
    pos_t pos;                  /* column and row containing the ufo */
    direction_t direction;      /* direction that the UFO is moving */
    uint8_t ufo_hit_ground;     /* 0 when not on fire, otherwise flame count */
    pos_t shot_pos;             /* x and y coordinate of ufo shot */
    direction_t shot_direction; /* direction the ufo shot is moving */
    uint8_t shot_hit_ground;    /* 0 if false, otherwise phase of explosion */
    uint8_t number_died;        /* number of ufos that died (tank score) */
    WINDOW *win;                /* ncurses window for the ufo and its shot */
    sound_data_t *sound_data;
};


/* cchar_t for unicode charaters used in this file */
static const cchar_t GROUND_CHAR = {WA_NORMAL, L"▔", 0};
static const cchar_t UFO_SHOT_CHAR = {WA_NORMAL, L"●", 0};

ufo_info_t *ufo_initialize(WINDOW *window, sound_data_t *sound_data)
{
    ufo_info_t *ufo;
    ufo = (ufo_info_t *)malloc(sizeof(struct ufo_info_t));

    if (NULL == ufo)
    {
        return NULL;    /* allocation failed */
    }

    /* start without a ufo and no shot */
    ufo->pos.x = 0;
    ufo->pos.y = 0;
    ufo->direction = DIR_NONE;
    ufo->ufo_hit_ground = 0;
    ufo->shot_pos.x = -1;
    ufo->shot_pos.y = -1;
    ufo->shot_direction = DIR_NONE;
    ufo->shot_hit_ground = 0;
    ufo->number_died = 0;
    ufo->win = window;
    ufo->sound_data = sound_data;

    srand((unsigned int)time(NULL));    /* seed the random number generator */

    return ufo;
}


void ufo_move(ufo_info_t *ufo)
{
    WINDOW *win;

    win = ufo->win;

    switch (ufo->direction)
    {
        case DIR_NONE:
            if (DIR_NONE != ufo->shot_direction)
            {
                /* a shot is still falling, hold off */
                break;
            }

            /* no ufo or shot make a ufo */
            ufo->pos.y = UFO_TOP + (rand() % (UFO_BOTTOM - UFO_TOP));

            if (rand() % 2)
            {
                /* start on left */
                ufo->pos.x = 0;
                ufo->direction = DIR_RIGHT;
            }
            else
            {
                /* start on right */
                ufo->pos.x = V20_COLS - 4;
                ufo->direction = DIR_LEFT;
            }

            ufo_shot_decision(ufo);
            mvwaddstr(win, ufo->pos.y, ufo->pos.x, "<*>");
            break;

        case DIR_RIGHT:
            /* ufo is moving right */
            if ((UFO_BOTTOM == ufo->pos.y) && (V20_COLS - 3 == ufo->pos.x))
            {
                /* we're at the bottom , done with this one */
                mvwaddstr(win, ufo->pos.y, ufo->pos.x, "   ");
                ufo->pos.x = 0;
                ufo->pos.y = 0;
                ufo->direction = DIR_NONE;
            }
            else if (V20_COLS == ufo->pos.x)
            {
                /* ufo is at the edge, remove old ufo */
                mvwaddstr(win, ufo->pos.y, ufo->pos.x, "   ");

                /* go down one row */
                ufo->pos.y++;
                ufo->pos.x = 0;
                mvwaddstr(win, ufo->pos.y, ufo->pos.x, "<*>");
            }
            else
            {
                /* normal right move */
                mvwaddstr(win, ufo->pos.y, ufo->pos.x, " <*>");
                ufo->pos.x++;
            }

            ufo_shot_decision(ufo);
            break;

        case DIR_LEFT:
            /* ufo is moving left */
            if (2 == ufo->pos.x)
            {
                /* ufo is at the left edge, remove old ufo */
                mvwaddstr(win, ufo->pos.y, ufo->pos.x, "   ");

                /* go up a row */
                ufo->pos.y--;

                if (UFO_TOP > ufo->pos.y)
                {
                    /* we're at the top, done with this one */
                    ufo->pos.x = 0;
                    ufo->pos.y = 0;
                    ufo->direction = DIR_NONE;
                }
                else
                {
                    /* wrap around */
                    ufo->pos.x = V20_COLS - 2;
                    mvwaddstr(win, ufo->pos.y, ufo->pos.x, "<*>");
                }
            }
            else
            {
                /* normal left move */
                ufo->pos.x--;
                mvwaddstr(win, ufo->pos.y, ufo->pos.x, "<*> ");
            }

            ufo_shot_decision(ufo);
            break;

        case DIR_FALLING_RIGHT:
            /* ufo is falling right, remove old ufo */
            mvwaddstr(win, ufo->pos.y, ufo->pos.x, "   ");

            if (V20_COLS == ufo->pos.x)
            {
                /* go down one row and start at left */
                ufo->pos.x = 0;
                ufo->pos.y++;
                mvwaddstr(win, ufo->pos.y, ufo->pos.x, "<*>");
            }
            else
            {
                /* normal falling right move */
                ufo->pos.x++;
                ufo->pos.y++;
                mvwaddstr(win, ufo->pos.y, ufo->pos.x, "<*>");
            }

            if (TANK_TREAD_ROW == ufo->pos.y)
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
            mvwaddstr(win, ufo->pos.y, ufo->pos.x, "   ");

            if (2 == ufo->pos.x)
            {
                /* ufo is at the left edge, wrap around */
                ufo->pos.x = V20_COLS - 2;
                ufo->pos.y++;
                mvwaddstr(win, ufo->pos.y, ufo->pos.x, "<*>");
            }
            else
            {
                /* normal left move */
                ufo->pos.x--;
                ufo->pos.y++;
                mvwaddstr(win, ufo->pos.y, ufo->pos.x, "<*>");
            }

            if (TANK_TREAD_ROW == ufo->pos.y)
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
                mvwaddstr(win, ufo->pos.y - 1, ufo->pos.x, "   ");
                mvwaddstr(win, ufo->pos.y, ufo->pos.x, "   ");

                /* redraw the ground */
                mvwhline_set(win, V20_ROWS - 1, 0, &GROUND_CHAR, V20_COLS);

                /* stop the fire sound */
                select_sound(ufo->sound_data, SOUND_OFF);

                ufo->number_died += 1;      /* credit tank with kill */
            }
            else
            {
                ufo->ufo_hit_ground += 1;
                wattron(win, COLOR_PAIR(3));       /* fire color */

                if (ufo->ufo_hit_ground % 2)
                {
                    mvwaddstr(win, ufo->pos.y - 1, ufo->pos.x, "◣◣◣");
                }
                else
                {
                    mvwaddstr(win, ufo->pos.y - 1, ufo->pos.x, "◢◢◢");
                }

                wattroff(win, COLOR_PAIR(3));
            }
            break;

        default:
            break;      /* this shouldn't happen */
    }

    wrefresh(win);
}


pos_t ufo_get_pos(const ufo_info_t *ufo)
{
    return ufo->pos;
}


uint8_t ufo_get_tank_score(const ufo_info_t *ufo)
{
    return ufo->number_died;
}


sound_error_t ufo_set_falling(ufo_info_t *ufo)
{
    sound_error_t sound_error;

    if (DIR_LEFT == ufo->direction)
    {
        ufo->direction = DIR_FALLING_LEFT;
    }
    else if (DIR_RIGHT == ufo->direction)
    {
        ufo->direction = DIR_FALLING_RIGHT;
    }

    /* start the ufo falling sound */
    next_ufo_sound(ufo->sound_data);
    sound_error = restart_sound_stream(ufo->sound_data);

    return sound_error;
}


void ufo_shot_decision(ufo_info_t *ufo)
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
        if (ufo->pos.x + ufo->pos.y < 23)
        {
            return;
        }
    }
    else if (DIR_RIGHT == ufo->direction)
    {
        /* going right */
        if (ufo->pos.x - ufo->pos.y > -1)
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
        /* prime the position for ufo_move_shot()  */
        if (DIR_RIGHT == ufo->direction)
        {
            /* shot will head right */
            ufo->shot_direction = DIR_FALLING_RIGHT;
            ufo->shot_pos.x = ufo->pos.x;
        }
        else
        {
            /* shot will head left */
            ufo->shot_direction = DIR_FALLING_LEFT;
            ufo->shot_pos.x = ufo->pos.x + 2;
        }

        ufo->shot_pos.y = ufo->pos.y;           /* UFO row */
    }
}


void ufo_move_shot(ufo_info_t *ufo)
{
    cchar_t c;
    WINDOW *win;

    if ((DIR_NONE == ufo->shot_direction) || (0 != ufo->shot_hit_ground))
    {
        /* there is no shot to move */
        return;
    }

    win = ufo->win;

    /* erase old shot if it hasn't been overwritten */
    mvwin_wch(win, ufo->shot_pos.y, ufo->shot_pos.x, &c);

    if (UFO_SHOT_CHAR.chars[0] == c.chars[0])
    {
        mvwaddch(win, ufo->shot_pos.y, ufo->shot_pos.x, ' ');
    }

    if (TANK_TREAD_ROW == ufo->shot_pos.y)
    {
        /* done with shot */
        ufo->shot_direction = DIR_NONE;
        ufo->shot_hit_ground = 1;
        wrefresh(win);
        return;
    }

    /* update shot position */
    ufo->shot_pos.y++;

    if (DIR_FALLING_RIGHT == ufo->shot_direction)
    {
        /* shot is headed right */
        ufo->shot_pos.x++;
    }
    else if (DIR_FALLING_LEFT == ufo->shot_direction)
    {
        /* shot is headed left */
        ufo->shot_pos.x--;
    }

    /* draw the new shot */
    mvwadd_wch(win, ufo->shot_pos.y, ufo->shot_pos.x, &UFO_SHOT_CHAR);
    wrefresh(win);
}


pos_t ufo_get_shot_pos(const ufo_info_t *ufo)
{
    return ufo->shot_pos;
}


void ufo_clear_shot(ufo_info_t *ufo, bool erase)
{
    if (erase && (-1 != ufo->shot_pos.y))
    {
        /* erase any existing shot */
        cchar_t c;

        /* erase old shot if it hasn't been overwritten */
        mvwin_wch(ufo->win, ufo->shot_pos.y, ufo->shot_pos.x, &c);

        if (UFO_SHOT_CHAR.chars[0] == c.chars[0])
        {
            mvwaddch(ufo->win, ufo->shot_pos.y, ufo->shot_pos.x, ' ');
        }
    }

    /* clear shot data */
    ufo->shot_pos.x = -1;
    ufo->shot_pos.y = -1;
    ufo->shot_direction = DIR_NONE;
}


bool ufo_shot_is_falling(const ufo_info_t *ufo)
{
    return (DIR_NONE != ufo->shot_direction);
}


bool ufo_shot_is_exploding(const ufo_info_t *ufo)
{
    return (0 != ufo->shot_hit_ground);
}


int ufo_shot_hit_ground(ufo_info_t *ufo)
{
    int clean_up = 0;
    WINDOW *win;
    int8_t shot_x;

    win = ufo->win;
    shot_x = ufo->shot_pos.x;

    switch (ufo->shot_hit_ground)
    {
        case 1:
            /* just lines */
            ufo->shot_hit_ground++;
            mvwaddstr(win, TANK_TREAD_ROW, shot_x - 2, "╲ │ ╱");
            break;

        case 2:
            /* full explosion */
            ufo->shot_hit_ground++;
            mvwaddstr(win, TANK_TURRET_ROW, shot_x - 3, "•• • ••");
            mvwaddstr(win, TANK_TREAD_ROW, shot_x - 2, "╲ │ ╱");
            break;

        case 3:
            /* dots */
            ufo->shot_hit_ground++;
            mvwaddstr(win, TANK_TURRET_ROW, shot_x - 3, "•• • ••");
            mvwaddstr(win, TANK_TREAD_ROW, shot_x - 2, "     ");
            break;

        case 4:
            /* clean-up */
            ufo->shot_hit_ground = 0;
            mvwaddstr(win, TANK_TURRET_ROW, shot_x - 3, "       ");
            mvwaddstr(win, TANK_TREAD_ROW, shot_x - 2, "     ");
            ufo->shot_pos.x = -1;
            ufo->shot_pos.y = -1;
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
