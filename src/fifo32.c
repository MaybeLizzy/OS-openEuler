/* FIFO库 */
#include "bootpack.h"

void fifo32_init(struct FIFO32 *fifo, int size, int *buf, struct TASK *task)
/* FIFO缓冲区初始化 */
{
    fifo->size = size;
    fifo->buf = buf;
    fifo->free = size; /* 空的 */
    fifo->flags = 0;
    fifo->next_w = 0;  /* 写位置 */
    fifo->next_r = 0;  /* 读取位置 */
    fifo->task = task; /* 输入数据时要触发的任务 */
    return;
}

int fifo32_put(struct FIFO32 *fifo, int data)
/* 将数据发送到FIFO并存储 */
{
    if (fifo->free == 0)
    {
        /* 没有空间，它溢出了 */
        fifo->flags |= FLAGS_OVERRUN;
        return -1;
    }
    fifo->buf[fifo->next_w] = data;
    fifo->next_w++;
    if (fifo->next_w == fifo->size)
    {
        fifo->next_w = 0;
    }
    fifo->free--;

    if (fifo->task != 0)
    {
        if (fifo->task->flags != 2)
        {                                /* 如果任务正在休眠 */
            task_run(fifo->task, -1, 0); /* 我会叫醒你 */
            printf("task(%x)被唤醒...\n", fifo->task);
        }
    }

    return 0;
}

int fifo32_get(struct FIFO32 *fifo)
/* 从FIFO中获取一个数据 */
{
    int data;
    if (fifo->free == fifo->size)
    {
        /* 如果缓冲区为空，暂时返回-1 */
        return -1;
    }
    data = fifo->buf[fifo->next_r];
    fifo->next_r++;
    if (fifo->next_r == fifo->size)
    {
        fifo->next_r = 0;
    }
    fifo->free++;
    return data;
}

int fifo32_status(struct FIFO32 *fifo)
/* 报告有多少数据 */
{
    return fifo->size - fifo->free;
}
