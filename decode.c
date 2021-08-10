#include "structs.h"
#include "bit_stream.h"
#include <stdio.h>
#include <stdlib.h>

#include "debug.h"

huff_tree *read_tree(FILE *input, unsigned char *buff, int *pos) {
	huff_tree *tree = NULL;
	int bit;

	if (*pos >= 8) {
		*pos = 0;
		*buff = getc(input);
		if (feof(input)) return NULL; //something wrong
	}
	bit = GET_BIT(*buff, (*pos)++);

	if (bit == 0) { //internal
		tree = create_node(0, INTERNAL, 1);
		tree->left = read_tree(input, buff, pos);
		tree->right = read_tree(input, buff, pos);
		return tree;
	}
	else {

		if (*pos >= 8) {
			*pos = 0;
			*buff = getc(input);
			if (feof(input)) return NULL; //something wrong
		}
		bit = GET_BIT(*buff, (*pos)++);

		if (bit == 0) { //sym
			unsigned char sym;

			for (int i = 0; i < 8; i++) {
				if (*pos >= 8) {
					*pos = 0;
					*buff = getc(input);
					if (feof(input)) return NULL; //something wrong
				}
				SET_CON(sym, i, GET_BIT(*buff, (*pos)++));
			}
			
			return create_node(sym, LEAF, 1);
		}
		else { //stop sequence
			return create_node(0, STOP, 0);
		}
	}
}

int decode_text(FILE *input, FILE *output, stc_tree *tree) {
	unsigned char sym;
	char outsym;
	int index = 0;
	char leaf_type = INTERNAL; //or LEAF

	while (leaf_type != STOP) {
		sym = getc(input);

		for (int n = 0; n < 8; n++) {

			if (tree[index].children[0] < 0) {
				leaf_type = -(tree[index].children[0]) - 1;
				outsym = -(tree[index].children[1]);
				if (leaf_type == STOP) break;
				putc(outsym, output);
				index = 0;
			}
			index = tree[index].children[GET_BIT(sym, n)];

		}
	}
}

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
	FILE *input = fopen(input_path, "rb");
	if (!input) {
		return 0;
	}


	unsigned char buff = 0;
	int pos = 8;
	huff_tree *tree = read_tree(input, &buff, &pos);

	int tree_size = get_tree_size(tree);
	stc_tree *stc = (stc_tree*)malloc(sizeof(stc_tree) * tree_size);
	dyn_to_stc(tree, stc, 0, 0);

	free_tree(tree);

	decode_text(input, output, stc);

	fclose(input);
	fclose(output);

	return 0;
}