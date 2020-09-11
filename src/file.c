/* file相关 */

#include <stdio.h>
#include "bootpack.h"
#include <malloc.h>
#define NULL 0
#define LEN sizeof(struct LNode)
//#define len sizeof(struct node)
#define MEMSIZE 100
#define MINSIZE 2
#define MAX_DISK_QUEUE 3

char *main_mem2;
struct node *workDir; //定义当前工作目录
struct node root;	  //定义根目录

char path[100]; //定义路径信息

char Dir_IsUsed[LIMIT_DIRECTPRY / sizeof(struct node)];
char OpenFileTable_IsUsed[LIMIT_OPEN_FILE_TABLE / sizeof(struct LNode)];

LinkList L;				  //文件打开表
MEMINFO MemList[MEMSIZE]; //磁盘空间信息表
int disk_queue[MAX_DISK_QUEUE];
struct Timer *disk_timer;

//初始化文件打开链表
void InitList(char *main_mem)
{
	disk_timer = timer_alloc();
	for (int i = 0; i < MAX_DISK_QUEUE; i++)
	{
		disk_queue[i] = 0;
	}
	main_mem2 = main_mem;
	/*
	int i;
	
	for(i=0;i<LIMIT_OPEN_FILE_TABLE/sizeof(struct LNode);i++)
	{
		if(OpenFileTable_IsUsed[i] == 0)
			break;
	}
	if(i==LIMIT_OPEN_FILE_TABLE/sizeof(struct LNode))
	{
		printf("No enough Memory!\n");
		return;
	}	
	L = (struct LNode*)(main_mem2 + AR_OPEN_FILE_TABLE)+i;
	OpenFileTable_IsUsed[i] = 1;
	if (L == NULL)  //初始化只含有头结点的空单链表
	{
		printf("结点分配失败\n");		
	}
	else{
		L->next = NULL;	
		L->size = 0;
		L->count = 0;
		
		L->file_node = NULL;
	}*/
	L = NULL;
}

//初始化空间信息表
void InitMemList()
{
	int x;
	MEMINFO temp = {0, 0, 'e'};
	for (x = 0; x < MEMSIZE; x++)
	{
		MemList[x] = temp;
	} //起始地址为0
	MemList[0].start = 0;
	//空间初始为最大
	MemList[0].Size = MEMSIZE;
	//状态为空闲
	MemList[0].status = 'f';
}

//初始化根目录
void get_init()
{
	strcpy(root.name, "root");
	root.type = 0;
	root.next = NULL;
	root.sub = NULL;
	root.father = NULL;

	workDir = &root;

	strcpy(path, "root");
}

void InitFCB(char fileName[])
{
	struct node *p;
	//p = new node;
	p = (struct node *)malloc(LEN);
	p->fcb.clustno = NULL;
	p->fcb.date = NULL;
	strcpy(p->fcb.name, fileName);
	p->fcb.permissions = NULL;
	strcpy(p->fcb.reserve, NULL);
	p->fcb.size = NULL;
	p->fcb.time = NULL;
	strcpy(p->fcb.type, NULL);
}

//初始化新结点函数
/*
struct node *init()
{
	struct node *p;
	//p = new node;
	p=(struct node *)malloc(LEN);
	strcpy(root.name, "");
	root.type = 0;
	root.next = NULL;
	root.sub = NULL;
	root.father = NULL;
	InitFCB(root.name);

	return p;
}*/

//进入某个目录
void cd(char dirName[])
{
	struct node *p;
	int flag = 0;

	p = workDir->sub;
	if (p == NULL)
	{
		printf("错误,'%s'子目录不存在\n", dirName);
	}
	else
	{
		while (p)
		{
			if (p->type == 0)
			{
				if (!strcmp(p->name, dirName))
				{
					flag = 1;
					break;
				}
			}
			p = p->next;
		}
		if (flag == 1)
		{
			workDir = p;
			strcat(path, "\\ ");
			strcat(path, p->name);
			printf("工作目录已进入'%s'\n", dirName);
		}
		else
		{
			printf("错误,'%s'子目录不存在\n", dirName);
			//cout << "错误,\"" << dirName << "\"子目录不存在" << endl;
		}
	}
}

