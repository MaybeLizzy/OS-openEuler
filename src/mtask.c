/* マルチタスク関係 */

#include "bootpack.h"
#include <stdio.h>

struct TASKCTL *taskctl;
struct TIMER *task_timer;
sem_t mutex_task;
int tr_reg;
int HLT_INFO;

void task_b_main(struct TSS32 *tss, int sel, int priority)
{
    printf("task_b_%d: %d\n", sel, tss->eax);
    tss->eax += priority;
}

struct TASK *task_now(void)
{
    struct TASKLEVEL *tl = &taskctl->level[taskctl->now_lv];
    return tl->tasks[tl->now];
}

void task_add(struct TASK *task)
{
    struct TASKLEVEL *tl = &taskctl->level[task->level];
    tl->tasks[tl->running] = task;
    tl->running++;
    task->flags = 2; /* 動作中 */
    return;
}

void task_remove(struct TASK *task)
{
    int i;
    struct TASKLEVEL *tl = &taskctl->level[task->level];

    /* taskがどこにいるかを探す */
    for (i = 0; i < tl->running; i++)
    {
        if (tl->tasks[i] == task)
        {
            /* ここにいた */
            break;
        }
    }

    tl->running--;
    if (i < tl->now)
    {
        tl->now--; /* ずれるので、これもあわせておく */
    }
    if (tl->now >= tl->running)
    {
        /* nowがおかしな値になっていたら、修正する */
        tl->now = 0;
    }
    task->flags = 1; /* スリープ中 */

    /* ずらし */
    for (; i < tl->running; i++)
    {
        tl->tasks[i] = tl->tasks[i + 1];
    }

    return;
}

void task_switchsub()
{
    int i;
    HLT_INFO = 0;
    /* 一番上のレベルを探す */
    for (i = 0; i < MAX_TASKLEVELS; i++)
    {
        if (taskctl->level[i].running > 0)
        {
            break; /* 見つかった */
        }
    }
    taskctl->now_lv = i;
    taskctl->lv_change = 0;
    return;
}

void task_idle()
{
    if (HLT_INFO == 0)
    {
        printf("当前无任务执行，进入哨兵任务HLT...\n");
        HLT_INFO = 1;
    }
}

struct TASK *task_init(struct MEMMAN *memman, char *t)
{
    int i;
    HLT_INFO = 0;
    sem_init(&mutex_task, 0, 1);
    struct TASK *task, *idle;
    // taskctl = (struct TASKCTL *)memman_alloc_1k(memman, sizeof(struct TASKCTL));
    taskctl = (struct TASKCTL *)t;

    for (i = 0; i < MAX_TASKS; i++)
    {
        taskctl->tasks0[i].flags = 0;
        taskctl->tasks0[i].sel = TASK_GDT0 + i;
	for(int j=0;j<MAX_OPEN_FILE_NUM;j++)
	{
		taskctl->tasks0[i].fhandle[j] = 0;
	}
        set_segmdesc(gdt + TASK_GDT0 + i, sizeof(struct TSS32), (char *)&taskctl->tasks0[i].tss, AR_TSS32);
    }

    for (i = 0; i < MAX_TASKLEVELS; i++)
    {
        taskctl->level[i].running = 0;
        taskctl->level[i].now = 0;
    }

    task = task_alloc();
    task->flags = 2;    /* 動作中マーク */
    task->priority = 1; /* 0.02秒 */
    task->level = 1;    /* 最高レベル */
    task_add(task);
    task_switchsub(); /* レベル設定 */
    load_tr(task->sel);

    task_timer = timer_alloc();
    timer_set_time(task_timer, task->priority);

    idle = task_alloc();
    idle->tss.esp = memman_alloc_1k(memman, 1 * 1024) + 1 * 1024;
    idle->tss.eip = (int)&task_idle;
    idle->tss.es = 1 * 8;
    idle->tss.cs = 2 * 8;
    idle->tss.ss = 1 * 8;
    idle->tss.ds = 1 * 8;
    idle->tss.fs = 1 * 8;
    idle->tss.gs = 1 * 8;
    task_run(idle, MAX_TASKLEVELS - 1, 1);

    return task;
}

struct TASK *task_alloc(void)
{
    int i;
    struct TASK *task;
    for (i = 0; i < MAX_TASKS; i++)
    {
        if (taskctl->tasks0[i].flags == 0)
        {
            task = &taskctl->tasks0[i];
            task->flags = 1;               /* 使用中マーク */
            task->tss.eflags = 0x00000202; /* IF = 1; */
            task->tss.eax = 0;             /* とりあえず0にしておくことにする */
            task->tss.ecx = 0;
            task->tss.edx = 0;
            task->tss.ebx = 0;
            task->tss.ebp = 0;
            task->tss.esi = 0;
            task->tss.edi = 0;
            task->tss.es = 0;
            task->tss.ds = 0;
            task->tss.fs = 0;
            task->tss.gs = 0;
            task->tss.ldtr = 0;
            task->tss.iomap = 0x40000000;
            return task;
        }
    }
    return 0; /* もう全部使用中 */
}

void task_run(struct TASK *task, int level, int priority)
{
    if (level < 0)
    {
        level = task->level; /* 不改变水平 */
    }
    if (priority > 0)
    {
        task->priority = priority;
    }

    if (task->flags == 2 && task->level != level)
    {                      /* 動作中のレベルの変更 */
        task_remove(task); /* これを実行するとflagsは1になるので下のifも実行される */
    }
    if (task->flags != 2)
    {
        /* スリープから起こされる場合 */
        task->level = level;
        task_add(task);
    }

    taskctl->lv_change = 1; /* 次回タスクスイッチのときにレベルを見直す */
    return;
}

void task_sleep(struct TASK *task)
{
    struct TASK *now_task;
    if (task->flags == 2)
    {
        /* 動作中だったら */
        now_task = task_now();
        task_remove(task); /* これを実行するとflagsは1になる */
        if (task == now_task)
        {
            /* 自分自身のスリープだったので、タスクスイッチが必要 */
            task_switchsub();
            now_task = task_now(); /* 設定後での、「現在のタスク」を教えてもらう */
            farjmp(0, now_task->sel, now_task->priority);
        }
    }
    return;
}

void task_switch(void)
{
    struct TASKLEVEL *tl = &taskctl->level[taskctl->now_lv];
    struct TASK *new_task, *now_task = tl->tasks[tl->now];
    tl->now++;
    if (tl->now == tl->running)
    {
        tl->now = 0;
    }
    if (taskctl->lv_change != 0)
    {
        task_switchsub();
        tl = &taskctl->level[taskctl->now_lv];
    }
    new_task = tl->tasks[tl->now];
    timer_set_time(task_timer, new_task->priority);
    if (new_task != now_task)
    {
        farjmp(0, new_task->sel, new_task->priority);
    }
    else if (taskctl->now_lv != 1 && taskctl->now_lv < MAX_TASKS_LV)
    {
        int now = taskctl->level[taskctl->now_lv].now;
        struct TASK *task = taskctl->level[taskctl->now_lv].tasks[now];
        farjmp(0, task->sel, task->priority);
    }
    return;
}
