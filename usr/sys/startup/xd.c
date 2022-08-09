/*PCS specific*/

#include <sys/types.h>

static char *_Version = "@(#) RELEASE:  2.0  Sep 09 1986 /usr/sys/startup/xd.c ";

extern int tbf[];
static char *buf = (char*)&tbf[0];
static printx(), dump();

#define BLOCKSZ 1024

xd()
{
    int bno;
    int fd;

    for (;;) {
        printf("enter device name, empty line means end of function: ");
        gets(buf);
        if (buf[0] == '\0') return;

        if ((fd = sa_open(buf, 0)) >= 0) break;
        printf("cannot open %s \n", buf);
    }

    for (;;) {
        printf("please enter bno:");
        gets(buf);
        if (buf[0] == '\0') {
            sa_close(fd);
            return;
        }
        
        bno = atol(buf);
        if (bno < 0) break;

        sa_lseek(fd, bno * BLOCKSZ, 0);
        if (sa_read(fd, buf, BLOCKSZ) < 0)
            printf("cannot read block %D\n", bno);
        else {
            printf("\ndump of block %D\n", bno);
            dump(buf, BLOCKSZ);
        }
    }
}

static printx(data, sz)
int data, sz;
{
    int i, val;

    for (i = sz - 1; i >= 0; i--) {
        val = (data >> (i * 4)) & 0x0f;
        if (val < 10)
            putchar(val + '0');
        else
            putchar(val + '7');
    }
}

static dump(bp)
char *bp;
{
    int row, col;
    int val;
    char *addr;
    
    addr = bp;
    for (row = 0; row < 64; row++) {
        val = row * 16;
        printx(val, 4);
        printf("  ");
        for (col = 0; col < 16; col++) {
            val = *addr++;
            printx(val, 2);
        }
        printf("  ");
        addr -= 16;
        for (col = 0; col < 16; col++) {
            val = *addr++;
            if (val < ' ' || val == 0x7f)
                putchar(' ');
            else
                putchar(val);
        }
        printf("\n");
    }
    printf("\n");
}
