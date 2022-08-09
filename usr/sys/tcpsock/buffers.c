/* PCS specific */
#include "sys/types.h"
#include "sys/param.h"
#include "sys/sysmacros.h"
#include "sys/systm.h"
#include "sys/icc/unix_icc.h"
#include "tcpip/buffers.h"

UNIX_MESSAGE *Ffree_UNIX_Msg;
UNIX_MESSAGE Message_Pool[NR_UNIX_MESSAGES];
int Free_UNIX_Msgs;
int first_MSG_call = 1;
struct kbuffer Kbuffer_Pool[16];
int Free_Kbuffers;
struct kbuffer *Ffree_Kbuffer;
int first_Kbuf_call = 1;
UNIX_MESSAGE *lastaccessedmsg;

UNIX_MESSAGE *Request_msg()
{
    register UNIX_MESSAGE *up;
    register int i;
    int s;
    
    if (first_MSG_call) {
        s = spldisk();
        for (i = 0; i< (NR_UNIX_MESSAGES-1); i++)
            Message_Pool[i].link = &Message_Pool[i+1];
        Message_Pool[NR_UNIX_MESSAGES-1].link = 0;
        Ffree_UNIX_Msg = Message_Pool;
        lastaccessedmsg = Ffree_UNIX_Msg;
        Free_UNIX_Msgs = NR_UNIX_MESSAGES;
        first_MSG_call = 0;
        splx(s);
    }

    check_msg(1);
    while (Free_UNIX_Msgs == 0) {
        tracew(0x3000);
        sleep((caddr_t)&Free_UNIX_Msgs, PPIPE);
        tracew(0x3001);
    }
    check_msg(2);
    s = spldisk();
    
    up = Ffree_UNIX_Msg;
    Ffree_UNIX_Msg = Ffree_UNIX_Msg->link;
    lastaccessedmsg = Ffree_UNIX_Msg;
    up->link = 0;
    up->flag = 0;
    Free_UNIX_Msgs--;
    
    tracel((int)up - 0x80000000);
    check_msg(10);
    splx(s);
    return up;
}

void Release_msg(up)
register UNIX_MESSAGE *up;
{
    register UNIX_MESSAGE *fp;
    register int i;
    int s;

    if (first_MSG_call) return;

    check_msg(3);
    tracel((int)up - 0x70000000);
    
    for (i=0; i < NR_UNIX_MESSAGES; i++) {
        if (up == &Message_Pool[i]) break;
    }       
    if (i == NR_UNIX_MESSAGES) {
        tracew(0x3002);
        return;
    }

    check_msg(4);
    s = spldisk();
    
    for (fp = Ffree_UNIX_Msg; fp != 0; fp = fp->link) {
        if (fp == up) break;
    }
    if (fp) {
        tracew(0x3003);
        splx(s);
        sleep((caddr_t)check_msg, PPIPE);
    }

    check_msg(5);
    up->link = Ffree_UNIX_Msg;
    Ffree_UNIX_Msg = up;
    lastaccessedmsg = Ffree_UNIX_Msg;
    if (Free_UNIX_Msgs++ == 0) {
        tracew(0x3004);
        wakeup((caddr_t)&Free_UNIX_Msgs);
    }
    check_msg(6);
    splx(s);
}

struct kbuffer *Req_kbuffer()
{
    register struct kbuffer *kp;
    register int i;
    int s;

    if (first_Kbuf_call) {
        s = spldisk();
        for (i = 0; i < (NR_KBUFFERS-1); i++)
            Kbuffer_Pool[i].link = &Kbuffer_Pool[i+1];
        Kbuffer_Pool[NR_KBUFFERS-1].link = 0;
        Ffree_Kbuffer = Kbuffer_Pool;
        Free_Kbuffers = NR_KBUFFERS;
        first_Kbuf_call = 0;
        splx(s);
    }
    
    while (Free_Kbuffers == 0) {
        tracew(0x4000);
        sleep((caddr_t)&Free_Kbuffers, PPIPE);
        tracew(0x4001);
    }
    
    s = spldisk();
    kp = Ffree_Kbuffer;
    Ffree_Kbuffer = Ffree_Kbuffer->link;
    kp->link = 0;
    Free_Kbuffers--;
    tracel((int)kp - 0x60000000);
    splx(s);
    return kp;
}

void Rel_kbuffer(kp)
register struct kbuffer *kp;
{
    register struct kbuffer *fp;
    register int i;
    int s;

    if (first_Kbuf_call) return;

    tracel((int)kp - 0x50000000);
    for (i = 0; i < NR_KBUFFERS; i++) {
        if (kp == &Kbuffer_Pool[i]) break;
    }

    if (i == NR_KBUFFERS) {
        tracew(0x4002);
        return;
    }

    s = spldisk();
    for (fp = Ffree_Kbuffer; fp != 0; fp = fp->link) {
        if (fp == kp) break;
    }
    splx(s);
    if (fp) {
        tracew(0x4003);
        return;
    }

    s = spldisk();
    kp->link = Ffree_Kbuffer;
    Ffree_Kbuffer = kp;
    if (Free_Kbuffers++ == 0) {
        wakeup((caddr_t)&Free_Kbuffers);
        tracew(0x4004);
    }
    splx(s);
}

/* some delay loop by j&j */
void jjwait()
{
    int i;
    for (i = 0; i < 10000; i++);
}

check_msg(arg)
short arg;
{
    if (Ffree_UNIX_Msg != lastaccessedmsg || Ffree_UNIX_Msg == 0) {
        tracew(arg + 0x5000);
        printf("\r--->\007\007\007check_msg reports Ffree=%08lx, last=%08lx at %d\n",
            Ffree_UNIX_Msg, lastaccessedmsg, arg);
        printf("    Nr msgs=%d\n", Free_UNIX_Msgs);
        sleep((caddr_t)check_msg, PPIPE);
    }
}
