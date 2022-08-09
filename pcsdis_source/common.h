#ifndef __COMMON_H__
#define __COMMON_H__

#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>

#include <string>
#include <vector>
#include <deque>
#include <map>
#include <memory>
#include <algorithm>

#include <cassert>
#include <cstdio>
#include <cstring>
#include <cctype>

#define DIAGstream	std::cout

enum DiagFlag {
	DIAG_none = '-',
	DIAG_all = '+',
	DIAGbb = 'b',
	DIAGcase = 'c',
	DIAGexpr = 'e',
	DIAGfileread = 'f',
	DIAGinstr = 'i',
	DIAGloop = 'l',
	DIAGreloc = 'o',
	DIAGproc = 'p',
	DIAGsegment = 's',
	DIAGtrace = 't',
	DIAGsymbol = 'y',
};

enum InstrType {
	ITYPEunknown = 0,
	ITYPEjump =	1,
	ITYPEcond = 2,
	ITYPEmove = 3,
	ITYPEalu = 4,
	ITYPEbalu = 5,
	ITYPEcall = 6,
	ITYPEexit = 7,
	ITYPEcase1 = 8,
	ITYPEcase2 = 9,
	ITYPEpush =	10,
	ITYPEother = 11,	/*usually not in compiled code */
	ITYPEivar =	12,
	ITYPEcvar =	13,
	ITYPEpvar =	14,
	ITYPEbss = 15
};

enum ItemType {
	ITEMunknown = 0,
	ITEMbyte = 1,
	ITEMword = 2,
	ITEMlong = 3,
	ITEMinstr = 4,
	ITEMreloc = 5,
};

enum OpType {
	OP_INVALID = 0,
	OP_CCR = 1,
	OP_IMM = 2,
	OP_RTM = 3,
	OP_CALLM = 4,
	OP_CMPCHK2 = 5,
	OP_BITI = 6,
	OP_MOVES = 7,
	OP_CAS2 = 8,
	OP_CAS = 9,
	OP_BITD = 10,
	OP_MOVEP = 11,
	OP_MOVEB = 12,
	OP_MOVEW = 13,
	OP_MOVEL = 14,
	OP_MVSTS1 = 15,
	OP_MVSTC1 = 16,
	OP_MVSTS2 = 17,
	OP_MVSTC2 = 18,
	OP_MONOPX = 19,
	OP_EXTX = 20,
	OP_LINKX = 21,
	OP_MONOP = 22,
	OP_SWAP = 23,
	OP_BKPT = 24,
	OP_PEA = 25,
	OP_IMPLIED = 26,
	OP_CHK = 27,
	OP_MULDIVL = 28,
	OP_TRAP = 29,
	OP_UNLK = 30,
	OP_USP = 31,
	OP_MOVEM = 32,
	OP_IARG = 33,
	OP_MOVEC = 34,
	OP_LEA = 35,
	OP_QUICK = 36,
	OP_DBCC = 37,
	OP_IARGW = 38,
	OP_IARGL = 39,
	OP_SCC = 40,
	OP_BRX = 41,
	OP_MOVEQ = 42,
	OP_BCD = 43,
	OP_BCDX = 44,
	OP_BCD3 = 45,
	OP_MULDIVW = 46,
	OP_ALU = 47,
	OP_ALUA = 48,
	OP_DC = 49,
	OP_CMPM = 50,
	OP_EXG = 51,
	OP_SHFT = 52,
	OP_SHFTX = 53,
	OP_BF = 54,
	OP_BFD = 55,
	OP_BFS = 56,
	OP_JUMP = 57,
	OP_IMPLIEDEND = 58,
	OP_IARGEND = 59,
	OP_BRXEND = 60,
	OP_SR = 61,
};

enum OpWidth {
	OPWnone = -1,
	OPWshort = 0,
	OPWbyte = 1,
	OPWword = 2,
	OPWlong = 4,
	OPWext = 8
};

enum RegType {
	REGdata = 0,
	REGptr = 8		// this is the offset into register array!
};

enum DomDir {
	DOMfwd = 0,		// forward dominator
	DOMback = 1		// backward dominator
};

enum ArgType {
	ARG_none = 0,
	ARG_RegArg = 1,
	ARG_NumArg = 4,
	ARG_XnArg = 7,
	ARG_LblArg = 9,
	ARG_AindArg = 10,
	ARG_ApiArg = 11,
	ARG_ApdArg = 12,
	ARG_Aidx2Arg = 13,
	ARG_BfArg = 14,
	ARG_RelocArg = 15,
	ARG_IndexArg = 16,
	ARG_IndexPreArg = 17,
	ARG_IndexPostArg = 18,
	ARG_IndexBaseArg = 19,
	ARG_StringArg = 20
};

