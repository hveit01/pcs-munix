#ifndef _COFF_H_
#define _COFF_H_


/* structure of PCS COFF files,
 * from original PCS headers.
 * Note: PCS 68K uses incorrectly 0x178 = 0570 as magic -
 * this is actually VAX format
 */
#define PCSMAGIC	0x0178
#define PCSINVMAGIC	0x7801
#define MC68INVMAGIC 0x5001

#pragma pack(2)
#include <cstdint>

/*ident	"@(#)sgs-inc:common/filehdr.h	1.19"*/
struct filehdr {
	uint16_t	f_magic;	/* magic number */
	uint16_t	f_nscns;	/* number of sections */
	int32_t		f_timdat;	/* time & date stamp */
	int32_t		f_symptr;	/* file pointer to symtab */
	int32_t		f_nsyms;	/* number of symtab entries */
	uint16_t	f_opthdr;	/* sizeof(optional hdr) */
	uint16_t	f_flags;	/* flags */
};


/*
 *   Bits for f_flags:
 *
 *	F_RELFLG	relocation info stripped from file
 *	F_EXEC		file is executable  (i.e. no unresolved
 *				externel references)
 *	F_LNNO		line nunbers stripped from file
 *	F_LSYMS		local symbols stripped from file
 *	F_MINMAL	this is a minimal object file (".m") output of fextract
 *	F_UPDATE	this is a fully bound update file, output of ogen
 *	F_SWABD		this file has had its bytes swabbed (in names)
 *	F_AR16WR	this file has the byte ordering of an AR16WR (e.g. 11/70) machine
 *				(it was created there, or was produced by conv)
 *	F_AR32WR	this file has the byte ordering of an AR32WR machine(e.g. vax)
 *	F_AR32W		this file has the byte ordering of an AR32W machine (e.g. 3b,maxi)
 *	F_PATCH		file contains "patch" list in optional header
 *	F_NODF		(minimal file only) no decision functions for
 *				replaced functions
 */

#define  F_RELFLG	0000001
#define  F_EXEC		0000002
#define  F_LNNO		0000004
#define  F_LSYMS	0000010
#define  F_MINMAL	0000020
#define  F_UPDATE	0000040
#define  F_SWABD	0000100
#define  F_AR16WR	0000200
#define  F_AR32WR	0000400
#define  F_AR32W	0001000
#define  F_PATCH	0002000
#define  F_NODF		0002000

/*
 *	BELLMAC-32	Identification field
 *	F_BM32B		file contains BM32B code (as opposed to strictly BM32A)
 *	F_BM32MAU	file requires MAU (math arith unit) to execute
 */

#define	F_BM32ID	0160000
#define	F_BM32MAU	0040000
#define F_BM32B         0020000

/*	F_BM32RST	file has RESTORE work-around	*/

#define F_BM32RST	0010000

/*
 *	Flags for the INTEL chips.  If the magic number of the object file
 *	is IAPX16 or IAPX16TV or IAPX20 or IAPX20TV then if F_80186
 *	is set, there are some 80186 instructions in the code, and hence
 *	and 80186 or 80286 chip must be used to run the code.
 *	If F_80286 is set, then the code has to be run on an 80286 chip.
 *	And if neither are set, then the code can run on an 8086, 80186, or
 *	80286 chip.
 *	
 */

#define F_80186		010000
#define F_80286		020000

/*
 *   Magic Numbers
 */

	/* iAPX - the stack frame and return registers differ from
	 * 	  Basic-16 and x86 C compilers, hence new magic numbers
	 *	  are required.  These are cross compilers.
	 */

	/* Intel */
#define  IAPX16		0504
#define  IAPX16TV	0505
#define  IAPX20		0506
#define  IAPX20TV	0507
/* 0514, 0516 and 0517 reserved for Intel */

	/* Basic-16 */

#define  B16MAGIC	0502
#define  BTVMAGIC	0503


	/* x86 */

#define  X86MAGIC	0510
#define  XTVMAGIC	0511

	/* Intel 286 */
#define I286SMAGIC	0512
#define I286LMAGIC	0522	/* used by mc68000 (UNIX PC) and iAPX 286 */

	/* n3b */
/*
 *   NOTE:   For New 3B, the old values of magic numbers
 *		will be in the optional header in the structure
 *		"aouthdr" (identical to old 3B aouthdr).
 */
#define  N3BMAGIC	0550	/* 3B20 executable, no TV */
#define  NTVMAGIC	0551	/* 3B20 executable with TV */

	/*  MAC-32, 3B15, 3B5  */

