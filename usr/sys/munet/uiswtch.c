/* PCS specific */
static char *_Version = "@(#) RELEASE:  1.7  May 13 1987 /usr/sys/munet/uiswtch.c ";

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/inode.h>
#include <sys/dir.h>
#include <sys/file.h>
#include <sys/mount.h>
#include <sys/page.h>
#include <sys/region.h>
#include <sys/errno.h>
#include <sys/signal.h>
#include <sys/user.h>
#include <sys/var.h>
#include <sys/utsname.h>
#include <sys/acct.h>
#include <sys/stat.h>
#include <sys/conf.h>
#include <sys/munet/munet.h>
#include <sys/munet/mnbuf.h>
#include <sys/munet/mnnode.h>
#include <sys/munet/uisw.h>
#include <fcntl.h>

extern int toggle;                      /*nami.c*/
extern char symname[];                  /*nami.c*/
extern int schar(), uchar();            /*nami.c*/
extern struct file* falloc();           /*fio.c*/
extern short childrecv_ptno;

uisizesw(callno, node)
int callno, node;
{
    register short version; 
    register int vsize;
    register int ucount;
    
    switch (callno) {
    case UIIGUARD:
    case UIICCLO:
    case UIIGUID:
    case UIICONNVERS:
    case UIIPASS:
    case UIIDECR:
    case UIIINCR:
    case UIIICLO:
    case UIIREAD:
    case UIICLOF:
    case UIICONN:
    case UISREAD:
    case UISCLOS:
    case UISSEEK:
    case UISDUP:
    case UISLREAD:
    case UISLOCKF:
    case UISGETDENTS:
        return UIMIN_MNPKT;

    case UIISEXEC:
    case UIICOPEN:
    case UIIEXEC:
    case UISOPEN:
    case UISCREAT:
    case UISULINK:
    case UISCHDIR:
    case UISMKNOD:
    case UISCHMOD:
    case UISCHOWN:
    case UISACCES:
    case UISSACCT:
    case UISEXEC:
    case UISCHROT:
    case UISREADLINK:
    case UISRMDIR:
    case UISMKDIR:
        return UIMIN_MNPKT + UIMAXPATH;

    case UISFSTAT:
        return UIMIN_MNPKT + sizeof(struct stat);

    case UISFCNTL:
        return UIMIN_MNPKT + sizeof(struct flock);

    case UISSTAT:
    case UISLSTAT:
        return UIMIN_MNPKT + UIMAXPATH + sizeof(struct stat);

    case UIIWRITE:
    case UISWRITE:
    case UISLWRITE:
        ucount = u.u_count;
        ucount += UIMIN_MNPKT;
        version = WHAT_VERSION(node);
        switch (version) {
        case NOT_KNOWN:
        case OLD_VERSION:
            vsize = UIMIN_MNPKT + UIMAXDATA_OLD;
            break;
        case OPTIMIZED_NOFR:
            vsize = UIMAXPACKET;
            break;
        case OPTIMIZED_FRAG:
            vsize = sizeof(struct mnpacket);
            break;
        }
        if (ucount > vsize)
            return vsize;
        else
            return ucount;

    case UIIACCTW:
    case UISLINK:
    case UISUTIME:
    case UISIOCTL:
    case UISSYMLINK:
    default:
        return UIMIN_MNPKT + UIMAXDATA_OLD;
    }
}

