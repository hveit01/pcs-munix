#ifndef __INSTR_H__
#define __INSTR_H__

//include "cofffile.h"
#include "item.h"
#include "arg.h"

/*abstract*/class _Instr : public _Item
{
protected:
	Instr self;
	ItemVector words;
	Segment *seg;
	int size;
	bool end, endbb;
	String opstr;
	int opcode;
	Arg *arg1, *arg2, *arg3;
	Expr expr;
	OpWidth width;
	bool valid;
	
	int uword(); // read next word from instr stream
	int ulong(); // read next long from instr stream
	int sword(); // read next word from instr stream
	int slong(); // read next long from instr stream
	
	String mnemo(Item op) const;

	/* select some register fields */
	int offarg(int arg);
	int low(int arg) const { return arg & 0xff; }
	int rx() const { return opcode & 07; }
	int rx(int arg) const { return arg & 07; }
	int r8(int arg) const { return (arg >> 6) & 07; }
	int ry() const { return (opcode >> 9) & 07; }
	// note: r15 is bits 15-12, == r14 for data regs
	int r15(int arg) const { return arg >> 12; }
	
	/* produce some standard arguments */
	Arg *dareg15(int extw) const { return new XnArg(r15(extw), OPWnone, 1); }

	/* bit field selection fields */
	bool DO(int extw) const { return extw & 04000; }
	int OFFSET(int extw) const { return (extw >> 6) & 037; }
	bool DW(int extw) const { return extw & 040; }
	int WIDTH(int extw) const { return extw & 037; }

	/* these bits swap the arguments (direction flag) */
	bool dir0() const { return opcode & 1; }
	bool dir7() const { return opcode & 0x80; }
	bool dir8() const { return opcode & 0x100; }
	bool dir10() const { return opcode & 0x400; }
	bool dir11(int extw) const { return extw & 0x800; }

	bool IR() const { return opcode & 040; }
	bool RM() const { return opcode & 010; }

	/* mode bits in extw of 68020 instrs */
	bool IS(int extw) const { return extw & 0x40; }
	bool BS(int extw) const { return extw & 0x80; }

	/* classic 68000 index mode */
	bool idx68000(int extw) const { return !(extw & 0x100); }

	/* mul/div 64 bit register pairs */
	bool pair10(int extw) const { return extw & 0x400; }

	/* opcode extension for some strange instructions */
	bool op11(int extw) const { return extw & 0x800; }
		
	/* operation width selection bits */
	OpWidth width6() const { return (opcode & 0x40) ? OPWlong : OPWword; }
	OpWidth width67() const; // uses bits 7-6
	OpWidth width8() const { return (opcode & 0x100) ? OPWlong : OPWword; }
	OpWidth width109() const; // uses bist 10-9
	OpWidth width11(int extw) const { return (extw & 0x800) ? OPWlong : OPWword; }
		
	Arg *imm(OpWidth w);
	int scale(int extw) const;
	Arg *ea(uint16_t op, OpWidth isz=OPWnone);
	Arg *eaext(int, uint16_t);
	Arg *bfarg(int arg);
	Arg *cpureg(int arg) const { return new SpecArg(arg); }
	cString& shftop(int n) const;
	void swapargs(bool sw);
	void enqueue(int addr, cString& where="") const;
	Arg *local(int addr) const;
	void invalidinstr();
	
	static Segment *curseg;
	
	void linkbb(BB mybb, StmtType styp) const;

public:
	static void SetFocus(Segment *seg) { curseg = seg; }

	_Instr(int addr);
	_Instr(Item op);
	virtual ~_Instr();
	Instr Self(Instr s=0) { if (s) self = s; return s; }
	bool operator==(const Instr& other) const;
	
	ItemType IsA() const { return ITEMinstr; }
	
	Arg* Arg1() const { return arg1; }
	Arg* Arg2() const { return arg2; }
	Arg* Arg3() const { return arg3; }

	virtual InstrType IType() const { return ITYPEunknown; }
	virtual void FixType() { /*only for special postfixing cases */ }
	virtual int FixTarget() { return 0; /* only for special postfixing cases */ }
	static cString LField(OpWidth w);
	
	int Size() const { return size; }
	int Width() const { return width; }
	bool IsEnd() const { return end; }
	void SetEnd() { end = true; }
	bool IsEndBB() const { return endbb; }
	virtual bool IsBranch() const { return false; }
	void SetEndBB() { endbb = true; }
	bool IsValid() const { return valid; }
	void Invalidate() { valid = false; }
	
	const ItemVector& Words() const { return words; }
	virtual bool IsCondBranch() const { return false; }
	int Opcode() const { return opcode; }
	virtual Symbol Sym() const;
	virtual cString SymName() const;
	virtual int Target() const { assert(0); return -1; }
	virtual int Value() const { assert(0); return -1; }
	virtual void LinkBB(BB mybb) const;
	virtual Expr GetExpr() const;
	virtual Expr GetExpr2() const { return 0; }
	virtual Expr MakeExpr(BB mybb);

	virtual cString toAsmString() const;
	virtual cString toExprString() const;
	
	virtual int getRegs() const { return 0; }
	virtual int getParam() const; // -1 if not
	virtual int getVar() const; // -1 if not
	virtual int getWidth() const { return (int)width; }
	
	virtual void WriteAsm(std::ostream& os) const;
	virtual void DumpInstr(std::ostream& os) const;
};

