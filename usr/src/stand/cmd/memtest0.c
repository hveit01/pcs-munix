/*
 * these are some helpers for memtest 
 */

#define _ccr  ((short*)0x3ffe8006)

clratbcache()
{
    register short *ccrp = _ccr;
    register int ccr = *ccrp & 8;       /* preserve bit 3, clear 1,2 */
    *ccrp = ccr | 1;                    /* i.e. clear ATB, CACHE */
}

clrptrbr()
{
    register short *ccrp = _ccr;
    register int ccr = *ccrp & 8;       /* preserve bit 3, clear 0 */
    *ccrp = ccr | 0x06;                 /* clear PTR bit */
}

/* clear cache and set bit 3 , not used in code */
clrcache_sb3()
{
    register short *ccrp = _ccr;
    *ccrp = 0x0b;                       /* bit3=1, bit2=0, bit1=1,bit0=1 */
}

/* clear cache and clear bit 3 */
clrcache_cb3()
{
    register short *ccrp = _ccr;
    *ccrp = 0x03;                       /* bit3=0, bit2=0, bit1=1,bit0=1 */
}
