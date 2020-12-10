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

typedef enum
{
    DIR_LEFT,
    DIR_RIGHT,
    DIR_NONE
} direction_t;

typedef struct
{
    int x;
    int y;
    direction_t direction;
} ufo_into_t;


int handle_keypress(WINDOW* win, int tank_x);
int move_tank(WINDOW* win, int tank_x, direction_t direction);
void move_ufo(WINDOW* win, ufo_into_t *ufo);
void print_score(WINDOW* win, int tank_score, int ufo_score);

int main(void)
{
    const cchar_t ground = {A_NORMAL, L"▔", 0};
    WINDOW *v20_win;
    int win_x, win_y;
    int tank_x = 0;
    ufo_into_t ufo_info;
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

    /* color the background before creating a window S*/
    init_pair(1, COLOR_BLACK, COLOR_CYAN);
    bkgd(COLOR_PAIR(1));

    win_x = (COLS - V20_COLS) / 2;
    win_y =  (LINES - V20_COLS) / 2;
    v20_win = newwin(V20_ROWS, V20_COLS, win_y, win_x);

    refresh();          /* refresh the whole screen to show the window */

    wbkgd(v20_win, COLOR_PAIR(1));  /* now set the window background */

    /* print the banner */
    wprintw(v20_win, "** TANK VERSUS UFO. **");
    wprintw(v20_win, "Z-LEFT,C-RIGHT,B-FIRE ");
    wprintw(v20_win, "UFO:     TANK:");

    /* draw the ground */
    mvwhline_set(v20_win, V20_ROWS - 1, 0, &ground, V20_COLS);

    /* draw the tank on the left edge */
    tank_x = move_tank(v20_win, tank_x, DIR_NONE);

    /* start without a ufo */
    ufo_info.x = 0;
    ufo_info.y = 0;
    ufo_info.direction = DIR_NONE;

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

        tank_x = handle_keypress(v20_win, tank_x);

        if (tank_x < 0)
        {
            /* we got a quit key */
            break;
        }

        move_ufo(v20_win, &ufo_info);
        print_score(v20_win, tank_score, ufo_score);

        wrefresh(v20_win);
    }

    delwin(v20_win);
    endwin();
    return 0;
}


int handle_keypress(WINDOW* win, int tank_x)
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
            tank_x = -1;
            break;

        case 'Z':
        case 'z':
            tank_x = move_tank(win, tank_x, DIR_LEFT);
            break;

        case 'C':
        case 'c':
            tank_x = move_tank(win, tank_x, DIR_RIGHT);
            break;

        case 'B':
        case 'b':
            /* this is shoot */
            break;

        default:
            break;
    }

    return tank_x;
}


int move_tank(WINDOW *win, int tank_x, direction_t direction)
{
    if (DIR_NONE == direction)
    {
        /* redraw without moving */
        mvwaddstr(win, V20_ROWS - 4, tank_x + 3, "▖");
        mvwaddstr(win, V20_ROWS - 3, tank_x + 1, "▁██▁");
        mvwaddstr(win, V20_ROWS - 2, tank_x + 0, "▕OOOO▏");
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
        mvwaddstr(win, V20_ROWS - 2, tank_x + 0, "▕OOOO▏ ");
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
        mvwaddstr(win, V20_ROWS - 2, tank_x + 0, " ▕OOOO▏");
        tank_x += 1;
    }

    return tank_x;
}


void move_ufo(WINDOW* win, ufo_into_t *ufo)
{
    if (DIR_NONE == ufo->direction)
    {
        /* no ufo, make one */
        ufo->y = 4 + (rand() % 14);

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
        if (V20_COLS - 4 == ufo->x)
        {
            /* ufo is at the edge, remove it */
            mvwaddstr(win, ufo->y, ufo->x, "   ");

            ufo->x = 0;
            ufo->y = 0;
            ufo->direction = DIR_NONE;
            return;
        }

        mvwaddstr(win, ufo->y, ufo->x, " <*>");
        ufo->x++;
    }
    else
    {
        /* ufo is moving left */
        if (0 == ufo->x)
        {
            /* ufo is at the edge, remove it */
            mvwaddstr(win, ufo->y, ufo->x, "   ");

            ufo->x = 0;
            ufo->y = 0;
            ufo->direction = DIR_NONE;
            return;
        }

        ufo->x--;
        mvwaddstr(win, ufo->y, ufo->x, "<*> ");
    }
}


void print_score(WINDOW* win, int tank_score, int ufo_score)
{
    mvwprintw(win, 2, 5, "%d", ufo_score);
    mvwprintw(win, 2, 15, "%d", tank_score);
}