uisizeswx(callno, node, uucount)
int callno, node, uucount;
{
    register int vsize;

    switch (callno) {
    case UIISIGNAL:
    case UIIGUARD:
    case UIIWRITE:
    case UIICCLO:
    case UIIGUID:
    case UIIQUIT:
    case UIICONNVERS:
    case UIIPASS:
    case UIIDECR:
    case UIIINCR:
    case UIIXONN:
    case UIIICLO:
    case UIICLOF:
    case UIICONN:
    case UIICERB:
    case UIIREM:
    case UIIREMVERS:
    case UISWRITE:
    case UISCLOS:
    case UISSEEK:
    case UISDUP:
    case UISLWRITE:
    case UISLOCKF:
        return UIMIN_MNPKT;

    case UIISEXEC:
    case UIICOPEN:
    case UIIEXECN:
    case UISOPEN:
    case UISCREAT:
    case UISLINK:
    case UISULINK:
    case UISCHDIR:
    case UISMKNOD:
    case UISCHMOD:
    case UISCHOWN:
    case UISUTIME:
    case UISACCES:
    case UISSACCT:
    case UISEXEC:
    case UISCHROT:
    case UISSYMLINK:
    case UISRMDIR:
    case UISMKDIR:
        return UIMIN_MNPKT + UIMAXPATH;

    case UISFSTAT:
        return UIMIN_MNPKT + sizeof(struct stat);

    case UISFCNTL:
        return UIMIN_MNPKT + sizeof(struct flock);

    case UISSTAT:
    case UISLSTAT:
    return UIMIN_MNPKT + UIMAXPATH + sizeof(struct stat);

    case UIIIPC:
    case UIIXWHO:
    case UIIUWHO:
    case UIIACCTW:
    case UISIOCTL:
    case UISREADLINK:
        return UIMIN_MNPKT + UIMAXDATA_OLD;

    default:
        uucount = UIMAXDATA_NEW_FRAG;
        /*FALLTHRU*/

    case UIIXRUN:
    case UIIURUN:
    case UIIREAD:
    case UISREAD:
    case UISLREAD:
    case UISGETDENTS:
        uucount += UIMIN_MNPKT;
        switch (WHAT_VERSION(node)) {
        case NOT_KNOWN:
        case OLD_VERSION:
            vsize = UIMIN_MNPKT + UIMAXDATA_OLD;
            break;
        case OPTIMIZED_NOFR:
            vsize = UIMAXPACKET;
            break;
        case OPTIMIZED_FRAG:
            vsize = sizeof(struct mnpacket);
            break;
        }
        if (vsize > uucount)
            return uucount;
        else
            return vsize;
    }
}

