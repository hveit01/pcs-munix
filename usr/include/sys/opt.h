/* @(#)opt.h	6.1 */
#ifndef MESG
msgsys() { nosys(); }
msginit(){return(0);}
#endif

#ifndef SEMA
semsys(){nosys();}
semexit(){}
#endif

#ifndef RLOCK
lockf() { nosys(); }
locked() { return(0); }
unlock() {}
#endif

#ifndef FLOCK
getflck() { nosys(); }
setflck() { nosys(); }
cleanlocks() {}
#endif

#ifdef MASTERNODE
onmaster() { return 0; }
masterdev () {return (NODEV);}
dlnodeinit() {}
daddr_t dlalloc() {}
dlfree() {}
ino_t dlialloc() {}
dlwcsup() {}
dliread() {}
dliupdat() {}
dlstat() {}
diskless_swfree1() {return (0);}
diskless_swalloc() {return (0);}
#if !defined(DLSUPPORT) && defined(ICC_UNISON)
	chdirremrcv() {}
	dlreceive() {}
	rmdlnode() {}
	reqproc() { u.u_procp->p_ppid = 1; exit (0);  /*?? pause();*/ }
#endif DLSUPPORT
#else MASTERNODE
rmdlnode() {}
reqproc() { u.u_procp->p_ppid = 1; exit (0);  /*?? pause();*/ }
#endif MASTERNODE

#ifndef ICC_UNISON
reqproc() { u.u_procp->p_ppid = 1; exit (0);  /*?? pause();*/ }
locate() { u.u_locate_flag = 0; return(0);   /* return (LOCAL) */ }
uipanic() { panic("Munix Net not generated"); }
uidobig() { uipanic(); }
wcore(ip) struct inode *ip; { writei(ip); }
chdirremrcv() { uipanic(); }
uireadi() { uipanic(); }
readehd() { uipanic(); }
readowner() { uipanic(); }
uipacket() { nosys(); }
uimntdev() { return(NODEV); }
uisend() { uipanic(); }
uiioctl() { uipanic(); }
uimount() { u.u_error = ENXIO; }
uiumount() { u.u_error = ENXIO; }
fixuplan() {}
uifork() {}
uiexit() {}
uiupdate() {}
mastercon() { return(0); }
uicmp_input() { return(0); }
uifindnode3() { return(0); }
struct uiport *uigetport () { return ((char *)0); }
int newversion () { return (0); }
#endif

#ifndef VTTY
int vtty=0;
wr_vtty() { u.u_error = EINVAL; }
stderr_vtty() { u.u_error = EINVAL; }
eof_vtty() { u.u_error = EINVAL; }
rele_vtty() { u.u_error = EINVAL; }
vtty_ioctl() { u.u_error = EINVAL; }
free_vtty() { u.u_error = EINVAL; }
syvtty() { return(0); }
sel_vtty() {}
#endif

#ifdef PTY
#ifndef EXOS
int selwait;
xselwakeup() {}
#endif
#endif

#ifndef SHMEM
shmsys(){nosys();}
shmexec(){}
shmexit(){}
shmfork(){}
shmreset(){}
#endif

#if !defined(ICC_ETH) && !defined(ICC_UNISON)
lance_intr(){printf("lance_intr");}
#endif

#ifndef ICC
int icc_init() {}
#endif

#if !defined(ICC_SCC)
int scc_intr () {}
#endif
#if !defined(IW)
int iw_intr () {}
#endif
#if !defined(IS)
int is_intr () {}
#endif
#if !defined(IF)
int if_intr () {}
#endif
#if !defined(ICC_TCPIP)
int socket_intr() {}
int prot_intr() {}
int mgmt_intr() {}
#endif

ubsigvec(){}
ubsigblock(){}
ubsigsetmask(){}
ubsigpause(){}
ubsigstack(){}
ubkillpg(){}
ubwait(){}
ubsetpgrp(){}
ubgetpgrp(){}
gsignal(pgrp, sig) /* Until the inclusion of Berkeley signal stuff */
{
	signal(pgrp, sig);
}

#ifndef STREAM
stropen() { nosys(); }
strclose() { }
strread() {}
strwrite() {}
strioctl() {}
#endif

#ifndef COL
col_init() {}
col_putchar() {}
col_getchar() {}
struct {
	short ttychan;
	short graphchan;
} mxt_cbmt_tab[2];
#endif

#ifndef BIP
short bmt_newmode[1];
bmt_putchar(){}
bmt_getchar(){}
#endif
