#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#define BLOCK_SIZE 10000

int cmp_str(const char *string, int pos1, int pos2, int leng) {
	while (string[pos1] == string[pos2]) {
		pos1 = (pos1 + 1) % leng;
		pos2 = (pos2 + 1) % leng;
	}
	return string[pos1] < string[pos2];
}

void sort(const char *string, int *ptrs, int leng) {
	int j;
	for (int i = 0; i < leng - 1; i++) {
		int tmp = ptrs[i + 1];
		for (j = i; j >= 0 && cmp_str(string, tmp, ptrs[j], leng); j--) { //tmp < ptrs[j]
			ptrs[j + 1] = ptrs[j];
		}
		ptrs[j + 1] = tmp;
	}
}

//burrows-wheeler transform
int bwt(const char *in_string, int leng, char *out_string) {

	//pointers at in_string imitation full-leng permutation of this string
	int *ptrs = (int*)malloc(sizeof(int) * leng);
	for (int i = 0; i < leng; i++) {
		ptrs[i] = i;
	}

	//sorting this strings alphabeticaly
	sort(in_string, ptrs, leng);

	//getting last column in sorted "table" of permutations (array of pointers)
	for (int i = 0; i < leng; i++) {
		int ind = (ptrs[i] + (leng - 1)) % leng; //gettin last symbol in permutations (pointers in cyclic string)
		out_string[i] = in_string[ind];
	}

	//getting position of string in sorted array of pointers (strings)
	int pos = 0;
	while (pos < leng && ptrs[pos]) {
		pos++;
	}
	free(ptrs);
	return pos;
}

//it should be stable sort
void sort2(const char *string, int *ind, int leng) {
	int j;
	for (int i = 0; i < leng - 1; i++) {
		int tmp = ind[i + 1];
		for (j = i; j >= 0 && string[tmp] < string[ind[j]]; j--) {
			ind[j + 1] = ind[j];
		}
		ind[j + 1] = tmp;
	}
}

//could be modified (improve speed to linear)
void inverse_bwt(const char *in_string, int leng, char *out_string, int pos) {

	int *indexes = (int*)malloc(sizeof(int) * leng);
	for (int i = 0; i < leng; i++) {
		indexes[i] = i;
	}

	sort2(in_string, indexes, leng);

	int ind = indexes[pos];
	for (int i = 0; i < leng; i++) {
		out_string[i] = in_string[ind];
		ind = indexes[ind];
	}

	free(indexes);
}

int encode(char *in_path, char *out_path) {
	FILE *ifile = fopen(in_path, "rb");
	FILE *ofile = fopen(out_path, "wb");
	char *buffer = (char*)malloc(sizeof(char) * BLOCK_SIZE);
	char *out_buffer = (char*)malloc(sizeof(char) * BLOCK_SIZE);
	
	int readed;
	while ( (readed = fread(buffer, sizeof(char), BLOCK_SIZE, ifile)) ) {
		int pos = bwt(buffer, readed, out_buffer);
		fwrite(&readed, sizeof(int), 1, ofile) * sizeof(int);
		fwrite(&pos, sizeof(int), 1, ofile) * sizeof(int);
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

	int pos;
	int blk_size;

	int readed = 1;
	while (readed) {
		fread(&blk_size, sizeof(int), 1, ifile) * sizeof(int);
		fread(&pos, sizeof(int), 1, ifile) * sizeof(int);
		readed = fread(buffer, sizeof(char), blk_size, ifile);
		
		if (readed) {
			inverse_bwt(buffer, blk_size, out_buffer, pos);
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