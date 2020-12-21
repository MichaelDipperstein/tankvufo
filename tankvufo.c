/***************************************************************************
*                             Tank Versuses UFO
*
*   File    : tankvufo.c
*   Purpose : A tribute to the Tank-V-UFO, a Commodore VIC-20 Game
*             by Duane Later
*   Author  : Michael Dipperstein
*   Date    : November 26, 2006
*
****************************************************************************
*
* Tank Versuses UFO: A tribute to the Tank-V-UFO, a Commodore VIC-20 Game
*                    by Duane Later
*
* Copyright (C) 2020 by
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

const int V20_COLS = 22;
const int V20_ROWS = 23;
const int SCORE_ROW = 2;                    /* row containing the score */
const int TANK_SHOT_START = V20_ROWS - 4;   /* tank shots start indicator */
const int UFO_BOTTOM = TANK_SHOT_START - 2; /* lowest row with ufo */
const int UFO_TOP = SCORE_ROW + 2;          /* highest row with ufo */

typedef struct
{
    int x;
    int shot_x;
    int shot_y;
} tank_info_t;

typedef enum
{
    DIR_NONE,
    DIR_LEFT,
    DIR_RIGHT,
    DIR_FALLING_LEFT,
    DIR_FALLING_RIGHT
} direction_t;

typedef struct
{
    int x;
    int y;
    direction_t direction;
    int shot_x;
    int shot_y;
    direction_t shot_direction;
    int shot_hit_ground;
} ufo_info_t;

int handle_keypress(WINDOW* win, tank_info_t *tank);

int move_tank(WINDOW* win, int tank_x, direction_t direction);
void move_ufo(WINDOW* win, ufo_info_t *ufo);

void move_tank_shot(WINDOW* win, tank_info_t *tank);
void check_tank_shot(WINDOW* win, tank_info_t *tank, ufo_info_t *ufo);

void move_ufo_shot(WINDOW* win, ufo_info_t *ufo);
void ufo_shot_hit_ground(WINDOW* win, ufo_info_t *ufo);

void print_score(WINDOW* win, int tank_score, int ufo_score);

int main(void)
{
    const cchar_t ground = {A_NORMAL, L"▔", 0};
    WINDOW *v20_win;
    int win_x, win_y;
    tank_info_t tank_info;
    ufo_info_t ufo_info;
    int tank_score = 0;
    int ufo_score = 0;
    int tfd;
    struct itimerspec timeout;
    struct pollfd pfd;

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

    /* print the banner */
    wprintw(v20_win, "** TANK VERSUS UFO. **");
    wprintw(v20_win, "Z-LEFT,C-RIGHT,B-FIRE ");
    wprintw(v20_win, "UFO:     TANK:");

    /* draw the ground */
    mvwhline_set(v20_win, V20_ROWS - 1, 0, &ground, V20_COLS);


    /* start with tank on left and no shot */
    tank_info.x = 0;
    tank_info.shot_x = -1;
    tank_info.shot_y = -1;

    /* draw tank */
    move_tank(v20_win, tank_info.x, DIR_NONE);

    /* start without a ufo and no shot */
    ufo_info.x = 0;
    ufo_info.y = 0;
    ufo_info.direction = DIR_NONE;
    ufo_info.shot_x = 0;
    ufo_info.shot_y = 0;
    ufo_info.shot_direction = DIR_NONE;
    ufo_info.shot_hit_ground = 0;

    /* add the initial score of 0 - 0 */
    print_score(v20_win, tank_score, ufo_score);
    wrefresh(v20_win);

    wtimeout(v20_win, 0);               /* make wgetch non-blocking */
    srand((unsigned int)time(NULL));    /* seed the random number generator */

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

    /* read keys and move tank every 250ms until 'q' is pressed */
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

        wrefresh(v20_win);

        move_ufo(v20_win, &ufo_info);
        wrefresh(v20_win);

        move_tank_shot(v20_win, &tank_info);
        wrefresh(v20_win);

        if (0 == ufo_info.shot_hit_ground)
        {
            /* no shot exploding */
            move_ufo_shot(v20_win, &ufo_info);
        }

        if (tank_info.shot_y != -1)
        {
            /* check for ufo hit */
            check_tank_shot(v20_win, &tank_info, &ufo_info);
        }

        if (0 != ufo_info.shot_hit_ground)
        {
            /* shot is exploding on the ground */
            ufo_shot_hit_ground(v20_win, &ufo_info);
        }
        wrefresh(v20_win);

        print_score(v20_win, tank_score, ufo_score);
        wrefresh(v20_win);
    }

    delwin(v20_win);
    endwin();
    return 0;
}


