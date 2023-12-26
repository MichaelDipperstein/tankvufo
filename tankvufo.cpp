/***************************************************************************
*                              Tank Versus UFO
*
*   File    : tankvufo.cpp
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
#include <clocale>
#include <ncurses.h>
#include <cstdlib>
#include <cstring>
#include <sys/timerfd.h>
#include <sys/poll.h>
#include <unistd.h>
#include <cerrno>

#include "tankvufo.h"

#include "tank.h"
#include "ufo.h"
#include "sounds.h"

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
static constexpr int VOL_COLS = 10;
static constexpr int VOL_ROWS = 13;

/* cchar_t for unicode charaters used in this file */
static constexpr cchar_t GROUND_CHAR = {WA_NORMAL, L"▔", 0};
static constexpr cchar_t BOX_CHAR = {WA_NORMAL, L"█", 0};

static constexpr float VOLUME = 0.5;    /* base volume for sounds */

int main(void)
{
    tank_v_ufo_c *tvu;
    int win_x, win_y;

    /* timer and poll variables */
    int tfd;
    struct itimerspec timeout;
    struct pollfd pfd;

    /* used by sound library */
    sound_data_t sound_data;
    sound_error_t sound_error;

    tvu = new tank_v_ufo_c;

    /* vic-20 sized window for the game field */
    win_x = (COLS - V20_COLS) / 2;
    win_y = (LINES - V20_ROWS) / 2;
    tvu->v20_win = newwin(V20_ROWS, V20_COLS, win_y, win_x);

    if (NULL == tvu->v20_win)
    {
        perror("creating VIC-20 window");
        return 1;
    }

    /* window for volume meter */
    win_y = (LINES - VOL_ROWS) / 2;
    win_x += V20_COLS + VOL_COLS;
    tvu->make_vol_win(VOL_ROWS, VOL_COLS, win_y, win_x);

    if (NULL == tvu->v20_win)
    {
        delete tvu;
        perror("creating volume control window");
        return 1;
    }

    refresh();          /* refresh the whole screen to show the windows */

    /* now set the window color scheme */
    init_pair(2, COLOR_BLACK, COLOR_WHITE);
    wbkgd(tvu->v20_win, COLOR_PAIR(2));

    /* pair 3 will be for fire */
    init_pair(3, COLOR_RED, COLOR_WHITE);

    /* print the banner */
    wprintw(tvu->v20_win, "** TANK VERSUS UFO. **");
    wprintw(tvu->v20_win, "Z-LEFT,C-RIGHT,B-FIRE ");
    wprintw(tvu->v20_win, "UFO:     TANK:");

    /* draw the ground */
    mvwhline_set(tvu->v20_win, V20_ROWS - 1, 0, &GROUND_CHAR, V20_COLS);

    tvu->print_score(0, 0);              /* 0 - 0 score */
    wtimeout(tvu->v20_win, 0);           /* make wgetch non-blocking */

    /* initialize all of the sound stuff */
    sound_error = initialize_sounds();

    if (0 != sound_error)
    {
        handle_error(sound_error);
        delete tvu;
        return sound_error;
    }

    sound_error = create_sound_stream(&sound_data, VOLUME);
    if (0 != sound_error)
    {
        handle_error(sound_error);
        delete tvu;
        return sound_error;
    }

    /* now draw the volume level box */
    tvu->draw_volume_level_box();
    tvu->show_volume_level(VOLUME);

    tvu->ufo = ufo_initialize(tvu->v20_win, &sound_data);

    if (NULL == tvu->ufo)
    {
        close_sound_stream(&sound_data);
        end_sounds();
        delete tvu;
        perror("allocating ufo_info");
        return 1;
    }

    tvu->tank = tank_initialize(tvu->v20_win, &sound_data);

    if (NULL == tvu->tank)
    {
        close_sound_stream(&sound_data);
        end_sounds();
        delete tvu;
        perror("allocating tank_info");
        return 1;
    }

    tank_move(tvu->tank);                   /* draw tank */
    wrefresh(tvu->v20_win);

    /* create a timer fd that expires every 200ms */
    tfd = timerfd_create(CLOCK_MONOTONIC,0);

    if (tfd <= 0)
    {
        close_sound_stream(&sound_data);
        end_sounds();
        delete tvu;
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
        close_sound_stream(&sound_data);
        end_sounds();
        delete tvu;
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

        if (tvu->handle_keypress() < 0)
        {
            /* we got a quit key */
            break;
        }

        tank_move(tvu->tank);
        ufo_move(tvu->ufo);

        if (!tank_shot_hit(tvu->tank))
        {
            tank_shot_move(tvu->tank);
        }

        /* move ufo shot if need (shot exists and isn't exploding) */
        ufo_move_shot(tvu->ufo);

        if (tank_took_shot(tvu->tank))
        {
            /* check for ufo hit */
            tvu->check_tank_shot();
        }

        if (ufo_shot_is_exploding(tvu->ufo))
        {
            int clean_up;

            /* shot is exploding on the ground, animate it */
            clean_up = ufo_shot_hit_ground(tvu->ufo);

            if (clean_up)
            {
                /* redraw the ground and tank (tank can't be on fire) */
                mvwhline_set(tvu->v20_win, V20_ROWS - 1, 0, &GROUND_CHAR,
                    V20_COLS);
                tank_move(tvu->tank);
            }
        }
        else if (ufo_shot_is_falling(tvu->ufo))
        {
            /* check for tank hit */
            tvu->check_ufo_shot();
        }

        tvu->print_score(ufo_get_tank_score(tvu->ufo),
            tank_get_ufo_score(tvu->tank));
    }

    close_sound_stream(&sound_data);
    end_sounds();
    delete tvu;
    return 0;
}

