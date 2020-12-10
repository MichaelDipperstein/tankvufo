############################################################################
# Makefile for Tank versus UFO
############################################################################
CC = gcc
LD = gcc
CFLAGS = -Wall -Wextra `ncursesw6-config --cflags`
LDFLAGS = `ncursesw6-config --libs`

all:	tankvufo

tankvufo:	tankvufo.o
		$(LD) $< $(LDFLAGS) -o $@

tankvufo.o:	tankvufo.c
		$(CC) $(CFLAGS) -c $< -o $@

clean:
		rm tankvufo.o
		rm tankvufo