/****************************************************************************
 * The following instrs do not normally occur in compiler
 * generated code
 ****************************************************************************/

/* avoid C++-17 */
#define NCC(kind) NonCc ## kind ## Instr
#define NCCclass(kind,itype,stype)\
  class NCC(kind) : public _Instr {\
  public:\
    NCC(kind)(Item op);\
	InstrType IType() const { return itype; }\
	void LinkBB(BB mybb) const { linkbb(mybb, stype); }\
  }
#define NCCctor(kind) NCC(kind)::NCC(kind)(Item op)\
  : _Instr(op)

/* OP_CCR: instruction class "op ea, CCR" */
NCCclass(Ccr, ITYPEmove, STMTexpr);

/* OP_SR: instruction class "op ea, SR" */
NCCclass(Sr, ITYPEmove, STMTexpr);

/* OP_RTM: instruction class "rtm #imm" */
NCCclass(Rtm, ITYPEexit, STMTnone);

/* OP_CALLM: instruction class "callm #imm, ea" */
NCCclass(Callm, ITYPEcall, STMTnone);

/* OP_CMPCHK2: instruction class "cmp2.x/chk2.x RX, ea" */
NCCclass(CmpChk2, ITYPEother, STMTnone);

/* OP_MOVES: instruction class "moves.x RX, ea / ea, rx" */
NCCclass(Moves, ITYPEother, STMTnone);

/* OP_CAS2: instruction class "cas2.X dc1:dc2,du1:du2,(rn1):(rn2)" */
class Cas2Instr : public _Instr
{
protected:
	Arg *arg4, *arg5, *arg6;
public:
	Cas2Instr(Item op);
	~Cas2Instr() { delete arg4; delete arg5; delete arg6; }
	InstrType IType() const { return ITYPEother; }
	cString toAsmString() const;
};

/* OP_CAS: instruction class "cas dc,du,ea" */
NCCclass(Cas, ITYPEother, STMTnone);

/* OP_BITD: instruction class "bxxx dx, ea" */
NCCclass(Bitd, ITYPEbalu, STMTexpr);

/* OP_MOVEP: instruction class "movep.x, dx,ea/ea,dx" */
NCCclass(Movep, ITYPEother, STMTnone);

/* OP_MVST*: instruction class "move sr,ea / ea,sr" */
NCCclass(MoveSR0, ITYPEother, STMTnone);
NCCclass(MoveSR1, ITYPEother, STMTnone);

/* OP_MVST*: instruction class "move ccr,ea / ea,ccr" */
NCCclass(MoveCCR0, ITYPEother, STMTnone);
NCCclass(MoveCCR1, ITYPEother, STMTnone);

/* OP_SWAP: instruction class "swap dx" */
NCCclass(Swap, ITYPEother, STMTnone);

/* OP_BKPT: instruction class "bkpt #" */
NCCclass(Bkpt, ITYPEother, STMTnone);

/* OP_TRAP: instruction class "trap #" */
NCCclass(Trap, ITYPEother, STMTnone);

/* OP_USP: instruction class "move usp,ax/ax,usp" */
NCCclass(MoveUsp, ITYPEother, STMTnone);

/* OP_MOVEC: instruction class "movec cr,rx, / rx,cr" */
NCCclass(Movec, ITYPEother, STMTnone);

/* OP_CHK: instruction class "chk.x ea,dy" */
NCCclass(Chk, ITYPEother, STMTnone);

/* OP_BCDX: instruction class "op.x -(ax),-(ay) / dx,dy" */
NCCclass(BcdX0, ITYPEother, STMTnone);
NCCclass(BcdX1, ITYPEother, STMTnone);

