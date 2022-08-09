#ifndef __REGSET_H__
#define __REGSET_H__

#include "expr.h"

class _regs {
protected:
	Expr r1[8], r2[8];
	bool isreg[8];
	bool live[8];
	bool set1(int n, Expr expr);
	bool set2(int n, Expr expr);
	
public:
	_regs();
	_regs(const _regs& other);
	
	Expr Get(int n, int second) {
		live[n] = true;
		return second ? r2[n] : r1[n]; }
	bool Set(int n, Expr expr); // true=ok, false = invalidated
	void MakeReg(int n) { isreg[n] = true; }
	bool IsReg(int n) const { return isreg[n]; }
	void Live(int n) { live[n] = true; }
	void Kill(int n) { live[n] = false; }
	bool IsLive(int n) const { return live[n]; }
	void Dump(std::ostream& os) const;
};

class RegSet
{
protected:
	_regs a;
	_regs d;

	std::deque<Expr> exprstack;

public:
	RegSet() {}
	~RegSet() {}
	
	Expr GetA(int n, bool second=false) { return a.Get(n, second); }
	bool SetA(int n, Expr expr) { return a.Set(n, expr); }
	bool IsAReg(int n) { return a.IsReg(n); }
	void MakeAReg(int n) { a.MakeReg(n); }
	void KillA(int n)  { a.Kill(n); }
	bool IsALive(int n) const  { return a.IsLive(n); }

	Expr GetD(int n, bool second=false) { return d.Get(n, second); }
	bool SetD(int n,Expr expr) { return d.Set(n, expr); }
	bool IsDReg(int n) { return d.IsReg(n); }
	void MakeDReg(int n) { d.MakeReg(n); }
	void KillD(int n) { d.Kill(n);}
	bool IsDLive(int n) const { return d.IsLive(n); }

	void Push(Expr expr);
	Expr Pop();
	
	void Debug(std::ostream& os) const;
};

#endif
