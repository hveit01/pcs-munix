/*  "/usr/bip/cadmaus/cadmaus.h  bz  Fri 09 Nov 84  09:11";
	M --> struct cadmouse in bip ram:
	M->mx  M->my  M->mflags ...  look like device regs
		updated by local bip every 2 ms
		read+set from host bipioctl( MIOC* )
*/

struct cadmouse {
	short   cadmus_mkey, /*  pcs keybuf  */
		cadmus_mintno,
		mx, my, /* clipped */
		mkeys, mkeysprev,
		mflags,
		munused2,
		mprev,
		munused3,
/*  bip ram 5f614:  */
		mxw, myw, /* "wrapped" == raw  */
		mtimer,   /*  bipioctl( MIOCTIMEOUT, # .1 sec )  */
		mtime_ms; /*  down cadmaus  -= 2 every 2 ms  */
	};

#define  Mreset_timer  M->mtime_ms = M->mtimer * 100
#define	decisec	50			/* 50 x 2 ms = .1 s */

#ifdef  LocalBip
#define  M  ((struct cadmouse *)(0x40000                 + 0x1f600))
#else
#define  M  ((struct cadmouse *)((char*) addr + (1<<17)  + 0x1f600))
		/*                       addr = bip_addr[dev]  */
		/*  cadmus 1fe00 in pcs sys stack  */
#endif
	/*  M->mx at      5f604 if LocalBip,
			5 3f604 if host
			5: sysuser seg
			+ bit 17:  **must Bipmap1 = 2,  --> ram
	*/

