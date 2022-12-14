/*PCS modified*/
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
/*  @(#)clock.c 1.1 */
/*LINTLIBRARY*/

#include <sys/types.h>
#include <sys/times.h>
#include <sys/param.h>  /* for HZ (clock frequency in Hz) */
#define TIMES(B)    (B.tms_utime+B.tms_stime+B.tms_cutime+B.tms_cstime)

extern long times();
static long first = 0L;
static char firstcall = 0;                              /*pcs*/
extern int hertz();                                     /*pcs*/

long
clock()
{
    struct tms buffer;
    register long t;                                        

    if (times(&buffer) == -1L)                          /*pcs*/
        return -1;
    
    t = (TIMES(buffer) - first) * (1000000L/hertz());   /*pcs*/
    if (firstcall == 0) {                               /*pcs*/
        first = TIMES(buffer);                          /*pcs*/
        firstcall++;                                    /*pcs*/
    }                                                   /*pcs*/
    return t;                                           /*pcs*/
}
