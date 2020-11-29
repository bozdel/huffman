all: main.o
	gcc main.c -o huff

main.o: main.c
	gcc -c main.c

clean:
	rm *.o
