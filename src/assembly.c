#include "bootpack.h"
#include <stdio.h>

void io_out8(int port, int data)
{
    if (port == PIC0_OCW2 && data == 61)
    {
        printf("键盘中断已接收，正在进行处理...\n");
    }
    if (port == PIC0_OCW2 && data == 60)
    {
        printf("定时器中断已接收，正在进行处理...\n");
    }

    if (port == PIT_CTRL && data == 0x34)
    {
        printf("定时器基础设置已设定...\n");
    }

    return;
}

char io_in8(char port)
/* port应为OS取数据的端口，模拟时直接将数据作为形参port */
{
    printf("已从中断处，取得相关数据信息...\n");
    return port;
}

void farjmp(int eip, int cs, int priority)
{
    struct TSS32 *tss = (struct TSS32 *)((gdt + cs)->base);
    if (taskctl->now_lv != MAX_TASKLEVELS - 1)
    {
        task_b_main(tss, cs, priority);
    }
    else
    {
        task_idle();
    }
}

void load_tr(int task_sel)
{
    tr_reg = task_sel;
}

void asm_inthandler21(char c)
{
    // reserve();

    // protect();

    inthandler21(c);

    // reprotect();

    // recover();
}

void asm_inthandler20(char c)
{
    // reserve();

    // protect();

    inthandler20();

    // reprotect();

    // recover();
}

void io_load_eflags()
{
    // printf("EFLAGS寄存器已存储...\n");
}

void io_store_eflags()
{
    // printf("EFLAGS寄存器已恢复...\n");
}

void reserve()
{
    printf("正在保存现场...\n");
}

void protect()
{
    printf("正在进入内核态...\n");
}

void recover()
{
    printf("正在恢复现场...\n");
}

void reprotect()
{
    printf("正在进入用户态...\n");
}

void io_cli()
{
    // printf("已关闭中断...\n");
}