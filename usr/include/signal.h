#ifndef BADSIG			# if signal.h already read in
/*	@(#)signal.h	1.1	*/
#include<sys/signal.h>

extern	(*signal())();
#define BADSIG  (int (*)())-1
#endif

