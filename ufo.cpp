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

Ufo::Ufo(WINDOW *window, int upperLim, int lowerLim, Sounds &sound_obj) :
    soundObj(sound_obj)
{
    /* start without a ufo and no shot */
    pos.x = 0;
    pos.y = 0;
    upperLimit = upperLim;
    lowerLimit = lowerLim;
    direction = Tvu::DIR_NONE;
    ufoHitGround = 0;
    shotPos.x = -1;
    shotPos.y = -1;
    shotDirection = Tvu::DIR_NONE;
    shotHitGround = 0;
    numberDied = 0;
    win = window;
    getmaxyx(window, rows, cols);

    srand((unsigned int)time(NULL));    /* seed the random number generator */
}


void Ufo::Move(void)
{
    switch (direction)
    {
        case Tvu::DIR_NONE:
            if (Tvu::DIR_NONE != shotDirection)
            {
                /* a shot is still falling, hold off */
                break;
            }

            /* no ufo or shot make a ufo */
            pos.y = upperLimit + (rand() % (lowerLimit - upperLimit));

            if (rand() % 2)
            {
                /* start on left */
                pos.x = 0;
                direction = Tvu::DIR_RIGHT;
            }
            else
            {
                /* start on right */
                pos.x = cols - 4;
                direction = Tvu::DIR_LEFT;
            }

            UfoShotDecision();
            mvwaddstr(win, pos.y, pos.x, "<*>");
            break;

        case Tvu::DIR_RIGHT:
            /* ufo is moving right */
            if ((lowerLimit == pos.y) && (cols - 3 == pos.x))
            {
                /* we're at the bottom , done with this one */
                mvwaddstr(win, pos.y, pos.x, "   ");
                pos.x = 0;
                pos.y = 0;
                direction = Tvu::DIR_NONE;
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

            UfoShotDecision();
            break;

        case Tvu::DIR_LEFT:
            /* ufo is moving left */
            if (2 == pos.x)
            {
                /* ufo is at the left edge, remove old ufo */
                mvwaddstr(win, pos.y, pos.x, "   ");

                /* go up a row */
                pos.y--;

                if (upperLimit > pos.y)
                {
                    /* we're at the top, done with this one */
                    pos.x = 0;
                    pos.y = 0;
                    direction = Tvu::DIR_NONE;
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

            UfoShotDecision();
            break;

        case Tvu::DIR_FALLING_RIGHT:
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

            if (Tvu::TANK_TREAD_ROW == pos.y)
            {
                /* we're at the bottom, done with this one */
                direction = Tvu::DIR_LANDED;
                ufoHitGround = 0;
                soundObj.SelectSound(SOUND_ON_FIRE);
            }
            else
            {
                soundObj.NextUfoSound();
            }
            break;

        case Tvu::DIR_FALLING_LEFT:
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

            if (Tvu::TANK_TREAD_ROW == pos.y)
            {
                /* we're at the bottom, done with this one */
                direction = Tvu::DIR_LANDED;
                ufoHitGround = 0;
                soundObj.SelectSound(SOUND_ON_FIRE);
            }
            else
            {
                soundObj.NextUfoSound();
            }
            break;

        case Tvu::DIR_LANDED:
            if (10 == ufoHitGround)
            {
                ufoHitGround = 0;
                direction = Tvu::DIR_NONE;
                mvwaddstr(win, pos.y - 1, pos.x, "   ");
                mvwaddstr(win, pos.y, pos.x, "   ");

                /* redraw the ground */
                mvwhline_set(win, rows - 1, 0, &GROUND_CHAR, cols);

                /* stop the fire sound */
                soundObj.SelectSound(SOUND_OFF);

                numberDied += 1;      /* credit tank with kill */
            }
            else
            {
                ufoHitGround += 1;
                wattron(win, COLOR_PAIR(3));       /* fire color */

                if (ufoHitGround % 2)
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


Tvu::Pos Ufo::GetPos(void) const
{
    return pos;
}


uint8_t Ufo::GetUfosKilled(void) const
{
    return numberDied;
}


sound_error_t Ufo::SetFalling(void)
{
    if (Tvu::DIR_LEFT == direction)
    {
        direction = Tvu::DIR_FALLING_LEFT;
    }
    else if (Tvu::DIR_RIGHT == direction)
    {
        direction = Tvu::DIR_FALLING_RIGHT;
    }

    /* start the ufo falling sound */
    soundObj.NextUfoSound();
    soundObj.RestartSoundStream();

    sound_error_t soundError;
    soundError = soundObj.GetError();

    if (0 != soundError)
    {
        soundObj.HandleError();
    }

    return soundError;
}


void Ufo::UfoShotDecision(void)
{
    if ((Tvu::DIR_NONE != shotDirection) || (shotHitGround))
    {
        /* there's already a shot */
        return;
    }

    /* don't take a shot that will go over the edge */
    if (Tvu::DIR_LEFT == direction)
    {
        /* going left */
        if (pos.x + pos.y < 23)
        {
            return;
        }
    }
    else if (Tvu::DIR_RIGHT == direction)
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
        /* prime the position for MoveShot()  */
        if (Tvu::DIR_RIGHT == direction)
        {
            /* shot will head right */
            shotDirection = Tvu::DIR_FALLING_RIGHT;
            shotPos.x = pos.x;
        }
        else
        {
            /* shot will head left */
            shotDirection = Tvu::DIR_FALLING_LEFT;
            shotPos.x = pos.x + 2;
        }

        shotPos.y = pos.y;           /* UFO row */
    }
}


void Ufo::MoveShot(void)
{
    cchar_t c;

    if ((Tvu::DIR_NONE == shotDirection) || (0 != shotHitGround))
    {
        /* there is no shot to move */
        return;
    }

    /* erase old shot if it hasn't been overwritten */
    mvwin_wch(win, shotPos.y, shotPos.x, &c);

    if (UFO_SHOT_CHAR.chars[0] == c.chars[0])
    {
        mvwaddch(win, shotPos.y, shotPos.x, ' ');
    }

    if (Tvu::TANK_TREAD_ROW == shotPos.y)
    {
        /* done with shot */
        shotDirection = Tvu::DIR_NONE;
        shotHitGround = 1;
        wrefresh(win);
        return;
    }

    /* update shot position */
    shotPos.y++;

    if (Tvu::DIR_FALLING_RIGHT == shotDirection)
    {
        /* shot is headed right */
        shotPos.x++;
    }
    else if (Tvu::DIR_FALLING_LEFT == shotDirection)
    {
        /* shot is headed left */
        shotPos.x--;
    }

    /* draw the new shot */
    mvwadd_wch(win, shotPos.y, shotPos.x, &UFO_SHOT_CHAR);
    wrefresh(win);
}


Tvu::Pos Ufo::GetShotPos(void) const
{
    return shotPos;
}


void Ufo::ClearShot(bool erase)
{
    if (erase && (-1 != shotPos.y))
    {
        /* erase any existing shot */
        cchar_t c;

        /* erase old shot if it hasn't been overwritten */
        mvwin_wch(win, shotPos.y, shotPos.x, &c);

        if (UFO_SHOT_CHAR.chars[0] == c.chars[0])
        {
            mvwaddch(win, shotPos.y, shotPos.x, ' ');
        }
    }

    /* clear shot data */
    shotPos.x = -1;
    shotPos.y = -1;
    shotDirection = Tvu::DIR_NONE;
}


bool Ufo::IsShotFalling(void) const
{
    return (Tvu::DIR_NONE != shotDirection);
}


bool Ufo::IsShotExploding(void) const
{
    return (0 != shotHitGround);
}


int Ufo::UpdateShotPhase(void)
{
    int clean_up = 0;
    int8_t shot_x;

    shot_x = shotPos.x;

    switch (shotHitGround)
    {
        case 1:
            /* just lines */
            shotHitGround++;
            mvwaddstr(win, Tvu::TANK_TREAD_ROW, shot_x - 2, "╲ │ ╱");
            break;

        case 2:
            /* full explosion */
            shotHitGround++;
            mvwaddstr(win, Tvu::TANK_TURRET_ROW, shot_x - 3, "•• • ••");
            mvwaddstr(win, Tvu::TANK_TREAD_ROW, shot_x - 2, "╲ │ ╱");
            break;

        case 3:
            /* dots */
            shotHitGround++;
            mvwaddstr(win, Tvu::TANK_TURRET_ROW, shot_x - 3, "•• • ••");
            mvwaddstr(win, Tvu::TANK_TREAD_ROW, shot_x - 2, "     ");
            break;

        case 4:
            /* clean-up */
            shotHitGround = 0;
            mvwaddstr(win, Tvu::TANK_TURRET_ROW, shot_x - 3, "       ");
            mvwaddstr(win, Tvu::TANK_TREAD_ROW, shot_x - 2, "     ");
            shotPos.x = -1;
            shotPos.y = -1;
            shotDirection = Tvu::DIR_NONE;

            /* indicate need to redraw ground and tank */
            clean_up = 1;
            break;

        default:
            break;
    }

    wrefresh(win);
    return clean_up;
}