enum ExprType {
	EXPRunknown = 0,
	EXPRnumber = 1,
	EXPRstring = 2,
	EXPRvar = 3,
	EXPRarg = 4,
	EXPR1 = 5,
	EXPR2 = 6,
	EXPR3 = 7,
	EXPRareg = 8,
	EXPRdreg = 9,
	EXPRfunc = 10,
	EXPRasm = 11,
};

enum ExprAssoc {
	EXASSOCnone = 0,
	EXASSOCltor = 1,
	EXASSOCrtol = 2
};

enum ExprModifier {
	EXMODnone = 0,
	EXMODdec = 1,
	EXMODoct = 2,
	EXMODhex = 3
};

enum ExprOp {
	EXOP_unknown = 0,		//
	EXOP_PAREN = 1,			// (x)
	EXOP_BRACKET = 2,		// [x]
	EXOP_PTROFF = 3,		// x->y
	EXOP_OFF = 4,			// x.y
	EXOP_BOOLNOT = 5,		// !x
	EXOP_BITNOT = 6,		// ~x
	EXOP_LINC = 7,			// ++x
	EXOP_RINC = 8,			// x++
	EXOP_LDEC = 9,			// --x
	EXOP_RDEC = 10,			// x--
	EXOP_PLUS = 11,			// +x
	EXOP_MINUS = 12,		// -x
	EXOP_DEREF = 13,  		// *x
	EXOP_ADDR = 14,			// &x
	EXOP_CAST2 = 15,		// (shorttype)x
	EXOP_SIZEOF = 16,		// sizeof(x)

	/* need to be in that order */
	EXOP__MUL = 17,			// temporary
	EXOP_UMUL = 18,			// x*y unsigned
	EXOP_UMULASN = 19,		// x*=y unsigned
	EXOP_SMUL = 20,			// x*y signed
	EXOP_SMULASN = 21,		// x*=y signed
	EXOP__DIV = 22,			// temporary
	EXOP_UDIV = 23,			// x/y unsigned
	EXOP_UDIVASN = 24,		// x/=y unsigned
	EXOP_SDIV = 25,			// x/y signed
	EXOP_SDIVASN = 26,		// x/=y signed
	EXOP__MOD = 27,			// temporary
	EXOP_UMOD = 28,			// x%y unsigned
	EXOP_UMODASN = 29,		// x%y unsigned
	EXOP_SMOD = 30,			// x%y signed
	EXOP_SMODASN = 31,		// x%y signed

	EXOP_ADD = 32,			// x+y
	EXOP_SUB = 33,			// x-y
	
	EXOP_LSH = 35,			// x<<y
	EXOP_LSHASN = 36,		// x<<=y
	EXOP_URSH = 38,			// x>>y unsigned
	EXOP_URSHASN = 39,		// x>>=y unsigned
	EXOP_SRSH = 41,			// x>>y	signed
	EXOP_SRSHASN = 42,		// x>>=y signed

	EXOP_ULT = 43,			// x<y unsigned
	EXOP_SLT = 44,			// x<y signed
	EXOP_ULE = 45,			// x<=y unsigned
	EXOP_SLE = 46,			// x<=y signed
	EXOP_UGT = 47,			// x>y unsigned
	EXOP_SGT = 48,			// x>y signed
	EXOP_UGE = 49,			// x>=y unsigned
	EXOP_SGE = 50,			// x>=y signed
	EXOP_EQ = 51,			// x==y
	EXOP_NE = 52,			// x!=y
	EXOP_BITAND = 53,		// x&y
	EXOP_BITXOR = 54,		// x^y
	EXOP_BITOR = 55,		// x|y
	EXOP_BOOLAND = 56,		// x&&y
	EXOP_BOOLOR = 57,		// x||y
	EXOP_TERNARY = 58,		// x?y:z
	EXOP_ASN = 59,			// x=y

	EXOP_ADDASN = 60,		// x+=y
	EXOP_SUBASN = 61,		// x-=y
	EXOP_ANDASN = 62,		// x&=y
	EXOP_XORASN = 63,		// x^=y
	EXOP_ORASN = 64,		// x|=y

	EXOP_COMMA = 65,		// x,y
	EXOP_null = 66,			// null expression
	EXOP_scale = 67,		// synthetic scale(x, n)
	EXOP_bit = 68,			// synthetic bitoffset(off, sz)
	EXOP_push = 69,			// synthetic stack push
	EXOP_clr = 70,			// synthetic, translated into move #0, ea
	EXOP_tst = 71,			// synthetic, translated into cmp #0, ea
	EXOP_cmp = 72,			// synthetic, merged to boolop
	EXOP_func = 73,			// function call,
	EXOP_CAST4 = 74,		// (longtype)x
	EXOP_ones = 75,			// synthetic, bitset of all ones
	EXOP_zeroes = 76,			// synthetic, bitset of all zeros
};