/* OP_BCD3: instruction class "pack -(ax),-(ay),# / dx,dy,#" */
NCCclass(Bcd3, ITYPEother, STMTnone);

/* OP_CMPM: instruction class "cmpm (ax)+,(ay)+" */
NCCclass(Cmpm, ITYPEother, STMTnone);

/* OP_EXG: instruction class "exg rx,ry" */
NCCclass(Exg, ITYPEother, STMTnone);




/*****************************************************************************
 * The following instrs are generated by CC, and thus need special
 * treatment
 *****************************************************************************/

/* OP_MOVEB/W/L: instruction class "move.x, ea,ea" */
class MoveInstr : public _Instr
{
protected:
	void makeexpr();
public:
	MoveInstr(Item op, OpWidth sz);
	InstrType IType() const { return arg2->IsStkpush() ? ITYPEpush : ITYPEmove; }
	int Value() const { return arg1->Value(); }
	void LinkBB(BB mybb) const { linkbb(mybb, STMTexpr); }
	Expr MakeExpr(BB mybb);
};

/* OP_BITI: instruction class "bxxx #imm, ea" */
class BitiInstr : public _Instr
{
public:
	BitiInstr(Item op);
	InstrType IType() const { return ITYPEbalu; }
	int Value() const { return arg1->Value(); }
	void LinkBB(BB mybb) const { linkbb(mybb, STMTexpr); }
	// null expr
};

/* OP_IMM: instruction class "opi.x #imm, ea" */
class ImmInstr : public _Instr
{
public:
	ImmInstr(Item op, ExprOp exop);
	InstrType IType() const { return ITYPEalu; }
	int Value() const { return arg1->Value(); }
	void LinkBB(BB mybb) const { linkbb(mybb, STMTexpr); }
	Expr MakeExpr(BB mybb);
};

/* OP_EXTX: instruction class "ext.x dx" */
class ExtXInstr : public _Instr
{
public:
	ExtXInstr(Item op, ExprOp exop);
	InstrType IType() const { return ITYPEalu; }
	void LinkBB(BB mybb) const { linkbb(mybb, STMTexpr); }
	Expr MakeExpr(BB mybb);
};

/* OP_LINKX: instruction class "link.x ax, #" */
class LinkXInstr : public _Instr
{
public:
	LinkXInstr(Item op);
	InstrType IType() const { return ITYPEother; }
	Expr GetExpr() const { return 0; /* has no expr */ }
	int Value() const { return arg2->Value(); }
	Expr MakeExpr(BB curbb);
	void LinkBB(BB mybb) const { linkbb(mybb, STMTexpr); }
};

/* OP_MONOP: instruction class "op ea" */
class MonopInstr : public _Instr
{
protected:
	void makeexpr(ExprOp exop);
public:
	MonopInstr(Item op, ExprOp exop);
	InstrType IType() const { return ITYPEalu; }
	void LinkBB(BB mybb) const { linkbb(mybb, STMTexpr); }
	Expr MakeExpr(BB mybb);
};

/* OP_MONOPX: instruction class "op.x ea" */
class MonopXInstr : public MonopInstr
{
public:
	MonopXInstr(Item op, ExprOp exop);
};

/* OP_PEA: instruction class "pea ea" */
class PeaInstr : public _Instr
{
public:
	PeaInstr(Item op);
	InstrType IType() const { return ITYPEpush; }
	void LinkBB(BB mybb) const { linkbb(mybb, STMTexpr); }
	Expr MakeExpr(BB mybb);
};

/* OP_IMPLIED/1: instruction class "op" */
class ImpliedInstr : public _Instr
{
public:
	ImpliedInstr(Item op, ExprOp exop);
	InstrType IType() const { return end ? ITYPEexit : ITYPEother; }
	void LinkBB(BB mybb) const;
	Expr MakeExpr(BB mybb);
};

/* OP_MULDIVL: instruction class "mul/div/u/s dx,ea" */
class MulDivLInstr : public _Instr
{
protected:
	Expr expr2;
public:
	MulDivLInstr(Item op, ExprOp exop);
	InstrType IType() const { return ITYPEalu; }
	void LinkBB(BB mybb) const { linkbb(mybb, STMTexpr); }
	cString toAsmString() const;
	Expr MakeExpr(BB mybb);
	Expr GetExpr2() const { return expr2; }
};

/* OP_UNLK: instruction class "unlk ax" */
class UnlkInstr : public _Instr
{
public:
	UnlkInstr(Item op);
	InstrType IType() const { return ITYPEother; }
	Expr MakeExpr(BB cur) { return 0; }
};