//返回上一级
void back()
{
	char str[20];
	char str1[20] = "";
	int i = 0;
	int lastN = 0;
	int l = 0;
	struct node *p = NULL;
	p = (struct node *)malloc(LEN);
	if (workDir->father == NULL)
	{
		printf("已经是根目录了，无法回退\n");
	}
	else
	{
		p = workDir->father;
		lastN = strlen(workDir->name);
		workDir = p;
		strcpy(str, path);
		l = strlen(str);
		strncpy(str1, str, l - lastN - 2);
		strcpy(path, str1);
		printf("工作目录已进入'%s'\n", p->name);
	}
}

//申请磁盘空间
int Disk(int request)
{
	int i, j, t, flag;
	j = 0;
	flag = 0;
	t = MEMSIZE;
	//保存满足要求的最大空间
	for (i = 0; i < MEMSIZE && MemList[i].status != 'e'; i++)
	{
		if (MemList[i].Size >= request && MemList[i].status == 'f')
		{
			flag = 1;
			if (MemList[i].Size < t)
			{
				t = MemList[i].Size;
				j = i;
			}
		}
	}
	i = j;
	if (flag == 0)
	{
		printf("Not Enough Memory!\n");
		j = i;
		return -1;
	}
	else if (MemList[i].Size - request <= MINSIZE) //如果小于规定的最小差则将整个空间分配出去
	{
		MemList[i].status = 'u';
		Display();
		for (i = 0; i < MEMSIZE && MemList[i].status != 'e'; i++)
		{
		}
		return (MemList[i - 2].start);
	}
	else
	{
		//将i后的信息表元素后移
		for (j = MEMSIZE - 2; j > i; j--)
		{
			MemList[j + 1] = MemList[j];
		}

		//将i分成两部分，使用低地址部分
		MemList[i + 1].start = MemList[i].start + request;
		MemList[i + 1].Size = MemList[i].Size - request;
		MemList[i + 1].status = 'f';
		MemList[i].Size = request;
		MemList[i].status = 'u';
		Display();
		for (i = 0; i < MEMSIZE && MemList[i].status != 'e'; i++)
		{
		}
		//printf("%d\n",MemList[i-2].start);
		return (MemList[i - 2].start);
	}
}

//创建文件
void touch(char fileName[], int fileSize, char *main_mem)
{
	int flag = 0;
	int i;
	struct node *p, *q;

	int disk = Disk(fileSize); //磁盘调度算法

	//q = new node;
	//q = (struct node *)malloc(LEN);

	for (i = 0; i < LIMIT_DIRECTPRY / sizeof(struct node); i++)
	{
		if (Dir_IsUsed[i] == 0)
			break;
	}
	if (i == LIMIT_DIRECTPRY / sizeof(struct node) - 1)
	{
		printf("No enough Memory!\n");
		return;
	}

	q = (struct node *)(main_mem + AR_DIRECTORY) + i;
	Dir_IsUsed[i] = 1;

	strcpy(q->name, fileName);
	q->sub = NULL;

	q->type = 1;
	q->next = NULL;

	p = workDir->sub;

	if (p == NULL)
	{
		workDir->sub = q;
		if (disk >= 0)
		{
			strcpy(q->fcb.name, fileName);
			q->fcb.size = fileSize;
			q->fcb.clustno = disk;
			q->father = workDir;
			strcpy(q->fcb.type, "[file]");

			printf("'%s'文件创建成功\n", fileName);
		}
		else
			printf("磁盘调度失败，可能是无足够空间\n");
	}
	else
	{
		flag = 0;
		while (p != NULL)
		{
			if (p->type == 1 && strcmp(p->name, fileName) == 0) //文件名重复
			{
				flag = 1;
				printf("错误,'%s'文件已存在\n", fileName);
				//return;
			}
			p = p->next;
		}

		if (flag == 0) //文件不重复
		{

			if (disk < 0)
				printf("磁盘调度失败，可能是无足够空间\n");
			else
			{

				p = workDir->sub;
				while (p->next)
				{
					p = p->next;
				}
				p->next = q;
				strcpy(q->fcb.name, fileName);

				q->fcb.size = fileSize;
				q->fcb.clustno = disk;
				q->father = workDir;
				strcpy(q->fcb.type, "[file]");
				printf("'%s'文件创建成功\n", fileName);
			}
		}
	}
}

