/* PCS specific, uses mc68881 packed format and synthetic instruction */

/* m68881 packed format:
 * 99--9------87----66---------------0
 * 54  1      09    87               0
 * ||  |      ||    |+---------------+---- 17 BCD digit mantissa
 * ||  |      |+----+--------------------- ZERO
 * ||  +------+--------------------------- 3 BCD digit exponent
 * |+------------------------------------- sign of exp
 * +-------------------------------------- sign of mantissa
 */

#define SGN_MANT    8
#define SGN_EXP     4
#define MANT_DIGITS 17
#define EXP_DIGITS  3

double atof(p)
register char *p;
{
    /* the number is collected into up to 17 unpacked BCD digits 
     * plus the exponent and the two sign bits.
     * The packing buffer is then compressed into 12 BCD-packed bytes.
     */
    
    char packed[24];
    int dot_flg;
    int   fracsz;
    char *end;
    register int i, exp;
    register char *p1, *p2;

    /* clear packed buffer */
    for (i = 0; i < 24; i++)
        packed[i] = 0;

    /* skip leading blanks */
    while (*p==' ' || *p=='\t') p++;

    /* skip mantissa signs, will allow for "---10", i.e. = -10
     * by toggling sign bit i */
    i = 0;
    while (*p=='+' || *p=='-') {
        if (*p=='-')
            i = (i==0);
        p++;
    }
    if (i)
        packed[0] = SGN_MANT;

    /* skip leading zeros */
    while (*p=='0') p++;

    /* p starts mantissa, p1 advances until period or end of number */
    for (p1 = p; *p1 >= '0' && *p1 <= '9'; p1++);

    /* if period found set flag and advance */
    if (*p1=='.') {
        p1++;
        dot_flg = 1;
    } else
        dot_flg = 0;

    /* p2 starts of fractional part, p1 advances until end of number */
    for (p2 = p1; *p1>='0' && *p1<='9'; p1++);
    end = p1; /* end of mantissa, exponent may follow */

    /* more than MANT_DIGITS for integer part? */
    if ((p1 - p - dot_flg) > MANT_DIGITS)
        p1 = &p[dot_flg + MANT_DIGITS];

    /* calc (negative) size of fractional part */
    i = p2 - p1;
    if (i > 0)
        fracsz = i - dot_flg;
    else
        fracsz = i;

    /* copy digits of integer part int o pack buffer */
    for (i=0, p2 = p1, --p2, i=23;
         p2 >= p && ((*p2>='0' && *p2<='9') || *p2 == '.'); p2--) {
        if (*p2 != '.')
            packed[i--] = (*p2-'0');
    }

    /* check for exponent */
    p1 = end;
    exp = 0;
    /* has one? */
    if (*p1=='e' || *p1=='E') {
        p1++;
        
        /* skip over optional signs */
        i = 0;
        while (*p1 =='+' || *p1=='-') {
            if (*p1 == '-')
                i = (i==0);
            p1++;
        }
        
        /* build exponent */
        while (*p1 >= '0' && *p1 <= '9')
            exp = exp * 10 + (*p1++ - '0');
        
        /* if negative sign, make negative */
        if (i)
            exp = -exp;
    }

    /* adjust exponent, and set sign bit */
    exp += fracsz + 16;
    if (exp < 0)
        packed[0] |= SGN_EXP;

    /* if exp was negative, make positive */
    if (exp < 0)
        exp = -exp;

    /* fill up to 3 exponent digits into appropriate pack buffer fields */
    for (i = EXP_DIGITS; i >= 1 && exp != 0; i--) {
        packed[i] = exp % 10;
        exp = exp / 10;
    }

    /* pack buffer */
    for (i = 0; i < 12; i++)
        packed[i] = (packed[2*i]<<4) | packed[2*i+1];

    /* we have to convert the buffer into a FP number, use
     * the m68881 fmove.p instruction
        lea packed(a6), a5
        fmove.p (a5), fp0
    */
    0x4bee; 0xffcc;
    0xf215; 0x4c00;
}