int uisinsw(rbufid, rdev, ucount)
ushort rbufid;
dev_t rdev;
off_t ucount;
{
    register struct uupacket *up;
    int ret;
    struct mnpacket *pkptr;
    bufque *bq;
    

    bq = buf_pointer(rbufid);
    pkptr = (struct mnpacket*)bq->bq_pkptr;
    up = &pkptr->uu;
    ret = 0;
    
    switch (u.u_callno) {
    case UIIGUID:
        up->uu_ip = u.u_execp;
        break;

    case UIIREAD:
        up->uu_ip = u.u_execp;
        up->uu_offset = u.u_offset;
        up->uu_count = min(u.u_count, ucount);
        break;

    case UIIWRITE:
        up->uu_ip = u.u_execp;
        up->uu_offset = u.u_offset;
        up->uu_count = u.u_count > ucount ? ucount : u.u_count;
        if (buf_copy(rbufid, up->uu_data, u.u_base, up->uu_count, u.u_segflg, 0) == -1)
            u.u_error = EFAULT;
        break;

    case UISREAD:
    case UISWRITE:
    case UISLREAD:
    case UISLWRITE:
    case UISGETDENTS:
        ret = 1;
        up->uu_fd = u.u_rofile[u.u_munet];
        up->uu_offset = u.u_ofile[u.u_munet]->f_offset;
        up->uu_rwcnt = u.u_count;
        up->uu_argi[0] = u.u_pofile[u.u_munet];
        if (u.u_callno == UISWRITE || u.u_callno == UISLWRITE) {
            if (buf_copy(rbufid, up->uu_data, u.u_base, 
                    (uint)min(u.u_count,ucount), 0, 0) == -1)
                u.u_error = EFAULT;
        }
        break;

    case UIICCLO:
    case UIIICLO:
        up->uu_ip = u.u_execp;
        break;

    case UIIINCR:
        up->uu_altport = u.u_munet;
        break;

    case UIIDECR:
        {
            register int *uap = (int*)u.u_ap;
            up->uu_argi[0] = *uap & 0xff;
            break;
        }

    case UIIACCTW:
        {
            register uint i;
            register struct acct *uap = (struct acct*)up->uu_data;
            for (i=0; i < 8; i++)
                uap->ac_comm[i] = u.u_comm[i];
            uap->ac_utime = compress(u.u_utime);
            uap->ac_stime = compress(u.u_stime);
            uap->ac_etime = compress(lbolt - u.u_ticks);
            uap->ac_btime = u.u_start;
            uap->ac_uid = u.u_ruid;
            uap->ac_gid = u.u_rgid;
            uap->ac_mem = compress(u.u_mem);
            uap->ac_io = compress(u.u_ioch);
            uap->ac_rw = compress(u.u_ior + u.u_iow);
            uap->ac_tty = u.u_ttyp ? u.u_ttyd : -1;
            uap->ac_stat = u.u_munet;
            uap->ac_flag = u.u_acflag;
            up->uu_ip = acctp;
            break;
        }

    case UISFSTAT:
        {
            register struct a {
                int fd;
                struct stat *stat;
            } *uap = (struct a*)u.u_ap;
            up->uu_fd = u.u_rofile[uap->fd];
            copyin(uap->stat, up->uu_data, sizeof(struct stat));
            break;
        }

    case UIIEXEC:
    case UISEXEC:
        u.u_callno = UISEXEC;
        /*FALLTHRU*/
    case UIISEXEC:
        copypath(up->uu_data);
        break;

    case UISSTAT:
    case UISLSTAT:
        {
            register struct a {
                char *path;
                struct stat *stat;
            } *uap = (struct a*)u.u_ap;
            copypath(up->uu_data);
            copyin(uap->stat, &up->uu_data[UIMAXPATH], sizeof(struct stat));
            break;
        }

    case UIICOPEN:
    case UISULINK:
    case UISCHDIR:
    case UISCHMOD:
    case UISACCES:
    case UISCHROT:
    case UISRMDIR:
        {
            register struct a {
                char *path;
                int mode;
            } *uap;
            copypath(up->uu_data);
            if (u.u_callno == UISACCES || u.u_callno == UISCHMOD) {
                uap = (struct a*)u.u_ap;
                up->uu_mode = uap->mode;
            }
            break;
        }
        
    case UISSACCT:
        {
            register caddr_t *uap = (caddr_t*)u.u_ap;

            if (*uap == 0)
                up->uu_data[0] = 0;
            else
                copypath(up->uu_data);
            break;
        }

    case UISLINK:
    case UISSYMLINK:
        {
            register struct a {
                char *s1, *s2;
            } *uap = (struct a*)u.u_ap;
            u.u_dirp = uap->s1;
            if (u.u_callno == UISSYMLINK)
                u.u_dirp++;
            copypath(up->uu_data);
            u.u_dirp = uap->s2;
            copypath(&up->uu_data[UIMAXPATH]);
            break;
        }

    case UISREADLINK:
        copypath(up->uu_data);
        break;

    case UISUTIME:
        {
            register struct a {
                char *path;
                time_t **t;
            } *uap = (struct a*)u.u_ap;
            copypath(up->uu_data);
            up->uu_mode = 0;
            if (uap->t) {
                copyin(uap->t, &up->uu_data[UIMAXPATH], 2*sizeof(time_t));
                up->uu_mode = 1;
            }
            break;
        }

    case UISCHOWN:
        {
            register struct a {
                char *path;
                int owner, group;
            } *uap = (struct a*)u.u_ap;
            copypath(up->uu_data);
            up->uu_argi[0] = uap->owner;
            up->uu_argi[1] = uap->group;
            break;
        }

    case UISDUP:
        {
            register int *uap = (int*)u.u_ap;
            up->uu_fd = u.u_rofile[*uap];
            break;
        }

    case UISMKNOD:
        {
            register struct a {
                char* path;
                int mode;
                short dev, ndev1, ndev2, fd;
            } *uap = (struct a*)u.u_ap;
            copypath(up->uu_data);
            up->uu_mode = uap->mode;
            up->uu_dev = uap->dev;
            up->uu_argi[0] = uap->ndev1;
            up->uu_argi[1] = uap->ndev2;
            up->uu_fd = uap->fd;
            up->uu_cmask = u.u_cmask;
            break;
        }
        
    case UISOPEN:
        {
            register struct a {
                char* path;
                int oflag;
                int mode;
            } *uap;
            ret = 1;
            uap = (struct a*)u.u_ap;
            copypath(up->uu_data);
            up->uu_argi[0] = uap->oflag;
            up->uu_mode = uap->mode;
            up->uu_cmask = u.u_cmask;
            break;
        }

    case UISCREAT:
    case UISMKDIR:
        {
            register struct a {
                char *path;
                int mode;
            } *uap;
            ret = 1;
            uap = (struct a*)u.u_ap;
            copypath(up->uu_data);
            up->uu_mode = uap->mode;
            up->uu_cmask = u.u_cmask;
            break;
        }

    case UIICLOF:
        up->uu_fd = u.u_rval1;
        break;

    case UISCLOS:
        {
            register int *uap;
            ret = 1;
            uap = (int*)u.u_ap;
            up->uu_fd = u.u_rofile[*uap];
            break;
        }

    case UISFCNTL:
        {
            register struct a {
                int fd;
                int cmd;
                struct flock *arg;
            } *uap;
            ret = 1;
            uap = (struct a*)u.u_ap;
            up->uu_fd = u.u_rofile[uap->fd];
            up->uu_argi[0] = uap->cmd;
            if (uap->cmd == F_GETLK || uap->cmd == F_SETLK || uap->cmd == F_SETLKW)
                copyin((caddr_t)uap->arg, up->uu_data, sizeof(struct flock));
            else
                up->uu_argi[1] = (int)uap->arg;
            break;
        }

    case UISIOCTL:
        {
            register struct a {
                int fd;
                int mode;
                union {
                    int iarg;
                    caddr_t carg;
                } u;
            } *uap;
            uint errsave;
            
            ret = 1;
            uap = (struct a*)u.u_ap;
            errsave = u.u_error;
            up->uu_fd = u.u_rofile[uap->fd];
            up->uu_mode = uap->mode;
            up->uu_argl[0] = uap->u.iarg;
            copyin(uap->u.carg, up->uu_data, 1024);
            u.u_error = errsave;
            break;
        }
        
    case UISSEEK:
        {
            register struct a {
                int fd, offset, whence;
            } *uap = (struct a*)u.u_ap;
            up->uu_fd = u.u_rofile[uap->fd];
            up->uu_offset = uap->offset;
            up->uu_mode = uap->whence;
            break;
        }
        
    case UISLOCKF:
        {
            register struct a {
                int fd, mode, length;
            } *uap;         
            ret = 1;
            uap = (struct a*)u.u_ap;
            up->uu_fd = u.u_rofile[uap->fd];
            up->uu_mode = uap->mode;
            up->uu_rwcnt = uap->length;
            break;
        }

    default:
        u.u_error = EINVAL;
        break;
    }

    return ret;
}

