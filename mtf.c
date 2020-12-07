#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifndef BLOCK_SIZE
#define BLOCK_SIZE 10000
#endif

//move-to-front transformation
//could be modified (improve speed to N*logM, N - l(string), M - |alphabet|)
void mtf(const char *string, int leng, char *out_string) {
	unsigned char *syms = (char*)malloc(sizeof(char) * 256);
	for (int i = 0; i < 256; i++) {
		syms[i] = i;
	}
	
	for (int i = 0; i < leng; i++) {
		unsigned char cur_sym = string[i];

		//writing out_sym
		out_string[i] = syms[cur_sym];

		//shifting syms [0,cur_sym) in syms
		for (int j = 0; j < 256; j++) {
			if (syms[j] < syms[cur_sym]) {
				syms[j]++;
			}
		}
		//moving to front readed sym
		syms[cur_sym] = 0;
	}
	free(syms);
}

void inverse_mtf(const char *string, int leng, char *out_string) {
	unsigned char *syms = (char*)malloc(sizeof(char) * 256);
	for (int i = 0; i < 256; i++) {
		syms[i] = i;
	}

	for (int i = 0; i < leng; i++) {

		//reading index of encoded sym
		unsigned char index = string[i];

		//getting this sym
		unsigned char out_sym = syms[index];

		//shifting other syms
		for (int j = index - 1; j >= 0; j--) {
			syms[j + 1] = syms[j];
		}
		//putting encoded sym at the beginning of list
		syms[0] = out_sym;

		//writing this sym
		out_string[i] = out_sym;
	}
	free(syms);
}

int encode(char *in_path, char *out_path) {
	FILE *ifile = fopen(in_path, "rb");
	FILE *ofile = fopen(out_path, "wb");
	char *buffer = (char*)malloc(sizeof(char) * BLOCK_SIZE);
	char *out_buffer = (char*)malloc(sizeof(char) * BLOCK_SIZE);
	
	int readed;
	while ( (readed = fread(buffer, sizeof(char), BLOCK_SIZE, ifile)) ) {
		mtf(buffer, readed, out_buffer);
		fwrite(&readed, sizeof(int), 1, ofile) * sizeof(int);
		fwrite(out_buffer, sizeof(char), readed, ofile);
	}
	
	fclose(ifile);
	fclose(ofile);
	return 0;
}

int decode(char *in_path, char *out_path) {
	FILE *ifile = fopen(in_path, "rb");
	FILE *ofile = fopen(out_path, "wb");
	char *buffer = (char*)malloc(sizeof(char) * BLOCK_SIZE);
	char *out_buffer = (char*)malloc(sizeof(char) * BLOCK_SIZE);

	int blk_size;

	int readed = 1;
	while (readed) {
		fread(&blk_size, sizeof(int), 1, ifile) * sizeof(int);
		readed = fread(buffer, sizeof(char), blk_size, ifile);

		if (readed) {
			inverse_mtf(buffer, blk_size, out_buffer);
			fwrite(out_buffer, sizeof(char), blk_size, ofile);
		}
	}

	fclose(ifile);
	fclose(ofile);
	return 0;
}

int main(int argc, char *argv[]) {
	char *oper = argv[1];
	char *input_path = argv[2];
	char *output_path = argv[3];
	if (*oper == 'e') {
		clock_t beg = clock();
		encode(input_path, output_path);
		clock_t end = clock();
		printf("time: %f\n", (float)(end - beg) / CLOCKS_PER_SEC);
	}
	else if (*oper == 'd') {
		clock_t beg = clock();
		decode(input_path, output_path);
		clock_t end = clock();
		printf("time: %f\n", (float)(end - beg) / CLOCKS_PER_SEC);
	}
	return 0;
}