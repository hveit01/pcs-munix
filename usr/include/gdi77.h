CNA     Not Yet Available

C       Most of the GDI constants are declared as PARAMETER and INTEGER.
C       Some of the GDI constants are declared as DATA and INTEGER
C               mainly because DATA supports hexadecimal constants.

        integer ABORTEVENT            
        integer AND                   
        integer BAalign                 
C       integer BAauxColorIndex                         name too long
	integer BAauxColorInde
C       integer BAfillColorIndex                        name too long
	integer BAfillColorInd
        integer BApattern               
        integer BAwriteMask             
        integer BAwriteMode             
        integer BLACK                 
        integer CAclipRect              
        integer CAcolorIndex            
        integer CAcurFlag               
        integer CAcurType               
        integer CAhotspot               
        integer CApattern               
        integer CAviewport              
        integer CAvisibility            
        integer CAwriteMask             
        integer CAwriteMode             
        integer CTbtab                  
        integer CTgtab                  
        integer CTrtab                  
        integer DAbitsPerPixel          
        integer DAbitsPerWord           
        integer DAdisplayType           
        integer DAkeyboardType          
        integer DAnoColTab              
        integer DAscreenSize            
        integer DAtabletType            
        integer DELTAEVENT            
        integer DEST                  
        integer EAcageRect              
        integer EAdelta                 
        integer EAmask                  
        integer ENTERRECT             
        integer ERdata                  
        integer ERevent                 
        integer ERposition              
        integer ERtime                  
        integer FAalign                 
C       integer FAauxColorIndex                         name too long
	integer FAauxColorInde
C       integer FAedgeColorIndex                        name too long
	integer FAedgeColorInd
        integer FAedgeStyle             
        integer FAedgeWidth             
C       integer FAfillColorIndex                        name too long
	integer FAfillColorInd
        integer FAfillPattern           
        integer FAhatchIndex            
C       integer FAinteriorStyle                         name too long
	integer FAinteriorStyl
        integer FAwriteMask             
        integer FAwriteMode             
        integer GANSI                 
        integer GARROW                 
        integer GBMTC1               
        integer GBMTC2               
        integer GCHERRYE             
        integer GCHERRYG             
        integer GCOLORED              
        integer GCROSS                
        integer GCROSSHAIR             
        integer GCWS1                
        integer GGERMAN               
        integer GHATCHED              
        integer GHOLLOW               
        integer GHORIZONTAL           
        integer GINVISIBLE             
        integer GKEYBRD              
        integer GMOUSE               
        integer GNEGATIVE             
        integer GNOKEYBOARD          
        integer GNOTABLET            
        integer GNOWAIT               
        integer GPATTERNED            
        integer GPOSITIVE             
        integer GPOSNEG               
        integer GPREH                
        integer GRASTER               
        integer GRUBBAND               
        integer GRUBFIXBOX             
        integer GRUBMOVBOX             
        integer GSTICK                
        integer GTABLET              
        integer GTILLNOW              
        integer GUSERCURSOR            
        integer GVERTICAL             
        integer GVIDEOGRAPH1         
        integer GVIDEOGRAPH2         
        integer GVISIBLE               
        integer GWAIT                 
        integer INVD                  
        integer INVDAND               
        integer INVDOR                
        integer INVS                  
        integer INVSAND               
        integer INVSOR                
        integer IXOR                  
        integer KEYDOWN               
        integer KEYUP                 
        integer LAcolorIndex            
        integer LAlineStyle             
        integer LAlineWidth             
        integer LAwriteMask             
        integer LAwriteMode             
        integer LEAVERECT             
        integer LEFTDOWN              
        integer LEFTUP                
        integer MIDDLEDOWN            
        integer MIDDLEUP              
        integer NAND                  
        integer NOR                   
        integer NULLEVENT             
        integer OR                    
        integer PENDOWNEVENT          
        integer PENUPEVENT            
        integer REcorner                
        integer REorigin                
        integer RIGHTDOWN             
        integer RIGHTUP               
        integer STORE                 
        integer SZBitbltAttr            
        integer SZColTab                
        integer SZCurAttr               
        integer SZDeviceAttr            
        integer SZEventAttr             
        integer SZEventRec              
        integer SZFillAttr              
        integer SZLineAttr              
        integer SZPattern               
        integer SZRectangle             
        integer SZTextAttr              
