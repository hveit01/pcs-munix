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
/*  @(#)cuserid.c   1.3 */
/*  3.0 SID #   1.2 */
/*LINTLIBRARY*/
#include <stdio.h>
#include <pwd.h>

extern char *strcpy(), *getlogin();
extern int getuid();
extern struct passwd *getpwuid();
static char res[L_cuserid];

char *
cuserid(s)
char    *s;
{
    register struct passwd *pw;
    register char *p;

    if (s == NULL)
        s = res;
    p = getlogin();
    if (p != NULL)
        return (strcpy(s, p));
    pw = getpwuid(getuid());
    endpwent();
    if (pw != NULL)
        return (strcpy(s, pw->pw_name));
    *s = '\0';
    return (NULL);
}
