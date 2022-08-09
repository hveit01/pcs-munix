/* @(#)nami.c   6.4 */
static char* _Version = "@(#) RELEASE:  1.3  May 13 1987 /usr/sys/os/nami.c ";

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/sysinfo.h>
#include <sys/inode.h>
#include <sys/mount.h>
#include <sys/dir.h>
#include <sys/proc.h>
#include <sys/signal.h>
#include <sys/user.h>
#include <sys/errno.h>
#include <sys/buf.h>
#include <sys/var.h>

extern schar(), uchar();
extern bcopy(), copyout();

#define MAXSYMNAME (MAXPATHLEN*NSYMBUF)
char symname[MAXSYMNAME];   /*pcs buffer for symlink components */
int toggle;                 /*pcs counter for symlink paths */

/*
 * Convert a pathname into a pointer to
 * an inode. Note that the inode is locked.
 *
 * func = function called to get next char of name
 *  &uchar if name is in user space
 *  &schar if name is in system space
 * flag = 0 if name is sought
 *  1 if name is to be created
 *  2 if name is to be deleted
 *  0x40 if no following of symlinks
 */
struct inode *
namei(func, flag)
register (*func)();
{
    register struct inode *dp;
    register c;
    register char *cp;
    register struct buf *bp;
    register i;
    register dev_t rdev;
    off_t eo, off;
    struct mount *mp;
    daddr_t bn;
    caddr_t dirp;
    char *cp1;
    int c0;
    dev_t dev;          /*pcs*/
    int nofollow;
    int symmax;         /*pcs max nesting */
    int nslash;         /*pcs max slashes in path */
    caddr_t dptgt;
    int (*cpfunc)();    /*pcs function to copy out result, sys or user */
    char *sp;           /*pcs pointer to symlink component */

    dptgt = u.u_dirp;
    cpfunc = func == schar ? bcopy : copyout;
    
    if (u.u_locate_flag == 1) {
        u.u_locate_flag = 0;
        if (u.u_namei_rv)
            plock(u.u_namei_rv);
        return u.u_namei_rv;
    }

    nofollow = (flag & DONT_FOLLOW) == 0;
    flag &= ~DONT_FOLLOW;
    symmax = 0;
    
    if (flag == 1)
        u.u_pdir = 0;

    /*
     * If name starts with '/' start from
     * root; otherwise start from current dir.
     */
    
    sysinfo.namei++;

retry:
    dirp = u.u_dirp;
    c = (*func)();
    if (c == 0) {
        u.u_error = ENOENT;
        return 0;
    }
    
    c0 = c;
    if (c == '/') {
        dp = u.u_rdir;
        if (dp == 0)
            dp = rootdir;
        while (c == '/')
            c = (*func)();
        
        if (c == '\0' && flag) {
            u.u_error = ENOENT;
            return 0;
        }
    } else
        dp = u.u_cdir;

    mp = dp->i_mount;
    if (iget(mp, dp->i_number) == 0)
        return 0;

    if ((c0 != '/' && (dev = u.u_cdirdev) != -1) ||
        (c0 == '/' && (dev = u.u_crootdev) != -1)) {
        if (c0 == '/') 
            u.u_dirp--;

        c = 0;
        if (u.u_callno != 9 &&
            u.u_callno != 0x53) {
            prele(dp);
            uisend(dev, 0);
            plock(dp);
            if (u.u_error == EXPATH) {
                mp = mp->m_omnt;
                u.u_error = 0;
                c = (*func)();
            } else {
                if (u.u_error == EXSYMPATH) {
                    if (++symmax > NSYMBUF) {
                        u.u_error = ELOOP;
                        goto out;
                    }
                    u.u_error = 0;
                    func = schar;
                    iput(dp);
                    goto retry;
                }
            }
        }
    }

cloop:
    /*
     * Here dp contains pointer
     * to last component matched.
     */

    if (u.u_error != 0)
        goto out;
        
    if (c == 0)
        return dp;
    
    /*
     * If there is another component,
     * gather up name into users' dir buffer.
     */
    
        
    cp1 = u.u_dirp-1;

    cp = &u.u_dent.d_name[0];
    while (c != '/' && c != '\0' && u.u_error==0) {
        if (cp < &u.u_dent.d_name[DIRSIZ])
            *cp++ = c;
        c = (*func)();
    }

    while (cp < &u.u_dent.d_name[DIRSIZ])
        *cp++ = '\0';

    for (nslash = 0; c == '/'; ) {
        nslash = 1;
        c = (*func)();
    }

    if (flag == 1 && c=='\0' && u.u_error == 0) {
        cp = &u.u_dent.d_name[0];
        while (cp < &u.u_dent.d_name[DIRSIZ]) {
            if (*cp++ & 0200)
                u.u_error = EFAULT;
        }
    }

seloop:
    /*
     * dp must be a directory and
     * must have X permission.
     */

    if ((dp->i_mode & IFMT) != IFDIR || dp->i_nlink==0)
        u.u_error = ENOTDIR;
    else
        access(dp, IEXEC);

    if (u.u_error)
        goto out;

    /*
     * set up to search a directory
     */
    u.u_offset = 0;
    u.u_count = dp->i_size;
    u.u_pbsize = 0;
    bp = 0;
    eo = 0;

    if (dp == u.u_rdir || dp == rootdir) {
        if (u.u_dent.d_name[0] == '.' &&
            u.u_dent.d_name[1] == '.' &&
            u.u_dent.d_name[2] == '\0' &&
            u.u_procp->p_flag & SFSERV) {
                
            if ( (symname > u.u_dirp || 
                  u.u_dirp >= &symname[MAXSYMNAME]) != 0) {
                    u.u_munet = cp1 - dirp;
                    u.u_error = EXPATH;
                    goto out;
                }
            
        }
    }

eloop:

    /*
     * If at the end of the directory,
     * the search failed. Report what
     * is appropriate as per flag.
     */

    if (u.u_count == 0) {
        if (bp)
            brelse(bp);
        if (flag == 1 && c=='\0') {
            if (access(dp, IWRITE))
                goto out;
            u.u_pdir = dp;
            if (eo)
                u.u_offset = eo;
            u.u_count = sizeof(struct direct);
            bmap(dp, 0);
            if (u.u_error)
                goto out;
            return 0;
        }
        u.u_error = ENOENT;
        goto out;
    }

    /*
     * Read the next directory block
     */
    if(bp != NULL)
        brelse(bp);
    sysinfo.dirblk++;
    bn = bmap(dp, B_READ);
    if (u.u_error)
        goto out;
    if (bn < 0) {
        u.u_error = EIO;
        goto out;
    }
    bp = bread(dp->i_dev, bn);
    if (u.u_error) {
        brelse(bp);
        goto out;
    }

    /*
     * Search directory block. searchdir() returns
     * offset of matching entry, or empty entry, or -1.
     */

    cp = bp->b_un.b_addr + u.u_pboff;
    switch (off = searchdir(cp, (uint)u.u_pbsize,  u.u_dent.d_name)) {
    default:
        cp += off;
        if ((u.u_dent.d_ino = ((struct direct*)cp)->d_ino))
            break;
        if (eo == 0)
            eo = u.u_offset + off;
    case -1:
        u.u_offset += u.u_pbsize;
        u.u_count -= u.u_pbsize;
        goto eloop;
    }

    /*
     * Here a component matched in a directory.
     * If there is more pathname, go back to
     * cloop, otherwise return.
     */
    u.u_offset += off + sizeof(struct direct);
    if (bp)
        brelse(bp);
    
    if (flag == 2 && c == '\0') {
        if (access(dp, IWRITE))
            goto out;
        return dp;
    }
    if (u.u_dent.d_ino == ROOTINO &&
        dp->i_number == ROOTINO &&
        u.u_dent.d_name[1] == '.') {
        if (mount != mp) {
            iput(dp);
            dp = mp->m_inodp;
            plock(dp);
            dp->i_count++;
            mp = mp->m_omnt;
            goto seloop;
        }
    }
    iput(dp);
    dp = iget(mp, u.u_dent.d_ino);
    if (dp == 0)
        return 0;

    mp = dp->i_mount;
    if ((dp->i_mode & IFMT) == IFLNK && (nofollow!=0 || nslash != 0)) {
        if (dp->i_size >= (MAXPATHLEN-1))
            u.u_error = E2BIG;
        
        if (++symmax > NSYMBUF)
            u.u_error = ELOOP;

        if (u.u_error)
            goto out;

        if (++toggle >= NSYMBUF)
            toggle = 0;

        sp = &symname[toggle << 8];

        u.u_base = sp;
        u.u_offset = 0;
        u.u_count = dp->i_size;
        u.u_segflg = 1;
        readi(dp);
        if (u.u_error)
            goto out;

        i = dp->i_size;
        cp = &sp[i];
        if (nslash) {
            *cp++ = '/';
            i++;
            while (c > 0) {
                if (++i <= (MAXPATHLEN-1))
                    *cp++ = c;
                c = (*func)();
            }
        }
        
        *cp++ = '\0';
        if (c < 0)
            u.u_error = EFAULT;
        else if (i >= (MAXPATHLEN-1))
            u.u_error = E2BIG;
        else if (*sp != '/')
                u.u_error = EINVAL;

        if (u.u_error == 0 &&
            sp[1] == '.' &&
            sp[2] == '.' &&
            (sp[3] == '/' || sp[3] == '\0')) {
            if ((u.u_procp->p_flag & SFSERV) != 0) {
                if (newversion(3) != 0) {
                    u.u_dirp = sp;
                    for (i = 1; *sp++ != 0 && i < MAXPATHLEN; i++);

                    if (i > 100)
                        u.u_error = E2BIG;
                    if (u.u_error == 0)
                        (*cpfunc)(u.u_dirp, dptgt, i);
                    if (u.u_error == 0)
                        u.u_error = EXSYMPATH;
                }
            }
        }
        if (u.u_error)
            goto out;
        func = schar;
        iput(dp);
        u.u_dirp = sp;
        goto retry;
    }
    
    if (onmaster(dp)) {
        u.u_dirp = cp1;
        rdev = masterdev();
        if (rdev == -1)
            u.u_error = ENOENT;

        if (u.u_error )
            goto out;

        for (mp = &mount[1]; mp < (struct mount*)v.ve_mount; mp++) {
            if (mp->m_flags == MINUSE &&
                (mp->m_inodp->i_flag & ILAND) &&
                mp->m_dev == rdev) {
                iput(dp);
                c = '\0';
                if (u.u_callno != 9 && u.u_callno != 0x53) {
                    if (fsbyte(dirp) != '/') {
                        chdirremrcv(rdev);
                        u.u_dirp = &cp1[1];
                    }
                    uisend(rdev, 0);
                    if (u.u_error == EXPATH)
                        u.u_error = ENOENT;
                    if (u.u_error == EXSYMPATH) {
                        if (++symmax > NSYMBUF) {
                            u.u_error = ELOOP;
                            return 0;
                        }
                        u.u_error = 0;
                        func = schar;
                        goto retry;
                    }
                }
                dp = mp->m_inodp;
                plock(dp);
                dp->i_count++;
                break;
            }
        }
        goto cloop; 
    }

    if (u.u_error == 0 && (dp->i_flag & ILAND) != 0) {
        c = 0;
        if (nslash)
            u.u_dirp--;
        if (u.u_callno != 9 && u.u_callno != 0x53) {
            prele(dp);
            uisend(dp->i_mount->m_dev, 0);
            plock(dp);
            if (u.u_error == EXPATH) {
                mp = mp->m_omnt;
                u.u_error = 0;
                c = (*func)();
            } else if (u.u_error == EXSYMPATH) {
                if (++symmax > NSYMBUF) {
                    u.u_error = ELOOP;
                    goto out;
                }
                u.u_error = 0;
                func = schar;
                iput(dp);
                goto retry;
            }
        }
    }
    goto cloop; 

out:
    iput(dp);
    return 0;
}