C       integer TAboxColorIndex                         name too long
	integer TAboxColorInde
        integer TAcolorIndex            
        integer TAfontId                
        integer TAfontType              
        integer TAwriteMask             
        integer TAwriteMode             
        integer TKEYDOWNEVENT         
        integer TKEYUPEVENT           
        integer WHITE                 
        integer XOR                   

        integer ABORTMASK             
        integer ANYEVENT              
        integer DELTAMASK             
        integer ENTERRECTMASK         
        integer GCURCLIP              
        integer GCURCOLI              
        integer GCURHOTSP             
        integer GCURPAT               
        integer GCURTYPE              
        integer GCURVIEW              
        integer GCURVIS               
        integer GCURWMASK             
        integer GCURWMODE             
        integer GDASH              
        integer GDASHDOT           
        integer GDASHDOTDO         
        integer GDOT               
        integer GSOLID             
        integer KEYDOWNMASK           
        integer KEYUPMASK             
        integer LDOWNMASK             
        integer LEAVERECTMASK         
        integer LUPMASK               
        integer MDOWNMASK             
        integer MUPMASK               
        integer NULLMASK              
        integer PENDOWNMASK           
        integer PENUPMASK             
        integer RDOWNMASK             
        integer RUPMASK               
        integer TKEYDOWNMASK          
        integer TKEYUPMASK            


	integer gcreatemenu
	integer*2 gcreatebitmap
	integer geterrno
	integer getshort
	integer ggetlocatorkey
	integer ggetmenuitem
	integer xpoint
	integer ypoint
	integer*2 gdownfont
	integer*4 apoint
	integer*4 getlong
	integer*4 ggetlocatorpos
	integer*4 gstring
C       integer gcreatewindow


C  The next lines give the type of some variables used in GDI.
C  These lines are just informative.
C       integer   time
C       integer*2 bmid
C       integer*2 charset
C       integer*2 colid
C       integer*2 device
C       integer*2 fid
C       integer*2 font
C       integer*2 key
C       integer*2 menuid
C       integer*2 menitem
C       integer*2 coltab(48)
C       integer*4 mask
C       integer*4 mpoint
C       integer*4 point
C       integer*4 point1
C       integer*4 point2
C       integer*4 point3
C       integer*4 point4


C GDI functions which are (nearly) called directly
	externc garc
	externc gbitblt
	externc gbitbltsys
	externc gbitbltusr
	externc gbuffering
	externc gchord
	externc gcircle
	externc gclearwindow
	externc gcreatebitmap
C g_create_menu
CNA     externc gcreatewindow
CNA     externc gdeletewindow
	externc gdeletemenu
CNA     externc gdeletewindow
	externc gdisplaymenu
C g_down_font
	externc gerasemenu
C g_error
	externc gflushbuffer
	externc gflushevntqueu
	externc gflushgetevnt
	externc gfreefont
	externc ggetbitbltattr
	externc ggetcolormap
	externc ggetcursorattr
	externc ggetdeviceattr
	externc ggetevnt
	externc ggetevntattr
	externc ggetfillattr
	externc ggetlineattr
	externc ggetlocatorkey
	externc ggetlocatorpos
	externc ggetmenuitem
	externc ggettextattr
	externc ginitgdi
	externc gline
	externc glnkcursor
CNA     externc gmodifywindow
	externc gpie
	externc gpoint
	externc gpolygon
	externc gpolyline
	externc grectangle
	externc gresetattr
	externc gresetgdi
	externc gselectcolmap
	externc gsetbitbltattr
	externc gsetcliprect
	externc gsetcolormap
	externc gsetcursorattr
	externc gsetcursor
	externc gselectcolorma
	externc gsetevntattr
	externc gsetfillattr
	externc gsetlineattr
	externc gsettextattr
	externc gsetvt100chars
	externc gsetvt100font
	externc gtexture
	externc gtrackmenu
	externc gunlnkcursor

