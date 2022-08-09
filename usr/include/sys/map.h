/* @(#)map.h	6.4 */
typedef struct map {
	unsigned long   m_size;
	unsigned long   m_addr;
} map_t;

extern struct map pgmap[], sptmap[], listmap[], dmamap[], dma18map[];

#define	mapstart(X)	&X[1]
#define	mapwant(X)	X[0].m_addr
#define	mapsize(X)	X[0].m_size
#define	mapdata(X)	{(X)-2, 0} , {0, 0}
#define	mapinit(X, Y)	X[0].m_size = (Y)-2
