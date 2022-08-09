/* pcs */

m881_get_cntrl()
{
    0xf200; 0xb000; /* fmove.l fpcr, d0 */
}

m881_put_cntrl(ctrl)
register int ctrl;
{
    0xf207; 0x9000; /* fmove.l d7, fpcr */
}

m881_get_status()
{
    0xf200; 0xa800; /* fmove.l fpsr, d0 */
}

m881_put_status(status)
register int status;
{
    0xf207; 0x8800; /* fmove.l d7, fpsr */
}

m881_get_iar()
{
    0xf200; 0xa400; /* fmove.l fpiar, d0 */
}
