############################################################################
# Makefile for Tank versus UFO
############################################################################
CC = gcc -g
LD = gcc -g
CFLAGS = -Wall -Wextra `pkg-config ncursesw portaudio-2.0 --cflags`
LDFLAGS = `pkg-config ncursesw portaudio-2.0 --libs`

SD_FILES = sound_data.h sound_data/on_fire.h sound_data/tank_shot.h \
	sound_data/ufo_falling.h sound_data/explode.h

all:	tankvufo

tankvufo:	tankvufo.o tank.o ufo.o sounds.o
		$(LD) $^ $(LDFLAGS) -o $@

tankvufo.o:	tankvufo.c sounds.h tankvufo.h tank.h ufo.h
		$(CC) $(CFLAGS) -c $< -o $@

tank.o:	tank.c tank.h sounds.h tankvufo.h
		$(CC) $(CFLAGS) -c $< -o $@

ufo.o:	ufo.c ufo.h sounds.h tankvufo.h
		$(CC) $(CFLAGS) -c $< -o $@

sounds.o:	sounds.c sounds.h $(SD_FILES)
		$(CC) -c $< -Wall -Wextra `pkg-config portaudio-2.0 --cflags` -o $@

clean:
		rm tankvufo.o tank.o ufo.o sounds.o
		rm tankvufo
