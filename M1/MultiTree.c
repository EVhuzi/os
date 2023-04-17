#include <string.h>
#include "MultiTree.h"

void MFTree_Init(MFTree *tree, void (*destroy)(void *data))
{
	/* Initialize the binary tree */
	tree->size = 0;
	tree->destroy = destroy;
	tree->root = NULL;
	return;
}

void MFTree_Destroy(MFTree tree)
{
	MFTree_rem_fir(&tree, NULL);
	memset(&tree, 0, sizeof(tree));
	return;
}

int MFTree_ins_fir(MFTree *tree, MFTreeNode *node, const void *data)
{
	MFTreeNode* new_node, **position;
	if (node == NULL)
	{
		if (MFTree_size(tree) > 0)
			return -1;
		position = &tree->root;
	}
	else
	{
		if (MFTree_fir(node) != NULL)
			return -1;
		position = &node->fir;
	}
	
	if ((new_node = (MFTreeNode*)malloc(sizeof(MFTreeNode))) == NULL)
	{
		return -1;
	}	
	/* Insert the node into the tree. */
	new_node->data = (void*)data;
	new_node->fir = NULL;
	new_node->bro = NULL;
	*position = new_node;

	tree->size++;
	
	return 0;
}

int MFTree_ins_bro(MFTree *tree, MFTreeNode *node, const void *data)
{
	MFTreeNode* new_node, **position;
	if (node == NULL)
	{
		if (MFTree_size(tree) > 0)
			return -1;
		position = &tree->root;
	}
	else
	{ 
		if (MFTree_bro(node) != NULL)
			return -1;
		position = &node->bro;
	}
	if ((new_node = (MFTreeNode*)malloc(sizeof(MFTreeNode))) == NULL)
		return -1;

	/* Insert the node into the tree. */
	new_node->data = (void*)data;
	new_node->fir = NULL;
	new_node->bro = NULL;
	*position = new_node;

	tree->size++;

	return 0;
}

int MFTree_Insert(MFTree *tree, MFTreeNode *node, const void *data)
{
	int ret;
	if (tree->root == NULL)
	{
		MFTree_ins_fir(tree, node, data);
	}
  else if (tree->compare != NULL)
	{
		ret = tree->compare(node->data, data);
		if (ret > 0)
		{
			if (node->fir == NULL)
				MFTree_ins_fir(tree, node, data);
			else
				MFTree_Insert(tree, node->fir, data);
		}
		else if (ret < 0)//if (tree->compare(node->data, data) < 0)
		{
			if (node->bro == NULL)
				MFTree_ins_bro(tree, node, data);
			else
				MFTree_Insert(tree, node->bro, data);
		}
	}
	return 0;
}

void MFTree_rem_fir(MFTree *tree, MFTreeNode *node)
{
	MFTreeNode **position;

	if (MFTree_size(tree) == 0)
		return;
	if (node == NULL)
		position = &tree->root;
	else
		position = &node->fir;
	
	if (*position != NULL)
	{
		MFTree_rem_fir(tree, *position);
		MFTree_rem_bro(tree, *position);

		if (tree->destroy != NULL)
			tree->destroy((*position)->data);

		free(*position);
		*position = NULL;

		tree->size--;
	}
	return;
}

void MFTree_rem_bro(MFTree *tree, MFTreeNode *node)
{
	MFTreeNode **position;

	if (MFTree_size(tree) == 0)
		return;
	if (node == NULL)
		position = &tree->root;
	else
		position = &node->bro;
	
	if (*position != NULL)
	{
		MFTree_rem_fir(tree, *position);
		MFTree_rem_bro(tree, *position);

		if (tree->destroy != NULL)
			tree->destroy((*position)->data);

		free(*position);
		*position = NULL;

		tree->size--;
	}
}

int MFTree_Compare(const void *key1, const void *key2)
{
	if ((*(int*)key1) < *((int*)key2))
	{
		return -1;
	}
	else if ((*(int*)key1) > *((int*)key2))
	{
		return 1;
	}
	else
		return 0;
}
void MFTree_Display(MFTreeNode* node)
{
	if (node == NULL)
		return;
	else
	{
		if (node->fir != NULL)
			MFTree_Display(node->fir);
		if (node->bro != NULL)
			MFTree_Display(node->bro);
		
		printf("%d\n", *(int*)node->data);
	}
	return;
}
/*int main()
{
	int k = 0;
	int* i; //= 7, j = 8, k = 9, a = 5, b = 4;
	MFTree tree;
	tree.compare = MFTree_Compare;

	MFTree_Init(&tree, free);
	while (k < 5)
	{
		i = (int*)malloc(sizeof(int)); 
		*i = k;
		MFTree_Insert(&tree, tree.root, (void*)i);
		k++;
	}
	MFTree_Display(tree.root);
	MFTree_Destroy(tree);	
	return 0; 
}*/
