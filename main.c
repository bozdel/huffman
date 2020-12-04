#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <math.h>
#include <unistd.h>

#define LEAF 0
#define STOP 1
#define INTERNAL 2

#define GET_BIT(a,n) ( ((a) >> (n)) & 1     )
#define SET_BIT(a,n) ( (a) |= (1 << (n))    )
#define CLR_BIT(a,n) ( (a) &= (~(1 << (n))) )

//#include "../bit_stream/bit_stream.h"

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

//-------------from bit_stream-----------------
#define BUFF_SIZE 1000
#define ALL 0

//try to add dllist *cur_byte
typedef struct {
    FILE *file;
    unsigned char *buff;
    int pos;
} bit_stream;

//need to rename this func. it doesn't reflect the essence of func
void flush_buff(bit_stream *stream, int flushing_size) {
    if (flushing_size == ALL) {
      unsigned char *cur_byte = &(stream->buff[stream->pos / 8]);
      while (stream->pos % 8 != 0) {
        CLR_BIT(*cur_byte, stream->pos % 8);
        stream->pos++;
      }
      fwrite(stream->buff, sizeof(char), stream->pos / 8, stream->file);
    }

    fwrite(stream->buff, sizeof(char), flushing_size, stream->file);
	for (int i = 0; i < BUFF_SIZE - flushing_size; i++) {
		stream->buff[i] = stream->buff[i + flushing_size];
	}
    stream->pos -= flushing_size * 8;
}

bit_stream *open_bs(const char *path, char *type) {
    bit_stream *stream = (bit_stream*)malloc(sizeof(bit_stream));

    stream->buff = (unsigned char*)calloc(BUFF_SIZE, sizeof(char));
    stream->file = fopen(path,type);
    stream->pos = -1;

    return stream;
}

void close_bs(bit_stream *stream) {
	//complete writing (zeros)
	flush_buff(stream, ALL);
    fclose(stream->file);
    free(stream->buff);
    free(stream);
}

void write_bit(bit_stream *stream, int bit) {
	if (stream->pos == -1) stream->pos = 0; //shit code
    if (stream->pos + 1 > BUFF_SIZE * 8) flush_buff(stream, BUFF_SIZE);

    unsigned char *cur_byte = &(stream->buff[stream->pos / 8]);

    if (bit) {
    	SET_BIT(*cur_byte, stream->pos % 8);
    }
    else {
    	CLR_BIT(*cur_byte, stream->pos % 8);
    }

    stream->pos++;
}

/*void write_sym2(bit_stream *stream, unsigned char sym) {
	if (stream->pos == -1) stream->pos = 0; //shit code
    if (stream->pos + 8 > BUFF_SIZE * 8) flush_buff(stream, BUFF_SIZE - 1);

    unsigned char *cur_byte;

    int bit;
    for (int i = 0; i < 8; i++) {
        cur_byte = &(stream->buff[stream->pos / 8]);

        bit = get_nth_bit(sym, i);
        set_nth_bit(cur_byte, stream->pos % 8, bit);

        stream->pos++;
    }
}*/

void write_sym(bit_stream *stream, unsigned char sym) {
	for (int i = 0; i < 8; i++) {
		write_bit(stream, sym & 1);
		sym >>= 1;
	}
}

int read_bit(bit_stream *stream, int *bit) {
	if (stream->pos + 1 > BUFF_SIZE * 8 || stream->pos == -1) {
		int readed = fread(stream->buff, sizeof(char), BUFF_SIZE, stream->file);
		if (!readed) {
			return 0;
		}
		stream->pos = 0;
	}

	unsigned char cur_byte = stream->buff[stream->pos / 8];

	*bit = GET_BIT(cur_byte, stream->pos % 8);

	stream->pos++;

	return 1;
}