C  Constants, structures from 'gdi.h'

C    constant definitions

C    Write Modes
	parameter (WHITE         =0  )
	parameter (AND           =1  )
	parameter (INVDAND       =2  )
	parameter (STORE         =3  )
	parameter (INVSAND       =4  )
	parameter (DEST          =5  )
	parameter (XOR           =6  )
	parameter (OR            =7  )
	parameter (NOR           =8  )
	parameter (IXOR          =9  )
	parameter (INVD          =10 )
	parameter (INVDOR        =11 )
	parameter (INVS          =12 )
	parameter (INVSOR        =13 )
	parameter (NAND          =14 )
	parameter (BLACK         =15 )

C    Interior Styles
	parameter (GHOLLOW       =0  )
	parameter (GCOLORED      =1  )
	parameter (GPATTERNED    =2  )
	parameter (GHATCHED      =3  )

C    Hatch Indices
	parameter (GHORIZONTAL   =0  )
	parameter (GVERTICAL     =1  )
	parameter (GPOSITIVE     =2  )
	parameter (GNEGATIVE     =3  )
	parameter (GCROSS        =4  )
	parameter (GPOSNEG       =5  )

C    Font Types
	parameter (GRASTER       =0 )
	parameter (GSTICK        =1 )

C    Character Sets
	parameter (GANSI         =0 )
	parameter (GGERMAN       =1 )

C    Event Types
	parameter (NULLEVENT     =0  )
	parameter (LEFTDOWN      =1  )
	parameter (MIDDLEDOWN    =2  )
	parameter (RIGHTDOWN     =3  )
	parameter (LEFTUP        =4  )
	parameter (MIDDLEUP      =5  )
	parameter (RIGHTUP       =6  )
	parameter (KEYDOWN       =7  )
	parameter (KEYUP         =8  )
	parameter (ENTERRECT     =9  )
	parameter (LEAVERECT     =10 )
	parameter (ABORTEVENT    =11 )
	parameter (DELTAEVENT    =12 )
	parameter (PENDOWNEVENT  =13 )
	parameter (PENUPEVENT    =14 )
	parameter (TKEYDOWNEVENT =15 )
	parameter (TKEYUPEVENT   =16 )

C    Cursor Types
	parameter (GARROW         =0)
	parameter (GCROSSHAIR     =1)
	parameter (GUSERCURSOR    =2)
	parameter (GRUBBAND       =3)
	parameter (GRUBMOVBOX     =4)
	parameter (GRUBFIXBOX     =5)

C    Event Time
	parameter (GNOWAIT       =1)
	parameter (GWAIT         =-1)
	parameter (GTILLNOW      =-1)

C    Cursor & Window Visibility
	parameter (GINVISIBLE     =0)
	parameter (GVISIBLE       =1)

C    Input Devices
	parameter (GKEYBRD      =0)
	parameter (GMOUSE       =1)
	parameter (GTABLET      =2)

C    device types
	parameter (GNOKEYBOARD  =0)
	parameter (GPREH        =1)
	parameter (GCHERRYE     =2)
	parameter (GCHERRYG     =3)

	parameter (GNOTABLET    =0)
	parameter (GVIDEOGRAPH1 =1)
	parameter (GVIDEOGRAPH2 =2)

	parameter (GBMTC1       =1)
	parameter (GBMTC2       =2)
	parameter (GCWS1        =3)


C    some data structure definitions (copied from gdi.h)

C       integer*2 Fid           /* workstation file identifier  */
C       integer*2 Font          /* Font identifier              */
C       integer*2 Key           /* Key code                     */
C       integer*2 MenuId        /* Menu identifier              */
C       integer*2 MenItem       /* Menu item number             */
C       integer*2 DevType       /* Input device type            */
C       integer*2 BitmapId      /* User Bitmap Identifier       */

C       integer*4 EventMask
C       integer*4 EventMessage


