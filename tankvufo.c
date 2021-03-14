/***************************************************************************
*                             Tank Versuses UFO
*
*   File    : tankvufo.c
*   Purpose : A tribute to the Tank-V-UFO, a Commodore VIC-20 Game
*             by Duane Later
*   Author  : Michael Dipperstein
*   Date    : November 26, 2020
*
****************************************************************************
*
* Tank Versuses UFO: A tribute to the Tank-V-UFO, a Commodore VIC-20 Game
*                    by Duane Later
*
* Copyright (C) 2020, 2021 by
*       Michael Dipperstein (mdipperstein@gmail.com)
*
* This file is part of Tank Versuses UFO.
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
#include <string.h>
#include <stdlib.h>
#include <sys/timerfd.h>
#include <sys/poll.h>
#include <unistd.h>
#include <errno.h>

#include "sounds.h"

/* cchar_t for unicode charaters used in this program */
static const cchar_t GROUND_CHAR = {WA_NORMAL, L"▔", 0};
static const cchar_t UFO_SHOT_CHAR = {WA_NORMAL, L"●", 0};
static const cchar_t TANK_SHOT_CHAR = {WA_NORMAL, L"▪", 0};
static const cchar_t BOX_CHAR = {WA_NORMAL, L"█", 0};

/* screen dimenssions */
static const int V20_COLS = 22;
static const int V20_ROWS = 23;

/* score is witten to this row */
static const int SCORE_ROW = 2;

/* rows containing tank animatation */
static const int TANK_TREAD_ROW = V20_ROWS - 2;
static const int TANK_TURRET_ROW = TANK_TREAD_ROW - 1;
static const int TANK_GUN_ROW = TANK_TURRET_ROW - 1;
static const int TANK_SHOT_START_ROW = TANK_GUN_ROW - 1;

/* lowest and highest rows of ufo travel */
static const int UFO_BOTTOM = TANK_SHOT_START_ROW - 2;
static const int UFO_TOP = SCORE_ROW + 2;

static const float VOLUME = 0.5;        /* base volume for sounds */

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


/* struct containing tank related data */
typedef struct
{
    int x;                      /* leftmost tank coordinate */
    direction_t direction;      /* direction of next tank move */
    int shot_x;                 /* x coordinate of tank shot */
    int shot_y;                 /* y coordinate of tank shot */
    bool shot_hit_ufo;          /* true if the ufo was just hit (+ displayed) */
    uint8_t on_fire;            /* 0 when not on fire, otherwise flame count */
    sound_data_t *sound_data;
} tank_info_t;


/* struct containing ufo related data */
typedef struct
{
    int x;                      /* leftmost ufo coordinate */
    int y;                      /* row containing the ufo */
    direction_t direction;      /* direction that the UFO is moving */
    uint8_t ufo_hit_ground;     /* 0 when not on fire, otherwise flame count */
    int shot_x;                 /* x coordinate of ufo shot */
    int shot_y;                 /* y coordinate of ufo shot */
    direction_t shot_direction; /* direction the ufo shot is moving */
    uint8_t shot_hit_ground;    /* 0 if false, otherwise phase of explosion */
    sound_data_t *sound_data;
} ufo_info_t;

int handle_keypress(WINDOW* win, tank_info_t *tank);

uint8_t move_tank(WINDOW* win, tank_info_t *tank);
void move_tank_shot(WINDOW* win, tank_info_t *tank);
void check_tank_shot(WINDOW* win, tank_info_t *tank, ufo_info_t *ufo);

uint8_t move_ufo(WINDOW* win, ufo_info_t *ufo);
void make_ufo_shot(ufo_info_t *ufo);
void move_ufo_shot(WINDOW* win, ufo_info_t *ufo);
int ufo_shot_hit_ground(WINDOW* win, ufo_info_t *ufo);
void check_ufo_shot(tank_info_t *tank, ufo_info_t *ufo);

void print_score(WINDOW* win, const uint8_t tank_score,
    const uint8_t ufo_score);

