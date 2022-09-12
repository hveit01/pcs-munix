/*PCS MODIFIED*/
/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:stdio/fgets.c	3.7"
/*LINTLIBRARY*/
/*
 * This version reads directly from the buffer rather than looping on getc.
 * Ptr args aren't checked for NULL because the program would be a
 * catastrophic mess anyway.  Better to abort than just to return NULL.
 */
#include <stdio.h>
#include "stdiom.h"

#define MIN(x, y)	(x < y ? x : y)

extern int _filbuf();
extern char *memccpy();

char *
fgets(ptr, size, iop)
char *ptr;
register int size;
register FILE *iop;
{
	char *p, *ptr0 = ptr;
	register int n;

	/* Must first check if file is readable (MCD) */
	if (size <= 0 || /*PCS*/
		(iop->_flag & (_IORW|_IOREAD))==0) /*PCS, write only */
		return(NULL);

	for (size--; size > 0; size -= n) {
		if (iop->_cnt <= 0) { /* empty buffer */
			if (_filbuf(iop) == EOF) {
				if (ptr0 == ptr)
					return (NULL);
				break; /* no more data */
			}
			iop->_ptr--;
			iop->_cnt++;
		}
		n = MIN(size, iop->_cnt);
		if ((p = memccpy(ptr, (char *) iop->_ptr, '\n', n)) != NULL)
			n = p - ptr;
		ptr += n;
		iop->_cnt -= n;
		iop->_ptr += n;
		_BUFSYNC(iop);
		if (p != NULL)
			break; /* found '\n' in buffer */
	}
	*ptr = '\0';
	return (ptr0);
}
#if 0
			global	_fgets
      _fgets:
		link.w	a6, #-$24
		movem.l	#$20c0, var28(a6)
		move.l	arg4(a6), d7
		movea.l	arg8(a6), a5
		move.l	arg0(a6), var36(a6)
		tst.l	d7
		ble.s	lbl_28
      lbl_1c:
		move.b	$c(a5), d1
		andi.l	#$81, d1
		bne.s	lbl_2e
      lbl_28:
		clr.l	d0
		bra.w	lbl_ea
      lbl_2e:
		subq.l	#1, d7
		bra.w	lbl_da
      lbl_34:
		tst.l	4(a5)
		bgt.s	lbl_66
      lbl_3a:
		move.l	a5, -(sp)
		jsr 	(__filbuf).l
		addq.l	#4, a7
		cmpi.l	#-$1, d0
		bne.s	lbl_60
      lbl_4c:
		move.l	arg0(a6), d1
		cmp.l	var36(a6), d1
		bne.s	lbl_5c
      lbl_56:
		clr.l	d0
		bra.w	lbl_ea
      lbl_5c:
		bra.w	lbl_e0
      lbl_60:
		subq.l	#1, (a5)
		addq.l	#1, 4(a5)
      lbl_66:
		cmp.l	4(a5), d7
		bge.s	lbl_70
      lbl_6c:
		move.l	d7, d0
		bra.s	lbl_74
      lbl_70:
		move.l	4(a5), d0
      lbl_74:
		move.l	d0, d6
		move.l	d6, -(sp)
		pea 	($a).w
		move.l	(a5), -(sp)
		move.l	arg0(a6), -(sp)
		jsr 	(_memccpy).l
		adda.w	#$10, a7
		move.l	d0, var32(a6)
		beq.s	lbl_9a
      lbl_92:
		move.l	var32(a6), d6
		sub.l	arg0(a6), d6
      lbl_9a:
		add.l	d6, arg0(a6)
		sub.l	d6, 4(a5)
		add.l	d6, (a5)
		move.b	$d(a5), d1
		extb.l	d1
		lea 	(__bufendtab).l, a0
		move.l	(a0,d1.l*4), d2
		sub.l	(a5), d2
		tst.l	4(a5)
		bge.s	lbl_c0
      lbl_bc:
		clr.l	d0
		bra.s	lbl_c4
      lbl_c0:
		move.l	4(a5), d0
      lbl_c4:
		cmp.l	d2, d0
		ble.s	lbl_d2
      lbl_c8:
		move.l	a5, -(sp)
		jsr 	(__bufsync).l
		addq.l	#4, a7
      lbl_d2:
		tst.l	var32(a6)
		bne.s	lbl_e0
      lbl_d8:
		sub.l	d6, d7
      lbl_da:
		tst.l	d7
		bgt.w	lbl_34
      lbl_e0:
		movea.l	arg0(a6), a0
		clr.b	(a0)
		move.l	var36(a6), d0
      lbl_ea:
		movem.l	var28(a6), #$20c0
		unlk	a6
		rts
#endif
