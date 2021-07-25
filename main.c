#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
#include "bit_stream.h"

#define LEAF 0
#define STOP 1
#define INTERNAL 2

#define ENC 1
#define DEC 0

#define GET_BIT(a,n) ( ((a) >> (n)) & 1		)
#define SET_BIT(a,n) ( (a) |= (1 << (n))	)
#define CLR_BIT(a,n) ( (a) &= (~(1 << (n))) )



struct huff_opt {
	const char *input_fpath;
	const char *output_fpath;
	char type; //encode/decode
	char time;
	char keep;
};

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


void print_bin(bit_code code) {
	for (int i = 0; i < code.leng; i++) {
		printf("%d", GET_BIT(code.code, i));
	}
	printf("\n");
}

void print_bin_bits(long int bits, int size) {
	for (int i = 0; i < size; i++) {
		printf("%d", GET_BIT(bits, i));
	}
	printf(" ");
}

void print_table(bit_code *table) {
	for (int i = 0; i < 256; i++) {
		if (table[i].leng > 0) {
			printf("%c: ", i);
			print_bin(table[i]);
		}
	}
	printf("stop: ");
	print_bin(table[256]);
}

huff_tree *create_node(unsigned char sym, char type, int freq) {
	huff_tree *node = (huff_tree*)malloc(sizeof(huff_tree));
	node->sym = sym;
	node->freq = freq;
	node->type = type;
	node->next = NULL;
	node->left = NULL;
	node->right = NULL;
	return node;
}

void free_tree(huff_tree *root) {
	if (root) {
		free_tree(root->left);
		free_tree(root->right);
		free(root);
	}
}

void insertion_sort(huff_tree *array, int leng) {
	for (int i = 0; i < leng - 1; i++) {
		int j;
		huff_tree tmp = array[i + 1];
		for (j = i; j >= 0 && tmp.freq < array[j].freq; j--) {
			array[j + 1] = array[j];
		}
		array[j + 1] = tmp;
	}
}

huff_tree *remove_zeros(huff_tree *arr) {
	huff_tree *head = NULL;

	int i = 0;
	while (i < 256 && arr[i].freq == 0) i++;
	head = &arr[i]; // fix possible error: "i" can be 256 which means out of boundaries (arr[256])

	for ( ; i < 255; i++) {
		arr[i].next = &arr[i+1];
	}
	arr[255].next = NULL;
	return head;
}

void insert_node(huff_tree **head, huff_tree *node) {
	huff_tree **current = head;
	while (*current && (*current)->freq < node->freq) {
		current = &((*current)->next);
	}
	node->next = (*current);
	*current = node;
}

huff_tree *create_huff_tree(FILE *input, huff_tree **mem_block_ptr) {
	huff_tree *freqs = (huff_tree*)calloc(256, sizeof(huff_tree));
	*mem_block_ptr = freqs; //for further freeing tree

	//getting symbols' frequencies
	unsigned char sym;
	while (fread(&sym, sizeof(char), 1, input)) {
		freqs[sym].sym = sym;
		freqs[sym].freq++;
	}

	//preparing for creating huffman tree (sorting array)
	insertion_sort(freqs, 256);

	//removing redundant structs for unused symbols. making list
	huff_tree *head = remove_zeros(freqs);										   //!!!---fix memory leaking---!!!

	//adding safeword to denote end of file
	huff_tree *safeword = (huff_tree*)calloc(1, sizeof(huff_tree));
	safeword->next = head;
	safeword->type = STOP;
	head = safeword;

	//making tree

	while (head->next != NULL) {
		huff_tree *new_node = create_node(0, INTERNAL, head->freq + head->next->freq);
		new_node->left = head;
		new_node->right = head->next;
		head = head->next->next;
		insert_node(&head, new_node);
  	}
  	return head; //the tree lies here
}


//--------not so shit-code---------
void create_code_table(huff_tree *tree, bit_code *table, bit_code code) {
	if (tree) {
		int bit_num;
		switch (tree->type) {
			case LEAF:
				table[tree->sym].code = code.code;
				table[tree->sym].leng = code.leng;
				break;
			case STOP:
				//at 256-th index lies stop sequence
				table[256].code = code.code;
				table[256].leng = code.leng;
				break;
			case INTERNAL:
				bit_num = code.leng++;

				CLR_BIT(code.code, bit_num);
				create_code_table(tree->left, table, code);

				SET_BIT(code.code, bit_num);
				create_code_table(tree->right, table, code);
				break;
			default:
				break;
		}
	}
}

