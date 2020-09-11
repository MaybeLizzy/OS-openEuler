/* 计时器相关 */

#include "bootpack.h"
#include <pthread.h>

struct TIMERCTL timerctl;
#define TIMER_FLAGS_ALLOC 1 /* 安全状态 */
#define TIMER_FLAGS_USING 2 /* 计时器正在运行 */

void init_pit(void)
{
    int i;
    struct TIMER *t;
    io_out8(PIT_CTRL, 0x34);

    timerctl.count = 0;
    for (i = 0; i < MAX_TIMER; i++)
    {
        timerctl.timers0[i].flags = 0; /* 未使用 */
    }
    t = timer_alloc(); /* 我会得到一个 */
    t->timeout = 0xffffffff;
    t->flags = TIMER_FLAGS_USING;
    t->next = 0;                /* 最后 */
    timerctl.t0 = t;            /* 现在只有哨兵 */
    timerctl.next = 0xffffffff; /* 由于只有哨兵，所以哨兵的时间 */
    return;
}

struct TIMER *timer_alloc(void)
{
    int i;
    for (i = 0; i < MAX_TIMER; i++)
    {
        if (timerctl.timers0[i].flags == 0)
        {
            timerctl.timers0[i].flags = TIMER_FLAGS_ALLOC;
            return &timerctl.timers0[i];
        }
    }
    return 0; /* 見つからなかった */
}

void timer_free(struct TIMER *timer)
{
    timer->flags = 0; /* 未使用 */
    return;
}

void timer_init(struct TIMER *timer, struct FIFO32 *fifo, int data)
{
    timer->fifo = fifo;
    timer->data = data;
    return;
}

void timer_set_time(struct TIMER *timer, unsigned int timeout)
{
    int e;
    struct TIMER *t, *s;
    timer->timeout = timeout + timerctl.count;
    timer->flags = TIMER_FLAGS_USING;
    io_load_eflags();
    io_cli();
    t = timerctl.t0;
    if (timer->timeout <= t->timeout)
    {
        /* 在开始时 */
        timerctl.t0 = timer;
        timer->next = t; /* 接下来是t */
        timerctl.next = timer->timeout;
        io_store_eflags();
        sem_post(&mutex_task);
        return;
    }
    /* 找到放在哪里 */
    for (;;)
    {
        s = t;
        t = t->next;
        if (timer->timeout <= t->timeout)
        {
            /* 在s和t之间插入时 */
            s->next = timer; /* s旁边是计时器 */
            timer->next = t; /* 计时器旁边是t */
            io_store_eflags();
            return;
        }
    }
}

void inthandler20()
{
    struct TIMER *timer;
    char ts = 0;
    io_out8(PIC0_OCW2, 0x60); /* IRQ-00受付完了をPICに通知 */

    timerctl.count++;

    if (timerctl.next > timerctl.count)
    {
        return;
    }
    timer = timerctl.t0; /* 暂时将第一个地址分配给计时器 */
    for (;;)
    {
        /* 不要检查标志，因为计时器中的所有计时器都在运行 */
        if (timerctl.count < timer->timeout)
        {
            break;
        }
        /* 超时 */
        timer->flags = TIMER_FLAGS_ALLOC;

        if (timer != task_timer)
        {
            sem_wait(&mutex_task);
            fifo32_put(timer->fifo, timer->data);
            sem_post(&mutex_task);
        }
        else
        {
            ts = 1; /* task_timer已超时 */
        }
        timer = timer->next; /* 为定时器分配下一个定时器地址 */
    }
    timerctl.t0 = timer;
    timerctl.next = timer->timeout;
    if (ts != 0)
    {
        task_switch();
    }
    return;
}