C    Indices for GDI structures
	parameter (REorigin        =1)
	parameter (REcorner        =REorigin        +2)
	parameter (SZRectangle     =REcorner        +2-1)

	parameter (SZPattern       =16)

	parameter (CTrtab          =1)
	parameter (CTgtab          =CTrtab          +16)
	parameter (CTbtab          =CTgtab          +16)
	parameter (SZColTab        =CTbtab          +16-1)

	parameter (BAalign         =1)
	parameter (BApattern       =BAalign         +2)
C       parameter (BAfillColorIndex=BApattern       +16)name too long
	parameter (BAfillColorInd  =BApattern       +16)
C       parameter (BAauxColorIndex =BAfillColorIndex+1) name too long
	parameter (BAauxColorInde  =BAfillColorInd  +1)
C       parameter (BAwriteMode     =BAauxColorIndex +1) name too long
	parameter (BAwriteMode     =BAauxColorInde  +1)
	parameter (BAwriteMask     =BAwriteMode     +1)
	parameter (SZBitbltAttr    =BAwriteMask     +1-1)

	parameter (LAlineStyle     =1)
	parameter (LAlineWidth     =LAlineStyle     +2)
	parameter (LAcolorIndex    =LAlineWidth     +1)
	parameter (LAwriteMode     =LAcolorIndex    +1)
	parameter (LAwriteMask     =LAwriteMode     +1)
	parameter (SZLineAttr      =LAwriteMask     +1-1)

	parameter (FAedgeStyle     =1)
	parameter (FAedgeWidth     =FAedgeStyle     +2)
C       parameter (FAedgeColorIndex=FAedgeWidth     +1) name too long
	parameter (FAedgeColorInd  =FAedgeWidth     +1)
C       parameter (FAwriteMode     =FAedgeColorIndex+1) name too long
	parameter (FAwriteMode     =FAedgeColorInd  +1)
	parameter (FAwriteMask     =FAwriteMode     +1)
C       parameter (FAinteriorStyle =FAwriteMask     +1) name too long
	parameter (FAinteriorStyl  =FAwriteMask     +1)
C       parameter (FAhatchIndex    =FAinteriorStyle +1) name too long
	parameter (FAhatchIndex    =FAinteriorStyl  +1)
C       parameter (FAfillColorIndex=FAhatchIndex    +1) name too long
	parameter (FAfillColorInd  =FAhatchIndex    +1)
C       parameter (FAauxColorIndex =FAfillColorIndex+1) name too long
	parameter (FAauxColorInde  =FAfillColorInd  +1)
C       parameter (FAalign         =FAauxColorIndex +1) name too long
	parameter (FAalign         =FAauxColorInde  +1)
	parameter (FAfillPattern   =FAalign         +2)
	parameter (SZFillAttr      =FAfillPattern   +16-1)

	parameter (TAfontType      =1)
	parameter (TAfontId        =TAfontType      +1)
	parameter (TAcolorIndex    =TAfontId        +1)
C       parameter (TAboxColorIndex =TAcolorIndex    +1) name too long
	parameter (TAboxColorInde  =TAcolorIndex    +1)
C       parameter (TAwriteMode     =TAboxColorIndex +1) name too long
	parameter (TAwriteMode     =TAboxColorInde  +1)
	parameter (TAwriteMask     =TAwriteMode     +1)
	parameter (SZTextAttr      =TAwriteMask     +1-1)

	parameter (EAdelta         =1)
	parameter (EAcageRect      =EAdelta         +2)
	parameter (EAmask          =EAcageRect      +4)
	parameter (SZEventAttr     =EAmask          +2-1)

	parameter (ERevent         =1)
	parameter (ERdata          =ERevent         +1)
	parameter (ERtime          =ERdata          +2)
	parameter (ERposition      =ERtime          +2)
	parameter (SZEventRec      =ERposition      +2-1)

	parameter (CAcurFlag       =1)
	parameter (CAcurType       =CAcurFlag       +1)
	parameter (CAclipRect      =CAcurType       +1)
	parameter (CAviewport      =CAclipRect      +4)
	parameter (CAcolorIndex    =CAviewport      +4)
	parameter (CAwriteMode     =CAcolorIndex    +1)
	parameter (CAwriteMask     =CAwriteMode     +1)
	parameter (CApattern       =CAwriteMask     +1)
	parameter (CAhotspot       =CApattern       +16)
	parameter (CAvisibility    =CAhotspot       +2)
	parameter (SZCurAttr       =CAvisibility    +1-1)

	parameter (DAscreenSize    =1)
	parameter (DAbitsPerPixel  =DAscreenSize    +2)
	parameter (DAbitsPerWord   =DAbitsPerPixel  +1)
	parameter (DAkeyboardType  =DAbitsPerWord   +1)
	parameter (DAtabletType    =DAkeyboardType  +1)
	parameter (DAdisplayType   =DAtabletType    +1)
	parameter (DAnoColTab      =DAdisplayType   +1)
	parameter (SZDeviceAttr    =DAnoColTab      +1-1)