#define  WE32MAGIC	0560	/* WE 32000, no TV */
#define  FBOMAGIC	0560	/* WE 32000, no TV */
#define  RBOMAGIC	0562	/* reserved for WE 32000 */
#define  MTVMAGIC	0561	/* WE 32000 with TV */


	/* VAX 11/780 and VAX 11/750 */

			/* writeable text segments */
#define VAXWRMAGIC	0570
			/* readonly sharable text segments */
#define VAXROMAGIC	0575

	/* pdp11 */
/*			0401	UNIX-rt ldp */
/*			0405	pdp11 overlay */
/*			0407	pdp11/pre System V vax executable */
/*			0410	pdp11/pre System V vax pure executable */
/*			0411	pdp11 seperate I&D */
/*			0437	pdp11 kernel overlay */


	/* Motorola 68000/68008/68010/68020 */
#define	MC68MAGIC	0520
#define MC68KWRMAGIC	0520	/* writeable text segments */
#define	MC68TVMAGIC	0521
#define MC68KROMAGIC	0521	/* readonly shareable text segments */
#define MC68KPGMAGIC	0522	/* demand paged text segments */
#define	M68MAGIC	0210
#define	M68TVMAGIC	0211


	/* IBM 370 */
#define	U370WRMAGIC	0530	/* writeble text segments	*/
#define	U370ROMAGIC	0535	/* readonly sharable text segments	*/
/* 0532 and 0533 reserved for u370 */

	/* Amdahl 470/580 */
#define AMDWRMAGIC	0531	/* writable text segments */
#define AMDROMAGIC	0534	/* readonly sharable text segments */

	/* NSC */
/* 0524 and 0525 reserved for NSC */

	/* Zilog */
/* 0544 and 0545 reserved for Zilog */

#define	FILHDR	struct filehdr
#define	FILHSZ	sizeof(FILHDR)

#define ISCOFF(x) \
		(((x)==B16MAGIC) || ((x)==BTVMAGIC) || ((x)==X86MAGIC) \
		|| ((x)==XTVMAGIC) || ((x)==N3BMAGIC) || ((x)==NTVMAGIC) \
		|| ((x)==FBOMAGIC) || ((x)==VAXROMAGIC) || ((x)==VAXWRMAGIC) \
		|| ((x)==RBOMAGIC) || ((x)==MC68TVMAGIC) \
		|| ((x)==MC68MAGIC) || ((x)==M68MAGIC) || ((x)==M68TVMAGIC) \
		|| ((x)==IAPX16) || ((x)==IAPX16TV) \
		|| ((x)==IAPX20) || ((x)==IAPX20TV) \
		|| ((x)==U370WRMAGIC) || ((x)==U370ROMAGIC) || ((x)==MTVMAGIC) \
		|| ((x)==I286SMAGIC) || ((x)==I286LMAGIC) \
		|| ((x)==MC68KWRMAGIC) || ((x)==MC68KROMAGIC) \
		|| ((x)==MC68KPGMAGIC))

/*ident	"@(#)head:aouthdr.h	2.6"*/
typedef	struct aouthdr {
	int16_t	magic;		/* see magic.h				*/
	int16_t	vstamp;		/* version stamp			*/
	int32_t	tsize;		/* text size in bytes, padded to FW bdry */
	int32_t	dsize;		/* initialized data "  "		*/
	int32_t bsize;		/* uninitialized data "   "		*/
	int32_t entry;		/* entry pt.				*/
	int32_t	text_start;	/* base of text used for this file	*/
	int32_t data_start;	/* base of data used for this file	*/
} AOUTHDR;

/*ident	"@(#)sgs-inc:common/scnhdr.h	1.10"*/
struct scnhdr {
	char		s_name[8];	/* section name */
	int32_t		s_paddr;	/* physical address, aliased s_nlib */
	int32_t		s_vaddr;	/* virtual address */
	int32_t		s_size;		/* section size */
	int32_t		s_scnptr;	/* file ptr to raw data for section */
	int32_t		s_relptr;	/* file ptr to relocation */
	int32_t		s_lnnoptr;	/* file ptr to line numbers */
	uint16_t	s_nreloc;	/* number of relocation entries */
	uint16_t	s_nlnno;	/* number of line number entries */
	int32_t		s_flags;	/* flags */
};