void write_huff_tree(bit_stream *output, huff_tree *tree) {
	if (tree) {
		if (tree->type != INTERNAL) {
			write_bit(output, 1);
			write_bit(output, tree->type); //writing bit denoting type of leaf (0 - leaf, 1 - stop)
			if (tree->freq != 0) write_byte(output, tree->sym); //leaf contains symbol
		}
		else {
			write_bit(output, 0);
			write_huff_tree(output, tree->left);
			write_huff_tree(output, tree->right);
		}
	}
}

void encode_text(FILE *input, bit_code *table, bit_stream *output) {
	unsigned char sym;
	while (fread(&sym, sizeof(char), 1, input)) {
		//write_bits(output, table[sym].bit_arr, table[sym].leng);
		bit_code code = table[sym];
		for (int i = 0; i < code.leng; i++) {
			printf("%d", GET_BIT(code.code, i));
			//write_bit(output, GET_BIT(code.code, i));
		}
	}
	//write safeword
	bit_code code = table[256];
	for (int i = 0; i < code.leng; i++) {
		//write_bit(output, GET_BIT(code.code, i));
	}
}

void encode_text2(FILE *input, bit_code *table, FILE *output) {
	char sym;
	bit_code code;
	int byte_pos = 0;
	int code_pos = 0;

	unsigned char mask = 0;
	unsigned char bits = 0;
	unsigned char out = 0;
	int i = 0;
	while (sym != EOF) {
		i++;
		if (code_pos <= 0) {
			sym = getc(input);
			if (sym == EOF) break;
			code = table[sym];
			code_pos = code.leng;
		}
		if (byte_pos >= 8) {
			print_bin_bits(out, 8);
			out = 0;
			// putc(out, output);
			byte_pos = 0;
		}
		
		// printf("\n\nbyte_pos: %d\ncode_pos: %d\n", byte_pos, code_pos);

		int min = code_pos < (8 - byte_pos) ? code_pos : (8 - byte_pos);
		// printf("min: %d\n", min);
		mask = (1 << min) - 1;
		// printf("mask\n");
		// print_bin_bits(mask, 8);
		// printf("code\n");
		// print_bin_bits(code.code, sizeof(long int) * 8);

		bits = code.code & mask;
		// printf("bits\n");
		// print_bin_bits(bits, 8);
		out = out | (bits << byte_pos);
		// printf("out\n");
		// print_bin_bits(out, 8);

		code.code = code.code >> min;

		byte_pos += min;
		code_pos -= min;
	}
	code = table[256];
	code_pos = code.leng;
	while (code_pos > 0) {
		if (byte_pos >= 8) {
			print_bin_bits(out, 8);
			out = 0;
			byte_pos = 0;
		}
		int min = code_pos < (8 - byte_pos) ? code_pos : (8 - byte_pos);
		// printf("min: %d\n", min);
		mask = (1 << min) - 1;
		// printf("mask\n");
		// print_bin_bits(mask, 8);
		// printf("code\n");
		// print_bin_bits(code.code, sizeof(long int) * 8);

		bits = code.code & mask;
		// printf("bits\n");
		// print_bin_bits(bits, 8);
		out = out | (bits << byte_pos);
		// printf("out\n");
		// print_bin_bits(out, 8);

		code.code = code.code >> min;

		byte_pos += min;
		code_pos -= min;
	}
	print_bin_bits(out, 8);
}


