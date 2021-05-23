/***************************************************************************
*                              Tank Versus UFO
*
*   File    : tankvufo.c
*   Purpose : A tribute to the Tank-V-UFO, a Commodore VIC-20 Game
*             by Duane Later
*   Author  : Michael Dipperstein
*   Date    : November 26, 2020
*
****************************************************************************
*
* Tank Versus UFO: A tribute to the Tank-V-UFO, a Commodore VIC-20 Game
*                  by Duane Later
*
* Copyright (C) 2020, 2021 by
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
#include <locale.h>
#include <ncurses.h>
#include <stdlib.h>
#include <string.h>
#include <sys/timerfd.h>
#include <sys/poll.h>
#include <unistd.h>
#include <errno.h>

#include "tankvufo.h"
#include "tank.h"
#include "ufo.h"
#include "sounds.h"

/* screen dimensions */
const int V20_COLS = 22;
const int V20_ROWS = 23;

/* score is written to this row */
const int SCORE_ROW = 2;

/* rows containing tank animation */
const int TANK_TREAD_ROW = V20_ROWS - 2;
const int TANK_TURRET_ROW = TANK_TREAD_ROW - 1;
const int TANK_GUN_ROW = TANK_TURRET_ROW - 1;
const int TANK_SHOT_START_ROW = TANK_GUN_ROW - 1;

/* lowest and highest rows of ufo travel */
const int UFO_BOTTOM = TANK_SHOT_START_ROW - 2;
const int UFO_TOP = SCORE_ROW + 2;

/* cchar_t for unicode charaters used in this file */
static const cchar_t GROUND_CHAR = {WA_NORMAL, L"▔", 0};
static const cchar_t UFO_SHOT_CHAR = {WA_NORMAL, L"●", 0};
static const cchar_t BOX_CHAR = {WA_NORMAL, L"█", 0};

static const float VOLUME = 0.5;        /* base volume for sounds */

int handle_keypress(WINDOW* win, tank_info_t *tank);

void check_tank_shot(WINDOW* win, tank_info_t *tank, ufo_info_t *ufo);

void check_ufo_shot(tank_info_t *tank, ufo_info_t *ufo);

void print_score(WINDOW* win, const uint8_t tank_score,
    const uint8_t ufo_score);

