#include <semaphore.h>
#include <string.h>

/* main.c */
#define BUFF_SIZE 20

/* file.c*/
struct FCB //文件控制块
{
    char name[50]; //文件名
    char type[10];
    char reserve[10];
    char permissions;
    int size;    //如果是文件则表示文件的大小
    int clustno; //存放的起始磁盘数
    unsigned short time, date;
};

struct node
{
    char name[50];       //目录或文件的名字
    int type;            //0代表目录，1代表普通文件
    struct node *next;   //指向下一个兄弟结点的指针
    struct node *sub;    //指向第一个子结点的指针
    struct node *father; //指向父结点的指针
    struct FCB fcb;
};
typedef struct LNode //定义单链表节点类型
{
    //char data[20];
    struct node *file_node;
    int size;
    int count; //计数有多少进程打开了该文件
    struct LNode *next;
} LNode, *LinkList;
typedef struct _MemoryInfomation //磁盘
{
    int start;   //起始地址
    int Size;    //大小
    char status; //状态
} MEMINFO;
extern LinkList L;
extern char path[100];
void get_init();                                           //初始化根目录
struct node *init();                                       //初始化新结点
void cd(char dirName[]);                                   //进入某个目录
void touch(char fileName[], int fileSize, char *main_mem); //新建文件
void del(char fileName[], char *main_mem);                 //删除文件
void dir(struct node *p);                                  //显示本目录下所有兄弟目录和文件
void dirs(struct node *p, const char str[]);               //显示某个子目录下的文件
void ls();                                                 // 显示目录
void makedir(char dirName[], char *main_mem);              // 创建目录
void rm(char dirName[], char *main_mem);                   //删除目录
void back();                                               //退到上一层
void mv(char dirName[], char newName[]);                   //重命名目录
void rename_file(char fileName[], char newName[]);         //重命名文件
void readFile(char fileName[]);                            //读文件
void writeFile(char fileName[]);                           //写文件
void openFile(char fileName[]);                            //打开文件
void closeFile(char fileName[]);                           //关闭文件
void InitList(char *main_mem);                             //初始化文件打开链表
void ListInsert(struct node *pp, char *main_mem2);         //链表插入结点
int ListLength();                                          //获取链表长度
void printList();                                          //打印链表
void listDelete(char e[]);                                 //删除结点
int findList(char e[]);                                    //查找结点
int Disk(int request);                                     //分配磁盘空间
void checkFCB(char fileName[]);                            //创建文件FCB
void InitFCB(char fileName[]);                             //初始化FCB
void checkAllFCB();                                        //查看该目录下所有文件的FCB
void Disk_release(int number);                             //释放磁盘空间
void Display();
void InitMemList();

/* fifo32.h */
struct FIFO32
{
    int *buf;
    int next_r, next_w, size, free, flags;
    struct TASK *task;
};
void fifo32_init(struct FIFO32 *fifo, int size, int *buf, struct TASK *task);
int fifo32_put(struct FIFO32 *fifo, int data);
int fifo32_get(struct FIFO32 *fifo);
int fifo32_status(struct FIFO32 *fifo);
#define FLAGS_OVERRUN 0x0001

/* dsctbl.c */
struct SEGMENT_DESCRIPTOR
{
    int limit;
    char *base;
    int access_right;
};
struct GATE_DESCRIPTOR
{
    void (*offset)(char c);
    int selector;
    int access_right;
};
void init_gdtidt(struct SEGMENT_DESCRIPTOR *segment, struct GATE_DESCRIPTOR *gate);
void set_segmdesc(struct SEGMENT_DESCRIPTOR *sd, int limit, char *base, int ar);
void set_gatedesc(struct GATE_DESCRIPTOR *gd, void (*offset)(char c), int selector, int ar);
#define LIMIT_IDT 0x007ff
#define LIMIT_GDT 0x017ff
#define ADR_BOTPAK 0x02000
#define LIMIT_BOTPAK 0x00fff
#define AR_DATA32_RW 0x4092
#define AR_CODE32_ER 0x409a
#define AR_TSS32 0x0089
#define AR_INTGATE32 0x008e

/* memory.c */
#define MEMMAN_FREES 1024 /* これで約32KB */
#define MEMMAN_ADDR 0x003c0000
#define AR_DIRECTORY 0x02000
#define LIMIT_DIRECTPRY 0x02888 - 0x02000
#define AR_OPEN_FILE_TABLE 0x02889
#define LIMIT_OPEN_FILE_TABLE 0x02fff - 0x02889

