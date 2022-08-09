/*      @(#)values.h    1.6     */
#ifndef BITSPERBYTE
/* These values work with any binary representation of integers
 * where the high-order bit contains the sign */

/* A number used normally for size of a shift */
#define BITSPERBYTE	8
#define BITS(type)	(BITSPERBYTE * (int)sizeof(type))

/* Short, regular and long ints with only the high-order bit turned on */
#define HIBITS	((short)(1 << BITS(short) - 1))
#define HIBITI	(1 << BITS(int) - 1)
#define HIBITL	(1L << BITS(long) - 1)

/* Largest short, regular and long int */
#define MAXSHORT        ((short)~HIBITS)
#define MAXINT	(~HIBITI)
#define MAXLONG	(~HIBITL)

/* Various values which describe the binary floating-point representation
 * _EXPBASE	- the exponent base
 * DMAXEXP 	- the maximum exponent of a double (as returned by frexp())
 * FMAXEXP 	- the maximum exponent of a float  (as returned by frexp())
 * DMINEXP 	- the minimum exponent of a double (as returned by frexp())
 * FMINEXP 	- the minimum exponent of a float  (as returned by frexp())
 * MAXDOUBLE	- the largest double
			((_EXPBASE ** DMAXEXP) * (1 - (_EXPBASE ** -DSIGNIF)))
 * MAXFLOAT	- the largest float
			((_EXPBASE ** FMAXEXP) * (1 - (_EXPBASE ** -FSIGNIF)))
 * MINDOUBLE	- the smallest double (_EXPBASE ** (DMINEXP - 1))
 * MINFLOAT	- the smallest float (_EXPBASE ** (FMINEXP - 1))
 * DSIGNIF	- the number of significant bits in a double
 * FSIGNIF	- the number of significant bits in a float
 * DMAXPOWTWO	- the largest power of two exactly representable as a double
 * FMAXPOWTWO	- the largest power of two exactly representable as a float
 * _IEEE	- 1 if IEEE standard representation is used
 * _DEXPLEN	- the number of bits for the exponent of a double
 * _FEXPLEN	- the number of bits for the exponent of a float
 * _HIDDENBIT	- 1 if high-significance bit of mantissa is implicit
 * LN_MAXDOUBLE	- the natural log of the largest double  -- log(MAXDOUBLE)
 * LN_MINDOUBLE	- the natural log of the smallest double -- log(MINDOUBLE)
 */
#if FFP || IEEE         /* make FFP default */
#else
#define FFP 1
#endif

#ifdef FFP
#define MAXDOUBLE       0.9223317e19
#define MAXFLOAT        0.9223317e19
#define _DEXPLEN        7
#define _FEXPLEN        7
#define _HIDDENBIT      0
#define DMINEXP (-(DMAXEXP+1))
#define FMINEXP (-(FMAXEXP+1))
#define CUBRTHUGE       0.20971522e7
#define INV_CUBRTHUGE   0.47683711e-6
#define DMAXPOWTWO      FMAXPOWTWO
#define _IEEE           0
#else
#ifdef NSS_IEEE /* ieee, but double = float */
#define MAXFLOAT	((float)3.40282346638528860e+38)
#define MAXDOUBLE       MAXFLOAT
#define MINFLOAT	((float)1.40129846432481707e-45)
#define MINDOUBLE       MINFLOAT
#define _FEXPLEN	8
#define _DEXPLEN        _FEXPLEN
#define _HIDDENBIT	1
#define DMINEXP (-DMAXEXP)
#define FMINEXP (-FMAXEXP)
#define CUBRTHUGE       0.6981463519622e13
#define INV_CUBRTHUGE   0.14323644279e-12
#define DMAXPOWTWO      FMAXPOWTWO
#define _IEEE           1
#else
#ifdef IEEE
#define MAXDOUBLE	1.79769313486231470e+308
#define MAXFLOAT	((float)3.40282346638528860e+38)
#define MINDOUBLE	4.94065645841246544e-324
#define MINFLOAT	((float)1.40129846432481707e-45)
#define _DEXPLEN	11
#define _FEXPLEN	8
#define _HIDDENBIT	1
#define DMINEXP (-DMAXEXP)
#define FMINEXP (-FMAXEXP)
#define CUBRTHUGE	2.6196213420787355e102
#define INV_CUBRTHUGE	.38173571415718220434e-102
#define DMAXPOWTWO	((double)(1L << BITS(long) - 2) * \
				(1L << DSIGNIF - BITS(long) + 1))
#define _IEEE           1
#endif
#endif
#endif

#define _LENBASE	1
#define _EXPBASE	(1 << _LENBASE)
#define DSIGNIF	(BITS(double) - _DEXPLEN + _HIDDENBIT - 1)
#define FSIGNIF	(BITS(float)  - _FEXPLEN + _HIDDENBIT - 1)
#define FMAXPOWTWO	((float)(1L << FSIGNIF - 1))
#define DMAXEXP ((1 << (_DEXPLEN - 1)) - (2-_HIDDENBIT))
#define FMAXEXP ((1 << (_FEXPLEN - 1)) - (2-_HIDDENBIT))
/* original AT&T definition is wrong ?! UH@PCS
#define DMAXEXP ((1 << _DEXPLEN - 1) - 1 + _IEEE)
#define FMAXEXP	((1 << _FEXPLEN - 1) - 1 + _IEEE)
*/
#define LN_MAXDOUBLE	(M_LN2 * DMAXEXP)
#define LN_MINDOUBLE	(M_LN2 * (DMINEXP - 1))
#define H_PREC	(DSIGNIF % 2 ? (1L << DSIGNIF/2) * M_SQRT2 : 1L << DSIGNIF/2)
#define X_EPS	(1.0/H_PREC)
#define X_PLOSS	((double)(long)(M_PI * H_PREC))
#define X_TLOSS	(M_PI * DMAXPOWTWO)
#define M_LN2	0.69314718055994530942
#define M_PI	3.14159265358979323846
#define M_SQRT2	1.41421356237309504880
#define MAXBEXP	DMAXEXP /* for backward compatibility */
#define MINBEXP	DMINEXP /* for backward compatibility */
#define MAXPOWTWO	DMAXPOWTWO /* for backward compatibility */
#endif