int main(void)
{
    WINDOW *v20_win;
    int win_x, win_y;
    tank_info_t *tank_info;
    ufo_info_t ufo_info;
    uint8_t tank_score;
    uint8_t ufo_score;

    /* timer and poll variables */
    int tfd;
    struct itimerspec timeout;
    struct pollfd pfd;

    /* used by sound library */
    sound_data_t sound_data;
    sound_error_t sound_error;

    /* ncurses initialization */
    setlocale(LC_ALL, "");
    initscr();
    start_color();
    cbreak();
    noecho();
    curs_set(0);

    /* color the background before creating a window */
    init_pair(1, COLOR_BLACK, COLOR_CYAN);
    bkgd(COLOR_PAIR(1));

    win_x = (COLS - V20_COLS) / 2;
    win_y =  (LINES - V20_ROWS) / 2;
    v20_win = newwin(V20_ROWS, V20_COLS, win_y, win_x);

    refresh();          /* refresh the whole screen to show the window */

    /* now set the window color scheme */
    init_pair(2, COLOR_BLACK, COLOR_WHITE);
    wbkgd(v20_win, COLOR_PAIR(2));

    /* pair 3 will be for fire */
    init_pair(3, COLOR_RED, COLOR_WHITE);

    /* print the banner */
    wprintw(v20_win, "** TANK VERSUS UFO. **");
    wprintw(v20_win, "Z-LEFT,C-RIGHT,B-FIRE ");
    wprintw(v20_win, "UFO:     TANK:");

    /* draw the ground */
    mvwhline_set(v20_win, V20_ROWS - 1, 0, &GROUND_CHAR, V20_COLS);

    tank_score = 0;
    ufo_score = 0;

    print_score(v20_win, tank_score, ufo_score);    /* 0 - 0 score */
    wtimeout(v20_win, 0);               /* make wgetch non-blocking */

    /* initialize all of the sound stuff */
    sound_error = initialize_sounds();

    if (0 != sound_error)
    {
        delwin(v20_win);
        endwin();
        handle_error(sound_error);
        return sound_error;
    }

    sound_error = create_sound_stream(&sound_data, VOLUME);
    if (0 != sound_error)
    {
        delwin(v20_win);
        endwin();
        handle_error(sound_error);
        return sound_error;
    }

    initialize_ufo(&sound_data, &ufo_info);
    tank_info = tank_initialize(v20_win, &sound_data);

    if (NULL == tank_info)
    {
        delwin(v20_win);
        endwin();
        close_sound_stream(&sound_data);
        end_sounds();
        perror("allocating tank_info");
        return 1;
    }

    tank_move(tank_info);                   /* draw tank */
    wrefresh(v20_win);

    /* create a timer fd that expires every 200ms */
    tfd = timerfd_create(CLOCK_MONOTONIC,0);

    if (tfd <= 0)
    {
        delwin(v20_win);
        endwin();
        close_sound_stream(&sound_data);
        end_sounds();
        free(tank_info);
        perror("creating timerfd");
        return 1;
    }

    /* make the timer timeout in 200ms and repeat */
    timeout.it_value.tv_sec = 0;
    timeout.it_value.tv_nsec = 200 * 1000000;
    timeout.it_interval.tv_sec = 0;
    timeout.it_interval.tv_nsec = 200 * 1000000;

    if (timerfd_settime(tfd, 0, &timeout, 0) != 0)
    {
        delwin(v20_win);
        endwin();
        perror("setting timerfd");
        return 1;
    }

    /* set pollfd for timer read event */
    memset(&pfd, 0, sizeof(pfd));
    pfd.fd = tfd;
    pfd.events = POLLIN;
    pfd.revents = 0;

    /*
     * This is the event loop that makes the game work.
     * The timerfd expires every 200ms and starts the loop.
     * If handle_keypress sees a 'q' or a 'Q' the loop will be
     * exited causing the game to end.
     */
    while (poll(&pfd, 1, -1) > 0)
    {
        if (POLLIN == pfd.revents)
        {
            /* read the timer fd */
            uint64_t elapsed;

            read(tfd, &elapsed, sizeof(elapsed));
        }
        else
        {
            /* something went wrong */
            break;
        }

        if (handle_keypress(v20_win, tank_info) < 0)
        {
            /* we got a quit key */
            break;
        }

        ufo_score += tank_move(tank_info);
        tank_score += move_ufo(v20_win, &ufo_info);

        if (!tank_shot_hit(tank_info))
        {
            tank_shot_move(tank_info);
        }

        if ((DIR_NONE != ufo_info.shot_direction) &&
            (0 == ufo_info.shot_hit_ground))
        {
            /* shot is exists and not exploding */
            move_ufo_shot(v20_win, &ufo_info);
        }

        if (tank_took_shot(tank_info))
        {
            /* check for ufo hit */
            check_tank_shot(v20_win, tank_info, &ufo_info);
        }

        if (0 != ufo_info.shot_hit_ground)
        {
            int clean_up;

            /* shot is exploding on the ground */
            clean_up = ufo_shot_hit_ground(v20_win, &ufo_info);

            if (clean_up)
            {
                /* redraw the ground and tank (tank can't be on fire) */
                mvwhline_set(v20_win, V20_ROWS - 1, 0, &GROUND_CHAR, V20_COLS);
                tank_move(tank_info);
            }
        }
        else if (DIR_NONE != ufo_info.shot_direction)
        {
            /* check for tank hit */
            check_ufo_shot(tank_info, &ufo_info);
        }

        print_score(v20_win, tank_score, ufo_score);
    }

    delwin(v20_win);
    endwin();
    close_sound_stream(&sound_data);
    end_sounds();
    free(tank_info);
    return 0;
}


int handle_keypress(WINDOW* win, tank_info_t *tank)
{
    int ch;

    /* now read a character from the keyboard */
    ch = wgetch(win);
    flushinp();         /* flush the input buffer */

    tank_set_direction(tank, DIR_NONE);

    switch(ch)
    {
        case ERR:   /* no key pressed */
            break;

        case 'Q':
        case 'q':
            return -1;
            break;

        case 'Z':
        case 'z':
            if (!tank_is_on_fire(tank))
            {
                tank_set_direction(tank, DIR_LEFT);
            }
            break;

        case 'C':
        case 'c':
            if (!tank_is_on_fire(tank))
            {
                tank_set_direction(tank, DIR_RIGHT);
            }
            break;

        case 'B':
        case 'b':
            /* shoot */
            if (!tank_took_shot(tank) && !tank_is_on_fire(tank))
            {
                sound_error_t sound_error;

                /* there isn't a shot, so take it */
                tank_set_shot_pos(tank, tank_get_pos(tank) + 3,
                    TANK_SHOT_START_ROW);

                /* play sound */
                select_sound(tank_sound_data(tank), SOUND_TANK_SHOT);
                sound_error = restart_sound_stream(tank_sound_data(tank));

                if (0 != sound_error)
                {
                    handle_error(sound_error);
                }
            }
            break;

        case '+':
        case '=':
            /* increase the base volume */
            increment_volume(tank_sound_data(tank));
            break;

        case '-':
        case '_':
            /* decrease the base volume */
            decrement_volume(tank_sound_data(tank));
            break;

        default:
            break;
    }

    return 0;
}


