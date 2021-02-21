############################################################################
# Makefile for Tank versus UFO
############################################################################
CC = gcc
LD = gcc
CFLAGS = -Wall -Wextra `pkg-config ncursesw portaudio-2.0 --cflags`
LDFLAGS = `pkg-config ncursesw portaudio-2.0 --libs`

all:	tankvufo

tankvufo:	tankvufo.o sounds.o
		$(LD) $^ $(LDFLAGS) -o $@

tankvufo.o:	tankvufo.c sounds.h
		$(CC) $(CFLAGS) -c $< -o $@

sounds.o:	sounds.c sounds.h sound_data.h
		$(CC) -c $< -Wall -Wextra `pkg-config portaudio-2.0 --cflags` -o $@

clean:
		rm tankvufo.o sounds.o
		rm tankvufo
