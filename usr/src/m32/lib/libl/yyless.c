/* PCS libl support */
/*  @(#)yyless.c    2.1     */

#include <stdio.h>

extern char yytext[];
extern int yyleng;
extern int yyprevious;

yyless(x)
int x;
{
    register char *lastch, *ptr;

    lastch = &yytext[yyleng];

    if (x >= 0 && x <= yyleng)
        ptr = &yytext[x];
    else
        ptr = (char*)x;

    while (lastch > ptr) {
        yyunput(*--lastch);
    }
    *lastch = '\0';

    if (ptr > yytext)
        yyprevious = *--lastch;

    yyleng = ptr - yytext;
}
