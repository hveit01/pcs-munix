#ifndef __EXPR_H__
#define __EXPR_H__

#include "common.h"
#include "item.h"

struct ExopTbl {
	ExprOp op;
	int prec;
	ExprAssoc assoc;
	const char *lsym, *msym, *rsym;
};

/*abstract*/class _Expr {
protected:
	ExprOp op;
	bool excluded;
	String estr;
	const ExopTbl *findop(ExprOp op) const;
public:
	_Expr(ExprOp op=EXOP_unknown);
	virtual ~_Expr() {}
	virtual ExprType IsA() const { return EXPRunknown; }
	void Exclude() { excluded = true; }
	bool IsExcluded() const { return excluded; }
	ExprOp Op() const { return op; }
	void SetOp(ExprOp newop) { op = newop; }
	bool IsOp(ExprOp which) const { return op == which; }
	bool IsCond() const;
	void InvertCond();
	virtual bool operator==(cExpr& other) const { return false; }
	virtual int Prec(ExprOp op) const;
	int Prec() const { return Prec(op); }
	virtual ExprAssoc Assoc(ExprOp op) const;
	ExprAssoc Assoc() const { return Assoc(op); }
	bool IsNull() const { return op==EXOP_null; }

	virtual void SetArgcnt(int cnt) { /* only ExprFunc */ }
	virtual Expr Arg1() const;
	virtual Expr Arg2() const;
	virtual Expr Arg3() const;
	virtual int asNumber() const { assert(0); return 0; }
	virtual cString& asString() const { assert(0); return estr; }
	virtual cString& toExprString(bool paren=false, ExprModifier omode=EXMODnone) const;
	
	void WriteC(std::ostream& os, int indent) const;
};

class _ExprNumber : public _Expr
{
protected:
	int val;
	void makeexpr(ExprModifier omode);
public:
	_ExprNumber(int val);
	~_ExprNumber() {}
	ExprType IsA() const { return EXPRnumber; }
	bool operator==(cExpr& other) const;
	void SetMode(ExprModifier newmode);
	int Prec(ExprOp op) const { return 999; }
	ExprAssoc Assoc(ExprOp op) const { return EXASSOCrtol; }

	int asNumber() const { return val; }
	cString& toExprString(bool paren=false, ExprModifier omode=EXMODdec) const;
};
typedef std::shared_ptr<_ExprNumber> ExprNumber;


class _ExprString : public _Expr
{
protected:
	void makeexpr(cString& s);
public:
	_ExprString(cString& s);
	~_ExprString() {}
	ExprType IsA() const { return EXPRstring; }
	bool operator==(cExpr& other) const;
	int Prec(ExprOp op) const { return 999; }
	ExprAssoc Assoc(ExprOp op) const { return EXASSOCrtol; }

	cString& asString() const { return estr; }
	cString& toExprString(bool paren=false, ExprModifier omode=EXMODnone) const { return estr; }
};
typedef std::shared_ptr<_ExprString> ExprString;

class _ExprAreg : public _Expr
{
protected:
	int num;
public:
	_ExprAreg(int num);
	~_ExprAreg() {}
	ExprType IsA() const { return EXPRareg; }
	bool operator==(cExpr& other) const;
	int Prec(ExprOp op) const { return 999; }
	ExprAssoc Assoc(ExprOp op) const { return EXASSOCrtol; }

	int asNumber() const { return num; }
	cString& toExprString(bool paren=false,ExprModifier omode=EXMODnone) const { return estr; }
};
typedef std::shared_ptr<_ExprAreg> ExprAreg;

class _ExprDreg : public _Expr
{
protected:
	int num;
public:
	_ExprDreg(int num);
	~_ExprDreg() {}
	ExprType IsA() const { return EXPRdreg; }
	bool operator==(cExpr& other) const;
	int Prec(ExprOp op) const { return 999; }
	ExprAssoc Assoc(ExprOp op) const { return EXASSOCrtol; }

	int asNumber() const { return num; }
	cString& toExprString(bool paren=false, ExprModifier omode=EXMODnone) const { return estr; }
};
typedef std::shared_ptr<_ExprDreg> ExprDreg;


