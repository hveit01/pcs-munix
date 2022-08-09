#ifndef _ARG_H_
#define _ARG_H_

#include "common.h"
#include "dbyte.h"
#include "reloc.h"
#include "symbol.h"
#include "expr.h"

/*forward*/class ExprFactory;

/*abstract*/class Arg
{
protected:
	String astr;
	Expr expr;
	
	String asHex(int x, bool null=false) const;
public:
	Arg();
	virtual ~Arg() { }
	virtual ArgType IsA() const { return ARG_none; }
	virtual cString& toAsmString() { return astr; }
	virtual int Size() const { return astr.length(); }
	virtual bool IsStkpush() const { return false; }
	virtual Symbol Sym() const { return 0; }
	virtual int RegNum() const { return -1; }
	virtual int Areg() const { return -1; }
	virtual int Areg2() const { return -1; }
	virtual int Dreg() const { return -1; }
	virtual int Dreg2() const { return -1; }
	virtual int Value() const { return -1; }
	virtual int Target() const { return -1; }
	virtual bool IsPC() const { return false; }

	virtual int Param() const { return -1; }
	virtual int Var() const { return -1; }
	
	Expr MakeOff(Expr baseex, Expr offex); // build Expr(OFF/PTROFF, arg, expr)
	
	virtual Expr asExpr(ExprOpt opt=EXOPTnormal) { return expr; }
};

class RegArg : public Arg
{
protected:
	int reg1;
	RegType regtype;
	bool ispc;
public:
	RegArg(int reg1, RegType regtyp);
	RegArg(int reg1, const char* pre="", const char *post="");
	int RegNum() const { return reg1 + (int)regtype; } // note regtype is 0 or 8
	int Dreg() const { return reg1; }
	int Areg() const { return reg1+8; }
	bool IsPtr() const { return regtype==REGptr; }
	ArgType IsA(int typ) const { return ARG_RegArg; }
	bool IsPC() const { return ispc; }
};

class NumArg : public Arg
{
protected:
	int ival;
public:
	NumArg(int ival, cString& pre="", cString& post="");
	ArgType IsA() const { return ARG_NumArg; }
	int Value() const { return ival; }
};

class XnArg : public Arg
{
protected:
	int xreg1; // bit3=1 -> a, bit2-0 = reg
	int scale1; /* 1,2,4,8 */
	OpWidth width;
	
public:
	XnArg(int xreg, OpWidth w, int scale);
	int Areg() const { return xreg1 & 0x08 ? (xreg1 & 07) : -1; }
	int Dreg() const { return xreg1 & 0x08 ? -1 : (xreg1 & 07); }
	ArgType IsA() const { return ARG_XnArg; }
};

#define CCR	0xfff
#define SR	0xffe
#define USP 0x800
class SpecArg : public RegArg
{
public:
	SpecArg(int rnum);
};

class LblArg : public Arg
{
private:
	Symbol sym;
	int target;
public:
	LblArg(Symbol sym, int target);
	cString& toAsmString() { return sym->Name(); }
	Symbol Sym() const { return sym; }
	ArgType IsA() const { return ARG_LblArg; }
	int Target() const { return target; }
};

class AindArg : public RegArg
{
public:
	AindArg(int reg);
	ArgType IsA() const { return ARG_AindArg; }
};

// postincrement
class ApiArg : public RegArg
{
public:
	ApiArg(int reg) : RegArg(reg, "(", ")+") {}
	ArgType IsA() const { return ARG_ApiArg; }
};

// predecrement
class ApdArg : public RegArg
{
public:
	ApdArg(int reg);
	bool IsStkpush() const { return reg1==7; }
	ArgType IsA() const { return ARG_ApdArg; }
};

class Aidx2Arg : public RegArg
{
protected:
	int disp, param, var;
public:
	Aidx2Arg(int reg, int disp);
	ArgType IsA() const { return ARG_Aidx2Arg; }
	int Param() const { return param; }
	int Var() const { return var; }
};

class BfArg : public Arg
{
private:
	Arg *disp, *of, *wf;
public:
	BfArg(Arg *disp, Arg *o, Arg *w);
	~BfArg();
	ArgType IsA() const { return ARG_BfArg; }
};

class RelocArg : public Arg
{
private:
	Reloc rel;
	int off;
	const char* relname() const;
	void makeasm();
	void makeexpr(ExprOpt opt);
public:
	RelocArg(Reloc rel, int off);
	ArgType IsA() const { return ARG_RelocArg; }
	cString& toAsmString();
	Symbol Sym() const;
	int Target() const;
	Expr asExpr(ExprOpt opt=EXOPTnormal);
};

class IndexArg : public Arg
{
private:
	int disp, var;
	Arg *areg, *xreg;
public:
	IndexArg(int disp, Arg *areg, Arg *xreg);
	int Areg() const { return areg->Areg(); }
	int Areg2() const { return xreg->Areg2(); }
	int Dreg2() const { return xreg->Dreg2(); }
	ArgType IsA() const { return ARG_IndexArg; }
	int Var() const { return var; }
};

class IndexPreArg : public Arg
{
private:
	int base, outer;
	Arg *areg, *xreg;
public:
	IndexPreArg(int base, Arg *areg, Arg *xreg, int outer);
	int Areg() const { return areg->Areg(); }
	int Areg2() const { return xreg->Areg2(); }
	int Dreg2() const { return xreg->Dreg2(); }
	ArgType IsA() const { return ARG_IndexPreArg; }
};

class IndexPostArg : public Arg
{
private:
	int base, outer;
	Arg *areg, *xreg;
public:
	IndexPostArg(int base, Arg *areg, Arg *xreg, int outer);
	int Areg() const { return areg->Areg(); }
	int Areg2() const { return xreg->Areg2(); }
	int Dreg2() const { return xreg->Dreg2(); }
	ArgType IsA() const { return ARG_IndexPostArg; }
};

class IndexBaseArg : public Arg
{
private:
	int base;
	Arg *areg, *xreg;
public:
	IndexBaseArg(int base, Arg *areg, Arg *xreg);
	int Areg() const { return areg->Areg(); }
	int Areg2() const { return xreg->Areg2(); }
	int Dreg2() const { return xreg->Dreg2(); }
	ArgType IsA() const { return ARG_IndexBaseArg; }
};

class StringArg : public Arg
{
private:
	bool accumulate(char ch, bool inquote);
public:
	StringArg(const char *str);
	ArgType IsA() const { return ARG_StringArg; }
};

#endif
