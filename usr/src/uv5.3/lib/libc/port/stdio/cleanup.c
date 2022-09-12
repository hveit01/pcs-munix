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
/*	@(#)flsbuf.c	2.8	*/
/*LINTLIBRARY*/
#include <stdio.h>
#include "stdiom.h"
#include <sys/errno.h>

extern void free();
extern int errno, write(), close(), isatty();
extern char *malloc();
extern FILE *_lastbuf;
#if !u370
extern unsigned char *_stdbuf[];
#endif
extern unsigned char _smbuf[][_SBFSIZ];

/*
 * Flush buffers on exit
 */

void
_cleanup()
{
	register FILE *iop;
	register unsigned char *bp;
	register n;

	for(iop = _iob; iop < _lastbuf; iop++) {
		if ((iop->_flag & (_IOWRT|_IONBF))==_IOWRT &&
				(bp=iop->_base) && (n = iop->_ptr-bp) > 0)
			write(iop->_file, bp, n);
		close(iop->_file);
	}
}
