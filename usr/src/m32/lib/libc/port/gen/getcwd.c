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
/*  @(#)getcwd.c    1.2 */
/*LINTLIBRARY*/
/*
 * Library routine to GET the Current Working Directory.
 * arg1 is a pointer to a character buffer into which the
 * path name of the current directory is placed by the
 * subroutine.  arg1 may be zero, in which case the 
 * subroutine will call malloc to get the required space.
 * arg2 is the length of the buffer space for the path-name.
 * If the actual path-name is longer than (arg2-2), or if
 * the value of arg2 is not at least 3, the subroutine will
 * return a value of zero, with errno set as appropriate.
 */

#include <stdio.h>
#include <sys/errno.h>

extern FILE *popen();
extern char *malloc(), *fgets(), *strchr();
extern int errno, pclose();

char *
getcwd(arg1, arg2)
char    *arg1;
int arg2;
{
    FILE    *pipe;
    char    *trm;

    if(arg2 == 0) {
        errno = EINVAL;
        return(0);
    }
    if(arg1 == 0)
        if((arg1 = malloc((unsigned)arg2)) == 0) {
            errno = ENOMEM;
            return(0);
        }
    errno = 0;
    if((pipe = popen("pwd 2>/dev/null", "r")) == 0)     /*pcs*/
        return(0);
    
    errno = 0;                                          /*pcs*/
    if (fgets(arg1, arg2, pipe) == 0) {                 /*pcs*/
        errno = EACCES;                                 /*pcs*/
        return 0;                                       /*pcs*/
    }
    
    (void) pclose(pipe);
    trm = strchr(arg1, '\0');
    if(*(trm-1) != '\n') {
        errno = ERANGE;
        return(0);
    }
    *(trm-1) = '\0';
    return(arg1);
}
