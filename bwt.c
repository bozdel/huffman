#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#define BLOCK_SIZE 10000

int cmp_str(const char *string, int pos1, int pos2, int leng) {
	for (int i = 0; i < leng && string[pos1] == string[pos2]; i++) {
		pos1 = (pos1 + 1) % leng;
		pos2 = (pos2 + 1) % leng;
	}
	return string[pos1] < string[pos2];
}

void insertion_sort_gap(const char *string, int *ptrs, int leng, int cmp_leng, int gap) {
	int j;
	for (int i = 0; i < leng - gap; i++) {
		int tmp = ptrs[i + gap];
		for (j = i; j >= 0 && cmp_str(string, tmp, ptrs[j], cmp_leng); j -= gap) {
			ptrs[j + gap] = ptrs[j];
		}
		ptrs[j + gap] = tmp;
	}
}

void shell_sort(const char *string, int *ptrs, int leng, int cmp_leng) {
	//sedgewick
	int gap = 0;
	int i = 0;
	while (gap < leng) {
		int m1 = 1 << i;
		int m2 = 1 << (i / 2);
		gap = 9 * m1 - 9 * m2 + 1;
		i++;
	}
	i -= 2;
	for ( ; i >= 0; i--) {
		int m1 = 1 << i;
		int m2 = 1 << (i / 2);
		gap = 9 * m1 - 9 * m2 + 1;
		insertion_sort_gap(string, ptrs, leng, cmp_leng, gap);
	}
}

void merge(const char *string, int *dst, int *arr1, int *arr2, int l1, int l2, int cmp_leng) {
	int curr1 = 0;
	int curr2 = 0;
	int dst_pos = 0;

	while (curr1 < l1 && curr2 < l2) {
		if (!cmp_str(string, arr2[curr2], arr1[curr1], cmp_leng)) { //arr1[curr1] <= arr2[curr2]
			dst[dst_pos] = arr1[curr1];
			curr1++;
		}
		else {
			dst[dst_pos] = arr2[curr2];
			curr2++;
		}
		dst_pos++;
	}

	for ( ; curr1 < l1; curr1++) {
		dst[dst_pos] = arr1[curr1];
		dst_pos++;
	}
	for ( ; curr2 < l2; curr2++) {
		dst[dst_pos] = arr2[curr2];
		dst_pos++;
	}
}

// [start, end)
void merge_sort(const char *string, int *array, int start, int end, int cmp_leng) {
	int leng = end - start;

	if (leng < 2) {
		return;
	}

	merge_sort(string, array, start, start + leng / 2, cmp_leng);
	merge_sort(string, array, start + leng / 2, end, cmp_leng);
	
	int *tmp = (int*)malloc(sizeof(int) * leng);

	int *l_arr = array + start;
	int *r_arr = array + start + leng / 2;

	merge(string, tmp, l_arr, r_arr, leng / 2, leng - leng / 2, cmp_leng);

	for (int i = 0; i < leng; i++) {
		array[start + i] = tmp[i];
	}

	free(tmp);
}

void sort(const char *string, int *ptrs, int leng, int cmp_leng) {
	int j;
	for (int i = 0; i < leng - 1; i++) {
		int tmp = ptrs[i + 1];
		for (j = i; j >= 0 && cmp_str(string, tmp, ptrs[j], cmp_leng); j--) { //tmp < ptrs[j]
			ptrs[j + 1] = ptrs[j];
		}
		ptrs[j + 1] = tmp;
	}
}

int is_eq(int *arr1, int *arr2, int leng) {
	for (int i = 0; i < leng; i++) {
		if (arr1[i] != arr2[i]) return 0;
	}
	return 1;
}

int is_sorted(const char *string, int *arr, int leng) {
	for (int i = 0; i < leng - 1; i++) {
		if (string[arr[i]] > string[arr[i + 1]]) return 0;
	}
	return 1;
}

//burrows-wheeler transform
int bwt(const char *in_string, int leng, char *out_string) {

	//pointers at in_string imitation full-leng permutation of this string
	int *ptrs = (int*)malloc(sizeof(int) * leng);
	/*int *ptrs2 = (int*)malloc(sizeof(int) * leng);
	int *ptrs3 = (int*)malloc(sizeof(int) * leng);*/
	for (int i = 0; i < leng; i++) {
		ptrs[i] = i;
		/*ptrs2[i] = i;
		ptrs3[i] = i;*/
	}

	//sorting this strings alphabeticaly
	/*sort(in_string, ptrs, leng, leng);
	shell_sort(in_string, ptrs2, leng, leng);*/
	merge_sort(in_string, ptrs, 0, leng, leng);

	/*printf("sorted ptrs  by sort      : %d\n", is_sorted(in_string, ptrs, leng));
	printf("sorted ptrs2 by shell_sort: %d\n", is_sorted(in_string, ptrs2, leng));
	printf("sorted ptrs3 by merge_sort: %d\n", is_sorted(in_string, ptrs3, leng));

	printf("is equal ptrs and ptrs2: %d\n", is_eq(ptrs, ptrs2, leng));
	printf("is equal ptrs and ptrs3: %d\n", is_eq(ptrs, ptrs3, leng));*/

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


//could be modified (improve speed to linear)
void inverse_bwt(const char *in_string, int leng, char *out_string, int pos) {

	int *indexes = (int*)malloc(sizeof(int) * leng);
	/*int *indexes2 = (int*)malloc(sizeof(int) * leng);
	int *indexes3 = (int*)malloc(sizeof(int) * leng);*/
	for (int i = 0; i < leng; i++) {
		indexes[i] = i;
		/*indexes2[i] = i;
		indexes3[i] = i;*/
	}

	//it should be stable sort
	/*sort(in_string, indexes, leng, 0);
	shell_sort(in_string, indexes2, leng, 0);*/
	merge_sort(in_string, indexes, 0, leng, 0);

	/*printf("sorted indexes  by sort      : %d\n", is_sorted(in_string, indexes, leng));
	printf("sorted indexes2 by shell_sort: %d\n", is_sorted(in_string, indexes2, leng));
	printf("sorted indexes3 by merge_sort: %d\n", is_sorted(in_string, indexes3, leng));

	printf("is equal indexes and indexes2: %d\n", is_eq(indexes, indexes2, leng));
	printf("is equal indexes and indexes3: %d\n", is_eq(indexes, indexes3, leng));*/

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