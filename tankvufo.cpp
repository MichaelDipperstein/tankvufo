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

static constexpr float VOLUME = 0.5;    /* base volume for sounds */

int main(void)
{
    tank_v_ufo_t *tvu;
    int win_x, win_y;
    bool result;

    /* setup the ncurses field-of-play */
    tvu = new tank_v_ufo_t;

    if (nullptr == tvu)
    {
        perror("creating tank vs ufo object");
        return 1;
    }

    /* vic-20 sized window for the game field */
    win_x = (COLS - V20_COLS) / 2;
    win_y = (LINES - V20_ROWS) / 2;

    result = tvu->make_v20_win(V20_ROWS, V20_COLS, win_y, win_x);

    if (false == result)
    {
        delete tvu;
        perror("creating VIC-20 window");
        return 1;
    }

    /* window for volume meter */
    win_y = (LINES - VOL_ROWS) / 2;
    win_x += V20_COLS + VOL_COLS;
    result = tvu->make_vol_win(VOL_ROWS, VOL_COLS, win_y, win_x);

    if (false == result)
    {
        delete tvu;
        perror("creating volume control window");
        return 1;
    }

    refresh();          /* refresh the whole screen to show the windows */

    tvu->initialize_v20_win();

    /* now draw the volume level box */
    tvu->draw_volume_level_box();
    tvu->show_volume_level(VOLUME);

    /* initialize all of the sound stuff */
    sound_data_t sound_data;
    sound_error_t sound_error;

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

    /* create and initialize the tank and ufo */
    result = tvu->initialize_vehicles(&sound_data);

    if (false == result)
    {
        close_sound_stream(&sound_data);
        end_sounds();
        delete tvu;
        perror("allocating tank or ufo");
        return 1;
    }

    tvu->print_score();                     /* 0 - 0 score */
    tvu->refresh();

    /* timer and poll variables */
    int tfd;
    struct itimerspec timeout;
    struct pollfd pfd;

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

        tvu->move_tank();
        tvu->move_ufo();

        tvu->update_tank_shot();
        tvu->update_ufo_shot();

        tvu->print_score();
    }

    close_sound_stream(&sound_data);
    end_sounds();
    delete tvu;
    return 0;
}


tank_v_ufo_t::tank_v_ufo_t(void)
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


tank_v_ufo_t::~tank_v_ufo_t(void)
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
        delete tank;
    }

    if (ufo != nullptr)
    {
        delete ufo;
    }
}


void tank_v_ufo_t::print_score()
{
    mvwprintw(v20_win, SCORE_ROW, 5, "%d", tank->get_tanks_killed());
    mvwprintw(v20_win, SCORE_ROW, 15, "%d", ufo->get_ufos_killed());
    wrefresh(v20_win);
}


void tank_v_ufo_t::initialize_v20_win(void)
{
    /* set the window color scheme */
    init_pair(2, COLOR_BLACK, COLOR_WHITE);
    wbkgd(v20_win, COLOR_PAIR(2));

    /* pair 3 will be for fire */
    init_pair(3, COLOR_RED, COLOR_WHITE);

    /* print the banner */
    wprintw(v20_win, "** TANK VERSUS UFO. **");
    wprintw(v20_win, "Z-LEFT,C-RIGHT,B-FIRE ");
    wprintw(v20_win, "UFO:     TANK:");

    draw_ground();

    wtimeout(v20_win, 0);           /* make wgetch non-blocking */
}


bool tank_v_ufo_t::make_v20_win(int rows, int cols, int begin_x, int begin_y)
{
    bool result;
    result = false;

    v20_win = newwin(rows, cols, begin_x, begin_y);

    if (v20_win != nullptr)
    {
        result = true;
        v20_rows = rows;
        v20_cols = cols;
    }

    return result;
}


void tank_v_ufo_t::draw_ground(void)
{
    static cchar_t GROUND_CHAR = {WA_NORMAL, L"▔", 0};

    mvwhline_set(v20_win, v20_rows - 1, 0, &GROUND_CHAR, v20_cols);
}


