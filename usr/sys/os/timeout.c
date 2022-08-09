/* @(#)timeout.c    1.1 */
static char* _Version = "@(#) RELEASE:  1.1  Mar 23 1987 /usr/sys/os/timeout.c ";

#include "sys/param.h"
#include "sys/types.h"
#include "sys/systm.h"
#include "sys/sysinfo.h"
#include "sys/callo.h"
#include "sys/dir.h"
#include "sys/signal.h"
#include "sys/user.h"
#include "sys/page.h"
#include "sys/region.h"
#include "sys/proc.h"
#include "sys/psl.h"
#include "sys/var.h"

int timeid;

/*
 * timeout is called to arrange that fun(arg) is called in tim/HZ seconds.
 * An entry is sorted into the callout structure.
 * The time in each structure entry is the number of HZ's more
 * than the previous entry. In this way, decrementing the
 * first entry has the effect of updating all entries.
 *
 * The panic is there because there is nothing
 * intelligent to be done if an entry won't fit.
 */
timeout(fun, arg, tim)
int (*fun)();
caddr_t arg;
{
    register struct callo *p1, *p2;
    register int t;
    register int id;
    int s;
    
    t = tim;
    if ( t <= 0)
        whereami("timeout time <= 0 : %X %X %d\n", fun, arg, tim);
/* loc_2E: */
    p1 = &callout[0];
    s = splhi();
    
    if (callout[v.v_call-2].c_func)
        panic("Timeout table overflow");
/* loc_60: */
    while(p1->c_func != 0 && p1->c_time <= t) {
        t -= p1->c_time;
        p1++;
/* loc_62: */
/* loc_6A: */
    }
/* loc_74: */
    p1->c_time -= t;
    p2 = p1;
    while(p2->c_func != 0)
/* loc_7A: */
        p2++;
/* loc_80: */
    while(p2 >= p1) {
        (p2+1)->c_time = p2->c_time;
        (p2+1)->c_func = p2->c_func;
        (p2+1)->c_arg = p2->c_arg;
        (p2+1)->c_id = p2->c_id;
        p2--;
    }
    p1->c_time = t;
    p1->c_func = fun;
    p1->c_arg = arg;
    id = ++timeid;
    p1->c_id = id;
    splx(s);

    return id;
}

timein()
{
    register struct callo *p1, *p2;

    if (callout[0].c_func == NULL)
        return;
    if (callout[0].c_time <= 0) {
        p1 = &callout[0];
        while(p1->c_func != 0 && p1->c_time <= 0) {
            (*p1->c_func)(p1->c_arg);
            p1++;
        }
        p2 = &callout[0];
        while(p2->c_func = p1->c_func) {
            p2->c_time = p1->c_time;
            p2->c_arg = p1->c_arg;
            p2->c_id = p1->c_id;
            p1++;
            p2++;
        }
    }
}

#define PDELAY  (PZERO-1)
delay(ticks)
{
    int s;
    
    extern wakeup();

    if (ticks<=0)
        return;
    s = splhi();
    timeout(wakeup, (caddr_t)u.u_procp+1, ticks);
    sleep((caddr_t)u.u_procp+1, PDELAY);
    splx(s);
}

/*pcs*/
cancelto(fun, arg)
int (*fun)();
caddr_t arg;
{
    register struct callo *p1, *p2;
    register int t;
    int s;

    t = 0;
    s = splhi();

    for (p1 = &callout[0]; p1 < &callout[v.v_call-1]; p1++) {
        t += p1->c_time;
        if (p1->c_func == fun && p1->c_arg == arg) {
            p2 = p1++;
            p1->c_time += p2->c_time;
            while ((p2->c_func = p1->c_func) != 0) {
                p2->c_time = p1->c_time;
                p2->c_arg = p1->c_arg;
                p2->c_id = p1->c_id;
                p1++;
                p2++;
            }
            splx(s);
            return t;
        }
    }
    splx(s);
    return 0;
}

/*pcs*/
untimeout(id)
register int id;
{
    register struct callo *p1, *p2;
    register int s;
    register int t;
    
    t = 0;
    s = splhi();

    for (p1 = &callout[0]; p1 < &callout[v.v_call-1]; p1++) {
/* loc_270: */
        t += p1->c_time;
        if (p1->c_id == id) {
            p2 = p1++; 
            p1->c_time += p2->c_time;
            while ((p2->c_func = p1->c_func) != 0) {
/* loc_288: */
                p2->c_time = p1->c_time;
                p2->c_arg = p1->c_arg;
                p2->c_id = p1->c_id;
                p1++;
                p2++;
/* loc_2A2: */
            }
            splx(s);
            return t;
        }
/* loc_2B8: */
/* loc_2BE: */
    }
    splx(s);
    return 0;
} 
