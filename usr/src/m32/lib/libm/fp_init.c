/*pcs*/

/* defines for FP CTRL register */
#define FPCR_BSUN   0x8000
#define FPCR_SNAN   0x4000
#define FPCR_OPERR  0x2000
#define FPCR_OVFL   0x1000
#define FPCR_UNFL   0x0800
#define FPCR_DZ     0x0400
#define FPCR_INEX2  0x0200
#define FPCR_INEX1  0x0100

#define FPCR_PREC_E 0x0000
#define FPCR_PREC_S 0x0040
#define FPCR_PREC_D 0x0080
#define FPCR_RND_N  0x0000
#define FPCR_RND_Z  0x0010
#define FPCR_RND_MI 0x0020
#define FPCR_RND_PI 0x0030

#define __SYSCALL(N)    int no = N, dummy; 0x4e4e

fp_init()
{
    __SYSCALL(67);      /* m881_used */
    m881_put_cntrl(FPCR_BSUN|FPCR_SNAN|FPCR_OPERR|FPCR_OVFL|FPCR_DZ| FPCR_RND_Z);
}
