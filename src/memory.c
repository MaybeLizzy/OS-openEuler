/* 内存相关 */

#include "bootpack.h"

char IsUsed[LIMIT_DIRECTPRY/sizeof(struct node)];

unsigned int memtest(unsigned int start, unsigned int end)
{
    return end;
}

void memman_init(struct MEMMAN *man)
{
    man->frees = 0;    /* 公开信息数 */
    man->maxfrees = 0; /* 观察情况：最大释放量 */
    man->lostsize = 0; /* 未能发布的总大小 */
    man->losts = 0;    /* 释放失败的次数 */
    return;
}

unsigned int memman_total(struct MEMMAN *man)
/* あきサイズの合計を報告 */
{
    unsigned int i, t = 0;
    for (i = 0; i < man->frees; i++)
    {
        t += man->free[i].size;
    }
    return t;
}

char *memman_alloc(struct MEMMAN *man, unsigned int size)
/* 確保 */
{
    unsigned int i;
    char *a;

    for (i = 0; i < man->frees; i++)
    {
        if (man->free[i].size >= size)
        {
            /* 十分な広さのあきを発見 */
            a = man->free[i].addr;
            man->free[i].addr += size;
            man->free[i].size -= size;
            if (man->free[i].size == 0)
            {
                /* free[i]がなくなったので前へつめる */
                man->frees--;
                for (; i < man->frees; i++)
                {
                    man->free[i] = man->free[i + 1]; /* 構造体の代入 */
                }
            }
            return a;
        }
    }
    return 0; /* あきがない */
}

int memman_free(struct MEMMAN *man, unsigned int addr, unsigned int size)
/* 解放 */
{
    int i, j;
    /* まとめやすさを考えると、free[]がaddr順に並んでいるほうがいい */
    /* だからまず、どこに入れるべきかを決める */
    for (i = 0; i < man->frees; i++)
    {
        if (man->free[i].addr > addr)
        {
            break;
        }
    }
    /* free[i - 1].addr < addr < free[i].addr */
    if (i > 0)
    {
        /* 前がある */
        if (man->free[i - 1].addr + man->free[i - 1].size == addr)
        {
            /* 前のあき領域にまとめられる */
            man->free[i - 1].size += size;
            if (i < man->frees)
            {
                /* 後ろもある */
                if (addr + size == man->free[i].addr)
                {
                    /* なんと後ろともまとめられる */
                    man->free[i - 1].size += man->free[i].size;
                    /* man->free[i]の削除 */
                    /* free[i]がなくなったので前へつめる */
                    man->frees--;
                    for (; i < man->frees; i++)
                    {
                        man->free[i] = man->free[i + 1]; /* 構造体の代入 */
                    }
                }
            }
            return 0; /* 成功終了 */
        }
    }
    /* 前とはまとめられなかった */
    if (i < man->frees)
    {
        /* 後ろがある */
        if (addr + size == man->free[i].addr)
        {
            /* 後ろとはまとめられる */
            man->free[i].addr = addr;
            man->free[i].size += size;
            return 0; /* 成功終了 */
        }
    }
    /* 前にも後ろにもまとめられない */
    if (man->frees < MEMMAN_FREES)
    {
        /* free[i]より後ろを、後ろへずらして、すきまを作る */
        for (j = man->frees; j > i; j--)
        {
            man->free[j] = man->free[j - 1];
        }
        man->frees++;
        if (man->maxfrees < man->frees)
        {
            man->maxfrees = man->frees; /* 最大値を更新 */
        }
        man->free[i].addr = addr;
        man->free[i].size = size;
        return 0; /* 成功終了 */
    }
    /* 後ろにずらせなかった */
    man->losts++;
    man->lostsize += size;
    return -1; /* 失敗終了 */
}

unsigned int memman_alloc_1k(struct MEMMAN *man, unsigned int size)
{
    unsigned int a;
    size = (size + 0x3ff) & 0xffc00;
    a = memman_alloc(man, size);
    return a;
}

int memman_free_1k(struct MEMMAN *man, unsigned int addr, unsigned int size)
{
    int i;
    size = (size + 0x3ff) & 0xffc00;
    i = memman_free(man, addr, size);
    return i;
}
