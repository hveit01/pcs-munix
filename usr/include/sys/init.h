/* @(#)init.h   6.4 */
extern int clkstart(),cinit(),binit(),errinit(),iinit(),inoinit();
extern int dialog(), dialog1();
extern int fsinit(), finit(), strinit();
extern int flckinit(), bsd_init();
extern int scsi_init(), eginit(), uiinit(), dlnodeinit(), init_vtty(),
	   msginit(), seminit(), dsklinit(),
	   iw_init(), if_init(), is_init(),
	   icc_ieinit(), swinit(), sw2init(),
	   scoinit(), scsiinit(), col_init(), fx_init(),
	   mkvinit(), mainit(), svinit(), clock_init();
extern int x25reseticc(), sdlc_check(), msv_init();

/*	Array containing the addresses of the various initializing	*/
/*	routines executed by "main" at boot time.			*/

int (*init_tbl[])() = {
#ifdef COL
	col_init,
#endif
	cinit,
	binit,
	inoinit,
	fsinit,
/*      clkstart,       called in startup */
/*      errinit,        */
	finit,
#ifndef C20
#   ifdef SCSI
	scsi_init,
#   endif
#   ifdef XT
	sasi_init,
#   endif
#   ifdef EAGLE
	eginit,
#   endif
#   ifdef SW
	swinit,
#   endif
#   ifdef SW2
	sw2init,
#   endif
#else C20
#   ifdef C18
	scoinit,
#   else
	clock_init,
#   endif
#   ifdef FD
	scsiinit,
#   endif
#endif
#ifdef KD
	mkvinit,
	mainit,
	svinit,
#endif
#ifdef CGP
	fx_init,
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
#ifdef STREAM
	strinit,
#endif
#ifdef SOCKET
	bsd_init,
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

#ifndef C20
int (*icc_initprocs[]) () = {
#ifdef IW
		iw_init,
#endif
#ifdef IF
		if_init,
#endif
#ifdef IS
		is_init,
#endif
#ifdef ICC_UNISON
		uiinit,
#endif
#ifdef SOCKET
		icc_ieinit,
#endif
		0
};

#ifdef ICC
struct  down_initprocs down_initprocs[] = {
#ifdef SDLC
		sdlc_check,SDLC_ICCNR,
#endif SDLC
#ifdef X25 
		x25reseticc,X25_ICCNR,
#endif X25
#ifdef MSV1
		msv_init,MSV1_ICCNR,
#endif
		0L, 0
};
#endif  ICC
#endif  C20
