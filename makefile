all: main.o bit_stream.o
	gcc *.o -o huff -lm

main.o: main.c
	gcc -c main.c

bit_stream.o: bit_stream.c
	gcc -c bit_stream.c

clean:
	rm *.o
