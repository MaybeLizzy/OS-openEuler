#include <stdio.h>

#include <stdlib.h>

#include <pthread.h>

#include <unistd.h>

#include <semaphore.h>

#include <stdbool.h>

#include <string.h>

#include <sys/types.h>

#include "bootpack.h"

sem_t empty, full;

pthread_mutex_t mutex;

struct FIFO32 fifo;

struct GATE_DESCRIPTOR *int_table;

extern struct TASKCTL *taskctl;

char *main_mem;

struct MEMMAN *memman;

struct TASK *task_a;

char buf[BUFF_SIZE];

int pos;

int shutdown;

void *gain_inc(void *param)
{

	char temp;

	while (true)
	{

		sem_wait(&empty);

		/* acquire the mutex lock */

		pthread_mutex_lock(&mutex);

		/* critical section */

		/* 键盘 */

		// task_sleep(task_a);

		scanf("%c", &temp);

		int_table[21].offset(temp);

		/* ralease the mutex lock */

		pthread_mutex_unlock(&mutex);

		sem_post(&full);
	}
}

void *handle_inc(void *param)
{
	int tot = 0;

	int i;

	pos = 0;

	unsigned int memtotal;

	memman = (struct MEMMAN *)(main_mem + 0x03000);

	memtotal = memtest(0x04000, 0xfffff);

	memman_init(memman);

	memman_free(memman, main_mem + 0x04000, memtotal - 0x04000 + 1);

	init_keyboard(&fifo, 0);

	int_table = (struct GATE_DESCRIPTOR *)main_mem;

	init_gdtidt((struct SEGMENT_DESCRIPTOR *)(main_mem + 0x00800), (struct GATE_DESCRIPTOR *)main_mem);

	init_pic();

	init_pit();

	task_a = task_init(memman, main_mem + 0x04000);

	fifo.task = task_a;

	task_run(task_a, 1, 0);
	task_sleep(task_a);

	get_init(); //初始化根目录

	InitList(main_mem); //初始化文件打开链表
	InitMemList();

	sem_post(&empty); /* 取消CPU中断禁用，因为IDT /PIC初始化已完成 */

	while (true)
	{

		sem_wait(&full);

		/* acquire the mutex lock */

		pthread_mutex_lock(&mutex);

		/* critical section */

		int i = fifo32_get(&fifo); /* i为中断产生的信息，例如键盘按键、鼠标移动 */
		task_sleep(task_a);
		printf("任务a即将休眠...\n");
		/* ralease the mutex lock */

		pthread_mutex_unlock(&mutex);

		sem_post(&empty);

		/* 对信息i进行处理 */

		if (i != '\n')
		{
			buf[pos++] = i;
		}
		else
		{
			buf[pos] = '\0';
			if (strncmp("new process", buf, 11) == 0)
			{
				int index = 12;

				int mem = 0;
				while (buf[index] >= '0' && buf[index] <= '9')
				{
					mem = mem * 10 + buf[index++] - '0';
				}
				index++;

				int level = 0;
				while (buf[index] >= '0' && buf[index] <= '9')
				{
					level = level * 10 + buf[index++] - '0';
				}
				index++;

				int priority = 0;
				while (buf[index] >= '0' && buf[index] <= '9')
				{
					priority = priority * 10 + buf[index++] - '0';
				}
				index++;

				struct TASK *task_b = task_alloc();
				task_b->tss.esp = memman_alloc_1k(memman, mem * 1024) + mem * 1024;
				task_b->tss.eax = 1;
				task_b->tss.ebx = mem * 1024;
				task_b->tss.ecx = task_b->tss.esp - mem * 1024;
				task_b->tss.eip = &task_b_main;
				task_b->tss.es = 1 * 8;
				task_b->tss.cs = 2 * 8;
				task_b->tss.ss = 1 * 8;
				task_b->tss.ds = 1 * 8;
				task_b->tss.fs = 1 * 8;
				task_b->tss.gs = 1 * 8;
				task_run(task_b, level, priority);
			}
			else if (strncmp("sleep process", buf, 13) == 0)
			{
				int index = 14, i = 0, j = 0;
				struct TASK *task;
				int sel = 0;
				while (buf[index] >= '0' && buf[index] <= '9')
				{
					sel = sel * 10 + buf[index++] - '0';
				}
				index++;

				for (i = 0; i < MAX_TASKS; i++)
				{
					if (taskctl->tasks0[i].flags != 0 && taskctl->tasks0[i].sel == sel)
					{
						if (taskctl->tasks0[i].flags == 2)
						{
							task_sleep(&taskctl->tasks0[i]);
						}
					}
				}
			}
			else if (strncmp("remove process", buf, 14) == 0)
			{
				int index = 15, i = 0, j = 0;
				struct TASK *task;
				int sel = 0;
				while (buf[index] >= '0' && buf[index] <= '9')
				{
					sel = sel * 10 + buf[index++] - '0';
				}
				index++;

				for (i = 0; i < MAX_TASKS; i++)
				{
					if (taskctl->tasks0[i].flags != 0 && taskctl->tasks0[i].sel == sel)
					{
						if (taskctl->tasks0[i].flags == 2)
						{
							task_sleep(&taskctl->tasks0[i]);
						}
						else
						{
							taskctl->level[taskctl->tasks0[i].level].running++;
							task_remove(&taskctl->tasks0[i]);
						}
						taskctl->tasks0[i].flags = 0;
						memman_free_1k(memman, taskctl->tasks0[i].tss.ecx, taskctl->tasks0[i].tss.ebx);
						for (int j = 0; j < MAX_OPEN_FILE_NUM; j++)
						{
							if (taskctl->tasks0[i].fhandle[j] != 0)
							{
								taskctl->tasks0[i].fhandle[j]->count -= 1;
							}
							else
								break;
						}
					}
				}
			}
			else if (strncmp("wake process", buf, 12) == 0)
			{
				int index = 13, i = 0, j = 0;
				struct TASK *task;
				int sel = 0;
				while (buf[index] >= '0' && buf[index] <= '9')
				{
					sel = sel * 10 + buf[index++] - '0';
				}
				index++;

				for (i = 0; i < MAX_TASKS; i++)
				{
					if (taskctl->tasks0[i].flags != 0 && taskctl->tasks0[i].sel == sel)
					{
						task_run(&taskctl->tasks0[i], -1, -1);
						break;
					}
				}
			}
			else if (strcmp("shutdown", buf) == 0)
			{
				shutdown = 1;
			}
			else if (strncmp("process", buf, 7) == 0)
			{
				int sel = 0, index = 8, j;
				char filename[20];

				while (buf[index] >= '0' && buf[index] <= '9')
				{
					sel = sel * 10 + buf[index++] - '0';
				}
				index++;
				if (strncmp("open", buf + index, 4) == 0)
				{
					index += 5;
					strcpy(filename, buf + index);

					for (j = 0; j < MAX_TASKS; j++)
					{

						if (taskctl->tasks0[j].sel == sel && taskctl->tasks0[j].flags == 2)
						{
							openFile(filename);

							LinkList p;
							p = L;
							if (p == NULL)
								printf("文件打开表里无该文件\n");
							else
							{
								while (p != NULL)
								{
									if ((strcmp(p->file_node->name, filename)) == 0)
										break;
									else
										p = p->next;
								}
								if (p == NULL) //文件打开表里无该文件
								{
									printf("文件打开表里无该文件\n");
								}
								else
								{
									for (int k = 0; k < MAX_OPEN_FILE_NUM; k++)
									{
										if (taskctl->tasks0[j].fhandle[k] == 0)
										{
											taskctl->tasks0[j].fhandle[k] = p;
											break;
										}
									}
								}
							}

							break;
						}
					}

					if (j == MAX_TASKS)
					{
						printf("不存在该进程\n");
					}
				}
				else if (strncmp("close", buf + index, 5) == 0)
				{
					index += 6;
					strcpy(filename, buf + index);
					closeFile(filename); //系统打开表-1
					for (j = 0; j < MAX_TASKS; j++)
					{
						printf("000\n");
						if (taskctl->tasks0[j].sel == sel && taskctl->tasks0[j].flags != 0)
						{
							printf("1111\n");
							for (int k = 0; k < MAX_OPEN_FILE_NUM; k++)
							{
								printf("%d\n", k);
								if (taskctl->tasks0[j].fhandle[k] != NULL)
								{
									if (strcmp(taskctl->tasks0[j].fhandle[k]->file_node->name, filename) == 0)
									{
										printf("%d,%s\n", k, filename);
										taskctl->tasks0[j].fhandle[k] = 0;
										printf("nooooo\n");
									}
								}
								printf("blublu\n");
							}
							printf("yessss\n");
							break;
						}
					}
				}
			}
			else if (strcmp("check", buf) == 0)
			{
				check();
			}
			else if (buf[0] == 'c' && buf[1] == 'd')

			{

				op_cd(buf, pos);
			}

			else if (buf[0] == 't' && buf[1] == 'o' && buf[2] == 'u' && buf[3] == 'c' && buf[4] == 'h')

			{

				op_touch(buf, pos);
			}

			else if (buf[0] == 'd' && buf[1] == 'e' && buf[2] == 'l')

			{

				op_del(buf, pos);
			}

			else if (buf[0] == 'l' && buf[1] == 's')

			{

				op_ls();
			}

			else if (buf[0] == 'm' && buf[1] == 'k' && buf[2] == 'd' && buf[3] == 'i' && buf[4] == 'r')

			{

				op_mkdir(buf, pos);
			}

			else if (buf[0] == 'r' && buf[1] == 'm') //删除目录

			{

				op_rm(buf, pos);
			}

			else if (buf[0] == 'b' && buf[1] == 'a' && buf[2] == 'c' && buf[3] == 'k')

			{

				op_back();
			}

			else if (buf[0] == 'r' && buf[1] == 'e' && buf[2] == 'n' && buf[3] == 'a' && buf[4] == 'm' && buf[5] == 'e')

			{

				op_rename(buf, pos);
			}

			else if (buf[0] == 'm' && buf[1] == 'v')

			{

				op_mv(buf, pos);
			}

			else if (buf[0] == 'o' && buf[1] == 'p' && buf[2] == 'e' && buf[3] == 'n')

			{

				op_open(buf, pos);
			}

			else if (buf[0] == 'c' && buf[1] == 'l' && buf[2] == 'o' && buf[3] == 's' && buf[4] == 'e')

			{

				op_close(buf, pos);
			}

			else if (buf[0] == 'r' && buf[1] == 'e' && buf[2] == 'a' && buf[3] == 'd')

			{

				op_read(buf, pos);
			}

			else if (buf[0] == 'w' && buf[1] == 'r' && buf[2] == 'i' && buf[3] == 't' && buf[4] == 'e')

			{

				op_write(buf, pos);
			}

			else if (strcmp("checktable", buf) == 0)

			{

				op_check(buf, pos);
			}

			else if (buf[0] == 'f' && buf[1] == 'c' && buf[2] == 'b')

			{

				op_fcb(buf, pos);
			}

			else if (buf == '\r' || buf == '\n')

			{

				printf("%s:>#", path);
			}

			else

			{

				printf("命令错误\n");

				printf("%s:>#", path);
			}
			pos = 0;
		}
	}
}

