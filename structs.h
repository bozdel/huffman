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

//-------test-------------
// typedef int node_t;

#define INT_BITS sizeof(int) * 8
#define CHAR_BITS sizeof(char) * 8

#define SET_STOP(node) ( (node) |= (1 << INT_BITS - 2) )
#define IS_STOP(node) ( (1 << 30) & (node) )

#define SET_LEAF(node) ( (node) |= (1 << (INT_BITS - 1)) )
#define IS_LEAF(node) ( (1 << 31) & (node) )

#define GET_SYM(node) ( (node) & ((1 << CHAR_BITS) - 1) )
#define SET_SYM(node, sym) ( (node) |= (sym) )

#define GET_IND(node) ( node )
#define SET_IND(node, ind) ( (node) |= (ind) )

#define SET_REST(node) 

#define CLR_NODE(node) ( (node) &= 0 )

typedef struct bits {
	unsigned int value : 16;
	unsigned int rest : 12;
	unsigned int type : 2;
} bits;

typedef union node {
	char sym;
	short int index;
	bits mask;
} node_t;

typedef struct tree4 {
	node_t children[4];
} tree4;

typedef struct tree16 {
	node_t children[16];
} tree16;

int get_size_block(huff_tree *root, int depth);

int tree_to_tree4(huff_tree *root, tree4 *root4, int index, int free_space, int child_num, int depth);

int tree_to_tree4_v2(huff_tree *root, tree4 *root4, int index, int free_space, int child_num, int depth);

int get_size_block16(huff_tree *root, int depth);

int tree_to_tree16(huff_tree *root, tree16 *root16, int index, int free_space, int child_num, int depth);

//-------test-------------

int get_tree_size(huff_tree *root);

int dyn_to_stc(huff_tree *root, stc_tree *stc_root, int index, int free_space);

huff_tree *create_node(unsigned char sym, char type, int freq);

void free_tree(huff_tree *root);

void insert_node(huff_tree **head, huff_tree *node);

void insertion_sort(huff_tree *array, int leng);

huff_tree *remove_zeros(huff_tree *arr);

void create_code_table(huff_tree *tree, bit_code *table, bit_code code);

#endif