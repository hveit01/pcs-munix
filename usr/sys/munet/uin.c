/* PCS specific */
static char *_Version = "@(#) RELEASE:  1.0  Jul 09 1986 /usr/sys/munet/uin.c ";

#include <sys/types.h>

/* extracts class A,B,C network parts from a 32 bit ip address */
int in_netof(ip)
ipnetaddr ip;
{
    return (ip & 0x80000000)==0 ?
        (ip & 0xff000000) >> 24 :       /* class A network */
        (((ip & 0xc0000000)==0x80000000) ?
            (ip & 0xffff0000) >> 16 :   /* class B network */
            (ip & 0xffffff00) >> 8);    /* class C network */
}