void op_cd(char buf[], int len) //进入目录
{
	printf("%s:>#", path);
	printf("%s\n", buf);
	char name[10];
	char *s;
	int i = 0;
	int j;
	if (len == 0)
	{
		printf("error\n");
	}
	else
	{
		s = NULL;
		s = strchr(buf, ' ');
		if (s == NULL)
		{
			printf("命令错误，请加文件或目录名\n");
		}
		else
		{
			*s = '\0';
			strcpy(name, s + 1);
			cd(name);
		}
	}
}

void op_touch(char buf[], int len) //创建文件
{
	printf("%s:>#", path);
	printf("%s\n", buf);
	int fileSize;
	char command[20];
	char name[50];
	char *s, *s1;
	for (int i = 0; i <= len; i++)
		command[i] = buf[i];
	//printf("%s\n",command);
	if (len == 0)
	{
		printf("error\n");
	}
	else
	{
		s = NULL;
		s = strchr(command, ' ');
		//printf("%s\n",s);
		if (s != NULL)
		{
			*s = '\0';
		}
		else
		{
			printf("命令错误，请加文件或目录名\n");
		}
		s1 = strchr(s + 1, ' ');

		if (s1 != NULL)
		{
			*s1 = '\0';
			strcpy(name, s + 1);
			fileSize = atoi(s1 + 1);
			touch(name, fileSize, main_mem);
		}
		else
		{
			printf("命令错误，请加文件大小\n");
		}
	}
}