/*#define READSYM
#define WRITEBIT
#define END
//finite state machine style
void encode_text_bit_stream(FILE *input, bit_code *table, bit_stream *output) {
	int read = 0;
	int cur_pos = 0;
	bit_code code;
	unsigned char sym;
	while (state != END) {
		switch (state) {
			case READSYM:
				read = fread(&sym, sizeof(char), 1, input);
				if (read) {
					code = table[sym];
					state = WRTITEBIT;
				}
				else {
					state = END;
				}
				break;
			case WRITEBIT:
				if (cur_pos < code.leng) {
					write_bit(output, GET_BIT(code.code, cur_pos));
					cur_pos++;
					state = WRITEBIT;
				}
				else {
					cur_pos = 0;
					state = READSYM;
				}
				break;
			case END:
				break;
		}
	}
}

#ifndef READSYM
#define READSYM
#endif
#ifndef ADDBIT
#define ADDBIT
#endif
#ifndef WRITESYM
#define WRITESYM
#endif
#ifndef END
#define END
#endif

void encode_text_pure(FILE *input, bit_code *table, FILE *output) {
	int cur_code_pos = 0;
	int cur_byte_pos = 0;
	unsigned char sym;
	unsigned char out = 0;
	int read = 0;
	bit_code code;
	state = READSYM;
	while (state != END) {
		switch (state) {
			case READSYM:
				read = fread(&sym, sizeof(char), 1, input);
				if (read) {
					code = table[sym];
					cur_code_pos = 0;
					state = ADDBIT;
				}
				else {
					state = END;
				}
				break;
			case ADDBIT:
				if (cur_byte_pos < 8 && cur_code_pos < code.leng) {
					SET_CON(out, cur_byte_pos, GET_BIT(code.code, cur_code_pos));
					cur_code_pos++;
					cur_byte_pos++;
					state = ADDBIT;
				}
				else if (cur_byte_pos >= 8) {
					state = WRITESYM;
				}
				else if (cur_code_pos >= code.leng) {
					state = READSYM;
				}
				break;
			case WRITESYM:
				putc(out, output);
				out = 0;
				j = 0;
				break;
			case END:
				break;
		}
	}
}*/



int db_encode(const char *input_path, const char *output_path) {
	//creating code table (1-st file read)
  	FILE *input = fopen(input_path, "rb");
  	if (!input) {
  		return 1;
  	}
	bit_stream *file_tree = open_bs("./tree.hh", "wb");
	if (!file_tree) {
		return 1;
	}
	bit_stream *file_text = open_bs("./text.hh", "wb");

	FILE *file_text2 = fopen("./text2.hh", "wb");


	huff_tree *mem_block_ptr = NULL; //for further freeing tree;
	huff_tree *tree = create_huff_tree(input, &mem_block_ptr);

	//print_tree(tree);
	//printf("\n");

	//to contain table (including stop sequence)
	bit_code *table = (bit_code*)calloc(257, sizeof(bit_code));
	bit_code start_code; //just for passing to parameters
	start_code.leng = 0; //making setup
	start_code.code = 0; //making setup

	create_code_table(tree, table, start_code);

	print_table(table);

	// write_huff_tree(file_tree, tree);
	// flush_buff(file_tree, ALL);

	free(mem_block_ptr); //freeing huffman tree (dynamic)

	rewind(input);
	// encode_text(input, table, file_text);
	// rewind(input);
	encode_text2(input, table, file_text2);

	fclose(input);
	close_bs(file_tree);
	close_bs(file_text);

	fclose(file_text2);

	return 0;
}

int encode(const char *input_path, const char *output_path) {
	//creating code table (1-st file read)
  	FILE *input = fopen(input_path, "rb");
  	if (!input) {
  		return 1;
  	}
	bit_stream *output = open_bs(output_path, "wb");
	if (!output) {
		return 1;
	}

	huff_tree *mem_block_ptr = NULL; //for further freeing tree;
	huff_tree *tree = create_huff_tree(input, &mem_block_ptr);

	//print_tree(tree);
	//printf("\n");

	//to contain table (including stop sequence)
	bit_code *table = (bit_code*)calloc(257, sizeof(bit_code));
	bit_code start_code; //just for passing to parameters
	start_code.leng = 0; //making setup
	start_code.code = 0; //making setup

	create_code_table(tree, table, start_code);

	write_huff_tree(output, tree);

	free(mem_block_ptr); //freeing huffman tree (dynamic)

	rewind(input);
	encode_text(input, table, output);

	fclose(input);
	close_bs(output);

	return 0;
}


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

//--------------------static tree------------------------------------------
typedef struct stc_tree {
	int children[2];
} stc_tree;

