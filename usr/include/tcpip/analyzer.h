

#define MAX_NR_FRAGMENTS        20
#define MAX_NR_OPEN_FILES       20
#define MIN_BUFFERSIZE          500

struct openfile {
	char    *fname;         /* Pointer to argv-filename */
	int     fd;             /* file descriptor */
	boolean f_eof;          /* end of file reached ? */
	boolean preeof;         /* any premature eof occurred ? */
	long    nexttime;       /* timestamp of next record in file */
	long    sequ_nbr;       /* internal unique packet number */
	int     lostpack;       /* number of lost packets */
	int     packlen;        /* size of next packet to read */
	long    read_records;   /* nr of recors read from this file */
	};

struct fragment {
	char    *fptr;         /* pointer to malloced fragment buffer or NULL*/
	Uint    next_write;    /* offset in Buffer where next fragment goes */
	Uint    ident;         /* correllation between incoming fragments */
	};