uisoutsw(rbufid, rdev, ucount, node)
ushort rbufid;
dev_t rdev;
off_t ucount;
int node;
{
    register struct uupacket *up;
    struct mnpacket *pkptr;
    bufque *bq;

    bq = buf_pointer(rbufid);
    pkptr = (struct mnpacket*)bq->bq_pkptr;
    up = &pkptr->uu;
    
    u.u_error = up->uu_error;
    switch (u.u_callno) {
    case UIISEXEC:
    case UIICOPEN:
    case UISOPEN:
    case UISCREAT:
    case UISLINK:
    case UISULINK:
    case UISSTAT:
    case UISMKNOD:
    case UISCHMOD:
    case UISCHOWN:
    case UISCHDIR:
    case UISUTIME:
    case UISACCES:
    case UISSACCT:
    case UISEXEC:
    case UISCHROT:
    case UISSYMLINK:
    case UISREADLINK:
    case UISLSTAT:
    case UISRMDIR:
    case UISMKDIR:
        {
            register char *cp, *sp;
            register int i;

            if (u.u_error == EXPATH)
                u.u_dirp += up->uu_count;
            else if (u.u_error == EXSYMPATH) {
                toggle++;
                if (toggle >= 8) toggle = 0;
                sp = &symname[toggle << 8];
                i = UIMAXPATH;
                cp = up->uu_data;
                do {
                    if ((*sp++ = *cp++) == '\0') break;
                } while (--i != 0);
                if (i == 0) {
                    u.u_error = E2BIG;
                    break;
                }
                u.u_dirp = &symname[toggle<<8];
            }
        }
        break;
    }

    switch(u.u_callno) {
    case UISFSTAT:
        {
            register struct a {
                int fd;
                struct stat *stat;
            } *uap;
            register struct stat *st;
            register short mode;
            
            if (u.u_error == 0) {
                st = (struct stat*)up->uu_data;
                mode = st->st_mode & S_IFMT;
                if (mode == S_IFDIR || mode == S_IFREG)
                    st->st_rdev = rdev;
                 uap = (struct a*)u.u_ap;
                 copyout(up->uu_data, (caddr_t)uap->stat, sizeof(struct stat));
            }
            break;
        }
    
    case UISFCNTL:
        {
            register struct a {
                int fd;
                int cmd;
                struct flock *arg;
            } *uap = (struct a*)u.u_ap;
            if (u.u_error == 0)
                u.u_rrarg = up->uu_argi[0];
            if (uap->cmd == F_GETLK)
                copyout(up->uu_data, (caddr_t)uap->arg, sizeof(struct flock));
            break;
        }
    
    case UISLOCKF:
        break;

    case UISIOCTL:
        {
            register struct a {
                int fd;
                int mode;
                union {
                    int iarg;
                    caddr_t carg;
                } u;
            } *uap = (struct a*)u.u_ap;
            if (uap->u.carg && u.u_error==0)
                copyout(up->uu_data, uap->u.carg, up->uu_count);
            break;
        }
    
    case UISSTAT:
    case UISLSTAT:
        {
            register struct a {
                char *path;
                struct stat *stat;
                short *args;
            } *uap;
            register struct stat *st;
            register short mode;
            
            if (u.u_error == 0) {
                st = (struct stat*)&up->uu_data[UIMAXPATH];
                if ((mode = st->st_mode & S_IFMT)==S_IFDIR || mode == S_IFREG)
                    st->st_rdev = rdev;
                uap = (struct a*)u.u_ap;
                copyout(&up->uu_data[UIMAXPATH], (caddr_t)uap->stat, sizeof(struct stat));
                if (up->uu_mode) {
                    suword(&uap->args[0], up->uu_argi[0]);
                    suword(&uap->args[1], up->uu_argi[1]);
                    suword(&uap->args[2], up->uu_dev);
                }
            }
        break;
        }

    case UISCHDIR:
        if (u.u_error == 0)
            u.u_cdirdev = rdev;
        break;

    case UISCHROT:
        if (u.u_error == 0)
            u.u_crootdev = rdev;
        break;

    case UIICOPEN:
    case UIISEXEC:
    case UISEXEC:
        if (u.u_error == 0) {
            u.u_execp = up->uu_ip;
            bcopy(up->uu_data, (caddr_t)u.u_dent.d_name, DIRSIZ);
            if (u.u_callno == UIISEXEC) {
                u.u_rmt_id = rdev;
                u.u_rmt_dev = up->uu_dev;
                u.u_rmt_ino = up->uu_fd;
                u.u_rmt_mode = up->uu_mode;
            }
            u.u_idflag = up->uu_mode >> 8;
            u.u_isid = up->uu_argl[0];
        }
        break;

    case UIIGUID:
        if (u.u_error == 0) {
            u.u_rrflag = up->uu_mode >> 8;
            u.u_munet = up->uu_argl[0];
        }
        break;

    case UIIREAD:
        {
            int nread;
            
            if (u.u_error == 0) {
                nread = min(up->uu_count, ucount);
                buf_copy(rbufid, up->uu_data, u.u_base, nread, u.u_segflg, 1);
                u.u_base += nread;
                u.u_offset += nread;
                u.u_count -= nread;
            }
        }
        break;


    case UIIWRITE:
        {
            int nwrite = min(u.u_count, ucount);
            u.u_base += nwrite;
            u.u_offset += nwrite;
            u.u_count -= nwrite;
        }
        break;

    case UIICCLO:
    case UIIICLO:
        u.u_execp = 0;
        break;
    
    case UISOPEN:
    case UISCREAT:
        if (u.u_error == 0) {
            u.u_rrarg = up->uu_fd;
            if (WHAT_VERSION(node) >= OPTIMIZED_NOFR) {
                if ((up->uu_mode & IFMT)==IFIFO)
                    u.u_rrflag = EBUSY;
                else
                    u.u_rrflag = 0;
            }
        }
        break;

    case UISMKDIR:
        if (u.u_error == 0)
            u.u_rrflag = 0;
        break;

    case UISCLOS:
        {
            register int *uap = (int*)u.u_ap;
            if (u.u_error == 0)
                u.u_rofile[*uap] = 0;
            break;
        }
    
    case UISSEEK:
        {
            register int *uap = (int*)u.u_ap;
            if (u.u_error == 0) {
                u.u_rval1 = up->uu_offset;
                u.u_ofile[*uap]->f_offset = up->uu_offset;
            }
            break;
        }
    
    case UISREADLINK:
        {
            register struct a {
                char *fname;
                char *buf;
                int bufsiz;
            } *uap = (struct a*)u.u_ap;
            if (u.u_error == 0)
                buf_copy(rbufid, up->uu_data, uap->buf,
                    (u.u_rval1=(uint)min(uap->bufsiz, up->uu_rwcnt)), 0, 1);
            break;
        }
    
    case UISREAD:
    case UISWRITE:
    case UISLREAD:
    case UISLWRITE:
    case UISGETDENTS:
        {
            int nread; 
            u.u_pofile[u.u_munet] &= ~FINOD;
            if (u.u_error == 0) {
                u.u_pofile[u.u_munet] |= (up->uu_argi[0] & FAPPEND);
                if (u.u_callno == UISREAD || u.u_callno==UISLREAD || u.u_callno==UISGETDENTS) {
                    buf_copy(rbufid, up->uu_data, u.u_base, up->uu_count, 0, 1);
                    nread = up->uu_count;
                    u.u_base += nread;
                    u.u_offset += nread;
                    u.u_count -= nread;
                } else {
                    nread = min(u.u_count, ucount);
                    u.u_base += nread;
                    u.u_offset += nread;
                    u.u_count -= nread;
                }
                u.u_ofile[u.u_munet]->f_offset = up->uu_offset;
            }
            break;
        }

    case UIICLOF:
    case UISULINK:
    case UISCHMOD:
    case UISCHOWN:
    case UISACCES:
    case UISRMDIR:
        break;

    case UIIPASS:
    case UIIINCR:
        childrecv_ptno = up->uu_altport;
        break;

    case UIIACCTW:
    case UISLINK:
    case UISMKNOD:
    case UISUTIME:
    case UISSYMLINK:
        break;

    case UISSACCT:
        if (u.u_error == 0)
            acctp = up->uu_ip;
        break;
    
    case UISDUP:
        if (u.u_error == 0)
            u.u_rrarg = up->uu_fd;
        break;

    default:
        break;
    }
}

