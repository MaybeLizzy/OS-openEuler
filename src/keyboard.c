/* 键盘 */
#include "bootpack.h"

struct FIFO32 *keyfifo;
int keydata0;

void inthandler21(char c)
{
    int data;
    io_out8(PIC0_OCW2, 0x61); /* 通知PIC IRQ-01接受 */
    data = io_in8(c);
    fifo32_put(keyfifo, data + keydata0);
    return;
}

void init_keyboard(struct FIFO32 *fifo, int data0)
{
    /* 存储写目的地的FIFO缓冲区 */
    keyfifo = fifo;
    keydata0 = data0;
    return;
}
