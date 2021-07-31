#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
#include "bit_stream.h"
#include "encode.h"
#include "decode.h"


#define ENC 1
#define DEC 0




struct huff_opt {
	const char *input_fpath;
	const char *output_fpath;
	char type; //encode/decode
	char time;
	char keep;
};


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