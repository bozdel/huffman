all: main.o bit_stream.o structs.o encode.o decode.o
	gcc *.o -o huff -lm

main.o: main.c
	gcc -c main.c

bit_stream.o: bit_stream.c
	gcc -c bit_stream.c

structs.o: structs.c
	gcc -c structs.c

encode.o: encode.c
	gcc -c encode.c

decode.o: decode.c
	gcc -c decode.c

clean:
	rm *.o
