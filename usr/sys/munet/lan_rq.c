/* PCS specific */
static char *_Version = "@(#) RELEASE:  1.2  Feb 05 1987 /usr/sys/munet/lan_rq.c ";

#include "sys/types.h"
#include "sys/param.h"
#include "sys/sysmacros.h"
#include "rbq.h"

/* append a rbq at the end of a queue */
rq_append(que, rq)
register struct rbq **que;
register struct rbq *rq;
{
    register struct rbq *qq;

    rq->r_next = 0;                     /* no successor */
    if (*que) {
        for (qq = *que; qq->r_next; qq = qq->r_next); /*advance to end of list */
        qq->r_next = rq;                /* append at end */
    } else
        *que = rq;                      /* becomes new head */
}

/* release head of queue */
rq_relfirst(que)
register struct rbq **que;
{
    register ushort id;

    if (*que) {                         /* queue not empty? */
        id = (*que)->r_bufid;
        *que = (*que)->r_next;          /* unlink buffer */
        buf_rel(id);                    /* release this buffer */
    }
}

/* release all buffers */
rq_relall(que)
register struct rbq **que;
{
    register ushort id;
    while (*que) {                      /* while not empty */
        id = (*que)->r_bufid;           
        *que = (*que)->r_next;          /* to next element */
        buf_rel(id);
    }
}
