/* @(#)opt.h	6.1 */
#ifndef MESG
msgsys() { nopkg("MSG"); }
msginit(){return(0);}
#endif

#ifndef SEMA
semsys(){nopkg("SEMA");}
semexit(){}
#endif

#ifndef FLOCK
reclock() { return(0); }
cleanlocks () { }
#endif

#if !defined(MASTERNODE) || !defined(DLSUPPORT) || !defined(ICC_UNISON)
reqproc() {
	u.u_procp->p_ppid = 1;
	spl6();
	u.u_procp->p_parent = &proc [1];
	u.u_procp->p_sibling = proc [1].p_child;
	proc[1].p_child = u.u_procp;
	spl0();
	exit (0);  /*?? pause();*/
}
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
#  if !defined(DLSUPPORT) && defined(ICC_UNISON)
	chdirremrcv() {}
	dlreceive() {}
	rmdlnode() {}
#  endif DLSUPPORT
#else MASTERNODE
rmdlnode() {}
#endif MASTERNODE

#ifndef ICC_UNISON
#ifdef  MUNET
locate() { u.u_locate_flag = 0; return(0);   /* return (LOCAL) */ }
#endif
uipanic() { panic("Munix Net not generated"); }
uidobig() { uipanic(); }
wcore(ip) struct inode *ip; { WRITEI(ip); }
chdirremrcv() { uipanic(); }
uireadi() { uipanic(); }
readehd() { uipanic(); }
readowner() { uipanic(); }
uipacket() { nopkg("ICC_UNISON"); }
uimntdev() { return(NODEV); }
uisend() { uipanic(); }
uiioctl() { uipanic(); }
uimount() { u.u_error = ENXIO; }
uiumount() { u.u_error = ENXIO; }
fixuplan() {}
uifork() { uipanic(); }
uiexit() { uipanic(); }
uiupdate() {}
mastercon() { return(0); }
uicmp_input() { return(0); }
uifindnode3() { return(0); }
struct uiport *uigetport () { return ((char *)0); }
int newversion () { return (0); }
St_write() {}
ip_fill() {}
ip_fmore() {}
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
sel_vtty() { noreach(); }
#endif

#ifdef PTY
#ifndef EXOS
int selwait;
xselwakeup() {}
#endif
#endif

#ifndef SHMEM
shmsys(){nopkg("SHMEM");}
shmexec(){}
shmexit(){}
shmfork(){}
shmreset(){}
#endif

#if !defined(ICC_ETH) && !defined(ICC_UNISON) && !defined(SOCKET)
lance_intr(){printf("lance_intr");}
#endif

#ifndef ICC
int icc_init() {}
#endif

#ifndef MSV1
int msv_intr() {}
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
getmsg() {nopkg("STREAM");}
putmsg() {nopkg("STREAM");}
poll() {nopkg("STREAM");}
stropen() { nopkg("STREAM"); }
strclose() { }
strread() {}
strwrite() {}
strioctl() {}
runqueues() {}
strclean() {}
pollreset() {noreach();}
strpoll() {noreach();}
struct fmodsw fmodsw[1];
int fmodcnt = 0;
char qrunflag;
int pollwait;
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

#if !defined(CGP)
cgs_putchar() {}
cgs_getchar() {}
#endif

#ifndef BIP
short bmt_newmode[1];
bmt_putchar(){}
bmt_getchar(){}
struct {
	short ttychan;
	short graphchan;
} mxt_bmt_tab[2];
#endif

#ifndef RFS
advfs() {nopkg("RFS");}
unadvfs() {nopkg("RFS");}
rfstart() {nopkg("RFS");}
rfstop() {nopkg("RFS");}
rfsys() {nopkg("RFS");}
rlink() {nopkg("RFS");}
rmount() {nopkg("RFS");}
rumount() {nopkg("RFS");}
dustat() {noreach();}
racct() {noreach();}
clrcookie() {}
cpcookie() {noreach();}
del_sndd() {noreach();}
eqcookie() {noreach();}
rcopyin() {noreach();}
rcopyout() {noreach();}
rdebug() {nopkg("RFS");}
remfileop() {noreach();}
struct inode * remote_call() {noreach();}
riget() {noreach();}
rnamei0() {noreach();}
rnamei1() {noreach();}
rnamei2() {noreach();}
rnamei3() {noreach();}
rnamei4() {noreach();}
rlbumount() {noreach();}
remio() {noreach();}
unremio() {noreach();}
rfubyte() {noreach();}
rfuword() {noreach();}
rfulong() {noreach();}
rsubyte() {noreach();}
rsuword() {noreach();}
rsulong() {noreach();}
rem_date() {}
duustat() {nopkg("RFS");}
enable_cache() {noreach();}
disable_cache() {noreach();}
struct  rbuf *chk_rlist() {noreach(); return((struct rbuf *)0);}
int nservers, idleserver, msglistcnt;
struct srmnt srmount[1];
#endif

hdeexit() {} /* hard error disk log driver ?? */

#ifdef C20

#ifndef CS
csinit() {}
cswsrv() {}
csintr() {}
cstintr() {}
#endif CS

#endif C20

#ifndef SOCKET
char netisrflag;
service_net() { noreach();}
icc_socket_rint() { noreach();}
if_icc_err() {}
accept() { nopkg("SOCKET");}
bind() { nopkg("SOCKET");}
connect() { nopkg("SOCKET");}
getpeername() { nopkg("SOCKET");}
getsockname() { nopkg("SOCKET");}
getsockopt() { nopkg("SOCKET");}
listen() { nopkg("SOCKET");}
recv() { nopkg("SOCKET");}
recvfrom() { nopkg("SOCKET");}
recvmsg() { nopkg("SOCKET");}
socketpair() { nopkg("SOCKET");}
send() { nopkg("SOCKET");}
sendmsg() { nopkg("SOCKET");}
sendto() { nopkg("SOCKET");}
setsockopt() { nopkg("SOCKET");}
shutdown() { nopkg("SOCKET");}
socket() { nopkg("SOCKET");}
/*
  gethostname() { nopkg("SOCKET");}
  sethostname() { nopkg("SOCKET");}
  getdomainname() { nopkg("SOCKET");}
  setdomainname() { nopkg("SOCKET");}
*/
int soc_fstyp = -1;
soc_select() { noreach(); }
#endif