int get_tree_size(huff_tree *root) {
	if (root) {
		int left_size = get_tree_size(root->left);
		int right_size = get_tree_size(root->right);
		return left_size + right_size + 1;
	}
	return 0;
}

int dyn_to_stc(huff_tree *root, stc_tree *stc_root, int index, int free_space) {
	if (root->left) {
		stc_root[index].children[0] = ++free_space;
		free_space = dyn_to_stc(root->left, stc_root, stc_root[index].children[0], free_space);
	}
	if (root->right) {
		stc_root[index].children[1] = ++free_space;
		free_space = dyn_to_stc(root->right, stc_root, stc_root[index].children[1], free_space);
	}
	if (root->left == NULL && root->right == NULL) {
		stc_root[index].children[0] = -(root->type + 1);
		stc_root[index].children[1] = -(root->sym);
	}
	return free_space;
}
//--------------------static tree------------------------------------------


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

/*void print_code(bit_code code) {
	for (int i = 0; i < code.leng; i++) {
		printf("%d", GET_BIT(code.code, i));
	}
	printf("\n");
}

void print_code_table(bit_code *table) {
	for (int i = 0; i < 257; i++) {
		print_code(table[i]);
	}
}

int get_sym(int *sieve) {
	for (int i = 0; i < 257; i++) {
		if (sieve[i]) {
			return i;
		}
	}
}

int decode_text3(bit_stream *input, bit_code *table, FILE *output) {
	// print_code_table(table);
	int sym = 0;
	int sieve[257] = { 0 };
	int bit_num = 0;
	int bit;

	int matches_amo;
	while (sym != 256) {
		matches_amo = 0;
		// set_sieve(sieve, table);
		for (int i = 0; i < 257; i++) {
			if (table[i].leng > 0) {
				sieve[i] = 1;
				matches_amo++;
			}
		}
		while (matches_amo > 1) {
			// printf("matches: %d\n", get_matches_amo(sieve));
			read_bit(input, &bit);
			// printf("bit: %d\n", bit);
			// one_pass(sieve, bit, table, bit_num);
			matches_amo = 0;
			for (int i = 0; i < 257; i++) {
				if (GET_BIT(table[i].code, bit_num) != bit) {
					sieve[i] = 0;
				}
				else if (sieve[i]) {
					matches_amo++;
				}
			}
			bit_num++;
		}
		sym = get_sym(sieve);
		if (sym == 256) {
			return;
		}
		putc((char)sym, output);
		// printf("%c\n", (char)sym);
		bit_num = 0;
	}
}*/

/*void decode_text4(bit_stream *input, bit_code *table, FILE *output) {
	long int masks[257] = { 0 };
	for (int i = 0; i < 257; i++) {
		mask[i] = ~((1 << table[i].leng) - 1);
	}
	char byte = 0;
	while (read_byte(input, &byte)) {
		for (int i = 0; i < 257; i++) {
			if (byte & mask[i] == table[i].code) {
				putc((char)i, output);
				i << table[i].leng;
			}
		}
	}
}*/

/*typedef struct tree16 {
	int children[16];
} tree16;

int get_size_block(huff_tree *tree, int depth) {
	if (root) {
		int left_size = get_size_block(root->left, (depth + 1) % 4);
		int right_size = get_size_block(root->right, (depth + 1) % 4);
		return left_size + right_size + !depth; // depth == 0 means entering in new block
	}
	return 0;
}

void tree_to_tree16(huff_tree *root, tree16 *root16, int index, int free_space, int child_num, int depth) {
	if (root->left) {
		if (!depth) {
			root16[index],children[child_num] = ++free_space;
			tree_to_tree16(root->left, root16, root16[index].children[child_num], free_space, (child_num * 2) % 16, (depth + 1) % 4);
		}
		else {
			tree_to_tree16(root->left, root16, index, free_space, (child_num * 2) % 16, (depth + 1) % 4);
		}
	}
	if (root->right) {
		if (!depth) {
			root16[index].children[child_num] = ++free_space;
			tree_to_tree16(root->right, root16, root16[index].children[child_num], free_space, (child_num * 2 + 1) % 16, (depth + 1) % 4);
		}
		else {
			tree_to_tree16(root->right, root16, index, free_space, (children * 2 + 1) % 16, (depth + 1) % 4);
		}
	}
	if (root->left == NULL && root->right == NULL) {
		for (int i = child_num; i < (1 << (3 - depth)); i++) {
			root16[index].children[i] = -(root->sym);
		}
	}
}



int foo(FILE *input, FILE *output, stc_tree *tree) {
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
}

union bar {
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

	/*//--------------testing new verison
	//to contain table (including stop sequence)
	bit_code *table = (bit_code*)calloc(257, sizeof(bit_code));
	bit_code start_code; //just for passing to parameters
	start_code.leng = 0; //making setup
	start_code.code = 0; //making setup

	create_code_table(tree, table, start_code);

	decode_text3(input, table, output);
	//--------------testing new verison*/

	int tree_size = get_tree_size(tree);
	stc_tree *stc = (stc_tree*)malloc(sizeof(stc_tree) * tree_size);
	dyn_to_stc(tree, stc, 0, 0);

	free_tree(tree);

	decode_text2(input, output, stc);


	close_bs(input);
	fclose(output);

	return 0;
}

