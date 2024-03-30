CC = gcc
CFLAGS = -Wall

all: apager dpager hpager 

apager: apager.c
	$(CC) $(CFLAGS) -o apager apager.c

dpager: dpager.c
	$(CC) $(CFLAGS) -o dpager dpager.c

hpager: hpager.c
	$(CC) $(CFLAGS) -o hpager hpager.c

clean:
	rm -f apager dpager hpager