int handle_keypress(WINDOW* win, tank_info_t *tank)
{
    int ch;

    /* now read a character from the keyboard */
    ch = wgetch(win);
    flushinp();         /* flush the input buffer */

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
            tank->x = move_tank(win, tank->x, DIR_LEFT);
            break;

        case 'C':
        case 'c':
            tank->x = move_tank(win, tank->x, DIR_RIGHT);
            break;

        case 'B':
        case 'b':
            /* shoot */
            if (tank->shot_y == -1)
            {
                /* there isn't a shot, so take it */
                tank->shot_y = TANK_SHOT_START;
                tank->shot_x = tank->x + 3;
            }
            break;

        default:
            break;
    }

    return 0;
}


int move_tank(WINDOW *win, int tank_x, direction_t direction)
{
    if (DIR_NONE == direction)
    {
        /* redraw without moving */
        mvwaddstr(win, V20_ROWS - 4, tank_x + 3, "▖");
        mvwaddstr(win, V20_ROWS - 3, tank_x + 1, "▁██▁");
        mvwaddstr(win, V20_ROWS - 2, tank_x, "▕OOOO▏");
    }

    if (DIR_LEFT == direction)
    {
        if (tank_x == 0)
        {
            /* can't go furter left */
            return tank_x;
        }

        /* move to the left, add a trailing space to erase the old */
        tank_x -= 1;
        mvwaddstr(win, V20_ROWS - 4, tank_x + 3, "▖ ");
        mvwaddstr(win, V20_ROWS - 3, tank_x + 1, "▁██▁ ");
        mvwaddstr(win, V20_ROWS - 2, tank_x, "▕OOOO▏ ");
    }

    if (DIR_RIGHT == direction)
    {
        if (tank_x == V20_COLS - 6)
        {
            /* can't go furter right */
            return tank_x;
        }

        /* move to the right, add a leading space to erase the old */
        mvwaddstr(win, V20_ROWS - 4, tank_x + 3, " ▖");
        mvwaddstr(win, V20_ROWS - 3, tank_x + 1, " ▁██▁");
        mvwaddstr(win, V20_ROWS - 2, tank_x, " ▕OOOO▏");
        tank_x += 1;
    }

    return tank_x;
}