float get_entropy(FILE *file) {
	int syms[256] = {0};
	unsigned char sym;
	while (fread(&sym, sizeof(char), 1, file)) {
		syms[sym]++;
	}
	int total = 0;
	printf("frequencies\n");
	for (int i = 0; i < 256; i++) {
		total += syms[i];
		printf("%d ", syms[i]);
	}
	printf("total: %d\n", total);
	float probability[256] = {0};
	printf("probabilities\n");
	for (int i = 0; i < 256; i++) {
		probability[i] = (float)syms[i] / (float)total;
		printf("%f ", probability[i]);
	}
	float entropy = 0;
	for (int i = 0; i < 256 && probability[i]; i++) {
		entropy -= probability[i] * log2f(probability[i]);
	}
	return entropy;
}

int main(int argc, char *argv[]) {
	struct huff_opt opts;

	//add help

	opts.input_fpath = NULL;
	opts.output_fpath = NULL;
	opts.type = ENC;
	opts.time = 0;
	opts.keep = 0;

	const char *opt_string = "ktedi:o:";
	int opt = 0;

	while ( (opt = getopt(argc, argv, opt_string)) != -1) {
		switch (opt) {
			case 'e':
				opts.type = ENC;
				break;
			case 'd':
				opts.type = DEC;
				break;
			case 'i':
				opts.input_fpath = optarg;
				break;
			case 'o':
				opts.output_fpath = optarg;
				break;
			case 't':
				opts.time = 1;
				break;
			case 'k':
				opts.keep = 1;
				break;
			default:
				printf("error\n");
				break;
		}
	}

	if (!opts.input_fpath) {
		printf("missing input file\n");
		return 1;
	}
	if (!opts.output_fpath) {
		//encode
		if (opts.type == ENC) {
			char *output = (char*)malloc(sizeof(char) * strlen(opts.input_fpath) + 3);
			strcpy(output, opts.input_fpath);
			strcat(output, ".hh");
			// sprintf(output, "%s.hh", opts.input_fpath);
			opts.output_fpath = output;
		}
		//decode
		else {
			char *output = (char*)malloc(sizeof(char) * strlen(opts.input_fpath) - 3);
			memcpy(output, opts.input_fpath, strlen(opts.input_fpath) - 3);
			opts.output_fpath = output;
		}
	}

	if (opts.type == ENC) {
		clock_t enc_beg = clock();
		db_encode(opts.input_fpath, opts.output_fpath);
		clock_t enc_end = clock();
		if (opts.time) {
			printf("encode time: %f\n", (float)(enc_end - enc_beg) / CLOCKS_PER_SEC);
		}
	}
	else {
		clock_t dec_beg = clock();
		decode(opts.input_fpath, opts.output_fpath);
		clock_t dec_end = clock();
		if (opts.time) {
			printf("decode time: %f\n", (float)(dec_end - dec_beg) / CLOCKS_PER_SEC);
		}
	}

	if(!opts.keep) {
		if(remove(opts.input_fpath) == -1) {
			printf("error removing file\n");
		}
	}

	return 0;
}