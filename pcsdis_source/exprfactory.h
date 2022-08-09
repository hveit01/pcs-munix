#ifndef __EXPRFACTORY_H__
#define __EXPRFACTORY_H__

#include "common.h"
#include "expr.h"

class ExprFactory
{
private:
	ExprFactory();
	Expr null;
	Expr pcref;
	Expr a[8];
	Expr d[8];
	std::map<int, Expr> numtbl;
	std::map<String, Expr> strtbl;

public:
	static ExprFactory& Instance() {
		static ExprFactory instance;
		return instance;
	}
	~ExprFactory() {}
	ExprFactory(ExprFactory const&) = delete;
	void operator=(ExprFactory const&) = delete;

	Expr AReg(int n) const;
	Expr DReg(int n) const;
	Expr Number(int n);
	Expr CStr(cString& s);
	Expr Null() const;
	Expr Arg(int num) const;
	Expr Name(cString& name) const;
	Expr Function(Expr ex1) const;
	
	Expr Op(ExprOp op, Expr ex1) const;
	Expr Op(ExprOp op, Expr ex1, Expr ex2) const;
	Expr Op(ExprOp op, Expr ex1, Expr ex2, Expr ex3) const;
	Expr Asm(Instr inst, bool genwords) const;
};

#endif