class _ExprName : public _Expr
{
public:
	_ExprName(cString& nam);
	~_ExprName() {}
	ExprType IsA() const { return EXPRvar; }
	bool operator==(cExpr& other) const;
	int Prec(ExprOp op) const { return 999; }
	ExprAssoc Assoc(ExprOp op) const { return EXASSOCrtol; }

	cString& asString() const { return estr; }
	cString& toExprString(bool paren=false, ExprModifier omode=EXMODnone) const { return estr; }
};
typedef std::shared_ptr<_ExprName> ExprName;


class _ExprArg : public _Expr
{
protected:
	int num;
public:
	_ExprArg(int val);
	~_ExprArg() {}
	ExprType IsA() const { return EXPRarg; }
	virtual bool operator==(cExpr& other) const;
	int Prec(ExprOp op) const { return 999; }
	ExprAssoc Assoc(ExprOp op) const { return EXASSOCrtol; }

	int asNumber() const { return num; }
	cString& toExprString(bool paren=false, ExprModifier omode=EXMODnone) const { return estr; }
};
typedef std::shared_ptr<_ExprArg> ExprArg;

class _Expr1 : public _Expr
{
protected:
	Expr expr;
	void makeexpr(bool paren, ExprModifier omode);
public:
	_Expr1(ExprOp op, Expr expr);
	~_Expr1() {}
	ExprType IsA() const { return EXPR1; }
	bool operator==(cExpr& other) const;
	Expr Arg1() const { return expr; }

	cString& toExprString(bool paren=false,ExprModifier omode=EXMODnone) const;
};
typedef std::shared_ptr<_Expr1> Expr1;

class _Expr2 : public _Expr
{
protected:
	Expr left, right;
	void makeexpr(bool paren, ExprModifier omode);
public:
	_Expr2(ExprOp op, Expr left, Expr right);
	~_Expr2() {}
	ExprType IsA() const { return EXPR2; }
	bool operator==(cExpr& other) const;
	Expr Arg1() const { return left; }
	Expr Arg2() const { return right; }
	
	cString& toExprString(bool paren=false, ExprModifier omode=EXMODnone) const;
};
typedef std::shared_ptr<_Expr2> Expr2;

class _Expr3 : public _Expr
{
protected:
	Expr cond, left, right; 
	void makeexpr(bool paren, ExprModifier omode);
public:
	_Expr3(ExprOp op, Expr cond, Expr left, Expr right);
	~_Expr3() {}
	ExprType IsA() const { return EXPR3; }
	bool operator==(cExpr& other) const;
	Expr Arg1() const { return cond; }
	Expr Arg2() const { return left; }
	Expr Arg3() const { return right; }
	
	cString& toExprString(bool paren=false, ExprModifier omode=EXMODnone) const;
};
typedef std::shared_ptr<_Expr3> Expr3;

class _ExprFunc : public _Expr {
protected:
	String name;
	Expr call;
	int argcnt; // # of args required
	std::deque<Expr> args;
	Expr ret;
public:
	_ExprFunc(cString& name);
	_ExprFunc(Expr callexpr);
	~_ExprFunc() {}
	ExprType IsA() const { return EXPRfunc; }
	bool operator==(cExpr& other) const;
	cString& Name() const { return name; }

	void SetRet(Expr newret) { ret = newret; }
	Expr GetRet() const { return ret; }
	
	void SetArgcnt(int cnt) { argcnt = cnt; }
	int Argcnt() const { return argcnt; }
	void AddArg(Expr arg);
	Expr GetArg(int n) const;
	cString& toExprString(bool paren=false, ExprModifier omode=EXMODnone) const;
};
typedef std::shared_ptr<_ExprFunc> ExprFunc;

class _ExprAsm : public _Expr {
protected:
	Instr inst;
public:
	_ExprAsm(Instr inst, bool genwords);
	~_ExprAsm() {}
	ExprType IsA() const { return EXPRasm; }
	bool operator==(cExpr& other) const;
	cString& toExprString(bool paren=false, ExprModifier omode=EXMODhex) const { return estr; }
};
typedef std::shared_ptr<_ExprAsm> ExprAsm;

#endif
