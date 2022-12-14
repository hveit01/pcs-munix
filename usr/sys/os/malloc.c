/* @(#)malloc.c 6.3 */
static char* _Version = "@(#) RELEASE:  1.0  Jul 09 1986 /usr/sys/os/malloc.c ";

#include "sys/param.h"
#include "sys/types.h"
#include "sys/systm.h"
#include "sys/map.h"
#include "sys/page.h"

/* Masktab was formerly in mem.h;
 * It was moved to prevent multiple def'n errors.
 * (Previously only malloc.c #included mem.h)
 */
int masktab[] = {
    0x1,
    0x2,
    0x4,
    0x8,
    0x10,
    0x20,
    0x40,
    0x80,
    0x100,
    0x200,
    0x400,
    0x800,
    0x1000,
    0x2000,
    0x4000,
    0x8000,
    0x10000,
    0x20000,
    0x40000,
    0x80000,
    0x100000,
    0x200000,
    0x400000,
    0x800000,
    0x1000000,
    0x2000000,
    0x4000000,
    0x8000000,
    0x10000000,
    0x20000000,
    0x40000000,
    0x80000000,
};

/*
 * Allocate 'size' units from the given map.
 * Return the base of the allocated space.
 * In a map, the addresses are increasing and the
 * list is terminated by a 0 size.
 * The swap map unit is 512 bytes.
 * Algorithm is first-fit.
 */
unsigned int malloc(mp, size)
struct map *mp;
{
    register unsigned int a;
    register struct map *bp;

    for (bp = mapstart(mp); bp->m_size; bp++) {
        if (bp->m_size >= size) {
            a = bp->m_addr;
            bp->m_addr += size;
            if ((bp->m_size -= size) == 0) {
                do {
                    bp++;
                    (bp-1)->m_addr = bp->m_addr;
                } while ((bp-1)->m_size = bp->m_size);
                mapsize(mp)++;
            }
            return(a);
        }
    }
    return(0);
}

/*
 * Free the previously allocated space aa
 * of size units into the specified map.
 * Sort aa into map and combine on
 * one or both ends if possible.
 */
mfree(mp, size, a)
struct map *mp;
register unsigned int a;
{
    register struct map *bp;
    register unsigned int t;

    bp = mapstart(mp);
    for (; bp->m_addr<=a && bp->m_size!=0; bp++);
    if (bp>mapstart(mp) && (bp-1)->m_addr+(bp-1)->m_size == a) {
        (bp-1)->m_size += size;
        if (a+size == bp->m_addr) {
            (bp-1)->m_size += bp->m_size;
            while (bp->m_size) {
                bp++;
                (bp-1)->m_addr = bp->m_addr;
                (bp-1)->m_size = bp->m_size;
            }
            mapsize(mp)++;
        }
    } else {
        if (a+size == bp->m_addr && bp->m_size) {
            bp->m_addr -= size;
            bp->m_size += size;
        } else if (size) {
            if (mapsize(mp) == 0) {
                printf("\nDANGER: mfree map overflow %x\n", mp);
                printf("  lost %d items at %d\n", size, a);
                return;
            }
            do {
                t = bp->m_addr;
                bp->m_addr = a;
                a = t;
                t = bp->m_size;
                bp->m_size = size;
                bp++;
            } while (size = t);
            mapsize(mp)--;
        }
    }
    if (mapwant(mp)) {
        mapwant(mp) = 0;
        wakeup((caddr_t)mp);
    }
}

/********************/ 
/* public _malloc_at */ 
malloc_at(mp, addr, size)
struct map *mp;
register unsigned long size, addr;
{
    register struct map *bp;
    
    if (size == 0)
        return 0;
    for (bp = mapstart(mp); bp->m_size; bp++) {
        if (addr < bp->m_addr) 
            break;
            
        if (addr + size <= bp->m_addr + bp->m_size) {
            if (bp->m_addr == addr) {
                bp->m_addr += size;
                if ((bp->m_size -= size) == 0) {                        
                    do {
                        bp[-1].m_addr = (++bp)->m_addr;
                    } while ((bp[-1].m_size = bp->m_size) != 0);
                    mp->m_size++;
                }
                return 1;
            }
            
            if (addr + size != bp->m_addr + bp->m_size) {
                register struct map* tmpp;

                tmpp = bp;
                do {
                    tmpp++;
                } while (tmpp->m_size);
                tmpp[1].m_size = 0;
                do {
                    tmpp->m_addr = tmpp[-1].m_addr;
                    tmpp->m_size = tmpp[-1].m_size;
                } while (--tmpp != bp);
                if (mp->m_size == 0)
                    panic("malloc_at");
                mp->m_size--;
                (++tmpp)->m_size = bp->m_size;
                bp->m_size = addr - bp->m_addr;
                tmpp->m_addr = addr + size;
                tmpp->m_size -= size + bp->m_size;
            } else
                bp->m_size = addr - bp->m_addr;
            return 1;
        }
    }
    return 0;
} 
