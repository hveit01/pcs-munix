#ifndef BADSIG
/*	@(#)signal.h	1.1	*/
#include<sys/signal.h>

extern  void(*signal())();
extern  void(*sigset())();
#define BADSIG  (int (*)())-1
#endif

