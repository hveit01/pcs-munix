/* @(#)sysent.c 6.2 */
static char* _Version = "@(#) RELEASE:  1.4  Mar 24 1987 /usr/sys/os/sysent.c ";

#include "sys/param.h"
#include "sys/types.h"
#include "sys/systm.h"

/*
 * This table is the switch used to transfer
 * to the appropriate routine for processing a system call.
 * Each row contains the number of arguments expected
 * and a pointer to the routine.
 */

int alarm();
int chdir();
int chmod();
int chown();
int chroot();
int close();
int creat();
int dup();
int exec();
int exece();
int fcntl();
int fork();
int forkexec(); /*pcs*/
int fstat();
int ftime(); /*pcs*/
int getdents(); /*pcs*/
int getgid();
int getpid();
int getuid();
int gtime();
int gtty();
int hertz(); /*pcs*/
int ioctl();
int kill();
int link();
int lock();
int lockf(); /*pcs*/
int lstat(); /*pcs*/
int m881used(); /*pcs*/
int mkdir(); /*pcs*/
int mknod();
int msgsys();
int nap(); /*pcs*/
int nice();
int nosys();
int nullsys();
int open();
int pause();
int pipe();
int profil();
int ptrace();
int read();
int readlink(); /*pcs*/
int rexit();
int rmdir(); /*pcs*/
int saccess();
int sbreak();
int seek();
int semsys();
int setgid();
int select(); /*pcs*/
int setpgrp();
int setuid();
int shmsys();
int smount();
int ssig();
int stat();
int symlink(); /*pcs*/
int stime();
int stty();
int sumount();
int sync();
int sysacct();
int times();
int uadmin(); /*pcs*/
int ubgetpgrp(); /*pcs*/
int ubkillpg(); /*pcs*/
int ubsetpgrp(); /*pcs*/
int ubsigblock(); /*pcs*/
int ubsigpause(); /*pcs*/
int ubsigsetmask(); /*pcs*/
int ubsigstack(); /*pcs*/
int ubsigvec(); /*pcs*/
int ubwait(); /*pcs*/
int uipacket(); /*pcs*/
int ulimit();
int umask();
int unlink();
int utime();
int utssys();
int swapfunc();
int wait();
int write();

