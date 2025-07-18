# tankvufo
## A tribute to Tank-V-UFO, a Commodore VIC-20 Game by Duane Later

## Description
This archive contains C and C++ source code for an ncursesw implementation of a
game similar to the game of Tank-V-UFO written by Duane Later for the Commodore
VIC-20.  The game play is similar to that of the original game play, where
similar is based on my memory and YouTube videos.

The source code in this archive is not derived from the VIC-20 source code.
The [ncursesw](https://invisible-island.net/ncurses/ "ncursesw") and
[portaudio](http://www.portaudio.com/ "portaudio") are libraries are required
to build this source.

The source code for the Commodore VIC-20 version of Tank-V-UFO was published in
_"Personal Computing on the VIC-20 A Friendly Computer Guide"_

The files contained in this archive are made available under version 3 of the
GNU GPL.

## Files

| File Name  | Contents |
| ---        | ---      |
| explode.h  | Definition of tank shot explosion sound |
| Makefile   | GNU Makefile for this project (assumes gcc compiler and pkg-config) |
| main.cpp   | Source to handle all of the game logic |
| on_fire.h  | Definition of fire sound effect |
| README.MD  | This file |
| sound_data.h | Header including all sound effects |
| sounds.h   | Header for sound effect functions |
| sounds.c   | Sound effects implemented using PortAudio |
| tank.h     | Header for tank and tank shot functions |
| tank.cpp   | Source for tank and tank shot functions |
| tankvufo.h | Header for all of the game elements |
| tankvufo.cpp | Source to handle all of the game elements |
| tank_shot.h | Definition of tank shot sound effect |
| tvu_defs.h | Definitions of types and values used by this game |
| ufo.h      | Header for ufo and tank shot functions |
| ufo.cpp    | Source for ufo and tank shot functions |
| ufo_falling.h | Definition of ufo falling sound effect |

## Building
To build these files with GNU make and g++:
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

The + key and - key may be used to increase and decrease the volume.

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

03/13/21
* Added on fire sound effect
* New tank shot sound effect
* Each sound now exits in its own file
* Tank and UFO data structures now contain pointers to sound data
  * Sounds are updated when the event that triggers them happens

04/27/21
* Added volume control using the +/- keys

05/23/21
* Reset sound phase when changing sounds
* Partition source into smaller files

06/20/21
* Added a list of extra commands to the left of the game field
* Added a volume indicator to the right of the game field
* Added sound for when UFO is hit be a shot

06/26/21
* Process entire key buffer in main loop instead of flushing after first key

12/27/23
* Started migration to C++
  * So far just the main game field of play and tank

## TODO
- Handle overlapping tank and UFO fires
- Prevent tanks from firing while UFO is falling or on fire
- Prevent superseded sound from canceling current sound

## BUGS
- Sometimes spay from UFO shot hitting the ground doesn't clear


## AUTHOR
Michael Dipperstein (mdipperstein@gmail.com)
