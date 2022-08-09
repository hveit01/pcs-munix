/* no ref to VAX Unix */
static char* _Version = "@(#) RELEASE:  1.0  Jul 09 1986 /usr/sys/os/rtc.c ";

#include "sys/types.h"
#include "sys/param.h"
#include "sys/sysmacros.h"
#include "sys/errno.h"

/*avoid include systm.h - defines a weak nlandev symbol */
extern time_t time;

/* entire file is pcs only */

extern char _mfp[];
#define MFP_PORT    1
#define MFP_CMD     5

#define MFP_READY   0x80
#define MFP_A       0x40
#define MFP_W       0x20
#define MFP_R       0x10
#define MFP_DMASK   0x0f

#define MFP_WPORT(val)\
    _mfp[MFP_PORT] = val;\
    _mfp[MFP_PORT] = val;\
    _mfp[MFP_PORT] = val
#define MFP_RPORT(reg)\
    reg = (unsigned char)_mfp[MFP_PORT];\
    reg = (unsigned char)_mfp[MFP_PORT];\
    reg = (unsigned char)_mfp[MFP_PORT]

#define MFP_WADDR(reg, addr)\
    reg = (addr & MFP_DMASK) | (      MFP_W|MFP_R);\
    MFP_WPORT(reg);\
    reg = (addr & MFP_DMASK) | (MFP_A|MFP_W|MFP_R);\
    MFP_WPORT(reg)
#define MFP_WDATA(reg, val)\
    reg = (val & MFP_DMASK)  | (MFP_A|      MFP_R);\
    MFP_WPORT(reg);\
    reg = (val & MFP_DMASK)  | (MFP_A|MFP_W|MFP_R);\
    MFP_WPORT(reg)
#define MFP_RDATA(reg)\
    _mfp[MFP_CMD] = 0x70;\
    _mfp[MFP_PORT] = (MFP_A|MFP_W);\
    MFP_RPORT(reg);\
    MFP_WPORT(MFP_A|MFP_W|MFP_R)

#define MFP_WAITRDY()\
    _mfp[MFP_CMD] = 0x70;\
    while ( (_mfp[MFP_PORT] & MFP_READY)==0);\
    _mfp[MFP_CMD] = 0x7f;

write_rtc(addr, val)
int addr, val;
{
    register uint reg;
    
    MFP_WAITRDY();
    MFP_WADDR(reg, addr);
    MFP_WDATA(reg, val);
}

read_rtc(addr)
int addr;
{ 
    register uint reg;
    
    MFP_WAITRDY();
    MFP_WADDR(reg, addr);
    MFP_RDATA(reg);
    return reg & 0xf;
} 

/* number of seconds per month */
static int MLENGTH[] = {
    31*86400l, 28*86400l, 31*86400l, 30*86400l, 31*86400l, 30*86400l, 
    31*86400l, 31*86400l, 30*86400l, 31*86400l, 30*86400l, 31*86400l
};
static long year_offset;
static time_t time_offset;

set_year()
{
    register int y;
    register time_t t = time;
    register time_t sec;

    year_offset = 0;
    time_offset = 0;

    for (y = 1970; time_offset <= t; y++) {
        sec = 86400l*365;
        if ((y % 4) == 0)
            sec += 86400l;
        time_offset += sec;
        year_offset++;
    }
    time_offset -= sec;
    year_offset--;
}

year_to_seconds(year)
{
    register long yr;
    register time_t n;
    register time_t secs;

    n = 0;
    for (yr=1970; year > 0; yr++, year--) {
        secs = 86400l * 365;
        if ((yr % 4) == 0)
            secs += 86400l;
        n += secs;
    }
    return n;
}

read_clock()
{ 
    register s;
    register time_t secs;
    
    int yr, mo, dy;
    int hr, mi, se;

    s = splhi();

    yr = read_rtc(12) * 10 + read_rtc(11);
    mo = read_rtc(10) * 10 + read_rtc(9);
    dy = read_rtc(8) * 10 + read_rtc(7);
    hr = (read_rtc(5) & 3) * 10 + read_rtc(4);
    mi = read_rtc(3) * 10 + read_rtc(2);
    se = read_rtc(1) * 10 + read_rtc(0);

    splx(s);

    yr -= 70; /* year read is 0-99 - base it to years since 1970 */
    secs = year_to_seconds(yr);
    if ((yr % 4)==2 && mo > 2)  /* yr = 2 is 1972, the first leap year */
        secs += 86400l;

    while (--mo > 0) {
        secs += MLENGTH[mo-1];
    }

    while (--dy > 0)
        secs += 86400l;

    while (hr-- > 0)
        secs += 3600;

    time = secs + mi*60 + se;
} 

write_clock()
{
    register s;
    register int secs;
    register int msec;
    
    int yr, mo, dy;
    int hr, mi, se;

    set_year();
    yr = year_offset + 70;

    secs = time;
    secs -= time_offset;

    if ((yr % 4)==0 && secs >= (MLENGTH[0]+MLENGTH[1]))
        secs -= 86400l;

    msec = mo = 0;
    while (secs >= msec)
        msec += MLENGTH[mo++];
    secs -= msec - MLENGTH[mo-1];

    dy = 1;
    while (secs >= 86400l) {
        dy++; 
        secs -= 86400l;
    }
    
    hr = 0;
    while (secs >= 3600) {
        hr++;
        secs -= 3600;
    }

    mi = secs / 60;
    se = secs % 60;

    s = splhi();

    write_rtc(12, yr / 10);
    write_rtc(11, yr % 10);
    write_rtc(10, mo / 10);
    write_rtc(9,  mo % 10);
    write_rtc(8,  dy / 10);
    write_rtc(7,  dy % 10);
    
    write_rtc(5,  hr / 10 + 8);
    write_rtc(4,  hr % 10);
    write_rtc(3,  mi / 10);
    write_rtc(2,  mi % 10);
    write_rtc(1,  se / 10);
    write_rtc(0,  se % 10);

    splx(s);
} 

touch_clock()
{
    read_rtc(0);
} 
