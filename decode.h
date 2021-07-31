#ifndef DECODE_H
#define DECODE_H

#include "structs.h"
#include "bit_stream.h"

huff_tree *read_tree(bit_stream *stream);

int read_encoded_sym(bit_stream *input, huff_tree *tree, unsigned char *sym);

int decode_text(bit_stream *input, FILE *output, huff_tree *tree);

int decode_text2(bit_stream *input, FILE *output, stc_tree *tree);

int decode(const char *input_path, const char *output_path);

#endif