//删除文件
void del(char fileName[], char *main_mem)
{
	struct node *p, *q;
	int flag = 1;
	int in = 0; //判断文件count是否为0

	p = workDir->sub;
	if (p == NULL)
	{
		//cout << "错误,\"" << fileName << "\"文件不存在" << endl;
		printf("错误,'%s'文件不存在\n", fileName);
	}
	else //更改文件指针
	{
		while (p)
		{
			if (p->type == 1)
			{
				if (!strcmp(p->name, fileName))
				{
					in = findList(p->name); //返回文件打开计数
					if (in == 0)			//文件打开数为0
					{
						Disk_release(p->fcb.clustno);
						if (p == workDir->sub)
						{
							workDir->sub = p->next;
						}
						else
						{
							q = workDir->sub;
							while (q->next != p)
							{
								q = q->next;
							}
							q->next = p->next;
							free(p);
						}
						Dir_IsUsed[((int)p - (int)main_mem - (int)AR_DIRECTORY) / sizeof(struct node)] = 0;
						printf("'%s'文件已删除\n", fileName);
						flag = 0;
					}
					else
					{
						flag = 0;
						printf("文件正打开，请先关闭文件后再执行删除操作\n");
					}
					break;
				}
			}
			p = p->next;
		}
		if (flag == 1)
		{
			//cout << "错误,\"" << fileName << "\"文件不存在" << endl;
			printf("错误,'%s'文件不存在\n", fileName);
		}
	}
}

//释放磁盘空间
void Disk_release(int startadd)
{
	int i, number = 0;
	//输入的空间是使用的
	for (i = 0; i < MEMSIZE; i++)
	{
		if (MemList[i].start == startadd)
		{
			number = i;
			break;
		}
	}
	if (MemList[number].status == 'u')
	{
		MemList[number].status = 'f';		   //标志为空闲
		if (MemList[number + 1].status == 'f') //右侧空间为空则合并
		{
			MemList[number].Size += MemList[number + 1].Size;					   //大小合并
			for (i = number + 1; i < MEMSIZE - 1 && MemList[i].status != 'e'; i++) //i后面的空间信息表元素后移
			{
				if (i > 0)
				{
					MemList[i] = MemList[i + 1];
				}
			}
		}
		//左侧空间空闲则合并
		if (number > 0 && MemList[number - 1].status == 'f')
		{
			MemList[number - 1].Size += MemList[number].Size;
			for (i = number; i < MEMSIZE - 1 && MemList[i].status != 'e'; i++)
			{
				MemList[i] = MemList[i + 1];
			}
		}
	}
	else
	{
		printf("This Number is Not Exist or is Not Used!\n");
	}
	Display();
}

//打印磁盘空间信息
void Display()
{
	int i, used = 0; //记录可以使用的总空间量
	//printf("\n---------------------------------------------------\n");
	printf("磁盘使用信息：\n");
	printf("%5s%15s%15s%15s\n", "Number", "start", "size", "status");
	//printf("\n---------------------------------------------------\n");
	for (i = 0; i < MEMSIZE && MemList[i].status != 'e'; i++)
	{
		if (MemList[i].status == 'u')
		{
			used += MemList[i].Size;
		}
		printf("%5d%15d%15d%15s\n", i, MemList[i].start, MemList[i].Size, MemList[i].status == 'u' ? "USED" : "FREE");
	}
	//printf("\n----------------------------------------------\n");
	printf("Totalsize:%-10d Used:%-10d Free:%-10d\n", MEMSIZE, used, MEMSIZE - used);
}

