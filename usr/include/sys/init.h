/* @(#)init.h   6.4 */
extern int clkstart(),cinit(),binit(),errinit(),iinit(),inoinit();
extern int dialog(), dialog1();
extern int finit();
extern int flckinit();
extern int scsi_init(), eginit(), uiinit(), dlnodeinit(), init_vtty(),
	   msginit(), seminit(), dsklinit(),
	   iw_init(), if_init(), is_init(),
	   tcp_init(), swinit(), sw2init(), col_init();
extern int x25reseticc(), sdlc_check();

/*	Array containing the addresses of the various initializing	*/
/*	routines executed by "main" at boot time.			*/

int (*init_tbl[])() = {
#ifdef COL
	col_init,
#endif
	inoinit,
/*      clkstart,       called in startup */
	cinit,
	binit,
/*      errinit,        */
	finit,
#ifdef SCSI
	scsi_init,
#endif
#ifdef XT
	sasi_init,
#endif
#ifdef EAGLE
	eginit,
#endif
#ifdef SW
	swinit,
#endif
#ifdef SW2
	sw2init,
#endif
#if defined(DLSUPPORT) && !defined(MASTERNODE)
	dsklinit,
	dlnodeinit,
#endif
	dialog1,
	dialog,
	iinit,
#ifdef FLOCK
	flckinit,
#endif
#ifdef VTTY
	init_vtty,
#endif
#ifdef MESG
	msginit,
#endif
#ifdef SEMA
	seminit,
#endif
	0
};

int (*icc_initprocs[]) () = {
#if defined(IW)
		iw_init,
#endif
#if defined(IF)
		if_init,
#endif
#if defined(IS)
		is_init,
#endif
#if ICC_UNISON
		uiinit,
#endif
#ifdef ICC_TCPIP
		tcp_init,
#endif
		0
};

#ifdef ICC
int (*down_initprocs[]) () = {
#ifdef SDLC
		sdlc_check,
#endif SDLC
#ifdef X25 
		x25reseticc,
#endif X25
		0L
};
#endif
