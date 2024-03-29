CC = gcc
CFLAGS = -Wall

all: apager dpager hpager ELF

apager: apager.c
	$(CC) $(CFLAGS) -o apager apager.c

dpager: dpager.c
	$(CC) $(CFLAGS) -o dpager dpager.c

hpager: hpager.c
	$(CC) $(CFLAGS) -o hpager hpager.c

ELF: ELF.c
	$(CC) $(CFLAGS) -o ELF ELF.c

clean:
	rm -f apager dpager hpager ELF
