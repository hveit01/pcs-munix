/* FILE menu.h  VERSION 8.1  DATE 87/05/22  TIME 13:44:46 */

/***************************** menu.h **************************************
 *
 * simple pull down menus; structure declaration
 *
 *      who             when            why
 *
 *      BL              02.01.86
 *
 **************************************************************************/

typedef struct {
	/* first the screen coordinates                                 */
	Rectangle       screenRect;             /* where on screen      */
	Rectangle       hilightRect;            /* the selected item    */
	short           isHighLighted;          /* a flag               */
	/* relative to {0,0} coordinates                                */
	Rectangle       outRect;                /* outside Rectangle    */
	Rectangle       insideRect;             /* inset Rectangle      */
	short           nItems;                 /* how many strings in  */
	char            **pItems;               /* the strings itself   */
	short           fontid;                 /* what font to use     */
	Bitmap          *menuMap;               /* where are the bits   */
	Bitmap          *saveMap;               /* occupied data        */
	short           isOnScreen;             /* in front or not ??   */
	short           menuMagic;              /* a cryptic number     */
} Menu;

#define MENU_MAGIC      0x1f2e