/* the number of shared libraries in a .lib section in an absolute output file
 * is put in the s_paddr field of the .lib section header, the following define
 * allows it to be referenced as s_nlib
 */

#define s_nlib	s_paddr
#define	SCNHDR	struct scnhdr
#define	SCNHSZ	sizeof(SCNHDR)

/*
 * Define constants for names of "special" sections
 */

#define _TEXT ".text"
#define _DATA ".data"
#define _BSS  ".bss"
#define _TV  ".tv"
#define _INIT ".init"
#define _FINI ".fini"

/*
 * The low 2 bytes of s_flags is used as a section "type"
 */

#define STYP_REG	0x00		/* "regular" section:
						allocated, relocated, loaded */
#define STYP_DSECT	0x01		/* "dummy" section:
						not allocated, relocated,
						not loaded */
#define STYP_NOLOAD	0x02		/* "noload" section:
						allocated, relocated,
						 not loaded */
#define STYP_GROUP	0x04		/* "grouped" section:
						formed of input sections */
#define STYP_PAD	0x08		/* "padding" section:
						not allocated, not relocated,
						 loaded */
#define STYP_COPY	0x10		/* "copy" section:
						for decision function used
						by field update;  not
						allocated, not relocated,
						loaded;  reloc & lineno
						entries processed normally */
#define STYP_INFO	0x200		/* comment section : not allocated
						not relocated, not loaded */
#define STYP_LIB	0x800		/* for .lib section : same as INFO */
#define STYP_OVER	0x400		/* overlay section : relocated
						not allocated or loaded */
#define	STYP_TEXT	0x20		/* section contains text only */
#define STYP_DATA	0x40		/* section contains data only */
#define STYP_BSS	0x80		/* section contains bss only */

/*
 *  In a minimal file or an update file, a new function
 *  (as compared with a replaced function) is indicated by S_NEWFCN
 */

#define S_NEWFCN  0x100

/*
 * In 3b Update Files (output of ogen), sections which appear in SHARED
 * segments of the Pfile will have the S_SHRSEG flag set by ogen, to inform
 * dufr that updating 1 copy of the proc. will update all process invocations.
 */

#define S_SHRSEG	0x20


/*
 *  static char ID[] = "@(#) reloc.h: 1.3 12/29/82";
 */
struct coffreloc {
	int32_t	r_vaddr;	/* (virtual) address of reference */
	int32_t	r_symndx;	/* index into symbol table */
	int16_t	r_type;		/* relocation type */
};


/*
 *   relocation types for all products and generics
 */

/*
 * All generics
 *	reloc. already performed to symbol in the same section
 */
#define  R_ABS		0

/*
 * X86 generic
 *	8-bit offset reference in 8-bits
 *	8-bit offset reference in 16-bits 
 *	12-bit segment reference
 *	auxiliary relocation entry
 */
#define	R_OFF8		07
#define R_OFF16		010
#define	R_SEG12		011
#define	R_AUX		013

/*
 * B16 and X86 generics
 *	16-bit direct reference
 *	16-bit "relative" reference
 *	16-bit "indirect" (TV) reference
 */
#define  R_DIR16	01
#define  R_REL16	02
#define  R_IND16	03

/*
 * 3B generic
 *	24-bit direct reference
 *	24-bit "relative" reference
 *	16-bit optimized "indirect" TV reference
 *	24-bit "indirect" TV reference
 *	32-bit "indirect" TV reference
 */
#define  R_DIR24	04
#define  R_REL24	05
#define  R_OPT16	014
#define  R_IND24	015
#define  R_IND32	016

/* 
 * XL generics
 *	10-bit direct reference
 *	10-bit "relative" reference
 *	32-bit "relative" reference
 */
#define	R_DIR10		025
#define R_REL10		026
#define R_REL32		027

/*
 * 3B and M32 generics
 *	32-bit direct reference
 */
#define  R_DIR32	06

/*
 * M32 generic
 *	32-bit direct reference with bytes swapped
 */
#define  R_DIR32S	012
 
 /*
  * DEC Processors  VAX 11/780 and VAX 11/750 
  *
  */
 
#define R_RELBYTE	017
#define R_RELWORD	020
#define R_RELLONG	021
#define R_PCRBYTE	022
#define R_PCRWORD	023
#define R_PCRLONG	024


#define	RELOC	struct coffreloc
#define	RELSZ	10	/* sizeof(RELOC) */

	/* Definition of a "TV" relocation type */