//重命名文件
void rename_file(char fileName[], char newName[])
{

	struct node *p = NULL;
	int flag = 0;
	p = workDir->sub;
	int in = 0;

	if (p == NULL)
	{
		//cout << "错误,\"" << fileName << "\"文件不存在" << endl;
		printf("错误,'%s'文件不存在\n", fileName);
	}
	else
	{
		while (p)
		{
			if (p->type == 1)
			{
				if ((strcmp(p->name, fileName)) == 0)
				{

					in = findList(p->name);
					if (in == 0)
					{
						strcpy(p->name, newName);
						strcpy(p->fcb.name, newName);
						flag = 1;
						//cout << "已成功重命名为：" << p->name << endl;
						printf("已成功重命名为：%s\n", p->name);
					}
					else
					{
						flag = 1;
						//cout << "文件正打开，请关闭文件后再执行rename操作" << endl;
						printf("文件正打开，请关闭文件后再执行rename操作\n");
					}
					break;
				}
			}
			p = p->next;
		}
		if (flag == 0)
		{
			//cout << "该文件夹中无名为“" << fileName << "”的文件" << endl;
			printf("该文件夹中无名为'%s'的文件\n", fileName);
		}
	}
}

//在链表里找具体的点
int findList(char e[])
{
	LinkList p;
	p = L;
	if (p == NULL)
		return 0;
	else
	{
		while (p != NULL)
		{
			if ((strcmp(p->file_node->name, e)) == 0)
				break;
			else
				p = p->next;
		}

		if (p == NULL) //文件打开表里无该文件
		{
			return 0;
		}
		else
			return p->count;
	}
	//return 0;
}

//重命名目录
void mv(char dirName[], char newName[])
{
	struct node *p;
	int flag = 0;
	p = workDir->sub;
	if (p == NULL)
	{
		//cout << "错误,\"" << dirName << "\"目录不存在" << endl;
		printf("错误,'%s'目录不存在\n", dirName);
	}
	else
	{
		while (p)
		{
			if (p->type == 0)
			{
				if ((strcmp(p->name, dirName)) == 0)
				{
					strcpy(p->name, newName);
					flag = 1;
					//cout << "已成功重命名为：" << p->name << endl;
					printf("已成功重命名为：%s\n", p->name);
					break;
				}
			}
			p = p->next;
		}
		if (flag == 0)
		{
			//cout << "该文件夹中无名为“" << dirName << "”的文件" << endl;
			printf("该文件夹中无名为'%s'的目录\n", dirName);
		}
	}
}

//显示本目录下所有兄弟目录和文件
void dir(struct node *p)
{
	while (p)
	{
		if (p->type == 0)
		{
			printf("%10s%10s\n", p->name, "<DIR>");
		}
		else
		{
			printf("%10s%10s%10d\n", p->name, "<FILE>", p->fcb.size);
		}
		p = p->next;
	}
}

//显示某个子目录下的文件
void dirs(struct node *p, const char str[])
{
	char newstr[100];
	struct node *q;
	dir(p);

	q = p;
	if (q->sub)
	{
		strcpy(newstr, "");
		strcat(newstr, str);
		strcat(newstr, "\\");
		strcat(newstr, q->name);

		dirs(q->sub, newstr);
	}

	q = p;
	while (q->next)
	{
		if (q->next->sub)
		{
			strcpy(newstr, "");
			strcat(newstr, str);
			strcat(newstr, " \\");
			strcat(newstr, q->next->name);

			dirs(q->next->sub, newstr);
		}
		q = q->next;
	}
}