void check_tank_shot(WINDOW* win, tank_info_t *tank, ufo_info_t *ufo)
{
    int dx;
    sound_error_t sound_error;
    pos_t shot_pos;

    shot_pos = tank_get_shot_pos(tank);

    if (tank_shot_hit(tank))
    {
        /* clear explosion shot */
        mvwaddch(win, shot_pos.y - 1, shot_pos.x, ' ');
        mvwaddstr(win, shot_pos.y, shot_pos.x - 1, "   ");
        mvwaddch(win, shot_pos.y + 1, shot_pos.x, ' ');
        wrefresh(win);

        tank_set_shot_pos(tank, -1, -1);
        tank_set_shot_hit(tank, false);
        return;
    }

    if (shot_pos.y != ufo->y)
    {
        /* different rows, no hit */
        return;
    }

    dx = shot_pos.x - ufo->x;

    if ((dx < 0) || (dx > 2))
    {
        /* wide of ufo, no hit */
        return;
    }

    /* hit */
    tank_set_shot_hit(tank, true);

    wattron(win, COLOR_PAIR(3));       /* fire color */
    mvwadd_wch(win, shot_pos.y - 1, shot_pos.x, &BOX_CHAR);
    mvwaddstr(win, shot_pos.y, shot_pos.x - 1, "███");
    mvwadd_wch(win, shot_pos.y + 1, shot_pos.x, &BOX_CHAR);
    wattroff(win, COLOR_PAIR(3));

    if (DIR_LEFT == ufo->direction)
    {
        ufo->direction = DIR_FALLING_LEFT;
    }
    else if (DIR_RIGHT == ufo->direction)
    {
        ufo->direction = DIR_FALLING_RIGHT;
    }

    /* ufo shot magically disappears when UFO is hit */
    if (ufo->shot_y != -1)
    {
        cchar_t c;

        /* erase old shot if it hasn't been overwritten */
        mvwin_wch(win, ufo->shot_y, ufo->shot_x, &c);

        if (UFO_SHOT_CHAR.chars[0] == c.chars[0])
        {
            mvwaddch(win, ufo->shot_y, ufo->shot_x, ' ');
        }

        ufo->shot_x = -1;
        ufo->shot_y = -1;
        ufo->shot_direction = DIR_NONE;
    }

    wrefresh(win);

    /* start the ufo falling sound */
    next_ufo_sound(ufo->sound_data);
    sound_error = restart_sound_stream(ufo->sound_data);

    if (0 != sound_error)
    {
        handle_error(sound_error);
    }

    return;
}


void check_ufo_shot(tank_info_t *tank, ufo_info_t *ufo)
{
    int dx;
    bool hit;

    if (ufo->shot_y < TANK_GUN_ROW)
    {
        /* shot is above the tank */
        return;
    }

    dx = ufo->shot_x - tank_get_pos(tank);
    hit = false;

    /* check for hit by row */
    if (TANK_GUN_ROW == ufo->shot_y)
    {
        /* gun barrel row */
        if (3 == dx)
        {
            hit = true;
        }
    }
    else if (TANK_TURRET_ROW == ufo->shot_y)
    {
        /* turret row */
        if ((2 == dx) || (3 == dx))
        {
            hit = true;
        }
    }
    else if (TANK_TREAD_ROW == ufo->shot_y)
    {
        /* tread row */
        if ((dx > 0) && (dx < 5))
        {
            hit = true;
        }
    }

    if (hit)
    {
        /* record tank hit, start fire sound, stop ufo shot */
        sound_error_t sound_error;

        tank_set_on_fire(tank, true);
        select_sound(tank_sound_data(tank), SOUND_ON_FIRE);
        sound_error = restart_sound_stream(tank_sound_data(tank));

        if (0 != sound_error)
        {
            handle_error(sound_error);
        }

        ufo->shot_x = -1;
        ufo->shot_y = -1;
        ufo->shot_direction = DIR_NONE;
    }
}


void print_score(WINDOW* win, const uint8_t tank_score,
    const uint8_t ufo_score)
{
    mvwprintw(win, SCORE_ROW, 5, "%d", ufo_score);
    mvwprintw(win, SCORE_ROW, 15, "%d", tank_score);
    wrefresh(win);
}
