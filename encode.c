#include "structs.h"
#include "bit_stream.h"
#include <stdio.h>
#include <stdlib.h>

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
			write_bit(output, GET_BIT(code.code, i));
		}
	}
	//write safeword
	bit_code code = table[256];
	for (int i = 0; i < code.leng; i++) {
		write_bit(output, GET_BIT(code.code, i));
	}
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