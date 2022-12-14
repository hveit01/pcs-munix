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
/*  @(#)tolower.c   1.2 */
/*  3.0 SID #   1.2 */
/*LINTLIBRARY*/
/*
 * If arg is upper-case, return the lower-case, else return the arg.
 */

int
tolower(c)
register int c;
{
    if(c >= 'A' && c <= 'Z')
        c -= 'A' - 'a';
    return(c);
}