/* convert between Mul, Div and Mod */
inline ExprOp toAOp(ExprOp op) { return (ExprOp)(op+1); }
inline ExprOp toUOp(ExprOp op) { return (ExprOp)(op+1); }
inline ExprOp toUAOp(ExprOp op) { return (ExprOp)(op+2); }
inline ExprOp toSOp(ExprOp op) { return (ExprOp)(op+3); }
inline ExprOp toSAOp(ExprOp op) { return (ExprOp)(op+4); }
inline ExprOp divToMod(ExprOp op) { return (ExprOp)(op+5); }


enum ExprOpt {
	EXOPTnormal = 0,		// unmodified expression
	EXOPTsecond = 1,		// return second interpretation
};

enum StmtType {
	STMTnone = 0,
	STMTif = 2,			/*IF*/
	STMTdowhile = 5,
	STMTreturn = 6,
	STMTexpr = 7,
	STMT_case1 = 10,	/*temporary*/
	STMT_case2 = 11, 	/*temporary*/
	STMTgoto = 12,		/*GOTO*/
	STMT_table = 13, 	/*temporary*/
	STMT_null = 14,
	STMTcontinue = 15, 	/*CONTINUE */
	STMTbreak = 16, 	/*BREAK */
	STMTswitch = 17, 	/*SWITCH */
	STMTwhile = 18		/*WHILE-DO */
};

/*****************************************************************************/

/* abbreviation */
typedef std::string String;
typedef const String cString;

/*forward*/class _Symbol;
typedef std::shared_ptr<_Symbol> Symbol;
typedef const Symbol cSymbol;
typedef std::vector<Symbol> SymbolVector;

/*forward*/class _Reloc;
typedef std::shared_ptr<_Reloc> Reloc;
typedef std::vector<Reloc> RelocVector;

/*forward*/class _Instr;
typedef std::shared_ptr<_Instr> Instr;
typedef std::vector<Instr> InstrVector;

/*forward*/class _Expr;
typedef std::shared_ptr<_Expr> Expr;
typedef const Expr cExpr;
typedef std::vector<Expr> ExprVector;

/*forward*/ class _BB;
typedef std::shared_ptr<_BB> BB;
typedef std::vector<BB> BBVector;
typedef const BBVector cBBVector;

/*forward*/ class _Stmt;
typedef std::shared_ptr<_Stmt> Stmt;
typedef std::vector<Stmt> StmtVector;

/*forward*/class _CLoop;
typedef std::shared_ptr<_CLoop> CLoop;
typedef std::vector<CLoop> CLoopVector;

/*forward*/ class _CProc;
typedef std::shared_ptr<_CProc> CProc;
typedef std::vector<CProc> CProcVector;

/*forward*/ class _Node;
typedef std::shared_ptr<_Node> Node;
typedef std::vector<Node> NodeVector;

/*forward*/class Segment;
/*forward*/class ExprFactory;
/*forward*/class InstrFactory;

/*****************************************************************************/

char *Strdup(const char *s);
char* Strdup8(const char *s);
char* Strdup14(const char *s);
void dprint(std::ostream& os, cString& f, const char *v);
void Indent(std::ostream& os, int indent);
void Indent(std::ostream& os, int indent, cString& prefix);
void IndentNoCR(std::ostream& os, int indent, cString& prefix);
String Comment(cString& text);

/*****************************************************************************/

template<typename T>
inline void dprint(std::ostream& os, cString& f, T v)
{
	os << f << std::hex << v << std::endl;
}

template<typename T>
inline void swap2(T& in)
{
	union {
		T i;
		int8_t b[2];
	} u;
	u.i = in;
	int8_t s = u.b[1];
	u.b[1] = u.b[0];
	u.b[0] = s;
	in = u.i;
}

template<typename T>
inline void swap4(T& in)
{
	union {
		T i;
		int8_t b[4];
	} u;
	u.i = in;
	int8_t s = u.b[3];
	u.b[3] = u.b[0];
	u.b[0] = s;
	s = u.b[2];
	u.b[2] = u.b[1];
	u.b[1] = s;
	in = u.i;
}

template<typename T>
String to_hexstring(T i)
{
	std::stringstream str;
	str << std::hex << i;
	return str.str();
}

template<typename T>
String to_octstring(T i)
{
	std::stringstream str;
	str << std::oct << i;
	return str.str();
}

class Diag 
{
protected:
	static bool flagall;
	static std::map<DiagFlag,bool> flags;
	static cString flag2name(DiagFlag f);
public:
	Diag() {}
	~Diag() {}

	static bool IsDiag(DiagFlag);
	static bool NoDiag(DiagFlag f) { return !IsDiag(f); }
	static void Info(DiagFlag, cString& s1);
	static void Info(cString& s1);
	static void Trace(DiagFlag f, cString& s1);
	static void Error(cString& s1);
	static void Fatal(cString& msg, const char* file, int line);
#define FATALERROR(msg) Diag::Fatal(msg, __FILE__, __LINE__)

	static void Set(DiagFlag, bool tf);
};

#endif
