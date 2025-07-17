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
#include <ncurses.h>
#include <stdlib.h>
#include <time.h>

#include "ufo.h"

/* cchar_t for unicode charaters used in this file */
static const cchar_t GROUND_CHAR = {WA_NORMAL, L"▔", 0};
static const cchar_t UFO_SHOT_CHAR = {WA_NORMAL, L"●", 0};

ufo_t::ufo_t(WINDOW *window, int upper_lim, int lower_lim, sound_data_t *sd)
{
    /* start without a ufo and no shot */
    pos.x = 0;
    pos.y = 0;
    upper_limit = upper_lim;
    lower_limit = lower_lim;
    direction = DIR_NONE;
    ufo_hit_ground = 0;
    shot_pos.x = -1;
    shot_pos.y = -1;
    shot_direction = DIR_NONE;
    shot_hit_ground = 0;
    number_died = 0;
    win = window;
    getmaxyx(window, rows, cols);
    sound_data = sd;

    srand((unsigned int)time(NULL));    /* seed the random number generator */
}


void ufo_t::move(void)
{
    switch (direction)
    {
        case DIR_NONE:
            if (DIR_NONE != shot_direction)
            {
                /* a shot is still falling, hold off */
                break;
            }

            /* no ufo or shot make a ufo */
            pos.y = upper_limit + (rand() % (lower_limit - upper_limit));

            if (rand() % 2)
            {
                /* start on left */
                pos.x = 0;
                direction = DIR_RIGHT;
            }
            else
            {
                /* start on right */
                pos.x = cols - 4;
                direction = DIR_LEFT;
            }

            ufo_shot_decision();
            mvwaddstr(win, pos.y, pos.x, "<*>");
            break;

        case DIR_RIGHT:
            /* ufo is moving right */
            if ((lower_limit == pos.y) && (cols - 3 == pos.x))
            {
                /* we're at the bottom , done with this one */
                mvwaddstr(win, pos.y, pos.x, "   ");
                pos.x = 0;
                pos.y = 0;
                direction = DIR_NONE;
            }
            else if (cols == pos.x)
            {
                /* ufo is at the edge, remove old ufo */
                mvwaddstr(win, pos.y, pos.x, "   ");

                /* go down one row */
                pos.y++;
                pos.x = 0;
                mvwaddstr(win, pos.y, pos.x, "<*>");
            }
            else
            {
                /* normal right move */
                mvwaddstr(win, pos.y, pos.x, " <*>");
                pos.x++;
            }

            ufo_shot_decision();
            break;

        case DIR_LEFT:
            /* ufo is moving left */
            if (2 == pos.x)
            {
                /* ufo is at the left edge, remove old ufo */
                mvwaddstr(win, pos.y, pos.x, "   ");

                /* go up a row */
                pos.y--;

                if (upper_limit > pos.y)
                {
                    /* we're at the top, done with this one */
                    pos.x = 0;
                    pos.y = 0;
                    direction = DIR_NONE;
                }
                else
                {
                    /* wrap around */
                    pos.x = cols - 2;
                    mvwaddstr(win, pos.y, pos.x, "<*>");
                }
            }
            else
            {
                /* normal left move */
                pos.x--;
                mvwaddstr(win, pos.y, pos.x, "<*> ");
            }

            ufo_shot_decision();
            break;

        case DIR_FALLING_RIGHT:
            /* ufo is falling right, remove old ufo */
            mvwaddstr(win, pos.y, pos.x, "   ");

            if (cols == pos.x)
            {
                /* go down one row and start at left */
                pos.x = 0;
                pos.y++;
                mvwaddstr(win, pos.y, pos.x, "<*>");
            }
            else
            {
                /* normal falling right move */
                pos.x++;
                pos.y++;
                mvwaddstr(win, pos.y, pos.x, "<*>");
            }

            if (RowsAndCols::TANK_TREAD_ROW == pos.y)
            {
                /* we're at the bottom, done with this one */
                direction = DIR_LANDED;
                ufo_hit_ground = 0;
                select_sound(sound_data, SOUND_ON_FIRE);
            }
            else
            {
                next_ufo_sound(sound_data);
            }
            break;

        case DIR_FALLING_LEFT:
            /* ufo is falling left, remove old ufo */
            mvwaddstr(win, pos.y, pos.x, "   ");

            if (2 == pos.x)
            {
                /* ufo is at the left edge, wrap around */
                pos.x = cols - 2;
                pos.y++;
                mvwaddstr(win, pos.y, pos.x, "<*>");
            }
            else
            {
                /* normal left move */
                pos.x--;
                pos.y++;
                mvwaddstr(win, pos.y, pos.x, "<*>");
            }

            if (RowsAndCols::TANK_TREAD_ROW == pos.y)
            {
                /* we're at the bottom, done with this one */
                direction = DIR_LANDED;
                ufo_hit_ground = 0;
                select_sound(sound_data, SOUND_ON_FIRE);
            }
            else
            {
                next_ufo_sound(sound_data);
            }
            break;

        case DIR_LANDED:
            if (10 == ufo_hit_ground)
            {
                ufo_hit_ground = 0;
                direction = DIR_NONE;
                mvwaddstr(win, pos.y - 1, pos.x, "   ");
                mvwaddstr(win, pos.y, pos.x, "   ");

                /* redraw the ground */
                mvwhline_set(win, rows - 1, 0, &GROUND_CHAR, cols);

                /* stop the fire sound */
                select_sound(sound_data, SOUND_OFF);

                number_died += 1;      /* credit tank with kill */
            }
            else
            {
                ufo_hit_ground += 1;
                wattron(win, COLOR_PAIR(3));       /* fire color */

                if (ufo_hit_ground % 2)
                {
                    mvwaddstr(win, pos.y - 1, pos.x, "◣◣◣");
                }
                else
                {
                    mvwaddstr(win, pos.y - 1, pos.x, "◢◢◢");
                }

                wattroff(win, COLOR_PAIR(3));
            }
            break;

        default:
            break;      /* this shouldn't happen */
    }

    wrefresh(win);
}


