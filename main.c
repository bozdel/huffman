#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <math.h>
#include <unistd.h>

#define LEAF 0
#define STOP 1
#define INTERNAL 2

#define GET_BIT(a,n) ( ((a) & (1 << (n))) >> (n) )
#define SET_BIT(a,n) ( (a) |= (1 << (n))       )
#define CLR_BIT(a,n) ( (a) &= (~(1 << (n)))    )

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
	char bit_arr[8]; //encoded symbol
} bit_code;

void print_sym_bin(unsigned char sym) {
	for (int i = 0; i < 8; i++) {
		printf("%d ", sym % 2);
		sym >>= 1;
	}
}

//-------------from bit_stream-----------------
#define BUFF_SIZE 1000
#define ALL 0

//try to add dllist *cur_byte
typedef struct {
    FILE *file;
    unsigned char *buff;
    int pos;
} bit_stream;

void set_nth_bit(unsigned char *byte, int n, int bit) {
    //clearing nth bit
    *byte = *byte & (~(1 << n));
    //setting bit
    *byte = *byte | bit << n;
}

int get_nth_bit(unsigned char byte, int n) {
    return byte >> n & 1;
}

//need to rename this func. it doesn't reflect the essence of func
void flush_buff(bit_stream *stream, int flushing_size) {
    if (flushing_size == ALL) {
      unsigned char *cur_byte = &(stream->buff[stream->pos / 8]);
      while (stream->pos % 8 != 0) {
        //set_nth_bit(cur_byte, stream->pos % 8, 0);
        CLR_BIT(*cur_byte, stream->pos % 8);
        stream->pos++;
      }
      fwrite(stream->buff, sizeof(char), stream->pos / 8, stream->file);
	  //debug
	  /*for (int i = 0; i < stream->pos / 8; i++) {
		  print_sym_bin(stream->buff[i]);
	  }*/
	  //debug
    }
    fwrite(stream->buff, sizeof(char), flushing_size, stream->file);
	//debug
	/*for (int i = 0; i < flushing_size; i++) {
		print_sym_bin(stream->buff[i]);
	}*/
	//debug
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

    //set_nth_bit(cur_byte, stream->pos % 8, bit);
    if (bit) {
    	SET_BIT(*cur_byte, stream->pos % 8);
    }
    else {
    	CLR_BIT(*cur_byte, stream->pos % 8);
    }

    stream->pos++;
}

void write_sym2(bit_stream *stream, unsigned char sym) {
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
}

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

	//*bit = get_nth_bit(cur_byte, stream->pos % 8);
	*bit = GET_BIT(cur_byte, stream->pos % 8);

	stream->pos++;

	return 1;
}

int read_sym2(bit_stream *stream, unsigned char *sym) {
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
}

int read_sym(bit_stream *stream, unsigned char *sym) {
	int bit;
	for (int i = 0; i < 8; i++) {
		read_bit(stream, &bit);
		//set_nth_bit(sym, i, bit);
		if (bit) {
			SET_BIT(*sym, i);
		}
		else {
			CLR_BIT(*sym, i);
		}
	}
}
//-------------from bit_stream-----------------