void op_del(char buf[], int len) //删除文件
{
	printf("%s:>#", path);
	printf("%s\n", buf);
	char command[20];
	char name[50];
	char *s;
	for (int i = 0; i <= len; i++)
		command[i] = buf[i];
	if (len == 0)
	{
		printf("error\n");
	}
	else
	{
		s = NULL;
		s = strchr(command, ' ');
		//printf("%s\n",s);
		if (s == NULL)
		{
			printf("命令错误，请加文件或目录名\n");
		}
		else
		{
			*s = '\0';
			strcpy(name, s + 1);
			del(name, main_mem);
		}
	}
}

void op_ls() //显示目录
{
	printf("%s:>#", path);
	printf("%s\n", buf);
	ls();
}

void op_mkdir(char buf[], int len) //创建目录
{
	printf("%s:>#", path);
	printf("%s\n", buf);
	char command[20];
	char name[50];
	char *s;
	for (int i = 0; i <= len; i++)
		command[i] = buf[i];
	if (len == 0)
	{
		printf("error\n");
	}
	else
	{
		s = NULL;
		s = strchr(command, ' ');
		//printf("%s\n",s);
		if (s == NULL)
		{
			printf("命令错误，请加文件或目录名\n");
		}
		else
		{
			*s = '\0';
			strcpy(name, s + 1);
			makedir(name, main_mem);
		}
	}
}

