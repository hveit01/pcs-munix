/* PCS */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/munet/munet.h>
#include <sys/munet/uisw.h>
#include <sys/munet/uport.h>
#include <sys/munet/uisw.h>

static char *_Version = "@(#) 1.0  Apr 08 1987 /usr/src/munet32/libmunet/isreach.c";

int isreachable(node)
ipnetaddr node;
{
    struct ndinfo {
        ipnetaddr node;
        time_t seen;
    } arg;
    
    arg.node = node;
    
    if (uidoit(&arg, UINDINFO) != 0)
        return 0;
    
    if (arg.seen)
        return 1;
    else
        return 0;
}
