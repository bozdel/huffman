#ifndef STRUCTS_H
#define STRUCTS_H

#define LEAF 0
#define STOP 1
#define INTERNAL 2

typedef struct huff_tree {
	unsigned char sym;
	char type;
	int freq;
	struct huff_tree *next;
	struct huff_tree *left; //0
	struct huff_tree *right; //1
} huff_tree;

typedef struct bit_code {
	char leng; //zero leng - internal, negative - safeword (end of file)
	long int code; //encoded symbol
} bit_code;

typedef struct stc_tree {
	int children[2];
} stc_tree;

typedef struct bit_code {
	char leng; //zero leng - internal, negative - safeword (end of file)
	long int code; //encoded symbol
} bit_code;

typedef struct stc_tree {
	int children[2];
} stc_tree;

int get_tree_size(huff_tree *root);

int dyn_to_stc(huff_tree *root, stc_tree *stc_root, int index, int free_space);

huff_tree *create_node(unsigned char sym, char type, int freq);

void free_tree(huff_tree *root);

void insert_node(huff_tree **head, huff_tree *node);

void insertion_sort(huff_tree *array, int leng);

huff_tree *remove_zeros(huff_tree *arr);

void create_code_table(huff_tree *tree, bit_code *table, bit_code code);

#endif
