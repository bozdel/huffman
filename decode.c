#include "structs.h"
#include "bit_stream.h"
#include <stdio.h>
#include <stdlib.h>

huff_tree *read_tree(bit_stream *stream) {
	huff_tree *tree = NULL;
	int bit;
	int readed = read_bit(stream, &bit);
	if (!readed) return NULL; //something wrong

	if (bit == 0) { //internal
		tree = create_node(0, INTERNAL, 1);
		tree->left = read_tree(stream);
		tree->right = read_tree(stream);
		return tree;
	}
	else {
		readed = read_bit(stream, &bit);
		if (!readed) return NULL; //something wrong

		if (bit == 0) { //sym
			unsigned char sym;
			read_byte(stream, &sym);
			return create_node(sym, LEAF, 1);
		}
		else { //stop sequence
			return create_node(0, STOP, 0);
		}
	}
}


int decode_text(bit_stream *input, FILE *output, stc_tree *tree) {
	unsigned char sym;
	int index = 0;
	int leaf_type = INTERNAL;

	int bit;
	while (leaf_type != STOP) {
		while (tree[index].children[0] > 0) {
			read_bit(input, &bit);
			index = tree[index].children[bit];
		}
		leaf_type = -(tree[index].children[0]) - 1;
		sym = -(tree[index].children[1]);
		index = 0;
		if (leaf_type == LEAF) {
			putc(sym, output);
		}
	}
}




int decode(const char *input_path, const char *output_path) {
	FILE *output = fopen(output_path, "wb");
	if (!output) {
		return 1;
	}
	bit_stream *input = open_bs(input_path, "rb");
	if (!input) {
		return 1;
	}

	huff_tree *tree = read_tree(input);

	int tree_size = get_tree_size(tree);
	stc_tree *stc = (stc_tree*)malloc(sizeof(stc_tree) * tree_size);
	dyn_to_stc(tree, stc, 0, 0);

	free_tree(tree);

	decode_text(input, output, stc);


	close_bs(input);
	fclose(output);

	return 0;
}