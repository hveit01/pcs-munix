#include "exprfactory.h"

ExprFactory::ExprFactory()
	: numtbl(), strtbl()
{
	null = std::make_shared<_Expr>(EXOP_null);
	pcref = std::make_shared<_ExprAreg>(-1);
	for (int i=0; i<8; i++) {
		a[i] = std::make_shared<_ExprAreg>(i);
		d[i] = std::make_shared<_ExprDreg>(i);
	}
}

Expr ExprFactory::AReg(int n) const
{
	return n == -1 ? pcref : a[n];
}

Expr ExprFactory::DReg(int n) const
{
	return d[n];
}

Expr ExprFactory::Number(int i)
{
	auto ex = numtbl.find(i);
	if (ex == numtbl.end()) {
		Expr e = std::make_shared<_ExprNumber>(i);
		numtbl[i] = e;
		return e;
	} else
		return ex->second;
}

Expr ExprFactory::CStr(cString& s)
{
	auto ex = strtbl.find(s);
	if (ex == strtbl.end()) {
		Expr e = std::make_shared<_ExprString>(s);
		strtbl[s] = e;
		return e;
	} else
		return ex->second;
}

Expr ExprFactory::Null() const
{
	return null;
}

Expr ExprFactory::Op(ExprOp op, Expr ex1) const
{
	return std::make_shared<_Expr1>(op, ex1);
}

Expr ExprFactory::Op(ExprOp op, Expr ex1, Expr ex2) const
{
	return std::make_shared<_Expr2>(op, ex1, ex2);
}

Expr ExprFactory::Op(ExprOp op, Expr ex1, Expr ex2, Expr ex3) const
{
	return std::make_shared<_Expr3>(op, ex1, ex2, ex3);
}

Expr ExprFactory::Arg(int num) const
{
	return std::make_shared<_ExprArg>(num);
}

Expr ExprFactory::Name(cString& name) const
{
	return std::make_shared<_ExprName>(name);
}

Expr ExprFactory::Function(Expr ex1) const
{
	String name;
	if (ex1->IsA() == EXPRvar)
		return std::make_shared<_ExprFunc>(ex1->asString());
	else
		return std::make_shared<_ExprFunc>(ex1);
}

Expr ExprFactory::Asm(Instr inst, bool genwords) const
{
	return std::make_shared<_ExprAsm>(inst, genwords);
}