pos_t ufo_t::get_pos(void) const
{
    return pos;
}


uint8_t ufo_t::get_ufos_killed(void) const
{
    return number_died;
}


sound_error_t ufo_t::set_falling(void)
{
    sound_error_t sound_error;

    if (DIR_LEFT == direction)
    {
        direction = DIR_FALLING_LEFT;
    }
    else if (DIR_RIGHT == direction)
    {
        direction = DIR_FALLING_RIGHT;
    }

    /* start the ufo falling sound */
    next_ufo_sound(sound_data);
    sound_error = restart_sound_stream(sound_data);

    return sound_error;
}


void ufo_t::ufo_shot_decision(void)
{
    if ((DIR_NONE != shot_direction) || (shot_hit_ground))
    {
        /* there's already a shot */
        return;
    }

    /* don't take a shot that will go over the edge */
    if (DIR_LEFT == direction)
    {
        /* going left */
        if (pos.x + pos.y < 23)
        {
            return;
        }
    }
    else if (DIR_RIGHT == direction)
    {
        /* going right */
        if (pos.x - pos.y > -1)
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
        /* prime the position for move_shot()  */
        if (DIR_RIGHT == direction)
        {
            /* shot will head right */
            shot_direction = DIR_FALLING_RIGHT;
            shot_pos.x = pos.x;
        }
        else
        {
            /* shot will head left */
            shot_direction = DIR_FALLING_LEFT;
            shot_pos.x = pos.x + 2;
        }

        shot_pos.y = pos.y;           /* UFO row */
    }
}


void ufo_t::move_shot(void)
{
    cchar_t c;

    if ((DIR_NONE == shot_direction) || (0 != shot_hit_ground))
    {
        /* there is no shot to move */
        return;
    }

    /* erase old shot if it hasn't been overwritten */
    mvwin_wch(win, shot_pos.y, shot_pos.x, &c);

    if (UFO_SHOT_CHAR.chars[0] == c.chars[0])
    {
        mvwaddch(win, shot_pos.y, shot_pos.x, ' ');
    }

    if (RowsAndCols::TANK_TREAD_ROW == shot_pos.y)
    {
        /* done with shot */
        shot_direction = DIR_NONE;
        shot_hit_ground = 1;
        wrefresh(win);
        return;
    }

    /* update shot position */
    shot_pos.y++;

    if (DIR_FALLING_RIGHT == shot_direction)
    {
        /* shot is headed right */
        shot_pos.x++;
    }
    else if (DIR_FALLING_LEFT == shot_direction)
    {
        /* shot is headed left */
        shot_pos.x--;
    }

    /* draw the new shot */
    mvwadd_wch(win, shot_pos.y, shot_pos.x, &UFO_SHOT_CHAR);
    wrefresh(win);
}


pos_t ufo_t::get_shot_pos(void) const
{
    return shot_pos;
}


void ufo_t::clear_shot(bool erase)
{
    if (erase && (-1 != shot_pos.y))
    {
        /* erase any existing shot */
        cchar_t c;

        /* erase old shot if it hasn't been overwritten */
        mvwin_wch(win, shot_pos.y, shot_pos.x, &c);

        if (UFO_SHOT_CHAR.chars[0] == c.chars[0])
        {
            mvwaddch(win, shot_pos.y, shot_pos.x, ' ');
        }
    }

    /* clear shot data */
    shot_pos.x = -1;
    shot_pos.y = -1;
    shot_direction = DIR_NONE;
}


bool ufo_t::is_shot_falling(void) const
{
    return (DIR_NONE != shot_direction);
}


bool ufo_t::is_shot_exploding(void) const
{
    return (0 != shot_hit_ground);
}


int ufo_t::update_shot_phase(void)
{
    int clean_up = 0;
    int8_t shot_x;

    shot_x = shot_pos.x;

    switch (shot_hit_ground)
    {
        case 1:
            /* just lines */
            shot_hit_ground++;
            mvwaddstr(win, RowsAndCols::TANK_TREAD_ROW, shot_x - 2, "╲ │ ╱");
            break;

        case 2:
            /* full explosion */
            shot_hit_ground++;
            mvwaddstr(win, RowsAndCols::TANK_TURRET_ROW, shot_x - 3, "•• • ••");
            mvwaddstr(win, RowsAndCols::TANK_TREAD_ROW, shot_x - 2, "╲ │ ╱");
            break;

        case 3:
            /* dots */
            shot_hit_ground++;
            mvwaddstr(win, RowsAndCols::TANK_TURRET_ROW, shot_x - 3, "•• • ••");
            mvwaddstr(win, RowsAndCols::TANK_TREAD_ROW, shot_x - 2, "     ");
            break;

        case 4:
            /* clean-up */
            shot_hit_ground = 0;
            mvwaddstr(win, RowsAndCols::TANK_TURRET_ROW, shot_x - 3, "       ");
            mvwaddstr(win, RowsAndCols::TANK_TREAD_ROW, shot_x - 2, "     ");
            shot_pos.x = -1;
            shot_pos.y = -1;
            shot_direction = DIR_NONE;

            /* indicate need to redraw ground and tank */
            clean_up = 1;
            break;

        default:
            break;
    }

    wrefresh(win);
    return clean_up;
}