/* OP_IARG: instruction class "rtd #" */
class IArgInstr : public _Instr
{
public:
	IArgInstr(Item op);
	IArgInstr(Item op, int len);
	InstrType IType() const { return ITYPEexit; }
	int Value() const { return arg1->Value(); }
};

/* OP_MOVEM: instruction class "movem #,ea / ea,# */
class MovemInstr : public _Instr
{
private:
	int regs;
public:
	MovemInstr(Item op);
	InstrType IType() const { return ITYPEother; }
	int Value() const { return arg1->Value(); }
	Expr MakeExpr(BB mybb);
	int getRegs() const { return regs; }
	int getVar() const { return -1; }
};

/* OP_LEA: instruction class "lea ea,ay" */
class LeaInstr : public _Instr
{
public:
	LeaInstr(Item op);
	InstrType IType() const { return ITYPEother; }
	void LinkBB(BB mybb) const { linkbb(mybb, STMTexpr); }
	Expr MakeExpr(BB mybb);
};

/* OP_QUICK: instruction class "addq.x #, ea" */
class QuickInstr : public _Instr
{
public:
	QuickInstr(Item op, ExprOp exop);
	InstrType IType() const { return ITYPEalu; }
	int Value() const { return arg1->Value(); }
	void LinkBB(BB mybb) const { linkbb(mybb, STMTexpr); }
	Expr MakeExpr(BB mybb);
};

/* OP_DBCC: instruction class "dbcc dx, ea" */
class DbccInstr : public _Instr
{
public:
	DbccInstr(Item op);
	bool IsCondBranch() const { return true; }
	InstrType IType() const { return ITYPEcond; }
	Symbol Sym() const { return arg2->Sym(); }
	int Target() const { return arg2->Target(); }
	void LinkBB(BB mybb) const;
};

/* OP_SCC: instruction class "scc dx" */
class SccInstr : public _Instr
{
public:
	SccInstr(Item op, ExprOp exop);
	InstrType IType() const { return ITYPEalu; }
	void LinkBB(BB mybb) const { linkbb(mybb, STMTexpr); }
	Expr MakeExpr(BB mybb);
};

/* OP_BRX: instruction class "bra.x/bsr.x/bcc.x ea" */
class BrxXInstr : public _Instr
{
protected:
	InstrType typ;
	ExprOp invop; /* inverse operation of branch, or EXOP_unknown */
	void makeopc(Item op, OpWidth w);
public:
	BrxXInstr(Item op, ExprOp invop);
	BrxXInstr(int target);
	InstrType IType() const { return typ; }
	Symbol Sym() const { return arg1->Sym(); }
	int Target() const { return arg1->Target(); }
	Expr GetExpr() const { return 0; /* has no expr */ }
	void LinkBB(BB mybb) const;
	Expr MakeExpr(BB mybb);
	bool IsBranch() const;
	int FixTarget();
};

/* OP_MOVEQ: instruction class "moveq #, dx" */
class MoveqInstr : public _Instr
{
public:
	MoveqInstr(Item op);
	InstrType IType() const { return ITYPEmove; }
	int Value() const { return arg1->Value(); }
	void LinkBB(BB mybb) const { linkbb(mybb, STMTexpr); }
	Expr MakeExpr(BB mybb);
};

/* OP_MULDIVW: instruction class "mul/div/u/s.w dx,ea" */
class MulDivWInstr : public _Instr
{
public:
	MulDivWInstr(Item op, ExprOp exop);
	InstrType IType() const { return ITYPEalu; }
	void LinkBB(BB mybb) const { linkbb(mybb, STMTexpr); }
	Expr MakeExpr(BB mybb);
};

/* OP_ALU: instruction class "op.x ea,dx / dx,ea" */
class AluInstr : public _Instr
{
public:
	AluInstr(Item op, ExprOp exop);
	InstrType IType() const { return ITYPEalu; }
	int Value() const { return arg1->Value(); }
	void LinkBB(BB mybb) const { linkbb(mybb, STMTexpr); }
	Expr MakeExpr(BB mybb);
};

/* OP_ALUA: instruction class "op.x ea,ax" */
class AluAInstr : public _Instr
{
public:
	AluAInstr(Item op, ExprOp exop);
	InstrType IType() const { return ITYPEalu; }
	int Value() const { return arg1->Value(); }
	void LinkBB(BB mybb) const { linkbb(mybb, STMTexpr); }
	Expr MakeExpr(BB mybb);
};

