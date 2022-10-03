/*  sys/bitmap.h  with bipctl at 3ffe0  */
typedef long (*Funcp)();

struct bip_device {
	char    ram[ 0x3e000 ],
		bss[  0x1000 ];
	Funcp   func; /*  host -> bitmap call box  */
	long    args[6],
		funcret;
	char    wbuf[ 1024 ],
		keys[   32 ],
		funcs[  32 ];
	short   intr_state;
#define idle 0
#define rintr 1
#define wintr 2
		/*  <-> unix bip tty driver /usr/src/sys/dev/bip.c:
		 *  biprint sets intr_state = 0   => keyb can qintr again
		 */
	short   rintno, wintno; /*  E0/4  =>  qbus interrupt on key  */
	char    skip[ 0x3ffe0 - 0x3f466 ];
/*  3ffe0: bip ctl:  */
	short   go,
		intr_local68,
		map0,
		map1,
		mousereg,
		mregs[  3 ],
		sio[    4 ],
		pixctl[ 4 ]; /*  sizeof( struct device) == 0x40000  */
};

#define  Bip_map1   addr->map1 = 0x40000 >> 17  /* so bip->ram is local ram  */

#define  Bip_write( addr, buf, len )    \
	{ Bip_map1;                     \
	while( addr->func != 0 ) {}   \
	bcopy( buf, addr->wbuf, len );  \
	addr->args[0] = len;            \
	addr->func = 1;                 \
	}

#define  Bip_putc( addr, c )            \
	{ Bip_map1;                     \
	while( addr->func != 0 ) {}     \
	addr->wbuf[0] = c;              \
	addr->args[0] = 1;              \
	addr->func = 1;                 \
	}

#define  Bip_key  addr->keys[0]
#define  Bip_getc( addr, c )            \
	{ Bip_map1;                     \
	while( addr->keys[0] == 0 ) {   \
	    addr->intr_state = 0;       \
	}                               \
	c = addr->keys[0];              \
	    addr->keys[0] = 0;          \
	}