int main(void)
{
    WINDOW *v20_win;
    int win_x, win_y;
    tank_info_t tank_info;
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

    /* start with tank on left and no shot */
    tank_info.x = 0;
    tank_info.shot_x = -1;
    tank_info.shot_y = -1;
    tank_info.shot_hit_ufo = false;
    tank_info.on_fire = 0;

    /* start without a ufo and no shot */
    ufo_info.x = 0;
    ufo_info.y = 0;
    ufo_info.direction = DIR_NONE;
    ufo_info.ufo_hit_ground = 0;
    ufo_info.shot_x = 0;
    ufo_info.shot_y = 0;
    ufo_info.shot_direction = DIR_NONE;
    ufo_info.shot_hit_ground = 0;

    tank_score = 0;
    ufo_score = 0;

    move_tank(v20_win, &tank_info);                 /* draw tank */
    print_score(v20_win, tank_score, ufo_score);    /* 0 - 0 score */
    wrefresh(v20_win);

    wtimeout(v20_win, 0);               /* make wgetch non-blocking */
    srand((unsigned int)time(NULL));    /* seed the random number generator */

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

    tank_info.sound_data = &sound_data;
    ufo_info.sound_data = &sound_data;

    /* create a timer fd that expires every 200ms */
    tfd = timerfd_create(CLOCK_MONOTONIC,0);

    if (tfd <= 0)
    {
        delwin(v20_win);
        endwin();
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

        if (handle_keypress(v20_win, &tank_info) < 0)
        {
            /* we got a quit key */
            break;
        }

        ufo_score += move_tank(v20_win, &tank_info);
        tank_score += move_ufo(v20_win, &ufo_info);

        if (!tank_info.shot_hit_ufo)
        {
            move_tank_shot(v20_win, &tank_info);
        }

        if ((DIR_NONE != ufo_info.shot_direction) &&
            (0 == ufo_info.shot_hit_ground))
        {
            /* shot is exists and not exploding */
            move_ufo_shot(v20_win, &ufo_info);
        }

        if (tank_info.shot_y != -1)
        {
            /* check for ufo hit */
            check_tank_shot(v20_win, &tank_info, &ufo_info);
        }

        if (0 != ufo_info.shot_hit_ground)
        {
            int clean_up;

            /* shot is exploding on the ground */
            clean_up = ufo_shot_hit_ground(v20_win, &ufo_info);

            if (clean_up)
            {
                /* redraw the ground  and tank */
                mvwhline_set(v20_win, V20_ROWS - 1, 0, &GROUND_CHAR, V20_COLS);
                move_tank(v20_win, &tank_info);
            }
        }
        else if (DIR_NONE != ufo_info.shot_direction)
        {
            /* check for tank hit */
            check_ufo_shot(&tank_info, &ufo_info);
        }

        print_score(v20_win, tank_score, ufo_score);
    }

    delwin(v20_win);
    endwin();
    close_sound_stream(&sound_data);
    end_sounds();
    return 0;
}


int handle_keypress(WINDOW* win, tank_info_t *tank)
{
    int ch;

    /* now read a character from the keyboard */
    ch = wgetch(win);
    flushinp();         /* flush the input buffer */

    tank->direction = DIR_NONE;

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
            if (!tank->on_fire)
            {
                tank->direction = DIR_LEFT;
            }
            break;

        case 'C':
        case 'c':
            if (!tank->on_fire)
            {
                tank->direction = DIR_RIGHT;
            }
            break;

        case 'B':
        case 'b':
            /* shoot */
            if ((tank->shot_y == -1) && (!tank->on_fire))
            {
                sound_error_t sound_error;

                /* there isn't a shot, so take it */
                tank->shot_y = TANK_SHOT_START_ROW;
                tank->shot_x = tank->x + 3;

                /* play sound */
                select_sound(tank->sound_data, SOUND_TANK_SHOT);
                sound_error = restart_sound_stream(tank->sound_data);

                if (0 != sound_error)
                {
                    handle_error(sound_error);
                }
            }
            break;

        default:
            break;
    }

    return 0;
}


