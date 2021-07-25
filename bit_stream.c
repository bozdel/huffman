#include <stdlib.h>
#include "bit_stream.h"

bit_stream *open_bs(const char *path, char *type) {
	bit_stream *stream = (bit_stream*)malloc(sizeof(bit_stream));

	stream->buff = (unsigned char*)calloc(BUFF_SIZE, sizeof(char));
	stream->file = fopen(path,type);
	if (!stream->file) {
		free(stream->buff);
		free(stream);
		return NULL;
	}
	stream->pos = -1;

	return stream;
}

void close_bs(bit_stream *stream) {
	//complete writing (zeros)
	flush_buff(stream, ALL);
	fclose(stream->file);
	free(stream->buff);
	free(stream);
}

void write_bit(bit_stream *stream, int bit) {
	if (stream->pos == -1) stream->pos = 0; //shit code
	if (stream->pos + 1 > BUFF_SIZE * 8) flush_buff(stream, BUFF_SIZE);

	unsigned char *cur_byte = &(stream->buff[stream->pos / 8]);

	SET_CON(*cur_byte, stream->pos % 8, bit);
	
	stream->pos++;
}

void write_byte(bit_stream *stream, unsigned char sym) {
	for (int i = 0; i < 8; i++) {
		write_bit(stream, sym & 1);
		sym >>= 1;
	}
}

int read_bit(bit_stream *stream, int *bit) {
	if (stream->pos + 1 > BUFF_SIZE * 8 || stream->pos == -1) {
		int readed = fread(stream->buff, sizeof(char), BUFF_SIZE, stream->file);
		if (!readed) {
			return 0;
		}
		stream->pos = 0;
	}

	unsigned char cur_byte = stream->buff[stream->pos / 8];

	*bit = GET_BIT(cur_byte, stream->pos % 8);

	stream->pos++;

	return 1;
}

int read_byte(bit_stream *stream, unsigned char *sym) {
	int bit;
	for (int i = 0; i < 8; i++) {
		read_bit(stream, &bit);

		SET_CON(*sym, i, bit);
	}
}

void flush_buff(bit_stream *stream, int flushing_size) {
	if (flushing_size == ALL) {
	  unsigned char *cur_byte = &(stream->buff[stream->pos / 8]);
	  while (stream->pos % 8 != 0) {
		CLR_BIT(*cur_byte, stream->pos % 8);
		stream->pos++;
	  }
	  fwrite(stream->buff, sizeof(char), stream->pos / 8, stream->file);
	}

	fwrite(stream->buff, sizeof(char), flushing_size, stream->file);
	for (int i = 0; i < BUFF_SIZE - flushing_size; i++) {
		stream->buff[i] = stream->buff[i + flushing_size];
	}
	stream->pos -= flushing_size * 8;
}