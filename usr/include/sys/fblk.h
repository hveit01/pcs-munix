/* @(#)fblk.h	6.1 */
struct	fblk
{
	short   df_nfree;
	daddr_t	df_free[NICFREE];
};
