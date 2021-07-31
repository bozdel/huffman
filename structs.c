#include "structs.h"
#include "bit_stream.h"
#include <stdlib.h>

//--------------------static tree------------------------------------------


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

void insert_node(huff_tree **head, huff_tree *node) {
	huff_tree **current = head;
	while (*current && (*current)->freq < node->freq) {
		current = &((*current)->next);
	}
	node->next = (*current);
	*current = node;
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