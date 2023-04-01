#include <stdio.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include "MultiTree.h"

#define DEF_MODE	(S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH|S_IWOTH)
#define DEF_UMASK	(S_IWGRP|S_IWOTH)

typedef struct
{
	char ID[10];
	char PPID[10];
	char Name[40];
}PSTreeNode;

static MFTree tree;

void GetPPidAndName(char* buf)
{
	char *p;
	char *delim = "\n";
  PSTreeNode* node;

	if ((node = (PSTreeNode*)malloc(sizeof(PSTreeNode))) == NULL)
	{
		printf("Malloc pstree node fail!");
		while(1);
	}

	p = strtok(buf, delim);
	while (p != NULL)
	{
		if (p[0] == 'N' && p[1] == 'a' && p[2] == 'm' && p[3] == 'e')
		{
			//node->Name = &p[5];
			strcpy(node->Name, &p[5]);
			//printf("Name %s ", node->Name);
		}
		else if (p[0] == 'P' && p[1] == 'i' && p[2] == 'd')
		{
			//node->ID = &p[5];
			strcpy(node->ID, &p[5]);
			//printf("Pid %s ", node->ID);
		}
		else if (p[0] == 'P' && p[1] == 'P' && p[2] == 'i' && p[3] == 'd')
		{
			strcpy(node->PPID, &p[6]);
			//printf("PPid %s \n", node->PPID);
			break; 
		}
		p = strtok(NULL, delim);
	}
	MFTree_Insert(&tree, tree.root, node);
	return;
}

void ReadDir(char* DirName, int nSpace)
{
	struct dirent *dep;
	int fd;
  char buf[128];
	DIR* streamp;
  char FilePath[80];
	char *delim = " ";
  char *p;

  streamp = opendir(DirName);	
  while ((dep = readdir(streamp)) != NULL)
	{
		if ((dep->d_name[0] >= '0') && (dep->d_name[0] <= '9'))
		{
			//printf("%s\n", dep->d_name);
			strcpy(buf, DirName);
	    buf[strlen(DirName)] = '/';	
	  	strcpy(buf + strlen(DirName) + 1, dep->d_name);
			ReadDir(buf, nSpace + 1);
		}
    if (strcmp(dep->d_name, "status") == 0)
		{
			strcpy(FilePath, DirName);
			strcpy(FilePath + strlen(DirName), "/status");
			fd = open(FilePath, O_RDONLY, 0);
			if (fd == -1)
			{
				printf("Open File fail:%s %d\n", dep->d_name, errno);
			}
			if (read(fd, buf, 127) == -1)
			{
				printf("Read file fail:%d\n", errno);
			}
			
			GetPPidAndName(buf);
			close(fd);
		}
		/*if (tree.size >= 200)
			break;*/

	}
  closedir(streamp);
}

MFTreeNode* FindPPID(char* ppid, MFTreeNode* node)
{
	MFTreeNode* RetNode = NULL;

	if (node == NULL)
		return RetNode;
	//Need to adjust.
	if (strcmp(((PSTreeNode*)(node->data))->ID, ppid) == 0)
	{
		return node;
	}
	else
	{
		if (node->bro != NULL)
			RetNode = FindPPID(ppid, node->bro);

		if (node->fir != NULL && RetNode == NULL)
			RetNode = FindPPID(ppid, node->fir);
	}
	return RetNode;
}

int CompareFunc(const void* node1, const void* node2)
{
	MFTreeNode* mft_node;
	mft_node = FindPPID(((PSTreeNode*)node2)->PPID, tree.root->fir);

	if (mft_node == NULL)
	{
		mft_node = tree.root;
	}
	
	if (mft_node->fir == NULL)
	{
		MFTree_ins_fir(&tree, mft_node, node2);
	}
	else
	{
		mft_node = mft_node->fir;
		while (mft_node->bro != NULL)
			mft_node = mft_node->bro;
		MFTree_ins_bro(&tree, mft_node, node2);
	}

	return 0;
}
int nSpace = 0;

void DisplayPSTree(MFTreeNode* node)
{
	int i = 1, j = 0, k = 0;
	char p[100], *ptr;

	if (node->data != NULL)
	{
		while (i < nSpace)
		{
			printf("|--------");
			i++;
		}
		//printf("%s %s %s\n", ((PSTreeNode*)(node->data))->ID, ((PSTreeNode*)(node->data))->Name,  ((PSTreeNode*)(node->data))->PPID); 
		for (int n = 0; n < 3; n++)
		{
			if (n == 0)
				ptr = ((PSTreeNode*)(node->data))->ID;
			else if (n == 1)
				ptr = ((PSTreeNode*)(node->data))->Name;
			else
				ptr = ((PSTreeNode*)(node->data))->PPID;
			j = 0;
			while (ptr[j] != '\0')
			{
				p[k] = ptr[j]; 
				j++;
				k++;
			}
			p[k] = '\0';
		}
		printf("%s \n", p);
	}

	if (node == NULL)
		return;
	else
	{
		nSpace++;
		if (node->fir != NULL)
			DisplayPSTree(node->fir);
		if (nSpace)
			nSpace--;	
		if (node->bro != NULL)
			DisplayPSTree(node->bro);	
	}
}

int main(int argc, char *argv[]) {
  for (int i = 0; i < argc; i++) {
    assert(argv[i]);
    printf("argv[%d] = %s\n", i, argv[i]);
  }
  assert(!argv[argc]);
	
	//static MFTree tree;
	MFTree_Init(&tree, free); 
	tree.root = (MFTreeNode*)malloc(sizeof(MFTreeNode));
	tree.root->fir = tree.root->bro = NULL;
	tree.compare = CompareFunc;
	ReadDir(argv[1], 0);
  DisplayPSTree(tree.root); 
	MFTree_Destroy(tree);	
 	return 0;
}