void op_rm(char buf[], int len) //删除目录
{
	printf("%s:>#", path);
	printf("%s\n", buf);
	char command[20];
	char name[50];
	char *s;
	for (int i = 0; i <= len; i++)
		command[i] = buf[i];
	if (len == 0)
	{
		printf("error\n");
	}
	else
	{
		s = NULL;
		s = strchr(command, ' ');
		//printf("%s\n",s);
		if (s == NULL)
		{
			printf("命令错误，请加文件或目录名\n");
		}
		else
		{
			*s = '\0';
			strcpy(name, s + 1);
			rm(name, main_mem);
		}
	}
}

void op_back() //回退上级
{
	printf("%s:>#", path);
	printf("%s\n", buf);
	back();
}

void op_rename(char buf[], int len) //重命名文件
{
	printf("%s:>#", path);
	printf("%s\n", buf);
	char command[20];
	char name[50];
	char newName[40];
	char *s, *s1;
	for (int i = 0; i <= len; i++)
		command[i] = buf[i];
	//printf("%s\n",command);
	if (len == 0)
	{
		printf("error\n");
	}
	else
	{
		s = NULL;
		s = strchr(command, ' ');
		//printf("%s\n",s);
		if (s != NULL)
		{
			*s = '\0';
		}
		else
		{
			printf("命令错误，请加目录名\n");
		}
		s1 = strchr(s + 1, ' ');
		if (s1 != NULL)
		{
			*s1 = '\0';
			strcpy(name, s + 1);
			strcpy(newName, s1 + 1);
			rename_file(name, newName);
		}
		else
		{
			printf("命令错误，请加新文件名\n");
		}
	}
}

void op_mv(char buf[], int len) //重命名目录
{
	printf("%s:>#", path);
	printf("%s\n", buf);
	char command[20];
	char name[50];
	char newName[40];
	char *s, *s1;
	for (int i = 0; i <= len; i++)
		command[i] = buf[i];
	//printf("%s\n",command);
	if (len == 0)
	{
		printf("error\n");
	}
	else
	{
		s = NULL;
		s = strchr(command, ' ');
		//printf("%s\n",s);
		if (s != NULL)
		{
			*s = '\0';
		}
		else
		{
			printf("命令错误，请加目录名\n");
		}
		s1 = strchr(s + 1, ' ');
		if (s1 != NULL)
		{
			*s1 = '\0';
			strcpy(name, s + 1);
			strcpy(newName, s1 + 1);
			mv(name, newName);
		}
		else
		{
			printf("命令错误，请加新文件夹名\n");
		}
	}
}

void op_open(char buf[], int len) //打开文件
{
	printf("%s:>#", path);
	printf("%s\n", buf);
	char command[20];
	char name[50];
	char *s;
	for (int i = 0; i <= len; i++)
		command[i] = buf[i];
	if (len == 0)
	{
		printf("error\n");
	}
	else
	{
		s = NULL;
		s = strchr(command, ' ');
		//printf("%s\n",s);
		if (s == NULL)
		{
			printf("命令错误，请加文件或目录名\n");
		}
		else
		{
			*s = '\0';
			strcpy(name, s + 1);
			openFile(name);
		}
	}
}

void op_close(char buf[], int len) //关闭文件
{
	printf("%s:>#", path);
	printf("%s\n", buf);
	char command[20];
	char name[50];
	char *s;
	for (int i = 0; i <= len; i++)
		command[i] = buf[i];
	if (len == 0)
	{
		printf("error\n");
	}
	else
	{
		s = NULL;
		s = strchr(command, ' ');
		//printf("%s\n",s);
		if (s == NULL)
		{
			printf("命令错误，请加文件或目录名\n");
		}
		else
		{
			*s = '\0';
			strcpy(name, s + 1);
			closeFile(name);
		}
	}
}

void op_read(char buf[], int len) //读文件
{
	printf("%s:>#", path);
	printf("%s\n", buf);
	char command[20];
	char name[50];
	char *s;
	for (int i = 0; i <= len; i++)
		command[i] = buf[i];
	if (len == 0)
	{
		printf("error\n");
	}
	else
	{
		s = NULL;
		s = strchr(command, ' ');
		//printf("%s\n",s);
		if (s == NULL)
		{
			printf("命令错误，请加文件或目录名\n");
		}
		else
		{
			*s = '\0';
			strcpy(name, s + 1);
			readFile(name);
		}
	}
}

