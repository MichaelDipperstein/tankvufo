# tankvufo
## A tribute to Tank-V-UFO, a Commodore VIC-20 Game by Duane Later

## Description
This archive contains C source code for an ncursesw implementation of a game
similar to the game of Tank-V-UFO written by Duane Later for the Commodore
VIC-20.  The game play is similar to that of the original game play, where
similar is based on my memory and YouTube videos.

The source code in this archive is not derived from the VIC-20 source code.
The [ncursesw](https://invisible-island.net/ncurses/ "ncursesw") and
[portaudio](http://www.portaudio.com/ "portaudio") and libraries are required
to build this source.

The source code for the Commodore VIC-20 version of Tank-V-UFO was published in
_"Personal Computing on the VIC-20 A Friendly Computer Guide"_

The files contained in this archive are made available under version 3 of the
GNU GPL.

## Files

| File Name  | Contents |
| ---        | ---      |
| Makefile   | GNU Makefile for this project (assumes gcc compiler and pkg-config) |
| README.MD  | This file |
| sound_data.h | Header with arrays defining sound effcets |
| sounds.h   | C header for sound effect functions |
| sounds.c   | Sound effects implemented using PortAudio |
| tankvufo.c | C source to handle all of the game logic |

## Building
To build these files with GNU make and gcc:
1. Open a terminal
2. Change directory to the directory containing this archive
3. Enter the command "make" from the command line.

**NOTE:** The [ncursesw](https://invisible-island.net/ncurses/ "ncursesw")
library and the [portaudio](http://www.portaudio.com/ "portaudio") library are
required to build this code.  pkg-config must be configured for both libraries.

## Game Play
Control the tank and try to shoot the UFO without being shot.  The tank is
controlled using the keyboard.

Z moves left, C moves right, and B fires.

Only one tank shot may be fired at a time.  A second tanks shot cannot be fired
until the first tank shot goes off the screen.

The Q key may be used to quit the game.

## History
12/09/20
* Initial release

12/24/20
* Added graphical explosions and fire

02/07/21
* Added sound for crashing UFO

02/15/21
* Removed ncursesw6-config from the Makefile
* Added this README file

## TODO
- Add sound for burning tank and UFO
- Add sound for shots fired

## BUGS
- No known bugs

## AUTHOR
Michael Dipperstein (mdipperstein@gmail.com)
