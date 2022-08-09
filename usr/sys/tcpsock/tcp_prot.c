/* PCS specific */
#include "sys/types.h"
#include "sys/param.h"
#include "sys/sysmacros.h"
#include "sys/errno.h"
#include "sys/proc.h"
#include "sys/inode.h"
#include "sys/dir.h"
#include "sys/file.h"
#include "sys/reg.h"
#include "sys/signal.h"
#include "sys/user.h"

#define ON_UNIX 1
#include "sys/icc/unix_icc.h"
#include "tcpip/types.h"
#include "tcpip/buffers.h"
#include "tcpip/sockemu.h"
#include "tcpip/message.h"
#include "tcpip/protexit.h"
#include "sys/socket.h"
#include "netinet/in.h"

struct exit_struct exits[NR_EXITS];
int ffree_exit;

extern struct sock_info sockets[];


void cancel_user(sip)
register struct sock_info *sip;
{
    register short i, k;
    
    if (sip->so_firstexit == -1) return;
    
    i = k = sip->so_firstexit; 
    sip->so_firstexit = -1;
    while (exits[k].link != -1)
        i = exits[i].link;              /*is this correct? */
        
    exits[k].link = ffree_exit;
    ffree_exit = i;
}

short install_exit(sip, ex)
register struct sock_info *sip;
register struct exit_struct *ex;
{
    register short i, k, m;
    
    if (ffree_exit == -1) {
        u.u_error = ENOMEM;
        return -1;
    }

    i = ffree_exit;
    ffree_exit = exits[ffree_exit].link;
    exits[i].link = -1;
    exits[i].ip_or_lance = ex->ip_or_lance;
    exits[i].low_bound = ex->low_bound;
    exits[i].high_bound = ex->high_bound;
    exits[i].ident = ex->ident;
    
    m = -1;
    k = sip->so_firstexit;
    while (k != -1 && exits[k].low_bound < exits[i].high_bound) {
        m = k;
        k = exits[k].link;
    }
    exits[i].link = k;
    if (m == -1)
        sip->so_firstexit = i;
    else
        exits[m].link = i;
    return i;
}

boolean is_busy(ex1, ex2)
register struct exit_struct *ex1;
register struct exit_struct *ex2;
{
    if (ex1->ip_or_lance != ex2->ip_or_lance)
        return FALSE;
    
    if (ex1->low_bound > ex2->high_bound || ex1->high_bound < ex2->low_bound)
        return FALSE;
    
    return TRUE;
}

boolean prot_is_busy(ex)
register struct exit_struct *ex;
{
    register short i, k;

    for (i = 0; i < NR_OF_SOCKETS; i++) {
        if (sockets[i].so_state != SSTAT_UNUSED) {
            for (k = sockets[i].so_firstexit; k != -1; k = exits[k].link) {
                if (is_busy(ex, &exits[k]))
                    return TRUE;
            }
        }
    }
    return FALSE;
}