C    GDI function declarations
	externc apoint
	externc geterrno
	externc getlong
	externc getshort
	externc moveshorts
	externc putlong
	externc putshort
	externc xpoint
	externc ypoint

C    Line & Edge Styles
C       data GSOLID        /x'ffffffff'/
	parameter (GSOLID     = -1)
C       data GDASH         /x'ff00ff00'/
	parameter (GDASH      = -16711936)
C       data GDOT          /x'88888888'/
	parameter (GDOT       = -2004318072)
C       data GDASHDOT      /x'83f083f0'/
	parameter (GDASHDOT   = -2081389584)
C       data GDASHDOTDO    /x'f888f888'/
	parameter (GDASHDOTDO = -125241208)

C    Event Masks
C       data NULLMASK      /x'00000001'/
	parameter (NULLMASK   = 1)
C       data LDOWNMASK     /x'00000002'/
	parameter (LDOWNMASK  = 2)
C       data MDOWNMASK     /x'00000004'/
	parameter (MDOWNMASK  = 4)
C       data RDOWNMASK     /x'00000008'/
	parameter (RDOWNMASK  = 8)
C       data LUPMASK       /x'00000010'/
	parameter (LUPMASK    = 16)
C       data MUPMASK       /x'00000020'/
	parameter (MUPMASK    = 32)
C       data RUPMASK       /x'00000040'/
	parameter (RUPMASK    = 64)
C       data KEYDOWNMASK   /x'00000080'/
	parameter (KEYDOWNMASK= 128)
C       data KEYUPMASK     /x'00000100'/
	parameter (KEYUPMASK  = 256)
C       data ENTERRECTMASK /x'00000200'/
	parameter (ENTERRECTMASK = 512)
C       data LEAVERECTMASK /x'00000400'/
	parameter (LEAVERECTMASK = 1024)
C       data ABORTMASK     /x'00000800'/
	parameter (ABORTMASK  = 2048)
C       data DELTAMASK     /x'00001000'/
	parameter (DELTAMASK  = 4096)
C       data PENDOWNMASK   /x'00002000'/
	parameter (PENDOWNMASK= 8192)
C       data PENUPMASK     /x'00004000'/
	parameter (PENUPMASK  = 16384)
C       data TKEYDOWNMASK  /x'00008000'/
	parameter (TKEYDOWNMASK  = 32768)
C       data TKEYUPMASK    /x'00010000'/
	parameter (TKEYUPMASK = 65536)

C       data ANYEVENT      /x'ffffffff'/
	parameter (ANYEVENT   = -1)

C    Cursor Attribut Flags
C       data GCURTYPE      /x'0001'/
	parameter (GCURTYPE   = 1)
C       data GCURCLIP      /x'0002'/
	parameter (GCURCLIP   = 2)
C       data GCURVIEW      /x'0004'/
	parameter (GCURVIEW   = 4)
C       data GCURCOLI      /x'0008'/
	parameter (GCURCOLI   = 8)
C       data GCURWMASK     /x'0010'/
	parameter (GCURWMASK  = 16)
C       data GCURWMODE     /x'0020'/
	parameter (GCURWMODE  = 32)
C       data GCURPAT       /x'0040'/
	parameter (GCURPAT    = 64)
C       data GCURHOTSP     /x'0080'/
	parameter (GCURHOTSP  = 128)
C       data GCURVIS       /x'0100'/
	parameter (GCURVIS    = 256)