uint8_t move_tank(WINDOW* win, tank_info_t *tank)
{
    uint8_t score;
    score = 0;

    if (10 == tank->on_fire)
    {
        /* done with fire, restart on left */
        tank->on_fire = 0;
        tank->direction = DIR_NONE;
        mvwaddstr(win, TANK_GUN_ROW, tank->x + 3, " ");
        mvwaddstr(win, TANK_TURRET_ROW, tank->x + 1, "    ");
        mvwaddstr(win, TANK_TREAD_ROW, tank->x, "      ");
        tank->x = 0;
        score = 1;
        select_sound(tank->sound_data, SOUND_OFF);
    }

    if (tank->on_fire)
    {
        tank->on_fire += 1;
        mvwaddstr(win, TANK_GUN_ROW, tank->x + 3, " ");

        wattron(win, COLOR_PAIR(3));       /* fire color */

        if (tank->on_fire % 2)
        {
            mvwaddstr(win, TANK_TURRET_ROW, tank->x + 1, "◣◣◣◣");
        }
        else
        {
            mvwaddstr(win, TANK_TURRET_ROW, tank->x + 1, "◢◢◢◢");
        }

        wattroff(win, COLOR_PAIR(3));
        mvwaddstr(win, TANK_TREAD_ROW, tank->x, "▕OOOO▏");
        wrefresh(win);
        return score;
    }

    if (DIR_NONE == tank->direction)
    {
        /* redraw without moving */
        mvwaddstr(win, TANK_GUN_ROW, tank->x + 3, "▖");
        mvwaddstr(win, TANK_TURRET_ROW, tank->x + 1, "▁██▁");
        mvwaddstr(win, TANK_TREAD_ROW, tank->x, "▕OOOO▏");
    }
    else if (DIR_LEFT == tank->direction)
    {
        if (tank->x == 0)
        {
            /* can't go furter left */
            return score;
        }

        /* move to the left, add a trailing space to erase the old */
        tank->x -= 1;
        mvwaddstr(win, TANK_GUN_ROW, tank->x + 3, "▖ ");
        mvwaddstr(win, TANK_TURRET_ROW, tank->x + 1, "▁██▁ ");
        mvwaddstr(win, TANK_TREAD_ROW, tank->x, "▕OOOO▏ ");
    }
    else if (DIR_RIGHT == tank->direction)
    {
        if (tank->x == V20_COLS - 6)
        {
            /* can't go furter right */
            return score;
        }

        /* move to the right, add a leading space to erase the old */
        mvwaddstr(win, TANK_GUN_ROW, tank->x + 3, " ▖");
        mvwaddstr(win, TANK_TURRET_ROW, tank->x + 1, " ▁██▁");
        mvwaddstr(win, TANK_TREAD_ROW, tank->x, " ▕OOOO▏");
        tank->x += 1;
    }

    wrefresh(win);
    return score;
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


void move_tank_shot(WINDOW* win, tank_info_t *tank)
{
    if ((tank->shot_y < 0) || (tank->shot_hit_ufo))
    {
        return;     /* there's no shot */
    }

    if (tank->shot_y != TANK_SHOT_START_ROW)
    {
        /* erase old shot if it hasn't been overwritten */
        cchar_t c;
        mvwin_wch(win, tank->shot_y, tank->shot_x, &c);

        if (TANK_SHOT_CHAR.chars[0] == c.chars[0])
        {
            mvwaddch(win, tank->shot_y, tank->shot_x, ' ');

            /* move shot up */
            tank->shot_y--;
        }
        else if (tank->shot_y == (TANK_SHOT_START_ROW - 1))
        {
            /* delete the muzzle flash */
            mvwaddch(win, tank->shot_y + 1, tank->shot_x, ' ');
        }

        if (tank->shot_y != SCORE_ROW)
        {
            /* draw new shot */
            mvwadd_wch(win, tank->shot_y, tank->shot_x, &TANK_SHOT_CHAR);
        }
        else
        {
            /* done with shot */
            tank->shot_x = -1;
            tank->shot_y = -1;

            /* stop shot sound */
            select_sound(tank->sound_data, SOUND_OFF);
        }
    }
    else
    {
        /* muzzle flash */
        wattron(win, COLOR_PAIR(3));       /* fire color */
        mvwadd_wch(win, tank->shot_y, tank->shot_x, &BOX_CHAR);
        wattroff(win, COLOR_PAIR(3));
        tank->shot_y--;
    }

    wrefresh(win);
}


void check_tank_shot(WINDOW* win, tank_info_t *tank, ufo_info_t *ufo)
{
    int dx;
    sound_error_t sound_error;

    if (tank->shot_hit_ufo)
    {
        /* clear explosion shot */
        mvwaddch(win, tank->shot_y - 1, tank->shot_x, ' ');
        mvwaddstr(win, tank->shot_y, tank->shot_x - 1, "   ");
        mvwaddch(win, tank->shot_y + 1, tank->shot_x, ' ');
        wrefresh(win);

        tank->shot_x = -1;
        tank->shot_y = -1;
        tank->shot_hit_ufo = false;
        return;
    }

    if (tank->shot_y != ufo->y)
    {
        /* different rows, no hit */
        return;
    }

    dx = tank->shot_x - ufo->x;

    if ((dx < 0) || (dx > 2))
    {
        /* wide of ufo, no hit */
        return;
    }

    /* hit */
    tank->shot_hit_ufo = true;
    mvwadd_wch(win, tank->shot_y - 1, tank->shot_x, &BOX_CHAR);
    mvwaddstr(win, tank->shot_y, tank->shot_x - 1, "███");
    mvwadd_wch(win, tank->shot_y + 1, tank->shot_x, &BOX_CHAR);

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


void check_ufo_shot(tank_info_t *tank, ufo_info_t *ufo)
{
    int dx;
    bool hit;

    if (ufo->shot_y < TANK_GUN_ROW)
    {
        /* shot is above the tank */
        return;
    }

    dx = ufo->shot_x - tank->x;
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

        tank->on_fire = 1;
        select_sound(tank->sound_data, SOUND_ON_FIRE);
        sound_error = restart_sound_stream(tank->sound_data);

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