tank_v_ufo_c::tank_v_ufo_c(void)
{
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
}

tank_v_ufo_c::~tank_v_ufo_c(void)
{
    if (v20_win != nullptr)
    {
        delwin(v20_win);
    }

    if (vol_win != nullptr)
    {
        delwin(vol_win);
    }

    endwin();

    if (tank != nullptr)
    {
        free(tank);
    }

    if (ufo != nullptr)
    {
        free(ufo);
    }
}
int tank_v_ufo_c::handle_keypress()
{
    int ch;
    float vol;

    nodelay(v20_win, TRUE); /* make sure we're in no delay mode */
    tank_set_direction(tank, DIR_NONE);
    ch = 0;

    while (ERR != ch)
    {
        /* read the next character from the keyboard buffer */
        ch = wgetch(v20_win);

        switch(ch)
        {
            case ERR:   /* no more keys */
                break;

            case 'Q':
            case 'q':
                return -1;

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
                    sound_error = restart_sound_stream(
                        tank_sound_data(tank));

                    if (0 != sound_error)
                    {
                        handle_error(sound_error);
                    }
                }
                break;

            case '+':
            case '=':
                /* increase the base volume */
                vol = increment_volume(tank_sound_data(tank));
                show_volume_level(vol);
                break;

            case '-':
            case '_':
                /* decrease the base volume */
                vol = decrement_volume(tank_sound_data(tank));
                show_volume_level(vol);
                break;

            default:
                break;
        }
    }

    return 0;
}


