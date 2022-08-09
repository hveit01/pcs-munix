#ifdef print

#define _PRINTF(s)              printf(s)
#define _PRINTF1(s,a1)          printf(s,a1)
#define _PRINTF2(s,a1,a2)       printf(s,a1,a2)

#else

#define _PRINTF(s)
#define _PRINTF1(s,a1)
#define _PRINTF2(s,a1,a2)

#endif
