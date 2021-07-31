#ifndef DECODE_H
#define DECODE_H

#include "structs.h"
#include "bit_stream.h"

huff_tree *read_tree(bit_stream *stream);

int decode_text(bit_stream *input, FILE *output, stc_tree *tree);

int decode(const char *input_path, const char *output_path);

#endif