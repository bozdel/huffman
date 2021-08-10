#include "structs.h"
#include "bit_stream.h"
#include <stdio.h>
#include <stdlib.h>

// #include "debug.h"

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
	huff_tree *head = remove_zeros(freqs);										 //!!!---fix memory leaking---!!!

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

//replace buff and pos by struct bit_buff merging this two variables
//it needs a great clean-up. awful code (not-readable)
void write_huff_tree(FILE *output, huff_tree *tree, unsigned char *buff, int *pos) {
	if (tree) {
		if (tree->type != INTERNAL) {
			SET_BIT(*buff, *pos);
			if ((++(*pos)) >= 8) {
				putc(*buff, output);
				*pos = 0;
				*buff = 0;
			}
			SET_CON(*buff, *pos, tree->type);
			if ((++(*pos)) >= 8) {
				putc(*buff, output);
				*pos = 0;
				*buff = 0;
			}

			if (tree->type == LEAF) {
				unsigned char sym = tree->sym;
				for (int i = 0; i < 8; i++) {
					/*SET_CON(*buff, *pos, sym & 1);
					print_bin(*buff);
					sym >>= 1;*/
					SET_CON(*buff, *pos, GET_BIT(sym, i));
					if ((++(*pos)) >= 8) {
						putc(*buff, output);
						*pos = 0;
						*buff = 0;
					}
				}
			}
		}
		else {
			CLR_BIT(*buff, *pos);
			if ((++(*pos)) >= 8) {
				putc(*buff, output);
				*pos = 0;
				*buff = 0;
			}

			write_huff_tree(output, tree->left, buff, pos);
			write_huff_tree(output, tree->right, buff, pos);
		}
	}
}

void encode_text(FILE *input, bit_code *table, FILE *output, unsigned char buff, int pos) {
	unsigned char sym;

	//two buffers for streams (input stream - code / output stream - out)
	bit_code code;
	int code_pos = 0;

	unsigned char out = buff; //value for holding output bits (initialized by buff)
	int byte_pos = pos; //current position in output bits (initialized by position in buffer)

	//auxiliary variables
	unsigned char mask = 0;
	unsigned char bits = 0;

	while (!feof(input)) {
		if (code_pos <= 0) {
			sym = getc(input);
			if (feof(input)) break;
			code = table[sym];
			code_pos = code.leng;
		}
		if (byte_pos >= 8) {
			putc(out, output);
			out = 0;
			byte_pos = 0;
		}
		

		int min = code_pos < (8 - byte_pos) ? code_pos : (8 - byte_pos);

		mask = (1 << min) - 1;

		bits = code.code & mask;

		out = out | (bits << byte_pos);
		code.code = code.code >> min;

		byte_pos += min;
		code_pos -= min;
	}
	//writing safeword
	code = table[256];
	code_pos = code.leng;
	while (code_pos > 0) {
		if (byte_pos >= 8) {
			putc(out, output);
			out = 0;
			byte_pos = 0;
		}
		int min = code_pos < (8 - byte_pos) ? code_pos : (8 - byte_pos);

		mask = (1 << min) - 1;

		bits = code.code & mask;
		out = out | (bits << byte_pos);

		code.code = code.code >> min;

		byte_pos += min;
		code_pos -= min;
	}
	putc(out, output);
}

union buffer {
	__uint128_t pair;
	long int code[2];
};

void encode_text2(FILE *input, bit_code *table, FILE *output) {
	unsigned char sym;

	int c0l = 0, c1l = 0;		//length of the first code and the second code
	union buffer iobuff;		//buffer for read and write

	//auxiliary variables
	int free_c0l;				//complement length of the first code
	int eof = 0;

	while (!eof) {
		sym = getc(input);
		if ((eof = feof(input))) break;

		iobuff.code[1] = table[sym].code;
		c1l = table[sym].leng;

		free_c0l = sizeof(long int) * 8 - c0l;

		iobuff.code[0] <<= free_c0l;			//shifting the first code to connection with the second
		iobuff.pair >>= free_c0l;				//shifting down concatenated code to begining

		if (c1l > free_c0l) {					//first half of iobuff is full (it can be written to file)
			fwrite(&iobuff.code[0], sizeof(long int), 1, output);
			iobuff.pair >>= sizeof(long int) * 8;
			c0l -= sizeof(long int) * 8;
		}
		c0l += c1l;
	}


	//writing safeword
	iobuff.code[1] = table[256].code;
	c1l = table[256].leng;

	free_c0l = sizeof(long int) * 8 - c0l;

	iobuff.code[0] <<= free_c0l;
	iobuff.pair >>= free_c0l;

	if (c1l > free_c0l) {
		fwrite(&iobuff.code[0], sizeof(long int), 1, output);
		iobuff.pair >>= sizeof(long int) * 8;
		c0l -= sizeof(long int) * 8;
	}
	c0l += c1l;

	fwrite(&iobuff.code[0], sizeof(long int), 1, output);
}

int encode(const char *input_path, const char *output_path) {
	//creating code table (1-st file read)
	FILE *input = fopen(input_path, "rb");
	if (!input) {
		return 1;
	}
	FILE *output = fopen(output_path, "wb");
	if (!output) {
		return 1;
	}

	huff_tree *mem_block_ptr = NULL; //for further freeing tree;
	huff_tree *tree = create_huff_tree(input, &mem_block_ptr);

	//to contain table (including stop sequence)
	bit_code *table = (bit_code*)calloc(257, sizeof(bit_code));
	bit_code start_code; //just for passing to parameters
	start_code.leng = 0; //making setup
	start_code.code = 0; //making setup

	create_code_table(tree, table, start_code);


	unsigned char buff = 0;
	int pos = 0;
	write_huff_tree(output, tree, &buff, &pos);
	putc(buff, output); //finish writing tree

	free(mem_block_ptr); //freeing huffman tree (dynamic)

	rewind(input);

	encode_text2(input, table, output);

	fclose(input);
	fclose(output);

	return 0;
}

