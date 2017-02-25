CC=gcc
LIBS=-lreadline

all: khol

khol: khol.c
	$(CC) $^ -o $@ $(LIBS)

clean:
	- rm khol

.PHONY: all test clean
