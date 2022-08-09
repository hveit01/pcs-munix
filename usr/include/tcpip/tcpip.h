/****************************************************************
*       general  T C P / I P  include file                      *
*                                                               *
* The follwing include file defines various items used through- *
* out the whole tcp/ip software                                 *
*                                                               *
* change history :                                              *
*       01/09/86 josch  original version                        *
****************************************************************/


/* make an unique ioctl code for manager and function */
#define IOC(man,fct)    (((10 + man) << 8) + fct)


/* some macros used for FAST byte-transfers */

#define bytemove6(src,dst)      *(long *)(dst) = *(long *)(src), \
		       *(short *)((long)(dst)+4L) = *(short *)((long)(src)+4L)


#define bytemove12(src,dst)     *(long *)(dst) = *(long *)(src), \
		       *(long *)((long)(dst)+4L) = *(long *)((long)(src)+4L), \
		       *(long *)((long)(dst)+8L) = *(long *)((long)(src)+8L)
