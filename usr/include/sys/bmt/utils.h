/* FILE utils.h  VERSION 8.1  DATE 87/05/22  TIME 13:48:15 */

/*************************** utils.h ***************************************
 *
 * SUBJECT:     BITMAP GRAFICS
 *
 * some nice macros which are used by both grafic primitives and
 * programs.
 *
 *      who     when            why
 *
 *      BL      ????
 ***************************************************************************/
#define min(a,b)  ((a) < (b) ? (a) : (b))
#define max(a,b)  ((a) > (b) ? (a) : (b))

#define Rpt(p,q)        (p).x, (p).y, (q).x, (q).y,


/*
 * ptinrect() -- is point within a rectangle
 *                      returns 1 if within rect, 0 otherwise
 */
#define ptinrect(p,r)   (((p).x >= (r).origin.x) && ((p).x < (r).corner.x) &&   \
			    ((p).y >= (r).origin.y) && ((p).y < (r).corner.y))

/*
 * sum, diff, mul of points
 */
#define addm(p,q)        ((p).x + (q).x), ((p).y + (q).y)
#define subm(p,q)        ((p).x - (q).x), ((p).y - (q).y)
#define mulm(p,a)        ((p).x * (a)), ((p).y * (a))
#define divm(p,a)        ((p).x / (a)), ((p).y / (a))

/*
 * rectINrect() -- is s contained in r ??
 */
#define rectINrect(r,s)                                         \
	(((r).origin.x <= (s).origin.x) && ((r).origin.y <= (s).origin.y) &&\
	((r).corner.x >= (s).corner.x) && ((r).corner.y >= (s).corner.y))

/*
 * rectXrect -- do r and s intersect?
 */
#define rectXrectm(r,s) (((r).origin.x < (s).corner.x) &&               \
			 ((s).origin.x < (r).corner.x) &&               \
			 ((r).origin.y < (s).corner.y) &&               \
			 ((s).origin.y < (r).corner.y))


/*
 * rectXrectp -- do r and s intersect?
 */
#define rectXrectp(r,s) (((r)->origin.x < (s).corner.x) &&              \
			 ((s).origin.x < (r)->corner.x) &&              \
			 ((r)->origin.y < (s).corner.y) &&              \
			 ((s).origin.y < (r)->corner.y))

/*
 * intersect -- return intersected rectangle ; BL
 *              note: returns wrong value if there's no intersection
 */
#define intersect(resRect, r,s)                                 \
{       (resRect).origin.x = max((r).origin.x, (s).origin.x);         \
	(resRect).origin.y = max((r).origin.y, (s).origin.y);         \
	(resRect).corner.x = min((r).corner.x, (s).corner.x);         \
	(resRect).corner.y = min((r).corner.y, (s).corner.y);         \
}

#define transRect(tmpRect,orgRect)                                        \
{       (tmpRect).origin.x = (orgRect).origin.x & PIX_MASK;               \
	(tmpRect).origin.y = 0;                                           \
	(tmpRect).corner.x = (tmpRect).origin.x +                         \
			       (orgRect).corner.x - (orgRect).origin.x;   \
	(tmpRect).corner.y = (orgRect).corner.y - (orgRect).origin.y;   }

#define transPt(tmpPt, orgPt)                                             \
{       (tmpPt).x   = (orgPt).x &  PIX_MASK;                              \
	(tmpPt).y   = 0;                                                  }

#define shiftRect(tmpRect, orgPt)                                       \
{       (tmpRect).origin.x -= (orgPt).x & WORD_MASK;                    \
	(tmpRect).origin.y -= (orgPt).y;                                \
	(tmpRect).corner.x -= (orgPt).x & WORD_MASK;                    \
	(tmpRect).corner.y -= (orgPt).y;  }

#define shiftPt(tmpPt, orgPt)                                           \
{       (tmpPt).x -= (orgPt).x & WORD_MASK;                             \
	(tmpPt).y -= (orgPt).y;  }

#define spanRect(tmpRect,p,q)                                             \
{       (tmpRect).origin.x  = min((p).x, (q).x);                          \
	(tmpRect).origin.y  = min((p).y, (q).y);                          \
	(tmpRect).corner.x  = max((p).x, (q).x) + 1;                      \
	(tmpRect).corner.y  = max((p).y, (q).y) + 1;  }

#define patchLayToMap(tmpMap, lp)                                       \
{       (tmpMap).base   = (lp)->base;                                   \
	(tmpMap).width  = (lp)->width;                                  \
	(tmpMap).place     = (lp)->place;                               \
	(tmpMap).sizeofWord = (lp)->sizeofWord;                         \
	(tmpMap).bitsPerPixel = (lp)->bitsPerPixel;                     \
	(tmpMap).rect.origin.x  = 0;                                    \
	(tmpMap).rect.origin.y  = (lp)->rect.origin.y;                  \
	(tmpMap).rect.corner.x  = (tmpMap).width * 16;                  \
	(tmpMap).rect.corner.y  = (lp)->rect.corner.y;                  }

#define Rect(tRect,a,b,c,d)                                             \
{       tRect.origin.x = a;                                             \
	tRect.origin.y = b;                                             \
	tRect.corner.x = c;                                             \
	tRect.corner.y = d;                                             \
}

#define Pt(tmpPt,a,b)                                                   \
{       tmpPt.x = a;                                                    \
	tmpPt.y = b;                                                    \
}

/* checks if 'rect's origin is upper left from corner   */
#define isRect(rect)    (((rect).origin.x < (rect).corner.x) &&         \
			 ((rect).origin.y < (rect).corner.y))

