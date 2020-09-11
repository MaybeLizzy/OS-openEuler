#include "bootpack.h"

struct SEGMENT_DESCRIPTOR *gdt;
struct GATE_DESCRIPTOR *idt;

void init_gdtidt(struct SEGMENT_DESCRIPTOR *segment, struct GATE_DESCRIPTOR *gate)
{
    char *main_mem = (char *)gate;
    gdt = segment;
    idt = gate;
    int i;

    /* GDT初始化 */
    for (i = 0; i <= LIMIT_GDT / sizeof(*gdt); i++)
    {
        set_segmdesc(gdt + i, 0, main_mem, 0);
    }

    /* IDT初始化 */
    for (i = 0; i <= LIMIT_IDT / sizeof(*idt); i++)
    {
        set_gatedesc(idt + i, 0, 0, 0);
    }

    /* GDT设置 */
    set_segmdesc(gdt + 1, 0xfffff, main_mem, AR_DATA32_RW);
    set_segmdesc(gdt + 2, LIMIT_BOTPAK, main_mem + ADR_BOTPAK, AR_CODE32_ER);

    /* IDT设置 */
    set_gatedesc(idt + 20, asm_inthandler20, 2 * 8, AR_INTGATE32); /* 定时器 */
    set_gatedesc(idt + 21, asm_inthandler21, 2 * 8, AR_INTGATE32); /* 键盘 */
    return;
}

void set_segmdesc(struct SEGMENT_DESCRIPTOR *sd, int limit, char *base, int ar)
{
    sd->limit = limit;
    sd->base = base;
    sd->access_right = ar;
    return;
}

void set_gatedesc(struct GATE_DESCRIPTOR *gd, void (*offset)(char c), int selector, int ar)
{
    gd->offset = offset;
    gd->selector = selector;
    gd->access_right = ar;
    return;
}
