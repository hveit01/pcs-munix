/*PCS MODIFIED*/
/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:gen/ctype.c	1.4"
/*LINTLIBRARY*/
#include <ctype.h>

#define DEF_CHRPATH "/lib/chrclass/"
#define CHRPATH_SZ  14
#define CHRBUFSZ 257*sizeof(short)

char *getenv();

char _ctype_[] = { 0,

/*character classes */
/*	 0	 1	 2	 3	 4	 5	 6	 7  */
/* 0*/	_C,	_C,	_C,	_C,	_C,	_C,	_C,	_C,
/* 10*/	_C,	_S|_C,	_S|_C,	_S|_C,	_S|_C,	_S|_C,	_C,	_C,
/* 20*/	_C,	_C,	_C,	_C,	_C,	_C,	_C,	_C,
/* 30*/	_C,	_C,	_C,	_C,	_C,	_C,	_C,	_C,
/* 40*/	_S|_B,	_P,	_P,	_P,	_P,	_P,	_P,	_P,
/* 50*/	_P,	_P,	_P,	_P,	_P,	_P,	_P,	_P,
/* 60*/	_N|_X,	_N|_X,	_N|_X,	_N|_X,	_N|_X,	_N|_X,	_N|_X,	_N|_X,
/* 70*/	_N|_X,	_N|_X,	_P,	_P,	_P,	_P,	_P,	_P,
/*100*/	_P,	_U|_X,	_U|_X,	_U|_X,	_U|_X,	_U|_X,	_U|_X,	_U,
/*110*/	_U,	_U,	_U,	_U,	_U,	_U,	_U,	_U,
/*120*/	_U,	_U,	_U,	_U,	_U,	_U,	_U,	_U,
/*130*/	_U,	_U,	_U,	_P,	_P,	_P,	_P,	_P,
/*140*/	_P,	_L|_X,	_L|_X,	_L|_X,	_L|_X,	_L|_X,	_L|_X,	_L,
/*150*/	_L,	_L,	_L,	_L,	_L,	_L,	_L,	_L,
/*160*/	_L,	_L,	_L,	_L,	_L,	_L,	_L,	_L,
/*170*/	_L,	_L,	_L,	_P,	_P,	_P,	_P,	_C,
/*200*/	 0,	 0,	 0,	 0,	 0,	 0,	 0,	 0,
	 0,	 0,	 0,	 0,	 0,	 0,	 0,	 0,
	 0,	 0,	 0,	 0,	 0,	 0,	 0,	 0,
	 0,	 0,	 0,	 0,	 0,	 0,	 0,	 0,
	 0,	 0,	 0,	 0,	 0,	 0,	 0,	 0,
	 0,	 0,	 0,	 0,	 0,	 0,	 0,	 0,
	 0,	 0,	 0,	 0,	 0,	 0,	 0,	 0,
	 0,	 0,	 0,	 0,	 0,	 0,	 0,	 0,
	 0,	 0,	 0,	 0,	 0,	 0,	 0,	 0,
	 0,	 0,	 0,	 0,	 0,	 0,	 0,	 0,
	 0,	 0,	 0,	 0,	 0,	 0,	 0,	 0,
	 0,	 0,	 0,	 0,	 0,	 0,	 0,	 0,
	 0,	 0,	 0,	 0,	 0,	 0,	 0,	 0,
	 0,	 0,	 0,	 0,	 0,	 0,	 0,	 0,
	 0,	 0,	 0,	 0,	 0,	 0,	 0,	 0,
	 0,	 0,	 0,	 0,	 0,	 0,	 0,	 0,

/*tolower/toupper conversion*/
	  0,
	  0,  1,  2,  3,  4,  5,  6,  7,
	  8,  9, 10, 11, 12, 13, 14, 15,
	 16, 17, 18, 19, 20, 21, 22, 23,
	 24, 25, 26, 27, 28, 29, 30, 31,
	 32, 33, 34, 35, 36, 37, 38, 39,
	 40, 41, 42, 43, 44, 45, 46, 47,
	 48, 49, 50, 51, 52, 53, 54, 55,
	 56, 57, 58, 59, 60, 61, 62, 63,
	 64, 97, 98, 99,100,101,102,103,
	104,105,106,107,108,109,110,111,
	112,113,114,115,116,117,118,119,
	120,121,122, 91, 92, 93, 94, 95,
	 96, 65, 66, 67, 68, 69, 70, 71,
	 72, 73, 74, 75, 76, 77, 78, 79,
	 80, 81, 82, 83, 84, 85, 86, 87,
	 88, 89, 90,123,124,125,126,127,
	  0,  0,  0,  0,  0,  0,  0,  0,
	  0,  0,  0,  0,  0,  0,  0,  0,
	  0,  0,  0,  0,  0,  0,  0,  0,
	  0,  0,  0,  0,  0,  0,  0,  0,
	  0,  0,  0,  0,  0,  0,  0,  0,
	  0,  0,  0,  0,  0,  0,  0,  0,
	  0,  0,  0,  0,  0,  0,  0,  0,
	  0,  0,  0,  0,  0,  0,  0,  0,
	  0,  0,  0,  0,  0,  0,  0,  0,
	  0,  0,  0,  0,  0,  0,  0,  0,
	  0,  0,  0,  0,  0,  0,  0,  0,
	  0,  0,  0,  0,  0,  0,  0,  0,
	  0,  0,  0,  0,  0,  0,  0,  0,
	  0,  0,  0,  0,  0,  0,  0,  0,
	  0,  0,  0,  0,  0,  0,  0,  0,
	  0,  0,  0,  0,  0,  0,  0,  0
};

static char first_call = 1;

setchrclass(cls)
char *cls;
{
	register fd, ret_val = -1;
	char path[128];
	char buf[CHRBUFSZ];
	
	if (cls == 0) {
		if ((cls=getenv("CHRCLASS")) == 0 || *cls == '\0') {
			if (first_call) return 0;
			cls = "ascii";
		}
	}

	first_call = 0;
	strcpy(path, DEF_CHRPATH);
	strcpy(&path[CHRPATH_SZ], cls);
	if ((fd = open(path, 0)) >= 0) {
		if (read(fd, buf, CHRBUFSZ) == CHRBUFSZ) {
			memcpy(_ctype_, buf, CHRBUFSZ);
			ret_val = 0;
		}
		close(fd);
	}
	return ret_val;
}
