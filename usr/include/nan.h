/*	@(#)nan.h	1.2	*/
/* Handling of Not_a_Number's (only in IEEE floating-point standard) */

#define KILLFPE()	(void) kill(getpid(), 8)
#define KILLNaN(X)	if (NaN(X)) KILLFPE()

#if FFP || IEEE         /* make FFP default */
#else
#define FFP 1
#endif

#if     MOT_IEEE || NSC_IEEE
#define NaN(X)	(((union { double d; struct { unsigned :1, e:11; } s; } \
			*)&X)->s.e == 0x7ff)
#else
#ifdef  NSS_IEEE
#define NaN(X)  (((union { double d; struct { unsigned :1, e:8; } s; } \
			*)&X)->s.e == 0xff)
#else   /* FFP */
#define NaN(X) 0
#endif
#endif