// 显示所有目录
void ls()
{
	struct node *p;

	printf("当前目录下的目录及文件结构: \n");

	p = workDir->sub;
	if (p == NULL)
	{
		printf("无\n");
	}
	else
	{
		while (p)
		{
			if (p->type == 0)
			{
				printf("%5s%10s\n", p->name, "<DIR>");
				if (p->sub != NULL)
				{
					dirs(p->sub, p->name);
				}
			}
			else
			{
				printf("%5s%10s%10d\n", p->name, "<FILE>", p->fcb.size);
			}
			p = p->next;
		}
	}
}

// 创建目录
void makedir(char dirName[], char *main_mem)
{
	int flag = 1;
	struct node *p, *q;
	int i;

	//q = new node;
	//q=(struct node *)malloc(LEN);

	for (i = 0; i < LIMIT_DIRECTPRY / sizeof(struct node); i++)
	{
		if (Dir_IsUsed[i] == 0)
			break;
	}
	if (i == LIMIT_DIRECTPRY / sizeof(struct node) - 1)
	{
		printf("No enough Memory!\n");
		return;
	}

	q = (struct node *)(main_mem + AR_DIRECTORY) + i;
	Dir_IsUsed[i] = 1;

	//for(int j=0;j<5;j++)
	//printf("%d\n",IsUsed[j]);

	strcpy(q->name, dirName);
	q->sub = NULL;
	q->type = 0;
	q->next = NULL;
	q->father = workDir;

	p = workDir->sub;

	if (p == NULL)
	{
		workDir->sub = q;
		printf("'%s'子目录创建成功\n", dirName);
	}
	else
	{
		flag = 0;
		while (p)
		{
			if (p->type == 0)
			{
				if (!strcmp(p->name, dirName))
				{
					flag = 1;
					printf("错误,'%s'子目录已存在\n", dirName);
				}
			}
			p = p->next;
		}
		if (flag == 0)
		{
			p = workDir->sub;
			while (p->next)
			{
				p = p->next;
			}
			p->next = q;
			//cout << "\"" << dirName << "\"子目录创建成功" << endl;
			printf("'%s'子目录创建成功\n", dirName);
		}
	}
}

//删除目录
void rm(char dirName[], char *main_mem)
{
	struct node *p, *q;
	int flag = 0;

	p = workDir->sub;
	if (p == NULL)
	{
		//cout << "错误,\"" << dirName << "\"子目录不存在" << endl;
		printf("错误,'%s'子目录不存在\n", dirName);
	}
	else
	{
		while (p)
		{
			if (p->type == 0)
			{
				if (!strcmp(p->name, dirName))
				{
					flag = 1;
					break;
				}
			}
			p = p->next;
		}
		if (flag == 1)
		{
			if (p == workDir->sub)
			{
				workDir->sub = p->next;
			}
			else
			{
				q = workDir->sub;
				while (q->next != p)
				{
					q = q->next;
				}
				q->next = p->next;
				free(p);
			}
			//printf("%d\n",((int)p-(int)main_mem - (int)AR_DIRECTORY)/sizeof(struct node));
			Dir_IsUsed[((int)p - (int)main_mem - (int)AR_DIRECTORY) / sizeof(struct node)] = 0;
			printf("'%s'子目录已删除\n", dirName);
			//for(int j=0;j<5;j++)
			//printf("%d\n",IsUsed[j]);
		}
		else
		{
			printf("错误,'%s'子目录不存在\n", dirName);
		}
	}
}

//获取链表长度
int ListLength()
{

	LinkList p = L;
	int i = 0;
	if (p == NULL)
		return 0;
	else
	{
		while (p != NULL)
		{
			i++;
			p = p->next;
		}
		return i;
	}
}