#if N3B || U3B
#define ISTVRELOC(x)	((x==R_OPT16)||(x==R_IND24)||(x==R_IND32))
#endif
#if B16 || X86
#define ISTVRELOC(x)	(x==R_IND16)
#endif
#if M32
#define ISTVRELOC(x)	(x!=x)	/* never the case */
#endif

/*
 * static char ID_lnumh[] = "@(#) linenum.h: 1.1 1/7/82";
 */

/*  There is one line number entry for every 
    "breakpointable" source line in a section.
    Line numbers are grouped on a per function
    basis; the first entry in a function grouping
    will have l_lnno = 0 and in place of physical
    address will be the symbol table index of
    the function name.
*/
struct lineno
{
	union
	{
		int32_t	l_symndx ;	/* sym. table index of function name
						iff l_lnno == 0      */
		int32_t	l_paddr ;	/* (physical) address of line number */
	} 			l_addr ;
	uint16_t	l_lnno ;	/* line number */
} ;

#define	LINENO	struct lineno
#define	LINESZ	6	/* sizeof(LINENO) */

/*
 * static char ID_sclsh[] = "@(#) storclass.h: 1.1 1/7/82";
 */

/*
 *   STORAGE CLASSES
 */

#define  C_EFCN          -1    /* physical end of function */
#define  C_NULL          0
#define  C_AUTO          1     /* automatic variable */
#define  C_EXT           2     /* external symbol */
#define  C_STAT          3     /* static */
#define  C_REG           4     /* register variable */
#define  C_EXTDEF        5     /* external definition */
#define  C_LABEL         6     /* label */
#define  C_ULABEL        7     /* undefined label */
#define  C_MOS           8     /* member of structure */
#define  C_ARG           9     /* function argument */
#define  C_STRTAG        10    /* structure tag */
#define  C_MOU           11    /* member of union */
#define  C_UNTAG         12    /* union tag */
#define  C_TPDEF         13    /* type definition */
#define C_USTATIC	 14    /* undefined static */
#define  C_ENTAG         15    /* enumeration tag */
#define  C_MOE           16    /* member of enumeration */
#define  C_REGPARM	 17    /* register parameter */
#define  C_FIELD         18    /* bit field */
#define  C_BLOCK         100   /* ".bb" or ".eb" */
#define  C_FCN           101   /* ".bf" or ".ef" */
#define  C_EOS           102   /* end of structure */
#define  C_FILE          103   /* file name */

	/*
	 * The following storage class is a "dummy" used only by STS
	 * for line number entries reformatted as symbol table entries
	 */

#define  C_LINE		 104
#define  C_ALIAS	 105   /* duplicate tag */
#define  C_HIDDEN	 106   /* special storage class for external */
			       /* symbols in dmert public libraries  */




/*		Number of characters in a symbol name */
#define  SYMNMLEN	8
/*		Number of characters in a file name */
#define  FILNMLEN	14
/*		Number of array dimensions in auxiliary entry */
#define  DIMNUM		4

/*
 * static char ID_symsh[] = "@(#) syms.h: 1.5 2/24/83";
 */

#pragma pack(1)
//pragma align(1)
struct syment
{
	union
	{
		char		_n_name[SYMNMLEN];	/* old COFF version */
		struct
		{
			int32_t	_n_zeroes;	/* new == 0 */
			int32_t	_n_offset;	/* offset into string table */
		} _n_n;
/*		char		*_n_nptr[2];*/	/* allows for overlaying */
	} _n;
	int32_t			n_value;	/* value of symbol */
	int16_t			n_scnum;	/* section number */
	uint16_t		n_type;		/* type and derived type */
	int8_t			n_sclass;	/* storage class */
	int8_t			n_numaux;	/* number of aux. entries */
};

#define n_name		_n._n_name
#define n_nptr		_n._n_nptr[1]
#define n_zeroes	_n._n_n._n_zeroes
#define n_offset	_n._n_n._n_offset

/*
   Relocatable symbols have a section number of the
   section in which they are defined.  Otherwise, section
   numbers have the following meanings:
*/
        /* undefined symbol */
#define  N_UNDEF	0
        /* value of symbol is absolute */
#define  N_ABS          -1
        /* special debugging symbol -- value of symbol is meaningless */
#define  N_DEBUG	-2
	/* indicates symbol needs transfer vector (preload) */
#define  N_TV		(unsigned short)-3

	/* indicates symbol needs transfer vector (postload) */

#define  P_TV		(unsigned short)-4

/*
   The fundamental type of a symbol packed into the low 
   4 bits of the word.
*/