/* process received packet */
uiprdsw()
{
    register int dummy, node;
    struct inode *ip;
    struct uupacket *up;

#define UUHEADSZ (sizeof(struct uupacket)-UIMAXDATA_NEW_FRAG)
    struct pkt {
        char    head[UUHEADSZ];
        char    data[UIMAXPATH];
    } pkt;
    struct uupacket *pp;
    int callno;
    int limit;
    
    struct a {          /* see uipacket.c */
        struct uupacket *up;
        int cmd;
    } *uap;

    pp = (struct uupacket*)&pkt;        /* uupacket + UIMAXPATH payload */
    uap = (struct a*)u.u_ap;
    up = uap->up;
    
    callno = (short)fuword(&up->uu_callno);
    node = uifindnode3(fulong(&up->uu_node));
    switch (callno) {
    case UISSACCT:
        copyin(uap->up, pp, sizeof(struct pkt));
        u.u_callno = UISSACCT;
        u.u_dirp = pp->uu_data;
        switch (pp->uu_data[0]) {
        case 0:                         /* no path */
            if ((ulong)pp->uu_ip < SYSVA)
                u.u_error = EFAULT;
            if (u.u_error) break;
            if (pp->uu_ip) {            /* release inode */
                plock(pp->uu_ip);
                iput(pp->uu_ip);
            }
            break;
        default:
            {
                struct inode *ip2 = namei(schar, 0);
                if (ip2 == 0) break;
                if ((ip2->i_mode & IFMT) != IFREG)
                    u.u_error = EACCES;
                else
                    access(ip2, 0x80);
                if (u.u_error) {
                    iput(ip2);
                    break;
                }
                sulong(&up->uu_ip, ip2);
                prele(ip2);
                break;
            }
        }
        
        suword(&up->uu_error, u.u_error);
        break;

    case UIIACCTW:
        {
            off_t size;
            
            copyin(uap->up, pp, UUHEADSZ+sizeof(struct acct));
            if ((ulong)pp->uu_ip < SYSVA)
                u.u_error = EFAULT;
            if (u.u_error) {
                suword(&up->uu_error, u.u_error);
                break;
            }
            if ((ip = pp->uu_ip) == 0) break;

            if (ip->i_flag & ILAND) {
                suword(&up->uu_error, EXDEV);
                break;
            }
            plock(ip);
            size = ip->i_size;
            u.u_offset = size;
            u.u_base = pp->uu_data;
            u.u_count = sizeof(struct acct);
            u.u_segflg = 1;
            u.u_error = 0;
            u.u_limit = 5000;
            u.u_fmode = FWRITE;
            writei(ip);
            if (u.u_error)
                ip->i_size = size;
            prele(ip);
            suword(&up->uu_error, u.u_error);
            break;
        }

    case UIICCLO:
    case UIIICLO:
        ip = (struct inode*)fulong(&up->uu_ip);
        if ((ulong)ip < SYSVA)
            u.u_error = EFAULT;
        if (u.u_error) {
            suword(&up->uu_error, u.u_error);
            break;
        }
        if (ip) {
            plock(ip);
            iput(ip);
        } else
            printf("uiprdsw: tried to do iput with ip = NULL\n");
        break;

    case UIIGUID:
        ip = (struct inode*)fulong(&up->uu_ip);
        if ((ulong)ip < SYSVA)
            u.u_error = EFAULT;
        if (u.u_error) {
            suword(&up->uu_error, u.u_error);
            break;
        }
        if (ip) {
            suword(&up->uu_mode, ip->i_mode);
            suword(&up->uu_argi[0], ip->i_uid);
            suword(&up->uu_argi[1], ip->i_gid);
        } else
            u.u_error = EFAULT;
        break;

    case UIIREAD:
        {
            int cnt;

            u.u_base = (caddr_t)up->uu_data;
            u.u_offset = fulong(&up->uu_offset);
            cnt = u.u_count = fuword(&up->uu_count);
            u.u_segflg = 0;
            u.u_callno = UIIREAD;
            ip = (struct inode*)fulong(&up->uu_ip);
            if ((ulong)ip < SYSVA)
                u.u_error = EFAULT;
            if (u.u_error) {
                suword(&up->uu_error, u.u_error);
                break;
            }
            if (ip) {
                plock(ip);
                readi(ip);
                prele(ip);
                suword(&up->uu_datalen, uisizeswx(UIIREAD, node, cnt));
                if (u.u_count != 0)
                    u.u_error = EIO;
            } else
                u.u_error = EFAULT;
            suword(&up->uu_error, u.u_error);
            break;
        }

    case UIIWRITE:
        limit = u.u_limit;
        u.u_limit = maxmem << 2;
        u.u_base = up->uu_data;
        u.u_offset = fulong(&up->uu_offset);
        u.u_count = fuword(&up->uu_count);
        u.u_segflg = 0;
        u.u_callno = UIIWRITE;
        ip = (struct inode*)fulong(&up->uu_ip);
        if ((ulong)ip < SYSVA)
            u.u_error = EFAULT;
        if (u.u_error) {
            suword(&up->uu_error, u.u_error);
            break;
        }
        if (ip) {
            plock(ip);
            writei(ip);
            prele(ip);
        } else
            u.u_error = EFAULT;
        suword(&up->uu_error, u.u_error);
        u.u_limit = limit;
        break;

    default:
        break;
    }
}

