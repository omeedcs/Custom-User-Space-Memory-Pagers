CC = gcc
CFLAGS = -Wall

all: apager dpager hpager

apager: APager.c
	$(CC) $(CFLAGS) -o apager apager.c

dpager: DPager.c
	$(CC) $(CFLAGS) -o dpager dpager.c

hpager: HPager.c
	$(CC) $(CFLAGS) -o hpager hpager.c

clean:
	rm -f apager dpager hpager