bool tank_v_ufo_t::make_vol_win(int rows, int cols, int begin_x, int begin_y)
{
    bool result;
    result = false;

    vol_win = newwin(rows, cols, begin_x, begin_y);

    if (vol_win != nullptr)
    {
        result = true;
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

    return result;
}


void tank_v_ufo_t::draw_volume_level_box(void)
{
    wbkgd(vol_win, COLOR_PAIR(2));
    box(vol_win, 0, 0);
    mvwaddch(vol_win, 1, 1, '+');
    mvwaddch(vol_win, vol_rows - 3, 1, '-');
    mvwaddstr(vol_win, vol_rows - 2, 1, " VOLUME ");
}


void tank_v_ufo_t::show_volume_level(const float volume)
{
    static cchar_t BOX_CHAR = {WA_NORMAL, L"█", 0};
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


bool tank_v_ufo_t::initialize_vehicles(sound_data_t *sound_data)
{
    bool result;
    result = false;

    tank = new tank_t(v20_win, SCORE_ROW + 1, sound_data);

    if (nullptr != tank)
    {
        ufo = new ufo_t(v20_win, UFO_TOP, UFO_BOTTOM, sound_data);

        if (nullptr != ufo)
        {
            result = true;
        }
    }

    return result;
}


void tank_v_ufo_t::move_tank(void)
{
    tank->move();
}


void tank_v_ufo_t::move_ufo(void)
{
    ufo->move();
}


void tank_v_ufo_t::update_tank_shot(void)
{
    if (tank->was_shot_fired())
    {
        tank->move_shot();

        /* check for ufo hit */
        check_tank_shot();
    }
}


void tank_v_ufo_t::update_ufo_shot(void)
{
    /* move ufo shot if need (shot exists and isn't exploding) */
    ufo->move_shot();

    if (ufo->is_shot_exploding())
    {
        int clean_up;

        /* shot is exploding on the ground, animate it */
        clean_up = ufo->update_shot_phase();

        if (clean_up)
        {
            /* redraw the ground and tank (tank can't be on fire) */
            draw_ground();
            move_tank();
        }
    }
    else if (ufo->is_shot_falling())
    {
        /* check for tank hit */
        check_ufo_shot();
    }
}


int tank_v_ufo_t::handle_keypress()
{
    int ch;
    float vol;

    nodelay(v20_win, TRUE); /* make sure we're in no delay mode */
    tank->set_direction(DIR_NONE);
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
                if (!tank->is_on_fire())
                {
                    tank->set_direction(DIR_LEFT);
                }
                break;

            case 'C':
            case 'c':
                if (!tank->is_on_fire())
                {
                    tank->set_direction(DIR_RIGHT);
                }
                break;

            case 'B':
            case 'b':
                /* shoot */
                if (!tank->was_shot_fired() && !tank->is_on_fire())
                {
                    sound_error_t sound_error;
                    sound_data_t *sound_data;

                    /* there isn't a shot, so take it */
                    tank->shoot();

                    /* play sound */
                    sound_data = tank->get_sound_data();
                    select_sound(sound_data, SOUND_TANK_SHOT);
                    sound_error = restart_sound_stream(sound_data);

                    if (0 != sound_error)
                    {
                        handle_error(sound_error);
                    }
                }
                break;

            case '+':
            case '=':
                /* increase the base volume */
                vol = increment_volume(tank->get_sound_data());
                show_volume_level(vol);
                break;

            case '-':
            case '_':
                /* decrease the base volume */
                vol = decrement_volume(tank->get_sound_data());
                show_volume_level(vol);
                break;

            default:
                break;
        }
    }

    return 0;
}


void tank_v_ufo_t::check_tank_shot()
{
    bool just_hit;

    just_hit = tank->update_shot_hit(ufo->get_pos());

    if (true == just_hit)
    {
        /* just hit ufo */
        sound_error_t sound_error;

        sound_error = ufo->set_falling();

        if (0 != sound_error)
        {
            handle_error(sound_error);
        }

        /* ufo shot magically disappears when ufo is hit */
        ufo->clear_shot(true);
    }

    wrefresh(v20_win);
}


void tank_v_ufo_t::check_ufo_shot()
{
    int dx;
    bool hit;
    pos_t shot_pos;

    shot_pos = ufo->get_shot_pos();

    if (shot_pos.y < TANK_GUN_ROW)
    {
        /* shot is above the tank */
        return;
    }

    dx = shot_pos.x - tank->get_pos();
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
        sound_data_t *sound_data;

        tank->set_on_fire(true);

        sound_data = tank->get_sound_data();
        select_sound(sound_data, SOUND_ON_FIRE);
        sound_error = restart_sound_stream(sound_data);

        if (0 != sound_error)
        {
            handle_error(sound_error);
        }

        ufo->clear_shot(false);
    }
}
