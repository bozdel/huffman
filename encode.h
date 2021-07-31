#ifndef ENCODE_H
#define ENCODE_H

#include "structs.h"
#include "bit_stream.h"

huff_tree *create_huff_tree(FILE *input, huff_tree **mem_block_ptr);

void write_huff_tree(bit_stream *output, huff_tree *tree);

void encode_text(FILE *input, bit_code *table, bit_stream *output);

void encode_text2(FILE *input, bit_code *table, FILE *output);

int db_encode(const char *input_path, const char *output_path);

int encode(const char *input_path, const char *output_path);

#endif