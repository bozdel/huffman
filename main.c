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
	int i;
	for (i = 0; i < 256; i++) {
		if (table[i].leng > 0) {
			printf("%c: ", i);
			print_bin(table[i]);
		}
	}
	printf("stop: ");
	print_bin(table[256]);
	printf("leng: %d\n", i);
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
	int counter = 0;
	while (fread(&sym, sizeof(char), 1, input)) {
		//write_bits(output, table[sym].bit_arr, table[sym].leng);
		bit_code code = table[sym];
		for (int i = 0; i < code.leng; i++) {
			/*printf("%d", GET_BIT(code.code, i));
			if ((++counter) % 8 == 0) {
				printf(" ");
			}*/
			write_bit(output, GET_BIT(code.code, i));
		}
	}
	//write safeword
	bit_code code = table[256];
	for (int i = 0; i < code.leng; i++) {
		/*printf("%d", GET_BIT(code.code, i));
			if ((++counter) % 8 == 0) {
				printf(" ");
		}*/
		write_bit(output, GET_BIT(code.code, i));
	}
	// printf(" ");
}

void encode_text2(FILE *input, bit_code *table, FILE *output) {
	unsigned char sym;
	bit_code code;
	int byte_pos = 0;
	int code_pos = 0;

	unsigned char mask = 0;
	unsigned char bits = 0;
	unsigned char out = 0;
	int i = 0;
	while (!feof(input)) {
		i++;
		if (code_pos <= 0) {
			sym = getc(input);
			if (feof(input)) break;
			code = table[sym];
			code_pos = code.leng;
		}
		if (byte_pos >= 8) {
			// print_bin_bits(out, 8);
			putc(out, output);
			out = 0;
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
			// print_bin_bits(out, 8);
			putc(out, output);
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
	// print_bin_bits(out, 8);
	putc(out, output);
}






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

	// print_table(table);

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

	print_table(table);

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