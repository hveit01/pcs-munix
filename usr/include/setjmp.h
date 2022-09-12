#ifndef _JBLEN

#define _JBLEN  9
typedef long jmp_buf[_JBLEN];
extern int setjmp();
extern void longjmp();

#endif