//打印链表，即文件打开表
void printList()
{
	LNode *p;

	if (L != NULL)
	{
		p = L;
		int len = ListLength();
		printf("listlength:%d\n", len);
		printf("文件打开表：\n");
		printf("%10s %10s %15s\n", "文件名", "文件大小", "文件打开计数");
		for (int i = 0; i < len; i++)
		{
			//cout << setw(10)<< p->data <<setw(10)<<p->size<< setw(15) <<p->count<<endl;
			printf("%s%10d%15d\n", p->file_node->name, p->size, p->count);
			p = p->next;
		}
	}
	else
	{
		printf("暂无文件打开表\n");
	}
}

//打开文件
void openFile(char fileName[])
{
	struct node *p;
	p = workDir->sub;
	int flag = 0;
	if (p == NULL)
	{
		//cout << "该文件夹为空" << endl;
		printf("该文件夹为空\n");
	}
	else
	{
		while (p)
		{
			if (p->type == 1)
			{
				if (!strcmp(p->name, fileName))
				{
					//printf("1\n");
					ListInsert(p, main_mem2);
					printf("'%s'文件已打开\n", fileName);
					flag = 1;
					break;
				}
			}
			p = p->next;
		}
	}
	if (flag == 0)
	{
		printf("该文件夹下无名为'%s'的文件\n", fileName);
	}
	printList();
}

//链表中插入新节点
void ListInsert(struct node *pp, char *main_mem2)
{
	int i;
	struct LNode *n;
	for (i = 0; i < LIMIT_OPEN_FILE_TABLE / sizeof(struct LNode); i++)
	{
		if (OpenFileTable_IsUsed[i] == 0)
			break;
	}
	if (i == LIMIT_OPEN_FILE_TABLE / sizeof(struct LNode))
	{
		printf("No enough Memory!\n");
		return;
	}

	n = (struct LNode *)(main_mem2 + AR_OPEN_FILE_TABLE) + i;
	OpenFileTable_IsUsed[i] = 1;

	n->file_node = pp;
	//printf("")
	n->size = pp->fcb.size;
	n->count = 0;

	LinkList p;
	if (L == NULL)
	{
		printf("4\n");
		for (i = 0; i < LIMIT_OPEN_FILE_TABLE / sizeof(struct LNode); i++)
		{
			if (OpenFileTable_IsUsed[i] == 0)
				break;
		}
		if (i == LIMIT_OPEN_FILE_TABLE / sizeof(struct LNode) - 1)
		{
			printf("No enough Memory!\n");
			return;
		}

		L = (struct LNode *)(main_mem2 + AR_OPEN_FILE_TABLE) + i;
		OpenFileTable_IsUsed[i] = 1;

		L->size = pp->fcb.size;
		L->count = 1;
		L->file_node = pp;
	}
	else
	{
		p = L;
		while (p != NULL)
		{
			if (strcmp(p->file_node->name, pp->name) != 0)
				p = p->next;
			else
				break;
		}
		if (p == NULL)
		{
			n->next = L->next;
			L->next = n;
			n->count += 1;
		}
		else
		{
			p->count += 1;
		}
	}
}

//关闭文件
void closeFile(char fileName[])
{
	struct node *p;
	p = workDir->sub;
	int flag = 0;
	if (p == NULL)
	{
		printf("错误！该文件夹为空\n");
	}
	else
	{
		while (p)
		{
			if (p->type == 1)
			{
				if (strcmp(p->name, fileName) == 0)
				{
					listDelete(p->name);
					flag = 1;
					break;
				}
			}
			p = p->next;
		}
	}
	if (flag == 0)
	{
		printf("该文件夹下无名为'%s'的文件\n", fileName);
	}
	printList();
}

//删除链表上指定名字的一个结点
void listDelete(char e[])
{
	LinkList p = L;
	if (p == NULL)
		printf("文件打开表为空");
	else
	{
		while (p != NULL)
		{
			if (strcmp(p->file_node->name, e) != 0)
				p = p->next;
			else
				break;
		}
		if (p != NULL)
		{
			if (p->count == 1)
			{
				p->count = 0;
				printf("文件已完全关闭\n");
			}
			else
			{
				p->count -= 1;
				printf("文件已关闭一项，还剩%d未关闭\n", p->count);
			}
		}
	}
}

