/*PCS moved from ctime.c */

/*  Copyright (c) 1984 AT&T */
/*    All Rights Reserved   */

/*  THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T */
/*  The copyright notice above does not evidence any    */
/*  actual or intended publication of such source code. */

#ident  "@(#)libc-port:gen/time_comm.c  1.8.1.1"
/*
 * Functions that are common to ctime(3C) and cftime(3C)
 */
#include    <ctype.h>
#include    <stdio.h>
#include    <sys/types.h>
#include    <time.h>

#define SEC_IN_HOUR (60 * 60)
#define SEC_IN_DAY  (24 * 60 * 60)
#define MAXTZNAME   3
#define year_size(A)    (((A) % 4) ? 365 : 366)

long    timezone = 0;
long    altzone = 0;
int     daylight = 0;
extern char     *getenv();

struct tm *gmtime(), *localtime();
char    *ctime(), *asctime();
void    tzset();

/*static char   cbuf[26];*/

static time_t   start_dst ;    /* Start date of alternate time zone */
static time_t   end_dst;      /* End date of alternate time zone */
char    *tzname[] = {"GMT", "   "};
static short month_size[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

static char *getzname(), *gettime();

struct tm *
localtime(tim)
time_t  *tim;
{
    register struct tm *ct;
    static void getusa();
    int daybegin, dayend;
    time_t copyt, curr;

    tzset();
    copyt = *tim - timezone;
    ct = gmtime(&copyt);
    if (!daylight)
        return(ct);

    curr = ct->tm_yday*SEC_IN_DAY + ct->tm_hour*SEC_IN_HOUR + ct->tm_min*60 + ct->tm_sec;

    if ( start_dst == 0 && end_dst == 0)
        getusa(&daybegin, &dayend, ct);
    else
    {
        daybegin = start_dst;
        dayend  = end_dst;
    }
    if (curr >= daybegin && curr < dayend)
    {
        copyt = *tim - altzone;
        ct = gmtime(&copyt);
        ct->tm_isdst = 1;
    }
    return(ct);
}

struct tm *
gmtime(tim)
time_t  *tim;
{
    register int        d0, d1;
    long            hms, day;
    static struct tm    xtime;

    /* Break initial number into days */
    hms = *tim % SEC_IN_DAY;
    day = *tim / SEC_IN_DAY;
    if(hms < 0) {
        hms += SEC_IN_DAY;
        day -= 1;
    }

    /* Generate hours:minutes:seconds */
    xtime.tm_sec = hms % 60;
    d1 = hms / 60;
    xtime.tm_min = d1 % 60;
    d1 /= 60;
    xtime.tm_hour = d1;

    /* Generate day of the week. The addend is 4 mod 7 (1/1/1970 was Thursday) */

    xtime.tm_wday = (day + 7340036L) % 7;

    /* Generate year number */
    if(day >= 0)
        for(d1=70; day >= year_size(d1); d1++)
            day -= year_size(d1);
    else
        for(d1=70; day < 0; d1--)
            day += year_size(d1-1);
    xtime.tm_year = d1;
    xtime.tm_yday = d0 = day;

    /* Generate month */
    if(year_size(d1) == 366)
        month_size[1] = 29;
    for(d1 = 0; d0 >= month_size[d1]; d1++)
        d0 -= month_size[d1];
    month_size[1] = 28;
    xtime.tm_mday = d0 + 1;
    xtime.tm_mon = d1;
    xtime.tm_isdst = 0;
    return(&xtime);
}

void
tzset()
{
    char *p;
    char *getenv();

    if(p = getenv ("TZ")) {
        if ((p = getzname(p, tzname[0]))==0 ||
            (p = gettime(p, &timezone, 1))==0)  return;
        altzone = timezone - SEC_IN_HOUR;
        end_dst = 0;
        start_dst = 0;

        daylight = 0;
        if ((p = getzname(p, tzname[1]))==0) return;
        daylight = 1;
        if (*p == 0) return;
        if (*p != ';') {
            if ((p = gettime(p, &altzone, 1))==0 || *p != ';') return;
        }
        getdst(p+1, &start_dst, &end_dst);
    }
}

static char *
getzname(p,tz)
char *p, *tz;
{
  int n = MAXTZNAME;

  if (!isalpha(*p))
    return(0);
  do
  {
    *tz++ = *p ;
  } while (--n > 0 && isalpha(*++p) );
  while(isalpha(*p))
    p++;
  while(--n >= 0)
    *tz++ = ' ';        /* Pad with blanks */
  return(p);    
}

static char *
gettime(p,timez,f)
char *p;
time_t *timez;
int f;
{
    extern char* getdigit();
    register time_t t = 0;
    int d, sign = 0;

    d = 0;
    if (f)
        if ( (sign = (*p == '-')) || (*p == '+'))
            p++;
    if ( (p = getdigit(p,&d)) != 0)
    {
        t = d * 60 * 60;
        if (*p == ':')
        {
            if( (p = getdigit(p+1,&d)) != 0)
            {
                t += d * 60;
                if (*p == ':')
                {
                    if( (p = getdigit(p+1,&d)) != 0)
                        t += d;
                }
            }
        }
    }
    if(sign)
        *timez = -t;
    else
        *timez = t;
    return(p);
}

static char *
getdigit(ptr,d)
char *ptr;
int * d;
{


    if (!isdigit(*ptr))
        return(0);
    *d = 0;
    do
    {
        *d *= 10;
        *d += *ptr - '0';
    }while( (isdigit(*++ptr)));
    return(ptr);
}

static int
getdst(p,s,e)
char *p;
time_t *s, *e;
{
    int lsd,led;
    time_t st,et;
    st = et = 0;        /* Default for start and end time is 00:00:00 */
    if ( (p = getdigit(p,&lsd)) == 0 )
        return(0);
    lsd -= 1;   /* keep julian count in sync with date  1-366 */
    if ( (*p == '/') &&  ((p = gettime(p+1,&st,0)) == 0) )
            return(0);
    if (*p == ',')
    {
        if ( (p = getdigit(p+1,&led)) == 0 )
            return(0);
        led -= 1;   /* keep julian count in sync with date  1-366 */
        if ((*p == '/') &&  ((p = gettime(p+1,&et,0)) == 0) )
                return(0);
    }
    /* Convert the time into seconds */
    *s = (long)(lsd * SEC_IN_DAY + st);
    *e = (long)(led * SEC_IN_DAY + et - (timezone - altzone));
    return(1);
}

/* get begin/end of daylight saving time for USA */
static char *getusa(s, e, t)
int *s;
int *e;
struct tm *t;
{
    static struct {
        int yrbgn;
        int daylb;
        int dayle;
    } daytab[] = {
        87, 96, 303,    /*1987: new legislation - 1st Sun in April */
        76, 119,303,    /*1976*/
        75, 58, 303,    /*1975: Last Sun in Feb - last Sun in Oct */
        74, 5,  333,    /*1974: Jan 6 - last Sun. in Nov */
        70, 119,303     /*1970 start GMT */
    };
    int i = 0;
    
    if (timezone < SEC_IN_HOUR && timezone >= -(2*SEC_IN_HOUR)) {
        if (t->tm_year < 80) {
            *s = 0;
            *e = 0;
            return;
        } else {
            *s = 90;
            *e = 273;
        }
    } else {
        while (t->tm_year < daytab[i].yrbgn) /* can't be less than 70 */
            i++;
        *s = daytab[i].daylb; /* fall through the loop when in correct interval*/
        *e = daytab[i].dayle;
    }       
    *s = sunday(t, *s);
    *e = sunday(t, *e); 
    *s = (long)(*s * SEC_IN_DAY + 2*SEC_IN_HOUR);
    *e = (long)(*e * SEC_IN_DAY + SEC_IN_HOUR);
}

/*
 * The argument is a 0-origin day number.
 * The value is the day number of the first
 * Sunday on or after the day.
 */
static int
sunday(t, d)
register struct tm *t;
register int d;
{
    if(d >= 58)
        d += year_size(t->tm_year) - 365;
    return(d - (d - t->tm_yday + t->tm_wday + 700) % 7);
}

