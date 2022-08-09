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
/*  @(#)setvbuf.c   1.2         */
/*LINTLIBRARY*/
#include "stdio.h"

extern void free();
extern int isatty();

int
setvbuf(iop, buf, type, size)   /*pcs args swapped*/
register FILE *iop;
register char   *buf;
register int type;
register int size;
{
    char *malloc();
    
    if (size < 0)           /*pcs*/
        return -1;          /*pcs*/

    if(iop->_base != NULL && iop->_flag & _IOMYBUF)
        free((char*)iop->_base);
    iop->_flag &= ~(_IOMYBUF | _IONBF | _IOLBF);
    switch (type)  {
        /*note that the flags are the same as the possible values for type*/
        case _IONBF:
        /* file is unbuffered */
        iop->_flag |= _IONBF;
        _bufend(iop) = iop->_base = NULL;
        break;
        case _IOLBF:
        case _IOFBF:
        iop->_flag |= type;
        if (size <= 8) {
            size = BUFSIZ;
            buf = 0;
        }
        if (buf == 0)                           /*pcs*/
            iop->_base = (unsigned char*)       /*pcs*/
                malloc((unsigned)(size += 8));  /*pcs*/
        else                                    /*pcs*/
            iop->_base = (unsigned char*)buf;   /*pcs*/
        _bufend(iop) = iop->_base + size - 8;   /*pcs*/
        break;
        default:
        return -1;
    }
    iop->_ptr = iop->_base;
    iop->_cnt = 0;
    return 0;
}