//读文件
void readFile(char fileName[])
{

	struct node *p;
	p = workDir->sub;
	int flag = 0;
	int in = 0;
	if (p == NULL)
	{
		//cout << "错误！该文件夹为空" << endl;
		printf("错误！该文件夹为空\n");
	}
	else
	{
		while (p)
		{
			if (p->type == 1)
			{
				if (!strcmp(p->name, fileName))
				{
					in = findList(p->name);
					if (in == 0)
					{
						//cout << "该文件尚未打开，请先执行open" << endl;
						printf("该文件尚未打开，请先执行open\n");
						flag = 1;
					}
					else
					{
						//cout << "正在读" << fileName << "..." << endl;
						printf("正在读'%s'...\n", fileName);
						flag = 1;
					}
					break;
				}
			}
			p = p->next;
		}
	}
	if (flag == 0)
	{
		//cout << "该文件夹下无名为“" << fileName << "”的文件" << endl;
		printf("该文件夹下无名为'%s'的文件\n", fileName);
	}
	printList();
}

//写文件
void writeFile(char fileName[])
{
	struct node *p;
	p = workDir->sub;
	int flag = 0;
	int in = 0;
	if (p == NULL)
	{
		//cout << "错误！该文件夹为空" << endl;
		printf("错误！该文件夹为空\n");
	}
	else
	{
		while (p)
		{
			if (p->type == 1)
			{
				if (strcmp(p->name, fileName) == 0)
				{
					in = findList(p->name);
					if (in == 0)
					{
						printf("该文件尚未打开，请先执行open\n");
						flag = 1;
					}
					else
					{
						//cout << "正在写" << fileName << "..." << endl;
						printf("正在写'%s'...\n", fileName);
						flag = 1;
					}
					break;
				}
			}
			p = p->next;
		}
	}
	if (flag == 0)
	{
		//cout << "该文件夹下无名为“" << fileName << "”的文件" << endl;
		printf("该文件夹下无名为'%s'的文件\n", fileName);
	}
	printList();
}

//查看文件FCB
void checkFCB(char fileName[])
{
	struct node *p;
	p = workDir->sub;
	int flag = 0;
	if (p == NULL)
	{
		flag = 1;
		//cout << "错误！该文件夹为空" << endl;
		printf("错误！该文件夹为空\n");
	}
	else
	{
		while (p != NULL)
		{
			if (p->type == 1)
			{
				if (strcmp(p->name, fileName) == 0)
				{
					printf("该文件的FCB如下：\n");
					printf("%10s %10s %10s %10s\n", "文件名", "文件大小", "起始磁盘数", "文件类型");
					printf(" %s%10d%10d%10s\n", fileName, p->fcb.size, p->fcb.clustno, p->fcb.type);
					flag = 1;
					break;
				}
			}
			p = p->next;
		}
	}
	if (flag == 0)
	{
		//cout << "该文件夹下无“" << fileName << "”文件" << endl;
		printf("该文件夹下无名为'%s'的文件\n", fileName);
	}
}

void checkAllFCB()
{
	struct node *p;
	p = workDir->sub;
	int flag = 0;
	if (p == NULL)
	{
		flag = 1;
		printf("该文件夹所有文件的FCB如下：\n");
		printf("无\n");
	}
	else
	{
		printf("该文件夹所有文件的FCB如下：\n");
		printf("%10s %10s %10s %10s\n", "文件名", "文件大小", "起始磁盘数", "文件类型");
		while (p != NULL)
		{
			if (p->type == 1)
			{
				printf(" %s%10d%10d%10s\n", p->fcb.name, p->fcb.size, p->fcb.clustno, "[FILE]");
				flag = 1;
			}
			p = p->next;
		}
	}
	if (flag == 0)
	{
		printf("该文件夹下无文件\n");
	}
}