void op_write(char buf[], int len) //写文件
{
	printf("%s:>#", path);
	printf("%s\n", buf);
	char command[20];
	char name[50];
	char *s;
	for (int i = 0; i <= len; i++)
		command[i] = buf[i];
	if (len == 0)
	{
		printf("error\n");
	}
	else
	{
		s = NULL;
		s = strchr(command, ' ');
		//printf("%s\n",s);
		if (s == NULL)
		{
			printf("命令错误，请加文件或目录名\n");
		}
		else
		{
			*s = '\0';
			strcpy(name, s + 1);
			writeFile(name);
		}
	}
}

void op_check(char buf[], int len) //查看文件打开列表
{
	printf("%s:>#", path);
	printf("%s\n", buf);
	printList();
}

void op_fcb(char buf[], int len) //查看文件FCB
{
	printf("%s:>#", path);
	printf("%s\n", buf);
	char command[20];
	char name[50];
	char *s;
	for (int i = 0; i <= len; i++)
		command[i] = buf[i];
	if (len == 0)
	{
		printf("error\n");
	}
	else
	{
		s = NULL;
		s = strchr(command, ' ');
		//printf("%s\n",s);
		if (s == NULL)
		{
			checkAllFCB();
		}
		else
		{
			strcpy(name, s + 1);
			checkFCB(name);
		}
	}
}

void op_none()
{
	printf("%s:>#", path);
	printf("%s\n", buf);
	printf("没有这个命令\n");
}

void *timer_start(void *param)
{
	while (true)
	{
		sleep(1);
		int_table[20].offset(0);
	}
}

void check()
{

	int i;

	for (i = 1; i <= LIMIT_GDT / 8 && i <= 20; i++)
	{

		printf("gdt_%d: limit_%d base_%x ar_%d\n", i, (gdt + i)->limit, (gdt + i)->base - (int)main_mem,
			   (gdt + i)->access_right);
	}

	for (i = 0; i < memman->frees; i++)
	{

		printf("memman_%d: addr_%x size_%x\n", i, memman->free[i].addr - main_mem, memman->free[i].size);
	}

	for (i = 0; i < MAX_TASKLEVELS; i++)
	{
		int count = 0;
		for (int j = 0; j < MAX_TASKS; j++)
		{
			if (taskctl->tasks0[j].level == i && taskctl->tasks0[j].flags == 1)
			{
				count++;
			}
		}
		printf("task_level_%d : running_%d sleep_%d\n", i, taskctl->level[i].running, count);
	}
	printf("now_task_level； %d , now_task: %d\n", taskctl->now_lv, taskctl->level[taskctl->now_lv].now);

	for (i = 0; i < MAX_TASKS; i++)
	{
		int count = 0;
		char fileNames[MAX_OPEN_FILE_NUM][50];
		if (taskctl->tasks0[i].flags != 0)
		{
			for (int j = 0; j < MAX_OPEN_FILE_NUM; j++)
			{
				if (taskctl->tasks0[i].fhandle[j] != 0)
					strcpy(fileNames[count++], taskctl->tasks0[i].fhandle[j]->file_node->name);
			}

			printf("task_%d open_file: ", taskctl->tasks0[i].sel);
			for (int j = 0; j < count; j++)
			{
				printf("%s ", fileNames[j]);
			}
			putchar('\n');
		}
	}
}

int main()
{

	shutdown = 0;
	fifo.size = 5;

	fifo.buf = (int *)malloc(sizeof(int) * 5);

	fifo.free = fifo.size;

	fifo.flags = 0;

	fifo.next_w = 0; /* 書き込み位置 */

	fifo.next_r = 0; /* 読み込み位置 */

	fifo.task = NULL; /* データが入ったときに起こすタスク */

	sem_init(&empty, 0, 0);

	sem_init(&full, 0, 0);

	main_mem = (char *)malloc(1024 * 1024);

	pthread_t producer;

	pthread_t consumer;

	pthread_t timer;

	pthread_attr_t attr;

	pthread_attr_init(&attr);

	pthread_mutex_init(&mutex, NULL);

	/* create producer threads */

	pthread_create(&producer, &attr, gain_inc, NULL);

	/* create consumer threads */

	pthread_create(&consumer, &attr, handle_inc, NULL);

	/* create timer threads */

	pthread_create(&timer, &attr, timer_start, NULL);

	while (true)
	{
		sleep(1);
		if (shutdown == 1)
		{
			break;
		}
	}

	return 0;
}