void tank_v_ufo_c::check_tank_shot()
{
    int dx;
    sound_error_t sound_error;
    pos_t shot_pos;
    pos_t ufo_pos;

    shot_pos = tank_get_shot_pos(tank);
    ufo_pos = ufo_get_pos(ufo);

    if (tank_shot_hit(tank))
    {
        /* clear explosion shot */
        mvwaddch(v20_win, shot_pos.y - 1, shot_pos.x, ' ');
        mvwaddstr(v20_win, shot_pos.y, shot_pos.x - 1, "   ");
        mvwaddch(v20_win, shot_pos.y + 1, shot_pos.x, ' ');
        wrefresh(v20_win);

        tank_set_shot_pos(tank, -1, -1);
        tank_set_shot_hit(tank, false);
        return;
    }

    if (shot_pos.y != ufo_pos.y)
    {
        /* different rows, no hit */
        return;
    }

    dx = shot_pos.x - ufo_pos.x;

    if ((dx < 0) || (dx > 2))
    {
        /* wide of ufo, no hit */
        return;
    }

    /* hit */
    tank_set_shot_hit(tank, true);

    wattron(v20_win, COLOR_PAIR(3));       /* fire color */
    mvwadd_wch(v20_win, shot_pos.y - 1, shot_pos.x, &BOX_CHAR);
    mvwaddstr(v20_win, shot_pos.y, shot_pos.x - 1, "███");
    mvwadd_wch(v20_win, shot_pos.y + 1, shot_pos.x, &BOX_CHAR);
    wattroff(v20_win, COLOR_PAIR(3));

    sound_error = ufo_set_falling(ufo);

    if (0 != sound_error)
    {
        handle_error(sound_error);
    }

    /* ufo shot magically disappears when UFO is hit */
    ufo_clear_shot(ufo, true);

    wrefresh(v20_win);
    return;
}


void tank_v_ufo_c::check_ufo_shot()
{
    int dx;
    bool hit;
    pos_t shot_pos;

    shot_pos = ufo_get_shot_pos(ufo);

    if (shot_pos.y < TANK_GUN_ROW)
    {
        /* shot is above the tank */
        return;
    }

    dx = shot_pos.x - tank_get_pos(tank);
    hit = false;

    /* check for hit by row */
    if (TANK_GUN_ROW == shot_pos.y)
    {
        /* gun barrel row */
        if (3 == dx)
        {
            hit = true;
        }
    }
    else if (TANK_TURRET_ROW == shot_pos.y)
    {
        /* turret row */
        if ((2 == dx) || (3 == dx))
        {
            hit = true;
        }
    }
    else if (TANK_TREAD_ROW == shot_pos.y)
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

        ufo_clear_shot(ufo, false);
    }
}


void tank_v_ufo_c::print_score(const uint8_t tank_score,
    const uint8_t ufo_score)
{
    mvwprintw(v20_win, SCORE_ROW, 5, "%d", ufo_score);
    mvwprintw(v20_win, SCORE_ROW, 15, "%d", tank_score);
    wrefresh(v20_win);
}

void tank_v_ufo_c::make_vol_win(int rows, int cols, int begin_x, int begin_y)
{
    vol_win = newwin(rows, cols, begin_x, begin_y);

    if (vol_win != nullptr)
    {
        vol_rows = rows;
        vol_cols = cols;

        /* list of commands not in the original game */
        attron(A_UNDERLINE);
        mvprintw(2, 5, "ADDED COMMANDS");
        attroff(A_UNDERLINE);
        mvprintw(3, 2, "Q        - QUIT GAME");
        mvprintw(4, 2, "PLUS(+)  - VOLUME UP");
        mvprintw(5, 2, "MINUS(-) - VOLUME DOWN");
    }
}

void tank_v_ufo_c::draw_volume_level_box(void)
{
    wbkgd(vol_win, COLOR_PAIR(2));
    box(vol_win, 0, 0);
    mvwaddch(vol_win, 1, 1, '+');
    mvwaddch(vol_win, vol_rows - 3, 1, '-');
    mvwaddstr(vol_win, vol_rows - 2, 1, " VOLUME ");
}

void tank_v_ufo_c::show_volume_level(const float volume)
{
    int bars;
    int start_y;

    bars = (int)(volume * 10.0 + 0.5);
    start_y = (vol_rows - 2) - bars;

    /* erase old bar */
    mvwvline(vol_win, vol_rows - 2 - 10, 4, ' ', 10);
    mvwvline(vol_win, vol_rows - 2 - 10, 5, ' ', 10);

    /* draw new bar in color pair 3 color (same as fire) */
    wattron(vol_win, COLOR_PAIR(3));
    mvwvline_set(vol_win, start_y, 4, &BOX_CHAR, bars);
    mvwvline_set(vol_win, start_y, 5, &BOX_CHAR, bars);
    wattron(vol_win, COLOR_PAIR(3));
    wrefresh(vol_win);
}