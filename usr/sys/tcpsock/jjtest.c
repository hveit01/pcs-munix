/* PCS specific */
#include "tcpip/types.h"

#define TRACESIZE 4096
Uchar tracebuf[TRACESIZE];
int lasttrace = 0;

trace(arg)
Uchar arg;
{
    register int s = currpl();

    spldisk();
    if (lasttrace != TRACESIZE)
        tracebuf[lasttrace++] = arg;
    
    lasttrace %= TRACESIZE;
    splx(s);
}

tracew(arg)
Ushort arg;
{
    register Ushort *tp;
    register int s = currpl();
    
    spldisk();
    
    lasttrace = ((lasttrace+1) & 0xfffffffe) % TRACESIZE;
    tp = (Ushort*)&tracebuf[lasttrace];
    *tp = arg;
    lasttrace += sizeof(Ushort);
    
    lasttrace %= TRACESIZE;
    tp = (Ushort*)&tracebuf[lasttrace];
    *tp = 0;

    splx(s);
}

tracel(arg)
Ulong arg;
{
    register Ulong *tp;
    register int s = currpl();
    
    spldisk();

    lasttrace = ((lasttrace+1) & 0xfffffffe) % TRACESIZE;
    if (lasttrace >= (TRACESIZE-3))
        lasttrace = 0;

    tp = (Ulong*)&tracebuf[lasttrace];
    *tp = arg;
    lasttrace += sizeof(Ulong);
    lasttrace %= TRACESIZE;

    splx(s);
}
