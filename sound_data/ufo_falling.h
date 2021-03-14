/***************************************************************************
*                             Tank Versuses UFO
*
*   File    : ufo_falling.h
*   Purpose : Sound of UFO on falling for a tribute to the Tank-V-UFO, a
*             Commodore VIC-20 Game by Duane Later
*   Author  : Michael Dipperstein
*   Date    : March 13, 2021
*
****************************************************************************
*
* Tank Versuses UFO: A tribute to the Tank-V-UFO, a Commodore VIC-20 Game
*                    by Duane Later
*
* Copyright (C) 2021 by Michael Dipperstein (mdipperstein@gmail.com)
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
static const int FALL_FREQ = 1000;       /* base frequency for falling ufo */

/*
 * pseudo code for generating the 1000Hz ufo_falling sine wave used
 * for the 1000Hz / 2000Hz sound as the UFO falls.
 *
 * int size = SAMPLE_RATE / FALL_FREQ;
 * for(i = 0; i < size; i++)
 * {
 *     ufo_falling[i] = (float)sin(((double)i/(double)size) * M_PI * 2.0);
 * }
 *
 */
const float ufo_falling[] =
{
    0.000000,
    0.142315,
    0.281733,
    0.415415,
    0.540641,
    0.654861,
    0.755750,
    0.841254,
    0.909632,
    0.959493,
    0.989821,
    1.000000,
    0.989821,
    0.959493,
    0.909632,
    0.841254,
    0.755750,
    0.654861,
    0.540641,
    0.415415,
    0.281733,
    0.142315,
    0.000000,
    -0.142315,
    -0.281733,
    -0.415415,
    -0.540641,
    -0.654861,
    -0.755750,
    -0.841254,
    -0.909632,
    -0.959493,
    -0.989821,
    -1.000000,
    -0.989821,
    -0.959493,
    -0.909632,
    -0.841254,
    -0.755750,
    -0.654861,
    -0.540641,
    -0.415415,
    -0.281733,
    -0.142315
};