/* process write packet */
uipwtsw()
{
    register int i, node;
    register struct inode *ip;
    register int *unused1;
    register struct file *fp;

    int uerror;
    int unused2;
    int callno;
    struct uupacket *up;
    struct a {
        struct uupacket *up;
    } *uap = (struct a*)u.u_ap;

    up = uap->up;
    callno = (short)fuword(&up->uu_callno);
    uerror = fuword(&up->uu_error);
    node = uifindnode3(fulong(&up->uu_node));
    switch (callno) {
    case UIISEXEC:
    case UISEXEC:
        if (uerror)
            sulong(&up->uu_ip, 0);
        else {
            u.u_dirp = up->uu_data;
            u.u_callno = UIIEXECN;
            ip = namei(uchar, 0);
            if (ip) {
                if (access(ip, 0x40)) {
                    iput(ip);
                    ip = 0;
                } else {
                    if ((ip->i_mode & IFMT) != IFREG || !(ip->i_mode & (ICHG|IMOUNT|ILOCK))) {
                        iput(ip);
                        ip = 0;
                        u.u_error = EACCES;
                    } else
                        prele(ip);
                }
            }
            u.u_callno = UISDOIT;
            sulong(&up->uu_ip, ip);
            if (ip) {
                if (callno == UIISEXEC) {
                    suword(&up->uu_dev, ip->i_dev);
                    suword(&up->uu_fd, ip->i_number);
                }
                suword(&up->uu_mode, ip->i_mode);
                suword(&up->uu_argi[0], ip->i_uid);
                suword(&up->uu_argi[1], ip->i_gid);
            }
            if ((uerror = u.u_error) == 0)
                copyout(u.u_dent.d_name, up->uu_data, DIRSIZ);
            suword(&up->uu_error, u.u_error);
        }
        break;

    case UIICOPEN:
        if (uerror)
            sulong(&up->uu_ip, 0);
        else {
            if (u.u_uid != u.u_ruid) {
                u.u_callno = UISDOIT;
                suword(&up->uu_error, EACCES);
                sulong(&up->uu_ip, 0);
                return;
            }
            u.u_dirp = up->uu_data;
            u.u_callno = UIICOPEN;
            ip = namei(uchar, 1);
            if (ip == 0 && u.u_error == 0)
                ip = maknode(0666);
            if (ip && u.u_error == 0 && access(ip, 0x80)==0 &&
                    ((ip->i_mode & IFMT) == IFREG)) {
                itrunc(ip);
                prele(ip);
            } else {
                if (ip)
                    iput(ip);
                if (u.u_error == 0)
                    u.u_error = EACCES;
                ip = 0;
            }
            u.u_callno = UISDOIT;
            suword(&up->uu_error, u.u_error);
            sulong(&up->uu_ip, ip);
            uerror = u.u_error;
        }
        break;

    case UIIURUN:
        for (i = 3; i < NOFILE; i++) {
            if (u.u_ofile[i]) {
                closef(u.u_ofile[i]);
                u.u_ofile[i] = 0;
                u.u_rofile[i] = 0;
                u.u_pofile[i] = 0;
            }
        }
        if (u.u_error == 0) {
            for (i = 3; i != 0; i--) {
                u.u_ofile[i] = u.u_ofile[i-1];
                u.u_rofile[i] = u.u_rofile[i-1];
                u.u_pofile[i] = u.u_pofile[i-1];
            }
            u.u_ofile[0] = 0;
            u.u_rofile[0] = 0;
            u.u_pofile[0] = 0;
            ip = rootdir;
            fp = falloc(ip, 1);
            if (!fp) break;
            ip->i_count++;
        }
        break;

    case UIIXRUN:
        if (uerror) {
            closef(u.u_ofile[0]);
            for (i = 0; i<3; i++) {
                u.u_ofile[i] = u.u_ofile[i+1];
                u.u_rofile[i] = u.u_rofile[i+1];
                u.u_pofile[i] = u.u_pofile[i+1];
            }
            u.u_ofile[3] = 0;
            u.u_rofile[3] = 0;
            u.u_pofile[3] = 0;
        }
        break;

    default:
        break;
    }
    
    switch (callno) {
    case UIISEXEC:
    case UIICOPEN:
    case UISOPEN:
    case UISCREAT:
    case UISLINK:
    case UISULINK:
    case UISCHDIR:
    case UISMKNOD:
    case UISCHMOD:
    case UISCHOWN:
    case UISSTAT:
    case UISUTIME:
    case UISACCES:
    case UISSACCT:
    case UISEXEC:
    case UISCHROT:
    case UISSYMLINK:
    case UISREADLINK:
    case UISLSTAT:
    case UISRMDIR:
    case UISMKDIR:
        if (uerror == EXPATH)
            suword(&up->uu_count, (short)u.u_munet);
        break;
    }
}

int uifirst(callno)
int callno;
{
    static int unused;                  /* is declared in object file */
    
    switch (callno) {
    case UIISIGNAL:
    case UIIIPC:
    case UIIGUARD:
    case UIIWRITE:
    case UIICCLO:
    case UIICOPEN:
    case UIIEXECN:
    case UIIEXEC:
    case UIIGUID:
    case UIIQUIT:
    case UIIXRUN:
    case UIIURUN:
    case UIICONNVERS:
    case UIIXWHO:
    case UIIUWHO:
    case UIIPASS:
    case UIIDECR:
    case UIIINCR:
    case UIIXONN:
    case UIIICLO:
    case UIICLOF:
    case UIICONN:
    case UISREAD:
    case UISWRITE:
    case UISCLOS:
    case UISDUP:
    case UISFCNTL:
    case UISIOCTL:
    case UISLREAD:
    case UISLWRITE:
    case UISGETDENTS:
        return 0;
    default:
        return 1;
    }
}
