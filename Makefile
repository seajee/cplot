CC=gcc
CFLAGS=-Wall -Wextra -ggdb
LDFLAGS=`pkg-config --libs raylib` -lm

all: build/cplot

build/cplot: main.c mp.h
	@mkdir -p build/
	$(CC) $(CFLAGS) -o build/cplot main.c $(LDFLAGS)

clean:
	rm -rf build/