void move_ufo(WINDOW* win, ufo_info_t *ufo)
{
    if (DIR_NONE == ufo->direction)
    {
        if (DIR_NONE != ufo->shot_direction)
        {
            return;         /* a shot is still falling, hold off */
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

        mvwaddstr(win, ufo->y, ufo->x, "<*>");
    }
    else if (DIR_RIGHT == ufo->direction)
    {
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
    }
    else
    {
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
    }
}


void move_tank_shot(WINDOW* win, tank_info_t *tank)
{
    const cchar_t shot = {A_NORMAL, L"▪", 0};

    if (tank->shot_y < 0)
    {
        return;     /* there's no shot */
    }

    if (tank->shot_y != TANK_SHOT_START)
    {
        /* erase old shot */
        mvwaddch(win, tank->shot_y, tank->shot_x, ' ');
    }

    /* move shot up */
    tank->shot_y--;

    if (tank->shot_y != SCORE_ROW)
    {
        /* draw new shot */
        mvwadd_wch(win, tank->shot_y, tank->shot_x, &shot);
    }
    else
    {
        /* done with shot */
        tank->shot_x = -1;
        tank->shot_y = -1;
    }
}


void check_tank_shot(WINDOW* win, tank_info_t *tank, ufo_info_t *ufo)
{
    const cchar_t box = {A_NORMAL, L"█", 0};
    int dx;

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
    mvwadd_wch(win, tank->shot_y - 1, tank->shot_x, &box);
    mvwaddstr(win, tank->shot_y, tank->shot_x - 1, "███");
    mvwadd_wch(win, tank->shot_y + 1, tank->shot_x, &box);

    /* done with shot */
    tank->shot_x = -1;
    tank->shot_y = -1;

    return;
}


#if 0
void move_ufo_shot(WINDOW* win, ufo_info_t *ufo)
{
    const cchar_t shot = {A_NORMAL, L"●", 0};

    if (DIR_NONE == ufo->shot_direction)
    {
        /* no shot, should we make one? */
        if (DIR_NONE == ufo->direction)
        {
            return;     /* no ufo, so don't make shot */
        }

        /* i made up the shot frequency */
        if (0 == (rand() % 5))
        {
            /* take the shot */
            ufo->shot_x = ufo->x + 1;       /* UFO center */
            ufo->shot_y = ufo->y + 1;       /* 1 below UFO */
            ufo->shot_direction = ufo->direction;
        }
        else
        {
            return;     /* don't shoot */
        }
    }
    else
    {
        /* erase old shot */
        mvwaddch(win, ufo->shot_y, ufo->shot_x, ' ');

        if (V20_ROWS - 2 == ufo->shot_y)
        {
            /* TODO: draw ground explosion */

            /* done with shot */
            ufo->shot_x = -1;
            ufo->shot_y = -1;
            ufo->shot_direction = DIR_NONE;
            return;
        }

        /* update shot position */
        ufo->shot_y++;

        if (DIR_RIGHT == ufo->shot_direction)
        {
            /* shot is headed right */
            ufo->shot_x++;

            if (V20_COLS == ufo->shot_x)
            {
                /* wrap the shot */
                ufo->shot_x = 0;
            }
        }
        else
        {
            /* shot is headed left */
            ufo->shot_x--;

            if (0 == ufo->shot_x)
            {
                /* wrap the shot */
                ufo->shot_x = V20_COLS - 1;
            }
        }
    }

    /* draw the new shot */
    mvwadd_wch(win, ufo->shot_y, ufo->shot_x, &shot);
}
#else
void move_ufo_shot(WINDOW* win, ufo_info_t *ufo)
{
    const cchar_t shot = {A_NORMAL, L"●", 0};

    if (DIR_NONE == ufo->shot_direction)
    {
        /* no shot, should we make one? */
        if (DIR_NONE == ufo->direction)
        {
            return;     /* no ufo, so don't make shot */
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
        else
        {
            /* going right */
            if (ufo->x - ufo->y > -1)
            {
                return;
            }
        }

        /* i made up the shot frequency */
        if (0 == (rand() % 3))
        {
            /* take the shot */
            ufo->shot_x = ufo->x + 1;       /* UFO center */
            ufo->shot_y = ufo->y + 1;       /* 1 below UFO */
            ufo->shot_direction = ufo->direction;
        }
        else
        {
            return;     /* don't shoot */
        }
    }
    else
    {
        /* erase old shot */
        mvwaddch(win, ufo->shot_y, ufo->shot_x, ' ');

        if (V20_ROWS - 2 == ufo->shot_y)
        {
            /* done with shot */
            ufo->shot_direction = DIR_NONE;
            ufo->shot_hit_ground = 1;
            return;
        }

        /* update shot position */
        ufo->shot_y++;

        if (DIR_RIGHT == ufo->shot_direction)
        {
            /* shot is headed right */
            ufo->shot_x++;
        }
        else
        {
            /* shot is headed left */
            ufo->shot_x--;
        }
    }

    /* draw the new shot */
    mvwadd_wch(win, ufo->shot_y, ufo->shot_x, &shot);
}
#endif

void ufo_shot_hit_ground(WINDOW* win, ufo_info_t *ufo)
{
    const cchar_t ground = {A_NORMAL, L"▔", 0};

    switch (ufo->shot_hit_ground)
    {
        case 1:
            /* just lines */
            ufo->shot_hit_ground++;
            mvwaddstr(win, V20_ROWS - 2, ufo->shot_x - 2, "╲ │ ╱");
            break;

        case 2:
            /* full explosion */
            ufo->shot_hit_ground++;
            mvwaddstr(win, V20_ROWS - 3, ufo->shot_x - 3, "•• • ••");
            mvwaddstr(win, V20_ROWS - 2, ufo->shot_x - 2, "╲ │ ╱");
            break;

        case 3:
            /* dots */
            ufo->shot_hit_ground++;
            mvwaddstr(win, V20_ROWS - 3, ufo->shot_x - 3, "•• • ••");
            mvwaddstr(win, V20_ROWS - 2, ufo->shot_x - 2, "     ");
            break;

        case 4:
            /* clean-up */
            ufo->shot_hit_ground = 0;
            mvwaddstr(win, V20_ROWS - 3, ufo->shot_x - 3, "       ");
            mvwaddstr(win, V20_ROWS - 2, ufo->shot_x - 2, "     ");
            ufo->shot_x = -1;
            ufo->shot_y = -1;
            ufo->shot_direction = DIR_NONE;

            /* redraw the ground */
            mvwhline_set(win, V20_ROWS - 1, 0, &ground, V20_COLS);
            break;

        default:
            break;
    }
}


void print_score(WINDOW* win, int tank_score, int ufo_score)
{
    mvwprintw(win, SCORE_ROW, 5, "%d", ufo_score);
    mvwprintw(win, SCORE_ROW, 15, "%d", tank_score);
}
