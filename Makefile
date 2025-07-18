############################################################################
# Makefile for Tank versus UFO
############################################################################
CPP = g++ -g3
LD = g++ -g3

CFLAGS = -Wall -Wextra `pkg-config ncursesw portaudio-2.0 --cflags`
LDFLAGS = `pkg-config ncursesw portaudio-2.0 --libs`

SD_FILES = sound_data.h sound_data/on_fire.h sound_data/tank_shot.h \
	sound_data/ufo_falling.h sound_data/explode.h

all:	tankvufo

tankvufo:	main.o tankvufo.o tank.o ufo.o sounds.o
		$(LD) $^ $(LDFLAGS) -o $@

main.o:	main.cpp sounds.h tvu_defs.h
		$(CPP) $(CFLAGS) -c $< -o $@

tankvufo.o:	tankvufo.cpp sounds.h tankvufo.h tank.h ufo.h
		$(CPP) $(CFLAGS) -c $< -o $@

tank.o:	tank.cpp tank.h sounds.h tvu_defs.h
		$(CPP) $(CFLAGS) -c $< -o $@

ufo.o:	ufo.cpp ufo.h sounds.h tvu_defs.h
		$(CPP) $(CFLAGS) -c $< -o $@

sounds.o:	sounds.c sounds.h $(SD_FILES)
		$(CPP) -c $< -Wall -Wextra `pkg-config portaudio-2.0 --cflags` -o $@

clean:
		rm -f main.o tankvufo.o tank.o ufo.o sounds.o
		rm -f tankvufo
