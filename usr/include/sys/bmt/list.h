/* FILE list.h  VERSION 8.1  DATE 87/05/22  TIME 13:44:17 */

/************************** list.h *****************************************
 *
 * definitions for doble linked lists
 *
 *      who     when            why
 *
 *      BL      20. 6. 86       always the same routines and structures
 *************************************************************************/

typedef struct listElement {
	struct listElement *next;
	struct listElement *prev;
} ListElement;

typedef struct {
	ListElement *first;
	ListElement *last;
} ListHeader;
