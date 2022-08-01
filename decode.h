#ifndef DECODE_H
#define DECODE_H

#include "structs.h"
#include "bit_stream.h"

huff_tree *read_tree(bit_stream *stream);

int decode_text(bit_stream *input, FILE *output, stc_tree *tree);

//----test----
int decode_text_new(FILE *input, FILE *output, tree4 *tree);

int decode_text16(FILE *input, FILE *output, tree16 *tree);

//----test----

int decode(const char *input_path, const char *output_path);

#endif