struct FREEINFO
{ /* あき情報 */
    unsigned int size;
    char *addr;
};
struct MEMMAN
{ /* メモリ管理 */
    int frees, maxfrees, lostsize, losts, base;
    struct FREEINFO free[MEMMAN_FREES];
};
//extern char OpenFileTable_IsUsed[LIMIT_OPEN_FILE_TABLE/sizeof(struct LNode)];
//extern char Dir_IsUsed[LIMIT_DIRECTPRY/sizeof(struct node)];
unsigned int memtest(unsigned int start, unsigned int end);
void memman_init(struct MEMMAN *man);
unsigned int memman_total(struct MEMMAN *man);
char *memman_alloc(struct MEMMAN *man, unsigned int size);
int memman_free(struct MEMMAN *man, unsigned int addr, unsigned int size);
unsigned int memman_alloc_1k(struct MEMMAN *man, unsigned int size);
int memman_free_1k(struct MEMMAN *man, unsigned int addr, unsigned int size);

/* mtask.c */
#define MAX_TASKS 9 /* 最大タスク数 */
#define TASK_GDT0 3 /* TSSをGDTの何番から割り当てるのか */
#define MAX_TASKS_LV 10
#define MAX_TASKLEVELS 10
#define MAX_OPEN_FILE_NUM 3
struct TSS32
{
    int backlink, esp0, ss0, esp1, ss1, esp2, ss2, cr3;
    int eip, eflags, eax, ecx, edx, ebx, esp, ebp, esi, edi;
    int es, cs, ss, ds, fs, gs;
    int ldtr, iomap;
};
struct TASK
{
    int sel, flags; /* selはGDTの番号のこと */
    int level, priority;
    struct FIFO32 fifo;
    struct TSS32 tss;
    struct LNode *fhandle[MAX_OPEN_FILE_NUM]; //进程文件打开表
};
struct TASKLEVEL
{
    int running; /* 動作しているタスクの数 */
    int now;     /* 現在動作しているタスクがどれだか分かるようにするための変数 */
    struct TASK *tasks[MAX_TASKS_LV];
};
struct TASKCTL
{
    int now_lv;     /* 現在動作中のレベル */
    char lv_change; /* 次回タスクスイッチのときに、レベルも変えたほうがいいかどうか */
    struct TASKLEVEL level[MAX_TASKLEVELS];
    struct TASK tasks0[MAX_TASKS];
};
extern struct TIMER *task_timer;
extern struct SEGMENT_DESCRIPTOR *gdt;
struct TASK *task_now(void);
struct TASK *task_init(struct MEMMAN *memman, char *t);
struct TASK *task_alloc(void);
void task_run(struct TASK *task, int level, int priority);
void task_switch(void);
void task_idle();
void task_remove(struct TASK *task);
void task_sleep(struct TASK *task);
void task_b_main(struct TSS32 *tss, int sel, int priority);

/* int.c */
void init_pic(void);
#define PIC0_ICW1 0x0020
#define PIC0_OCW2 0x0020
#define PIC0_IMR 0x0021
#define PIC0_ICW2 0x0021
#define PIC0_ICW3 0x0021
#define PIC0_ICW4 0x0021
#define PIC1_ICW1 0x00a0
#define PIC1_OCW2 0x00a0
#define PIC1_IMR 0x00a1
#define PIC1_ICW2 0x00a1
#define PIC1_ICW3 0x00a1
#define PIC1_ICW4 0x00a1

/* assembly.c */
void io_out8(int port, int data);
char io_in8(char port);
void asm_inthandler20(char c);
void asm_inthandler21(char c);
void reserve();
void recover();
void protect();
void reprotect();
void farjmp(int eip, int cs, int priority);
void load_tr(int task_sel);
void io_load_eflags();
void io_cli();
void io_store_eflags();
extern int tr_reg;
extern struct TASKCTL *taskctl;

/* keyboard.c */
void inthandler21(char c);
void init_keyboard(struct FIFO32 *fifo, int data0);
#define PORT_KEYDAT 0x0060
#define PORT_KEYCMD 0x0064

/* timer.c */
#define MAX_TIMER 500
#define PIT_CTRL 0x0043
#define PIT_CNT0 0x0040
struct TIMER
{
    struct TIMER *next;
    unsigned int timeout, flags;
    struct FIFO32 *fifo;
    int data;
};
struct TIMERCTL
{
    unsigned int count, next;
    struct TIMER *t0;
    struct TIMER timers0[MAX_TIMER];
};
extern struct TIMERCTL timerctl;
extern sem_t mutex_task;
void init_pit(void);
struct TIMER *timer_alloc(void);
void timer_free(struct TIMER *timer);
void timer_init(struct TIMER *timer, struct FIFO32 *fifo, int data);
void timer_set_time(struct TIMER *timer, unsigned int timeout);
void inthandler20();
