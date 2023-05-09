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

union buffer {
	__uint16_t pair;
	char sym[2];
};

int decode_text_new(FILE *input, FILE *output, tree4 *tree) {
	unsigned char sym;
	char outsym;
	char leaf_type = INTERNAL; //or LEAF
	union buffer buff;
	sym = getc(input);
	buff.sym[0] = sym;

	int ind = 0;

	int bits_used = 8;
	while (leaf_type != STOP) {
		sym = getc(input);

		buff.pair <<= (bits_used - 8);
		buff.sym[1] = sym;
		buff.pair >>= (bits_used - 8);
		bits_used -= 8;

		
		while (bits_used < 8) {
			int chld_ind = GET_BIT(buff.sym[0], 0) * 2 + GET_BIT(buff.sym[0], 1);
			if (tree[ind].children[chld_ind].mask.type != INTERNAL) {
				leaf_type = tree[ind].children[chld_ind].mask.type;
				if (leaf_type == LEAF) {
					outsym = tree[ind].children[chld_ind].sym;

					buff.pair >>= (2 - tree[ind].children[chld_ind].mask.rest);
					bits_used += (2 - tree[ind].children[chld_ind].mask.rest);

					ind = 0;
					putc(outsym, output);
				}
				else {
					break;
				}
			}
			else {
				ind = tree[ind].children[chld_ind].index;

				buff.pair >>= 2;
				bits_used += 2;
			}

		}
		
	}
}



int decode_text16(FILE *input, FILE *output, tree16 *tree) {
	unsigned char sym;
	char outsym;
	char leaf_type = INTERNAL; //or LEAF
	union buffer buff;
	sym = getc(input);
	buff.sym[0] = sym;

	int ind = 0;

	int ind_reversed[16] = { 0, 8, 4, 12, 2, 10, 6, 14, 1, 9, 5, 13, 3, 11, 7, 15 };

	int bits_used = 8;
	while (leaf_type != STOP) {
		sym = getc(input);

		buff.pair <<= (bits_used - 8);
		buff.sym[1] = sym;
		buff.pair >>= (bits_used - 8);
		bits_used -= 8;

		
		while (bits_used < 8) {

			int chld_ind = GET_BIT(buff.sym[0], 0) * 8 + GET_BIT(buff.sym[0], 1) * 4 + GET_BIT(buff.sym[0], 2) * 2 + GET_BIT(buff.sym[0], 3);
			if (tree[ind].children[chld_ind].mask.type != INTERNAL) {
				leaf_type = tree[ind].children[chld_ind].mask.type;
				if (leaf_type == LEAF) {
					outsym = tree[ind].children[chld_ind].sym;

					buff.pair >>= (4 - tree[ind].children[chld_ind].mask.rest);
					bits_used += (4 - tree[ind].children[chld_ind].mask.rest);

					ind = 0;
					// printf("%c\n", outsym);
					putc(outsym, output);
				}
				else {
					break;
				}
			}
			else {
				ind = tree[ind].children[chld_ind].index;

				buff.pair >>= 4;
				bits_used += 4;
			}

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

	/*//-------test------

	// print_tree(tree, 0);

	int blk_size = get_size_block(tree, 0);
	// printf("tree size: %d\n", blk_size);

	tree4 *tr4 = (tree4*)calloc(blk_size, sizeof(tree4));
	for (int i = 0; i < blk_size; i++) {
		for (int j = 0; j < 4; j++) {
			tr4[i].children[j].mask.type = INTERNAL;
		}
	}
	// print_tree4(tr4, blk_size);

	tree_to_tree4_v2(tree, tr4, 0, 0, 0, 0);

	// printf("\n");
	// print_tree4(tr4, blk_size);

	decode_text_new(input, output, tr4);

	fclose(input);
	fclose(output);

	return 0;
	//-------test------*/

	//-------test------

	// print_tree(tree, 0);

	int blk_size = get_size_block16(tree, 0);
	// printf("tree size: %d\n", blk_size);

	tree16 *tr16 = (tree16*)calloc(blk_size, sizeof(tree16));
	for (int i = 0; i < blk_size; i++) {
		for (int j = 0; j < 16; j++) {
			tr16[i].children[j].mask.type = INTERNAL;
		}
	}
	// print_tree4(tr4, blk_size);

	tree_to_tree16(tree, tr16, 0, 0, 0, 0);

	// printf("\n");
	// print_tree16(tr16, blk_size);

	decode_text16(input, output, tr16);

	fclose(input);
	fclose(output);

	return 0;
	//-------test------

	free_tree(tree);

	decode_text(input, output, stc);

	fclose(input);
	fclose(output);

	return 0;
}