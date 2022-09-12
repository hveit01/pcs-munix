/* BSD compatiblity hack
 */

#ident "$Header: strings.h,v 1.2 86/12/12 14:12:15 paulm Exp $"

/* just get the S5 version of the stuff */
#include "../string.h"

/* plus BSD functions not in S5 */
extern char
	*index(),
	*rindex();
