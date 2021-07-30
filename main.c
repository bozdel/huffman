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

void free_internals(huff_tree *root) {
    if (root->type == INTERNAL) {
        free_internals(root->left);
        free_internals(root->right);
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
  int i = 0;
  huff_tree *head = NULL;
  while (i < 256 && arr[i].freq == 0) i++;
  if (i > 255) {
      head = NULL;
  }
  else {
      head = &arr[i];
  }
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

huff_tree *create_huff_tree(FILE *input, huff_tree **mem_block_ptr, huff_tree **safeword_ptr) {
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
	huff_tree *head = remove_zeros(freqs);                                           //!!!---fix memory leaking---!!!

	//adding safeword to denote end of file
	huff_tree *safeword = (huff_tree*)calloc(1, sizeof(huff_tree));
	*safeword_ptr = safeword;
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


//--------not so bad code---------
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
  	if (!input) {
  		return 1;
  	}
  	
	bit_stream *output = open_bs(output_path, "wb");
	if (!output) {
	    fclose(input);
		return 1;
	}

    huff_tree *safeword_ptr = NULL;
    huff_tree *mem_block_ptr = NULL; //for further freeing tree
	huff_tree *tree = create_huff_tree(input, &mem_block_ptr, &safeword_ptr);

	//to contain table (including stop sequence)
	bit_code *table = (bit_code*)calloc(257, sizeof(bit_code));
	bit_code start_code; //just for passing to parameters
	start_code.leng = 0; //making setup
	start_code.code = 0; //making setup

	create_code_table(tree, table, start_code);

	write_huff_tree(output, tree);
	
	free_internals(tree);
	free(mem_block_ptr); //freeing tree
	free(safeword_ptr);

	rewind(input);
	
	encode_text(input, table, output);

	fclose(input);
	close_bs(output);
    
    free(table);
    
	return 0;
}


huff_tree *read_tree(bit_stream *stream) {
	huff_tree *tree = NULL;
	int bit = 0;
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
			unsigned char sym = 0;
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
	int bit = 0;
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

	while (read_encoded_sym(input, tree, &sym)) {
		fwrite(&sym, sizeof(char), 1, output);
	}
}




int decode(const char *input_path, const char *output_path) {
	FILE *output = fopen(output_path, "wb");
	if (!output) {
		return 1;
	}
	bit_stream *input = open_bs(input_path, "rb");
	if (!input) {
	    fclose(output);
		return 1;
	}

	huff_tree *tree = read_tree(input);

	decode_text(input, output, tree);

	close_bs(input);
	fclose(output);
	
	free_tree(tree);

	return 0;
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
		//encode
		if (opts.type) {
			char *output = (char*)malloc(sizeof(char) * strlen(opts.input_fpath) + 3);
			strcpy(output, opts.input_fpath);
			strcat(output, ".hh");
			opts.output_fpath = output;
		}
		//decode
		else {
			char *output = (char*)malloc(sizeof(char) * strlen(opts.input_fpath) - 3);
			memcpy(output, opts.input_fpath, strlen(opts.input_fpath) - 3);
			opts.output_fpath = output;
		}
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

	if(!opts.keep) {
		if(remove(opts.input_fpath) == -1) {
			printf("error removing file\n");
		}
	}

	return 0;
}