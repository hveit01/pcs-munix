/* @(#)var.h	6.5 */
struct var {
	int	v_buf;
	int	v_call;
	int	v_inode;
	char *	ve_inode;
	int	v_file;
	char *	ve_file;
	int	v_mount;
	char *	ve_mount;
	int	v_proc;
	char *	ve_proc;
	int	v_region;
	int	v_clist;
	int	v_maxup;
	int	v_hbuf;
	int	v_hmask;
	int	v_pbuf;
	int	v_vhndfrac;	/* fraction of maxmem to set a limit for 
				   running vhand. see getpages and clock */
	int	v_maxpmem;	/* The maximum physical memory to use.
				   If v_maxpmem == 0, then use all
				   available physical memory.
				   Otherwise, value is amount of mem to
				   use (specified in pages).             */
	short   v_nflocks;      /*  NFLOCKS */
	int	v_autoup;	/* The age a delayed-write buffer must	*/
				/* have in seconds before bdflush will    */
				/* write it out.			*/
	int	v_nqueue;	/* Nbr of streams queues.		*/
	int	v_nstream;	/* Number of stream head structures.	*/
	int	v_nblk4096;	/* Number of 4096 bytes stream buffers.	*/
	int	v_nblk2048;	/* Number of 2048 bytes stream buffers.	*/
	int	v_nblk1024;	/* Number of 1024 bytes stream buffers.	*/
	int	v_nblk512;	/* Number of 512 bytes stream buffers.	*/
	int	v_nblk256;	/* Number of 256 bytes stream buffers.	*/
	int	v_nblk128;	/* Number of 128 bytes stream buffers.	*/
	int	v_nblk64;	/* Number of 64 bytes stream buffers.	*/
	int	v_nblk16;	/* Number of 16 bytes stream buffers.	*/
	int	v_nblk4;	/* Number of 4 bytes stream buffers.	*/
};
extern struct var v;
