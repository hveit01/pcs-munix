 /* #ident  "@(#)head:nan.h 1.10"       */
/* Handling of Not_a_Number's (only in IEEE floating-point standard) */
#if defined(FFP) || defined(IEEE)         /* make IEEE default */
#else
#define IEEE 1
#endif

#ifdef IEEE
typedef union 
{
         struct	
	 {
	    unsigned sign     : 1;
	    unsigned exponent :11;
            unsigned bits:20;
	    unsigned fraction_low:32;
         } inf_parts;
	 struct 
	 {
	    unsigned sign     : 1;
            unsigned exponent :11;
	    unsigned qnan_bit : 1;
	    unsigned bits     :19;
	    unsigned fraction_low: 32;
         } nan_parts;
         double d;

} dnan; 

	/* IsNANorINF checks that exponent of double == 2047 *
	 * i.e. that number is a NaN or an infinity	     */
	
#define IsNANorINF(X)  (((dnan *)&(X))->nan_parts.exponent == 0x7ff)

	/* IsINF must be used after IsNANorINF		*
 	 * has checked the exponent 			*/

#define IsINF(X)  (((dnan *)&(X))->inf_parts.bits == 0 &&  \
                    ((dnan *)&(X))->inf_parts.fraction_low == 0)

	/* IsPosNAN and IsNegNAN can be used 		*
 	 * to check the sign of infinities too		*/

#define IsPosNAN(X)  (((dnan *)&(X))->nan_parts.sign == 0)

#define IsNegNAN(X)  (((dnan *)&(X))->nan_parts.sign == 1)

	/* GETNaNPC gets the leftmost 32 bits 		*	
	 * of the fraction part				*/

#define GETNaNPC(dval)   (((dnan *)&(dval))->inf_parts.bits << 12 | \
			  ((dnan *)&(dval))->nan_parts.fraction_low>> 20) 

#define KILLFPE()       (void) kill(getpid(), 8)
#define NaN(X)  (((dnan *)&(X))->nan_parts.exponent == 0x7ff)
#define KILLNaN(X)      if (NaN(X)) KILLFPE()

#else

typedef double dnan;
#define IsINF(X)   0
#define IsPINF(X)  0
#define IsNegNAN(X)  0
#define IsPosNAN(X)  0
#define IsNAN(X)   0
#define GETNaNPC(X)   0L

#define NaN(X)  0
#define KILLNaN(X)
#endif
