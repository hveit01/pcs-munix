/*PCS modified */
/*
********************************************************************************
*                         Copyright (c) 1985 AT&T                              *
*                           All Rights Reserved                                *
*                                                                              *
*                                                                              *
*          THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T                 *
*        The copyright notice above does not evidence any actual               *
*        or intended publication of such source code.                          *
********************************************************************************
*/
/*  @(#)ttyslot.c   1.4 */
/*  @(#)ttyslot.c   1.2 */
/*LINTLIBRARY*/
/*
 * Return the number of the slot in the utmp file
 * corresponding to the current user: try for file 0, 1, 2.
 * Returns -1 if slot not found.
 */
#include <sys/types.h>
#include "utmp.h"
#define NULL    0
#define SXTDEV "/dev/sxt/"                      /*pcs*/

extern char *ttyname(), *strrchr();
extern int strncmp(), open(), read(), close();

int
ttyslot()
{
    struct utmp ubuf;
    register char *tp, *p;
    register int s, fd;

    if((tp=ttyname(0)) == NULL && (tp=ttyname(1)) == NULL &&
                    (tp=ttyname(2)) == NULL)
        return(-1);
        
    if (!strncmp(tp, SXTDEV, 9))                /*pcs*/
        p = tp+5;                               /*pcs*/
    else if((p=strrchr(tp, '/')) == NULL)
        p = tp;
    else
        p++;

    if((fd=open(UTMP_FILE, 0)) < 0)
        return(-1);
    s = 0;
    while(read(fd, (char*)&ubuf, sizeof(ubuf)) == sizeof(ubuf)) {
        if(    (ubuf.ut_type == INIT_PROCESS ||
            ubuf.ut_type == LOGIN_PROCESS ||
            ubuf.ut_type == USER_PROCESS ||
            ubuf.ut_type == DEAD_PROCESS ) &&
            strncmp(p, ubuf.ut_line, sizeof(ubuf.ut_line)) == 0){

            (void) close(fd);
            return(s);
        }
        s++;
    }
    (void) close(fd);
    return(-1);
}
