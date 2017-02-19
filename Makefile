CC=gcc
LIBS=-lreadline

khol:
	$(CC) khol.c -o $@ $(LIBS)

clean: rm khol
