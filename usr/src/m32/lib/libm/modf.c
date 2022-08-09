/* pcs , must be compiled with -fT */

#include <math.h>
#include <errno.h>

double
modf(x, y)
double x, *y;
{
    register int sign;
    register double xm, xi;
    double fintrz();
    
    xm = x;
    sign = 0;
    if (xm < 0.0) {
        sign = 1;
        xm = -xm;
    }
    xi = fintrz(xm);
    *y = sign ? -xi : xi;
    xm -= xi;
    return sign ? -xm : xm;
}

/* m6888 double format:
 *
 * 66----55------------------0
 * 32    10                  0
 * ||    |+------------------+----- mantissa
 * |+----+------------------------- signed exponent
 * +------------------------------- sign of mantissa
 */

#define EXP_MASK 0x7ff
#define EXP_OFFSET 1023
#define EXP_SHIFT 20

double
frexp(x, exp)
double x;
int *exp;
{
    register ex;
    union dbl {
        double d;
        long i[2];
    } cvt;

    cvt.d = x;
    ex = ((cvt.i[0] >> EXP_SHIFT) & EXP_MASK) - (EXP_OFFSET-1);
    *exp = ex;

    cvt.i[0] &= ~(EXP_MASK << EXP_SHIFT);
    cvt.i[0] |=  ((EXP_OFFSET-1) << EXP_SHIFT);
    return cvt.d;
}

double
ldexp(x, exp)
double x;
{
    register ex;
    union dbl {
        double d;
        long i[2];
    } cvt;
    
    if (x == 0.0)
        return 0.0;

    cvt.d = x;
    ex = ((cvt.i[0] >> EXP_SHIFT) & EXP_MASK) + exp;
    if (ex < 0) {
        errno = ERANGE;
        return 0.0;
    }
    if (ex > EXP_MASK) {
        errno = ERANGE;
        return x < 0 ? -MAXFLOAT : MAXFLOAT;
    }

    cvt.i[0] &= ~(EXP_MASK << EXP_SHIFT);
    cvt.i[0] |= ex << EXP_SHIFT;
    return cvt.d;
}
