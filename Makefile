CC=gcc
OUT=build/gol
SRCS=$(wildcard src/*.c)
LIBS=
CFLAGS=-Iinclude

ifeq ($(OS),Windows_NT)
	LIBS+=-Llibs/win/ -lnfd -lole32 -lcomctl32 -loleaut32 -luuid -lmingw32
endif

LIBS += -lSDL2main -lSDL2

all:
	$(CC) $(CFLAGS) -o $(OUT) $(SRCS) $(LIBS)
	$(OUT)

run:
	$(OUT)

build:
	$(CC) $(CFLAGS) -o $(OUT) $(SRCS) $(LIBS)