CC = gcc
CFLAGS = -Wall -g -static

all: apager dpager hpager hello_world adding_nums null data crazy_manipulation longstring_longmath extreme_page_faulting

apager: apager.c
	$(CC) $(CFLAGS) -o apager apager.c -Wl,-Ttext-segment=0x70000000

dpager: dpager.c
	$(CC) $(CFLAGS) -o dpager dpager.c -Wl,-Ttext-segment=0x70000000

hpager: hpager.c
	$(CC) $(CFLAGS) -o hpager hpager.c

hello_world: hello_world.c
	$(CC) $(CFLAGS) -o hello_world hello_world.c

adding_nums: adding_nums.c
	$(CC) $(CFLAGS) -o adding_nums adding_nums.c

null : null.c
	$(CC) $(CFLAGS) -o null null.c

data: data.c
	$(CC) $(CFLAGS) -o data data.c

crazy_manipulation: crazy_manipulation.c
	$(CC) $(CFLAGS) -o crazy_manipulation crazy_manipulation.c

longstring_longmath: longstring_longmath.c
	$(CC) $(CFLAGS) -o longstring_longmath longstring_longmath.c

extreme_page_faulting: extreme_page_faulting.c
	$(CC) $(CFLAGS) -o extreme_page_faulting extreme_page_faulting.c

clean: 
	rm -f apager dpager hpager hello_world adding_nums null data crazy_manipulation longstring_longmath extreme_page_faulting
