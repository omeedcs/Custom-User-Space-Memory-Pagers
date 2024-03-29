CC = gcc
CFLAGS = -Wall

all: helloworld sumoftwo

helloworld: HelloWorld.c
	$(CC) $(CFLAGS) -o helloworld HelloWorld.c

sumoftwo: SumOfTwo.c
	$(CC) $(CFLAGS) -o sumoftwo SumOfTwo.c

clean:
	rm -f helloworld sumoftwo
