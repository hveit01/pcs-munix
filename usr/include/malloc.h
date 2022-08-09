/*	@(#)malloc.h	1.2	*/

/*
	Constants defining mallopt operations
*/
#define M_MXFAST	1	/* set size of blocks to be fast */
#define M_NLBLKS	2	/* set number of block in a holding block */
#define M_GRAIN		3	/* set number of sizes mapped to one, for
				   small blocks */
#define M_KEEP		4	/* retain contents of block after a free until
				   another allocation */
/*
	structure filled by 
*/
struct mallinfo  {
	long arena;     /* total space in arena */
	long ordblks;   /* number of ordinary blocks */
	long smblks;    /* number of small blocks */
	long hblks;     /* number of holding blocks */
	long hblkhd;	/* space in holding block headers */
	long usmblks;	/* space in small blocks in use */
	long fsmblks;	/* space in free small blocks */
	long uordblks;	/* space in ordinary blocks in use */
	long fordblks;	/* space in free ordinary blocks */
	long keepcost;	/* cost of enabling keep option */
};	

char *malloc(),*lmalloc();
void free();
char *realloc(),*lrealloc();
int mallopt();
struct mallinfo mallinfo();
