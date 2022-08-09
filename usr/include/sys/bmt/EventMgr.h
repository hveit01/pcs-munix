/* FILE EventMgr.h  VERSION 8.1  DATE 87/05/22  TIME 13:33:37 */

/*--------------------------------------------------------------------*
 * EventMgr.h
 *
 *	Definitions and declarations for the event manager.
 *
 *  History:
 *		24 Apr 1985 awh		Added SysBeep().
 *		12 Mar 1985 awh		Edited modifiers.
 *		19 Jan 1985 awh		Added updateMask.
 *		16 Jan 1985 awh		Added [lmr]ButtonMask
 *					 & fixed [lr]ButtonBit.
 *		12 Jan 1985 rpd		Added keyUp and autoKey.
 *		 5 Dec 1984 awh	
 *		12 oct 1984 awh
 *
 * Environment:
 *
 *	<bool.h>	
 *	QuickDraw.h	Must be included (uses "Point")
 *--------------------------------------------------------------------*
 */


#define	lButtonMask	0x0080
#define	mButtonMask	0x0040
#define	rButtonMask	0x0020

#define	lButtonBit	0x0080
#define	mButtonBit	0x0040
#define	rButtonBit	0x0020
#define	cmdKey		0x0100
#define	shiftKey	0x0200
#define	alphaLock	0x0400
#define	optionKey	0x0800
#define	applKeypadMode	0x1000

typedef EventRecord *pEventRecord;


/*      Event types                                                     */

#define nullEvent       NULLEVENT
#define leftDown        LEFTDOWN
#define middleDown      MIDDLEDOWN
#define rightDown       RIGHTDOWN
#define leftUp          LEFTUP
#define middleUp        MIDDLEUP
#define rightUp         RIGHTUP
#define keyDown         KEYDOWN
#define keyUp           KEYUP
#define enterRect      ENTERRECT
#define leaveRect      LEAVERECT
#define abortEvent     ABORTEVENT
#define deltaEvent     DELTAEVENT
#define penDownEvent   PENDOWNEVENT
#define penUpEvent     PENUPEVENT
#define penDeltaEvent  PENDELTAEVENT
#define tkeyDownEvent  TKEYDOWNEVENT
#define tkeyUpEvent    TKEYUPEVENT

/*      Event masks                                                     */

#define nullMask               ((EventMask) NULLMASK)
#define lDownMask              ((EventMask) LDOWNMASK)
#define mDownMask              ((EventMask) MDOWNMASK)
#define rDownMask              ((EventMask) RDOWNMASK)
#define lUpMask                ((EventMask) LUPMASK)
#define mUpMask                ((EventMask) MUPMASK)
#define rUpMask                ((EventMask) RUPMASK)
#define keyDownMask            ((EventMask) KEYDOWNMASK)
#define keyUpMask              ((EventMask) KEYUPMASK)
#define enterRectMask          ((EventMask) ENTERRECTMASK)
#define leaveRectMask          ((EventMask) LEAVERECTMASK)
#define abortMask              ((EventMask) ABORTMASK)
#define deltaMask              ((EventMask) DELTAMASK)
#define penDownMask            ((EventMask) PENDOWNMASK)
#define penUpMask              ((EventMask) PENUPMASK)
#define penDeltaMask           ((EventMask) PENDELTAMASK)
#define tkeyDownMask           ((EventMask) TKEYDOWNMASK)
#define tkeyUpMask             ((EventMask) TKEYUPMASK)

#define anyEvent               ((EventMask) 0xffffffff)

#define Event_Mask(eid)         (1L<<(eid))
#define Event_Priority(eid)	1

#define MAXWINDOW	32
EventMask	global_mask;
unsigned long	window_mask[MAXWINDOW];