struct sysent sysent[] = 
{
    0,  1,  nosys,      "",         /*  0 = indir */
    4,  1,  rexit,      "s",        /*  1 = exit */
    0,  1,  fork,       "",         /*  2 = fork */
    12, 0,  read,       "slu",      /*  3 = read */
    12, 0,  write,      "slu",      /*  4 = write */
    12, 0,  open,       "lss",      /*  5 = open */
    4,  0,  close,      "s",        /*  6 = close */
    4,  0,  wait,       "l",        /*  7 = wait */
    8,  0,  creat,      "ls",       /*  8 = creat */
    8,  0,  link,       "ll",       /*  9 = link */
    4,  0,  unlink,     "l",        /* 10 = unlink */
    8,  0,  uipacket,   "ls",       /*pcs 11 = uipacket */
    4,  0,  chdir,      "l",        /* 12 = chdir */
    0,  1,  gtime,      "",         /* 13 = time */
    20, 0,  mknod,      "lsse",     /* 14 = mknod */
    8,  0,  chmod,      "ls",       /* 15 = chmod */
    12, 0,  chown,      "lss",      /* 16 = chown; now 3 args */
    4,  1,  sbreak,     "l",        /* 17 = break */
    12, 0,  stat,       "lll",      /* 18 = stat */
    12, 1,  seek,       "sls",      /* 19 = seek */
    0,  1,  getpid,     "",         /* 20 = getpid */
    12, 0,  smount,     "lls",      /* 21 = mount */
    4,  0,  sumount,    "l",        /* 22 = umount */
    4,  1,  setuid,     "s",        /* 23 = setuid */
    0,  1,  getuid,     "",         /* 24 = getuid */
    4,  1,  stime,      "l",        /* 25 = stime */
    16, 1,  ptrace,     "ssll",     /* 26 = ptrace */
    4,  1,  alarm,      "u",        /* 27 = alarm */
    12, 0,  fstat,      "sll",      /* 28 = fstat */
    0,  0,  pause,      "",         /* 29 = pause */
    8,  0,  utime,      "ll",       /* 30 = utime */
    0,  0,  nosys,      "",         /*pcs was 31 = stty */
    0,  0,  nosys,      "",         /*pcs was 32 = gtty */
    8,  0,  saccess,    "ls",       /* 33 = access */
    4,  1,  nice,       "s",        /* 34 = nice */
    4,  1,  ftime,      "l",        /*pcs was 35 = sleep */
    0,  0,  sync,       "",         /* 36 = sync */
    8,  1,  kill,       "ss",       /* 37 = kill */
    0,  1,  nosys,      "",         /*pcs was 38 = x */
    4,  1,  setpgrp,    "s",        /* 39 = setpgrp */
    0,  1,  nosys,      "",         /* 40 = tell - obsolete */
    8,  1,  dup,        "ss",       /* 41 = dup */
    4,  0,  pipe,       "l",        /* 42 = pipe */
    4,  1,  times,      "l",        /* 43 = times */
    16, 1,  profil,     "llls",     /* 44 = prof */
    12, 1,  fcntl,      "ss?",      /*pcs was 45 = proc lock */
    4,  1,  setgid,     "s",        /* 46 = setgid */
    0,  1,  getgid,     "",         /* 47 = getgid */
    8,  1,  ssig,       "sl",       /* 48 = sig */
    0,  0,  nosys,      "",         /*pcs 49 */
    0,  0,  nosys,      "",         /*pcs 50 */
    4,  0,  sysacct,    "l",        /* 51 = turn acct off/on */
    0,  0,  nosys,      "",         /*pcs 52 */
    4,  1,  lock,       "s",        /*pcs 53 */
    12, 0,  ioctl,      "ss?",      /* 54 = ioctl */
    0,  1,  nosys,      "",         /* 55 = x */
    12, 0,  forkexec,   "lll",      /*pcs was 56 = x */
    12, 1,  utssys,     "ssl",      /* 57 = utssys */
    4,  1,  swapfunc,   "l",        /* 58 = swap functions */
    12, 0,  exece,      "lll",      /* 59 = exece */
    4,  1,  umask,      "s",        /* 60 = umask */
    4,  0,  chroot,     "l",        /* 61 = chroot */
    0,  1,  fork,       "",         /*pcs was 62 = fcntl */
    12, 0,  read,       "lll",      /*pcs was 63 = ulimit */
    12, 0,  write,      "lll",      /*pcs 64 */
    8,  1,  ulimit,     "sl",       /*pcs 65, moved from 63 */
    0,  1,  hertz,      "",         /*pcs 66 */
    0,  1,  m881used,   "",         /*pcs 67 */
    12, 1,  ubsigvec,   "sll",      /*pcs 68 */
    4,  0,  ubsigblock, "l",        /*pcs 69 */
    4,  1,  ubsigsetmask,"l",       /*pcs 70 */
    4,  0,  ubsigpause, "l",        /*pcs 71 */
    8,  1,  ubsigstack, "ll",       /*pcs 72 */
    8,  1,  ubkillpg,   "ss",       /*pcs 73 */
    8,  0,  ubwait,     "ls",       /*pcs 74 */
    8,  1,  ubsetpgrp,  "ss",       /*pcs 75 */
    4,  1,  ubgetpgrp,  "s",        /*pcs 76 */
    24, 0,  msgsys,     "s?",       /*pcs 77, moved from 49 */
    20, 0,  semsys,     "s?",       /*pcs 78, moved from 53 */
    16, 0,  shmsys,     "s?",       /*pcs 79, moved from 52 */
    0,  1,  nullsys,    "",         /*pcs 80 */
    12, 0,  lockf,      "ssl",      /*pcs 81 */
    0,  1,  nosys,      "",         /*pcs 82 */
    8,  0,  symlink,    "ll",       /*pcs 83 */
    12, 0,  readlink,   "lls",      /*pcs 84 */
    12, 0,  lstat,      "lll",      /*pcs 85 */
    8,  0,  uadmin,     "ss",       /*pcs 86 */
    0,  1,  nosys,      "",         /*pcs 87 */
    0,  1,  nosys,      "",         /*pcs 88 */
    0,  1,  nosys,      "",         /*pcs 89 */
    0,  1,  nosys,      "",         /*pcs 90 */
    4,  1,  rmdir,      "l",        /*pcs 91 */
    8,  1,  mkdir,      "ls",       /*pcs 92 */
    0,  1,  nosys,      "",         /*pcs 93 */
    0,  1,  nosys,      "",         /*pcs 94 */
    0,  1,  nosys,      "",         /*pcs 95 */
    0,  1,  nosys,      "",         /*pcs 96 */
    12, 1,  getdents,   "slu",      /*pcs 97 */
    4,  1,  nap,        "s",        /*pcs 98 */
    20, 0,  select,     "sllll"     /*pcs 99 */
};

int sysentlen = 100;
