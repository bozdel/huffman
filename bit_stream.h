#ifndef BIT_STREAM_H
#define BIT_STREAM_H

#include <stdio.h>

#define GET_BIT(a,n)   ( ((a) >> (n)) & 1     )
#define SET_BIT(a,n)   ( (a) |= (1 << (n))    )
#define CLR_BIT(a,n)   ( (a) &= (~(1 << (n))) )
#define SET_CON(a,n,b) ( (a) = ((a) & (~(1 << (n)))) | ((b) << (n)) ) //setting conditionaly bit

#define BUFF_SIZE 1000
#define ALL 0

//try to add dllist *cur_byte
typedef struct {
	FILE *file;
	unsigned char *buff;
	int pos;
} bit_stream;

bit_stream *open_bs(const char *path, char *type);

void close_bs(bit_stream *stream);


void write_bit(bit_stream *stream, int bit);

void write_byte(bit_stream *stream, unsigned char sym);

int read_bit(bit_stream *stream, int *bit);

int read_byte(bit_stream *stream, unsigned char *sym);


void flush_buff(bit_stream *stream, int flushing_size);

#endif