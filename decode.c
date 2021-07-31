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

// returns 1 if symbol was readed, otherwise 0 (stop sequence)
int read_encoded_sym(bit_stream *input, huff_tree *tree, unsigned char *sym) {
	int bit;
	while (tree->type == INTERNAL) {
		read_bit(input, &bit);
		if (bit) {
			tree = tree->right;
		}
		else {
			tree = tree->left;
		}
	}
	*sym = tree->sym;
	return tree->type == LEAF; //or !(tree->type)
}

int decode_text(bit_stream *input, FILE *output, huff_tree *tree) {
	unsigned char sym;
	int read_res;

	while ( (read_res = read_encoded_sym(input, tree, &sym)) ) {
		fwrite(&sym, sizeof(char), 1, output);
	}

	if (read_res == 0) {
		return 1; //all is ok
	}
	else {
		return -1; //something wrong with reading
	}
}



int decode_text2(bit_stream *input, FILE *output, stc_tree *tree) {
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

/*int foo(FILE *input, FILE *output, stc_tree *tree) {
	char sym;
	char outsym;
	int index = 0;
	char leaf_type = INTERNAL; //or LEAF

	while (leaf_type != STOP) {
		sym = (char)getc(input);

		for (int n = 7; n >= 0; n--) {

			if (tree[index].children[0] < 0) {
				left_type = -(tree[index].children[0]) - 1;
				outsym = -(tree[index].children[1])
				putc(output, outsym);
				index = 0;
			}
			else {
				index = tree[index].children[GET_BIT(sym, n)];
			}

		}
	}
}*/

/*union bar {
	short int i;
	char alo[2];
};

int foo2(FILE *input, FILE *output, tree16 *tree) {

	unsigned char mask = ~((1 << 4) - 1);
	union bar buff;
	while (leaf_type != STOP) {
		getc(buff.alo[1]);
		while (buff.alo[1] > 0) {
			if (tree[index].children[0] < 0) {
				//left_type = -(tree[index].children[0]) - 1;
				//outsym = -(tree[index].children[baz]);
				int data_index = -tree[index].children[baz];
				leaf_type = data[data_index].type;
				outsym = data[data_index].sym;
				int offset = data[data_index].offset;
				putc(output, outsym);
				index = 0;
				buff.i >>= offset;
			}
			else {
				baz = buff.alo[0] & mask;
				index = tree[index].children[baz];
				buff.i <<= 4;
			}
		}
	}
}*/

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

	decode_text2(input, output, stc);


	close_bs(input);
	fclose(output);

	return 0;
}