/* PCS specific */
static char* _Version = "@(#) RELEASE:  1.1  Aug 01 1986 /usr/sys/munet/mninfo.c ";

#include "sys/types.h"
#include "sys/param.h"
#include "sys/proc.h"
#include "sys/dir.h"
#include "sys/errno.h"
#include "sys/signal.h"
#include "sys/user.h"
#include "sys/var.h"
#include <sys/munet/mninfo.h>   /* declares freeminfos */
#include <sys/munet/mnport.h>

int init_minfos()
{
    register struct munetinfo *mip;
    register struct munetinfo *mnext = 0;
    register int i;

    mip = &m_info[v.v_proc-1];
    while (mip >= m_info) {             /* build linked list of MUNET info bufs */
        mip->mi_flags = INFO_FREE;      /* initialize all fields */
        for (i=0; i<UIMAXNODES; i++)
            mip->mi_remport[i] = 0;
        mip->mi_uport = 0;
        mip->mi_kport = 0;
        mip->mi_next = mnext;
        mnext = mip;
        mip--;
    }
    
    freeminfos = m_info;                /* head of freelist */
    return 0;
}

/* allocate new minfo buffer */
struct munetinfo *get_minfo()
{
    register struct munetinfo *mip;
    register short s;

    if (freeminfos == 0) {
        printf("out of free infos\n");
        return 0;
    }

    s = spldisk();

    mip = freeminfos;                   /* get head of freelist */
    freeminfos = mip->mi_next;          /* unlink from freelist */
    mip->mi_flags = INFO_USED;          /* mark buffer as used */

    splx(s);

    return mip;
}

int put_minfo(mip)
register struct munetinfo *mip;
{
    register short s;
    register int i;
    
    s = spldisk();
    
    mip->mi_flags = INFO_FREE;          /* mark buffer as free */
    for (i=0; i<UIMAXNODES; i++)
        mip->mi_remport[i] = 0;         /* clear port numbers */
    mip->mi_uport = 0;
    mip->mi_kport = 0;
    
    mip->mi_next = freeminfos;          /* put back into freelist */
    freeminfos = mip;

    splx(s);
    return 0;
}

/* clear user/kernel port 
 * num=1 -> clear user port
 * num=2 -> clear kernel port 
 */
int cl_info_pt(mip, num)
register struct munetinfo *mip;
register short num;
{
    register short s = spldisk();

    if (mip==0) {                       /* arg = 0? return error */
        splx(s);
        u.u_error = EINVAL;
        return -1;
    }

    switch (num) {
    case 1:
        mip->mi_uport = 0;              /* clear user level port */
        break;
    case 2:
        mip->mi_kport = 0;              /* clear kernel level port */
        break;
    default:
        splx(s);
        u.u_error = EINVAL;
        return -1;
    }
    
    splx(s);
    return 0;
}

ins_minfo(mip, idx, port)
register struct munetinfo *mip;
register short idx, port;
{
    if (mip==0) {                       /* invalid arg? return error */
        printf("ins_minfo: no such info\n");
        u.u_error = EINVAL;
        return -1;
    }
    mip->mi_remport[idx] = port;        /* set port. IDX not checked! */
    return 0;
}
