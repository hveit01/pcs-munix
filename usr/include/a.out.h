#ident	"@(#)head:a.out.h	2.12"

#include <nlist.h>	/* included for all machines */

 /*		COMMON OBJECT FILE FORMAT

	For a description of the common object file format (COFF) see
	the Common Object File Format chapter of the UNIX System V Support 
	Tools Guide

 		OBJECT FILE COMPONENTS

 	HEADER FILES:
 			/usr/include/filehdr.h
			/usr/include/aouthdr.h
			/usr/include/scnhdr.h
			/usr/include/reloc.h
			/usr/include/linenum.h
			/usr/include/syms.h
			/usr/include/storclass.h

	STANDARD FILE:
			/usr/include/a.out.h    "object file" 
   */

#include "filehdr.h"
#include "aouthdr.h"
#include "scnhdr.h"
#include "reloc.h"
#include "linenum.h"
#include "syms.h"
