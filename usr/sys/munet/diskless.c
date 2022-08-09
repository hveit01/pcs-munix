/* PCS specific */
static char *_Version = "@(#) RELEASE:  1.3  Feb 05 1987 /usr/sys/munet/diskless.c ";

#include "sys/types.h"
#include "sys/param.h"
#include "sys/sysmacros.h"
#include "sys/systm.h"
#include "sys/proc.h"
#include "sys/inode.h"
#include "sys/ino.h"
#include "sys/dir.h"
#include "sys/file.h"
#include "sys/buf.h"
#include "sys/reg.h"
#include "sys/errno.h"
#include "sys/signal.h"
#include "sys/user.h"
#include "sys/var.h"
#include <sys/utsname.h>
#include <sys/port.h>
#include <sys/ether.h>
#include <sys/munet/munet.h>
#include <sys/munet/mnbuf.h>
#include <sys/munet/mnnode.h>
#include <sys/munet/diskless.h>
#include <sys/munet/uarp.h>
#include "rbq.h"

extern struct buf rmttab;

dlreceive(bufid, dlp)
ushort bufid;
register struct dlpacket *dlp;
{
    register struct dltable *dlt;
    register int i;

    if (master) {
        switch (dlp->rt_command) {
        case SWALLOC:
        case SWALLOCCAT:
        case SWALLOCCL:
        case SWFREE:
        case RMTRBLK:
        case RMTWBLK:
        case DLALLOC:
        case DLFREE:
        case DLIALLOC:
        case DLIFREE:
        case DLWCSUP:
        case DLIREAD:
        case DLIUPDAT:
        case SWFREE_M32:
        case DLSHUTDOWN:
            if (dlp->rt_index < 0 || dlp->rt_index >= maxdlnactv) {
                St_write("dlreceive_RPCD9", 8, 2, -1, 0x15);
                return;
            }
            dlt = &dltable[dlp->rt_index];
            dlt->dl_lastin = time;
            if (!dlt->dl_flags) {
                St_write("dlreceive_RPCD10", 8, 2, -1, 0x15);
                return;
            }
            if (dlt->dl_flags & DL_PROCESSING) {
                St_write("dlreceive_RPCD11", 8, 2 ,-1, 0x16);
                return;
            }
            dlt->dl_flags |= DL_PROCESSING;
            if (dlp->rt_seqno == dlt->dl_seqno)
                dlt->dl_flags |= DL_RETRY;
            break;

        case DLINIT:
        case DLMPOLL:
        case DLMREAD:
        case DLBOOT:
            dlp->rt_index = -1;
            for (i=0; i< maxdlnactv; i++) {
                if (dltable[i].dl_flags==0) {
                    if (dlp->rt_index < 0)
                        dlp->rt_index = i;
                } else if (UISAME(&dlp->e_source, &dltable[i].dl_addr)) {
                    dlp->rt_index = i;
                    break;
                }
            }
            if (dlp->rt_index < 0)
                return;

            dlt = &dltable[dlp->rt_index];
            dlt->dl_flags |= DL_TEMP;
            dlt->dl_ipaddr = dlp->ip_srcaddr;
            dlt->dl_lastin = time;
            
            if (req_q == 0)
                wakeup((caddr_t*)&req_q);
            req_q |= (1 << dlp->rt_index);
            break;

        case DLMASTER:
        case DLCLEAR:
        default:
            return;
        }

        dlt->dl_addr = dlp->e_source;
        dlt->dl_bufid = bufid;
        dlt->dl_seqno = dlp->rt_seqno;
        if ((dlt->dl_flags & DL_RETRY)==0) {    /* no retry */
            
            switch (dlt->dl_cmd = dlp->rt_command) {
            case SWALLOC:
                dlt->dl_size = dlp->rt_size;
                break;

            case SWFREE_M32:
                dlt->dl_size = dlp->rt_size;
                bcopy(dlp->rt_data, dlt->dl_wbuffer, dlp->rt_size << 2);
                dlt->dl_wbufbase = -1;
                dlt->dl_dev = dlp->rt_dev;
                break;

            case SWALLOCCAT:
            case SWFREE:
                dlt->dl_size = dlp->rt_size;
                dlt->dl_start = dlp->rt_start;
                break;
            
            case SWALLOCCL:
                dlt->dl_size = dlp->rt_size;
                break;
                
            case DLMPOLL:
            case DLBOOT:
                break;
            
            case DLMREAD:
                if (dlt->dl_rootdev == -1)
                    dlp->rt_dev = rootdev;
                else
                    dlp->rt_dev = comrootdev;
                dlp->rt_pageio = 0;
                dlp->rt_size = FsBSIZE(dlt->dl_dev);
                /*FALLTHRU*/
            case RMTRBLK:
            case RMTWBLK:
                dlt->dl_dev = dlp->rt_dev;
                dlt->dl_blkno = dlp->rt_blkno;
                dlt->dl_swdev = dlp->rt_pageio; /*swap flag */
                if (dlt->dl_dev == -1)
                    dlt->dl_dev = swapdev;
                dlt->dl_size = dlp->rt_size;
                if (dlt->dl_cmd == RMTWBLK) {
                    if (dlt->dl_swdev == 1) {
                        if ((dlt->dl_blkno & 0xfffffffc) != dlt->dl_wbufbase)
                            dlt->dl_wbufbase = dlt->dl_blkno & 0xfffffffc;
                        if (FsBSIZE(dlt->dl_dev) > dlt->dl_size)
                            dlt->dl_size = FsBSIZE(dlt->dl_dev);
                        buf_copy(bufid, dlp->rt_data, 
                            &dlt->dl_wbuffer[(dlt->dl_blkno & 3) * FsBSIZE(dlt->dl_dev)],
                            dlt->dl_size, 1, 1);
                        dlt->dl_blkno += (dlt->dl_size >> FsBSHIFT(dlt->dl_dev)) - 1;
                    } else {
                        bcopy(dlp->rt_data, dlt->dl_wbuffer, FsBSIZE(dlt->dl_dev));
                    }
                }
                break;

            case DLINIT:
                dlt->dl_dev = dlp->rt_dev;
                dlt->dl_swdev = dlp->rt_swdev;
                break;

            case DLIALLOC:
                dlt->dl_dev = dlp->rt_dev;
                break;

            case DLALLOC:
                dlt->dl_wbufbase = -1;
                dlt->dl_dev = dlp->rt_dev;
                dlt->dl_size = dlp->rt_size;
                break;

            case DLWCSUP:
            case DLSHUTDOWN:
                dlt->dl_wbufbase = -1;
                dlt->dl_dev = dlp->rt_dev;
                dlt->dl_size = dlp->rt_size;
                bcopy(dlp->rt_data, dlt->dl_wbuffer, dlp->rt_size*sizeof(int));
                break;
                
            case DLIUPDAT:
                dlt->dl_wbufbase = -1;
                bcopy (dlp->rt_data, dlt->dl_wbuffer, sizeof(struct dinode)+3*sizeof(int));
                /*FALLTHRU*/
            case DLIFREE:
            case DLIREAD:
                dlt->dl_dev = dlp->rt_dev;
                dlt->dl_blkno = dlp->rt_blkno;
                break;

            case DLFREE:
                dlt->dl_dev = dlp->rt_dev;
                dlt->dl_size = dlp->rt_size;
                bcopy(dlp->rt_data, dlt->dl_wbuffer, dlp->rt_size << 2);
                break;
            }
        }
        St_write("dlreceive_NFSD4", 7, 2, dlp->rt_command, 0x0f, RTTYPE);
        wakeup((caddr_t*)&dltable[dlp->rt_index]);
    } else {                            /* not master */
        if ((rmttab.b_flags & B_DONE) || dlp->rt_command==DLBOOT || 
                dlp->rt_command==DLMPOLL || dlp->rt_seqno != rmttab.b_seqno) {
            if (dlp->rt_seqno != rmttab.b_seqno)
                St_write("dlreceive_RPCD12", 8, 2, -1, 0x17);
            else
                St_write("dlreceive_RPCD13", 8, 2, -1, 0x19);
            return;
        }
        if (rmttab.av_forw)
            rmtintr(0, bufid);
    }
}


chdirremrcv(dev)
int dev;
{
    static char *slash = "/";
    int callno;                         /* save callno */
    
    u.u_dirp = &slash[1];

    callno = u.u_callno;
    u.u_callno = 12;                    /* chdir */
    uisend(dev, 0);
    u.u_callno = callno;

    if (u.u_error)
        return 0;
    else {
        u.u_cdirdev = -1;
        return 1;
    }
}
