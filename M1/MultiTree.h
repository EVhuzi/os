#ifndef MFTREE_H
#define MFTREE_H

#include <stdio.h>
#include <stdlib.h>

typedef struct node{
	void* data;
	struct node* fir;	//指向第一个子结点
	struct node* bro;	//指向兄弟节点1
}MFTreeNode;

typedef struct MFTree_
{
  int size;
	int (*compare)(const void *key1, const void *key2);
	void (*destroy)(void *data);
	MFTreeNode* root;
}MFTree;

/* Public Interface */
void MFTree_Init(MFTree *tree, void (*destroy)(void *data));
void MFTree_Destroy(MFTree tree);
int MFTree_ins_fir(MFTree *tree, MFTreeNode *node, const void *data);
int MFTree_ins_bro(MFTree *tree, MFTreeNode *node, const void *data);
int MFTree_Insert(MFTree *tree, MFTreeNode *node, const void *data);
int MFT_Compare(const void *key1, const void *key2);
void MFTree_rem_fir(MFTree *tree, MFTreeNode* node);
void MFTree_rem_bro(MFTree *tree, MFTreeNode* node);

//void MFTree_insert(MFTree *tree, MFTreeNode *node, const void *data);

#define MFTree_size(tree)	((tree)->size)
#define MFTree_root(tree)	((tree)->root)
#define MFTree_fir(node)	((node)->fir)
#define MFTree_bro(node)	((node)->bro)
#define MFTree_is_eob(node)	((node) == NULL)
#define MFTree_is_leaf(node)	((node)->fir != NULL && (node)->bro == NULL)
#define MFTree_is_fir(node)		((node)->fir != NULL && (node)->bro == NULL)
#define MFTree_is_bro(node)		((node)->fir == NULL && (node)->bro != NULL)
#define MFTree_data(node)			((node)->data)	
#endif