#define  _EF	".ef"

#define  T_NULL     0
#define  T_ARG      1          /* function argument (only used by compiler) */
#define  T_CHAR     2          /* character */
#define  T_SHORT    3          /* short integer */
#define  T_INT      4          /* integer */
#define  T_LONG     5          /* long integer */
#define  T_FLOAT    6          /* floating point */
#define  T_DOUBLE   7          /* double word */
#define  T_STRUCT   8          /* structure  */
#define  T_UNION    9          /* union  */
#define  T_ENUM     10         /* enumeration  */
#define  T_MOE      11         /* member of enumeration */
#define  T_UCHAR    12         /* unsigned character */
#define  T_USHORT   13         /* unsigned short */
#define  T_UINT     14         /* unsigned integer */
#define  T_ULONG    15         /* unsigned long */

/*
 * derived types are:
 */

#define  DT_NON      0          /* no derived type */
#define  DT_PTR      1          /* pointer */
#define  DT_FCN      2          /* function */
#define  DT_ARY      3          /* array */

/*
 *   type packing constants
 */

#define  N_BTMASK     017
#define  N_TMASK      060
#define  N_TMASK1     0300
#define  N_TMASK2     0360
#define  N_BTSHFT     4
#define  N_TSHIFT     2

/*
 *   MACROS
 */

	/*   Basic Type of  x   */

#define  BTYPE(x)  ((x) & N_BTMASK)

	/*   Is  x  a  pointer ?   */

#define  ISPTR(x)  (((x) & N_TMASK) == (DT_PTR << N_BTSHFT))

	/*   Is  x  a  function ?  */

#define  ISFCN(x)  (((x) & N_TMASK) == (DT_FCN << N_BTSHFT))

	/*   Is  x  an  array ?   */

#define  ISARY(x)  (((x) & N_TMASK) == (DT_ARY << N_BTSHFT))

	/* Is x a structure, union, or enumeration TAG? */

#define ISTAG(x)  ((x)==C_STRTAG || (x)==C_UNTAG || (x)==C_ENTAG)

#define  INCREF(x) ((((x)&~N_BTMASK)<<N_TSHIFT)|(DT_PTR<<N_BTSHFT)|(x&N_BTMASK))

#define  DECREF(x) ((((x)>>N_TSHIFT)&~N_BTMASK)|((x)&N_BTMASK))

/*
 *	AUXILIARY ENTRY FORMAT
 */

#pragma pack(1)
//pragma align(1)
union auxent
{
	struct
	{
		int32_t		x_tagndx;	/* str, un, or enum tag indx */
		union
		{
			struct
			{
				uint16_t	x_lnno;	/* declaration line number */
				uint16_t	x_size;	/* str, union, array size */
			} x_lnsz;
			int32_t	x_fsize;	/* size of function */
		} x_misc;
		union
		{
			struct			/* if ISFCN, tag, or .bb */
			{
				int32_t	x_lnnoptr;	/* ptr to fcn line # */
				int32_t	x_endndx;	/* entry ndx past block end */
			} 	x_fcn;
			struct			/* if ISARY, up to 4 dimen. */
			{
				uint16_t	x_dimen[DIMNUM];
			} 	x_ary;
		}		x_fcnary;
		uint16_t  x_tvndx;		/* tv index */
	} 	x_sym;
	struct
	{
		char	x_fname[FILNMLEN];
	} 	x_file;
	struct
	{
		int32_t		x_scnlen;          /* section length */
		uint16_t	x_nreloc;  /* number of relocation entries */
		uint16_t	x_nlinno;  /* number of line numbers */
	}	x_scn;

	struct
	{
		int32_t		x_tvfill;	/* tv fill value */
		uint16_t		x_tvlen;	/* length of .tv */
		uint16_t	x_tvran[2];	/* tv range */
	}	x_tv;	/* info about .tv section (in auxent of symbol .tv)) */
};

#define	SYMENT	struct syment
#define	SYMESZ	18	/* sizeof(SYMENT) */

#define	AUXENT	union auxent
#define	AUXESZ	18	/* sizeof(AUXENT) */

/*	Defines for "special" symbols   */

#define _ETEXT	"_etext"
#define	_EDATA	"_edata"
#define	_END	"_end"

#define _START	"_start"

#define _TVORIG	"_tvorig"
#define _TORIGIN	"_torigin"
#define _DORIGIN	"_dorigin"

#define _SORIGIN	"_sorigin"

#endif