void print_tree(huff_tree *tree) {
	if (tree) {
		printf("%c(%d) ", tree->sym, tree->sym);
		print_tree(tree->left);
		print_tree(tree->right);
	}
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



void cp_arr(char *destination, char *source, int size) {
	for (int i = 0; i < size; i++) {
		destination[i] = source[i];
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


//--------not so shit-code--------- couse been fixed (once)
void create_code_table2(huff_tree *tree, bit_code *table, bit_code code) {
  if (tree) {
    if (tree->left == NULL && tree->right == NULL) {
      if (tree->freq != 0) { //usual symbols
		cp_arr(table[tree->sym].bit_arr, code.bit_arr, 8);
        table[tree->sym].leng = code.leng;
      }
      else { //stop sequence (after all usual symbols at 256-th index)
		cp_arr(table[256].bit_arr, code.bit_arr, 8);
        table[256].leng = -code.leng;
      }
    }
    if (tree->left != NULL && tree->right != NULL) {
      int byte_num = code.leng / 8;
      int bit_num = code.leng % 8;

      code.leng++;

      //set_nth_bit(&(code.bit_arr[byte_num]), bit_num, 0);
      CLR_BIT(code.bit_arr[byte_num], bit_num);
      create_code_table2(tree->left, table, code);


      //set_nth_bit(&(code.bit_arr[byte_num]), bit_num, 1);
      SET_BIT(code.bit_arr[byte_num], bit_num);
      create_code_table2(tree->right, table, code);
    }
  }
}

void create_code_table(huff_tree *tree, bit_code *table, bit_code code) {
	if (tree) {
		switch (tree->type) {
			case LEAF:
				cp_arr(table[tree->sym].bit_arr, code.bit_arr, 8);
				table[tree->sym].leng = code.leng;
				break;
			case STOP:
				//at 256-th index lies stop sequence
				cp_arr(table[256].bit_arr, code.bit_arr, 8);
				table[256].leng = -code.leng;
				break;
			case INTERNAL:
				;
				int byte_num = code.leng / 8;
				int bit_num = code.leng % 8;

				code.leng++;

				CLR_BIT(code.bit_arr[byte_num], bit_num);
				create_code_table(tree->left, table, code);

				SET_BIT(code.bit_arr[byte_num], bit_num);
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
    bit_code code = table[sym];
    for (int i = 0; i < code.leng; i++) {
      int byte = code.bit_arr[i / 8];
      //int bit = get_nth_bit(byte, i % 8);
      int bit = GET_BIT(byte, i % 8);
      write_bit(output, bit);
	  //printf("%d ", bit);
    }
  }
  //write safeword
  bit_code code = table[256];
  for (int i = 0; i < -code.leng; i++) {
    int byte = code.bit_arr[i / 8];
    //int bit = get_nth_bit(byte, i % 8);
    int bit = GET_BIT(byte, i % 8);
    write_bit(output, bit);
	//printf("%d ", bit);
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

int decode(const char *input_path, const char *output_path) {
	FILE *output = fopen(output_path, "wb");
	bit_stream *input = open_bs(input_path, "rb");

	huff_tree *tree = read_tree(input);

	//printf("\n");
	//print_tree(tree);

	decode_text(input, output, tree);

	close_bs(input);
	fclose(output);

	return 0;
}

void read_bin(FILE *file) {
    unsigned char sym;
    while (fread(&sym, sizeof(char), 1, file)) {
        print_sym_bin(sym);
    }
    printf("\n");
}

void read_bin_buff(bit_stream *bs) {
	int bit;
	while (read_bit(bs, &bit)) {
		printf("%d ", bit);
	}
	printf("\n");
}

void print_arr(int arr[], int leng) {
	for (int i = 0; i < leng; i++) {
		printf("%d ", arr[i]);
	}
	printf("\n");
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

int main2(int argc, char *argv[]) {
	/*char *input_enc_path = "/home/vyacheslav/code/C/other1/huffman/test.jpg";
	char *output_enc_path = "/home/vyacheslav/code/C/other1/huffman/encoded.txt";

	char *input_dec_path = output_enc_path;
	char *output_dec_path = "/home/vyacheslav/code/C/other1/huffman/decoded.jpg";*/

	char *input_enc_path = argv[1];
	char *output_dec_path = argv[2];
	char *output_enc_path = (char*)malloc(sizeof(char) * 100);
	strcpy(output_enc_path, input_enc_path);
	strcat(output_enc_path, ".hh");
	char *input_dec_path = output_enc_path;

	clock_t beg = clock();
	encode(input_enc_path, output_enc_path);
	clock_t end = clock();

	clock_t beg2 = clock();
	decode(input_dec_path, output_dec_path);
	clock_t end2 = clock();

	float enc_t = (float)(end - beg) / CLOCKS_PER_SEC;
	float dec_t = (float)(end2 - beg2) / CLOCKS_PER_SEC;
	printf("encode time: %f\ndecode time: %f\n", enc_t, dec_t);

	return 0;
}