/* OP_DC: instruction class "invop opcode" */
class DcInstr : public _Instr
{
public:
	DcInstr(Item op);
	InstrType IType() const { return ITYPEother; }
};

/* OP_SHFT: instruction class "shift ea" */
class ShftInstr : public _Instr
{
public:
	ShftInstr(Item op, ExprOp exop);
	InstrType IType() const { return ITYPEalu; }
	void LinkBB(BB mybb) const { linkbb(mybb, STMTexpr); }
	Expr MakeExpr(BB mybb);
};

/* OP_SHFTX: instruction class "shift.x dx,dy" */
class ShftXInstr : public _Instr
{
public:
	ShftXInstr(Item op, ExprOp exop);
	InstrType IType() const { return ITYPEalu; }
	void LinkBB(BB mybb) const { linkbb(mybb, STMTexpr); }
	Expr MakeExpr(BB mybb);
};

/* OP_BF: instruction class "bf* {}(ax)" */
class BfInstr : public _Instr
{
protected:
	int makebits(int sz); // constructs a bitset of 1's of size sz
public:
	BfInstr(Item op, ExprOp exop);
	InstrType IType() const { return ITYPEbalu; }
	void LinkBB(BB mybb) const { linkbb(mybb, STMTexpr); }
	Expr MakeExpr(BB mybb);
};

/* OP_BFD: instruction class "bf* {}(ax),dx" */
class BfdInstr : public _Instr
{
public:
	BfdInstr(Item op);
	InstrType IType() const { return ITYPEbalu; }
	void LinkBB(BB mybb) const { linkbb(mybb, STMTexpr); }
	Expr MakeExpr(BB mybb);
};

/* OP_BFS: instruction class "bf* dx,{}(ax)" */
class BfsInstr : public _Instr
{
public:
	BfsInstr(Item op);
	InstrType IType() const { return ITYPEbalu; }
	void LinkBB(BB mybb) const { linkbb(mybb, STMTexpr); }
	Expr MakeExpr(BB mybb);
};

/* OP_JUMP: instruction class "jmp ea" */
class JumpInstr : public _Instr
{
protected:
	InstrType typ;
//	bool make_lcase1();
	bool make_case1();
	bool make_case2();
	void link_case1(BB mybb) const;
	void link_case2(BB mybb) const;
	int caselow;
	int casecnt;
	Symbol defcasesym;
public:
	JumpInstr(Item op);
	InstrType IType() const { return typ; }
	void FixType() { typ = ITYPEjump; }
	Symbol Sym() const { return arg1->Sym(); }
	Expr GetExpr() const { return 0; /* has no expr */ }
	void LinkBB(BB mybb) const;
	Expr MakeExpr(BB mybb);
};

/****************************************************************************/

class _DataInstr : public _Instr
{
protected:
	InstrType typ;
	bool makereloc(DByte b);
	bool makeascii(DByte b);
	bool makeword();
	bool makelong();

public:
	_DataInstr(int addr);
	_DataInstr(int addr, int sz);
	~_DataInstr() {}

	cString toAsmString() const;
	InstrType IType() const { return typ; }
	Symbol Sym() const { return arg1->Sym(); }
	int Target() const;
	int Value() const;
	void LinkBB(BB mybb) const;
	Expr MakeExpr(BB mybb);
	
	int getWidth() const { return size; }
};

typedef std::shared_ptr<_DataInstr> DataInstr;

/****************************************************************************/

class _BssInstr : public _Instr
{
public:
	_BssInstr(int addr, int sz);
	~_BssInstr() {}

	InstrType IType() const { return ITYPEbss; }
	Expr MakeExpr(BB mybb);
	
	int getWidth() const { return size; }
};

typedef std::shared_ptr<_BssInstr> BssInstr;

/****************************************************************************/

/*forward*/struct MatchTbl; // internal

class InstrFactory
{
private:
	InstrFactory() {}
	const MatchTbl *match(int opcode) const;
public:
	static InstrFactory& Instance() {
		static InstrFactory instance;
		return instance;
	}
	InstrFactory(InstrFactory const&) = delete;
	void operator=(InstrFactory const&) = delete;
	
	
	cString& GetMnemo(Item op) const;
	Instr MakeInst(Segment *seg, int addr);	// instruction
	Instr MakeCaseVal(Segment *seg, int addr, int csz);	// case const
	Instr MakeCaseTgt(Segment *seg, int addr);	// case target
	Instr MakeData(Segment *seg, int addr);	// data item
	Instr MakeBss(Segment *seg, int addr, int bsslen);	// BSS item
};

#include "bb.h"

#endif