/*int read_sym2(bit_stream *stream, unsigned char *sym) {
	if (stream->pos + 8 > BUFF_SIZE * 8 || stream->pos == -1) {
		int readed = fread(stream->buff, sizeof(char), BUFF_SIZE - 1, stream->file);
		if (!readed) {
			return 0;
		}
		//shifting buffer
		unsigned char tmp = stream->buff[BUFF_SIZE - 1];
		for (int i = BUFF_SIZE - 2; i > 0; i--) {
			stream->buff[i + 1] = stream->buff[i];
		}
		stream->buff[0] = tmp;

		stream->pos -= (BUFF_SIZE - 1) * 8;
	}

	for (int i = 0; i < 8; i++) {
		unsigned char cur_byte = stream->buff[stream->pos / 8];

		int bit = get_nth_bit(cur_byte, stream->pos % 8);
		set_nth_bit(sym, i, bit);

		stream->pos++;
	}

	return 1;
}*/

int read_sym(bit_stream *stream, unsigned char *sym) {
	int bit;
	for (int i = 0; i < 8; i++) {
		read_bit(stream, &bit);
		if (bit) {
			SET_BIT(*sym, i);
		}
		else {
			CLR_BIT(*sym, i);
		}
	}
}
//-------------from bit_stream-----------------

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
  int i = 0;
  huff_tree *head = NULL;
  while (i < 256 && arr[i].freq == 0) i++;
  head = &arr[i];
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

huff_tree *create_huff_tree(FILE *input) {
	huff_tree *freqs = (huff_tree*)calloc(256, sizeof(huff_tree));

	//getting symbols' frequencies
	unsigned char sym;
	while (fread(&sym, sizeof(char), 1, input)) {
		freqs[sym].sym = sym;
		freqs[sym].freq++;
	}

	//preparing for creating huffman tree (sorting array)
	insertion_sort(freqs, 256);

	//removing redundant structs for unused symbols. making list
	huff_tree *head = remove_zeros(freqs);                                           //!!!---fix memory leaking---!!!

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
			if (tree->freq != 0) write_sym(output, tree->sym); //leaf contains symbol
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
			write_bit(output, GET_BIT(code.code, i));
		}
	}
	//write safeword
	bit_code code = table[256];
	for (int i = 0; i < code.leng; i++) {
		write_bit(output, GET_BIT(code.code, i));
	}
}



int encode(const char *input_path, const char *output_path) {
	//creating code table (1-st file read)
  	FILE *input = fopen(input_path, "rb");
	bit_stream *output = open_bs(output_path, "wb");

	huff_tree *tree = create_huff_tree(input);

	//print_tree(tree);
	//printf("\n");

	//to contain table (including stop sequence)
	bit_code *table = (bit_code*)calloc(257, sizeof(bit_code));
	bit_code start_code; //just for passing to parameters
	start_code.leng = 0; //making setup

	create_code_table(tree, table, start_code);

	write_huff_tree(output, tree);

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
			read_sym(stream, &sym);
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

	while (leaf_type != STOP) {
		while (tree[index].children[0] > 0) {
			int bit;
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
	bit_stream *input = open_bs(input_path, "rb");

	huff_tree *tree = read_tree(input);

	int tree_size = get_tree_size(tree);
	stc_tree *stc = (stc_tree*)malloc(sizeof(stc_tree) * tree_size);
	dyn_to_stc(tree, stc, 0, 0);

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

	opts.input_fpath = NULL;
	opts.output_fpath = NULL;
	opts.type = 1;
	opts.time = 0;
	opts.keep = 0;


	const char *opt_string = "ktedi:o:";
	int opt = 0;

	while ( (opt = getopt(argc, argv, opt_string)) != -1) {
		switch (opt) {
			case 'e':
				opts.type = 1;
				break;
			case 'd':
				opts.type = 0;
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
		char *output = (char*)malloc(sizeof(char) * strlen(opts.input_fpath) + 3);
		strcpy(output, opts.input_fpath);
		strcat(output, ".hh");
		opts.output_fpath = output;
	}

	if (opts.type) {
		clock_t enc_beg = clock();
		encode(opts.input_fpath, opts.output_fpath);
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

	return 0;
}