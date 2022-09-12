/* @(#)tuneable.h	1.1 */

typedef struct tune {
	int	t_gpgslo;	/* If freemem < t_getpgslow, then start	*/
				/* to steal pages from processes.	*/
	int	t_gpgshi;	/* Once we start to steal pages, don't	*/
				/* stop until freemem > t_getpgshi.	*/
	int	t_age;		/* Steal pages every t_age cpu seconds	*/
	int	t_vhandr;	/* Run vhand once every t_vhandr seconds*/
				/* if freemem < t_vhandl.		*/
	int	t_vhandl;	/* Run vhand once every t_vhandr seconds*/
				/* if freemem < t_vhandl.		*/
	int	t_maxsc;	/* The maximum number of contiguous	*/
				/* pages which will be swapped out in a	*/
				/* single operation.			*/
	int	t_prep;		/* Max. number of pre-loaded pages	*/
	int	t_bdflushr;	/* The rate at which bdflush is run in	*/
				/* seconds.				*/
	int	t_minarmem;	/* The minimum available resident (not	*/
				/* swapable) memory to maintain in 	*/
				/* order to avoid deadlock.  In pages.	*/
	int	t_minasmem;	/* The minimum available swapable	*/
				/* memory to maintain in order to avoid	*/
				/* deadlock.  In pages.			*/
	int	t_maxumem;	/* The maximum size of a user's virtual	*/
				/* address space in pages.		*/
} tune_t;

extern tune_t	tune;
