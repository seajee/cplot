CC=gcc
CFLAGS=-Wall -Wextra -ggdb
LDFLAGS=`pkg-config --libs raylib` -lm

all: cplot

cplot: main.c
	$(CC) $(CFLAGS) -o cplot main.c $(LDFLAGS)

clean:
	rm cplot
