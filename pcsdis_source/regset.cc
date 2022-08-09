#include "regset.h"

/* keeps score and handles lifeness of registers:
 * 1. a register becomes alive if it is set (SetAReg/SetDReg)
 * 2. a register stays alive if it is referenced
 * 3. non-alive registers are reported when referenced
 * 4. at start of BB the liveness is copied from predecessor BBs (or clear at beginning)
 * 5. if a register is first referenced and was alive, it is flagged in previous BBs
 * 6. if a register is set and was not flagged, it is killed (marked nonalive) in prev BBs
 *
 * BBs are topologically sorted before handling register liveness (DFS number)
 * CProc::DoBB goes through all BBs in it and processes RegSet
 * A BB has an inset and outset that contains the active register contents.
 * At beginning the inset is copied over from the prev BBs - there a two available reg
 * slots to handle the ?: expression whioch cannot be correctly recognized
 * In the case that both slots are filled and a third value is to be stored, the register
 * is considered nonalive because it cannot be a valid ?: expression; it is just a remainder
 * of being set in previous BBs.
 * Registers from BBs with higher DFS number are ignored; they are part of a loop or reached
 * through GOTO - no way to transport liveness, except for register variables.
 * DoBB keeps an additional flag set whether a register has referenced (stays alive) before
 * or not (marking dead in previous BBs)
 */

_regs::_regs()
{
	for (int i=0; i<8; i++) {
		r1[i] = r2[i] = 0;
		isreg[i] = live[i] = false;
	}
}

_regs::_regs(const _regs& other)
{
	for (int i=0; i<8; i++) {
		set1(i, other.r1[i]);
		set2(i, other.r2[i]);
		isreg[i] = other.isreg[i];
	}
}

bool _regs::set1(int n, Expr expr)
{
	r1[n] = expr;
	r2[n] = 0;
	if (expr) live[n] = true;
	return true;
}

bool _regs::set2(int n, Expr expr)
{
	r2[n] = expr;
	if (expr) live[n] = true;
	return true;
}

bool _regs::Set(int n, Expr expr)
{
	if (r1[n] == 0) // slot 1 empty?
		return set1(n, expr);
	if (r2[n] == 0) // slot 2 empty?
		return set2(n, expr);
	
	/* both slots in use, parent must invalidate previous outset */
	set1(n, expr); // clears r2
	return false;
}

void _regs::Dump(std::ostream& os) const
{
	for (int i = 0; i < 8; i++) {
		os << (isreg[i] ? "reg" : "");
		os << i << "=";
		if (r1[i]) os << r1[i]->toExprString();
		else os << "null";
		if (r2[i]) os << " 2nd= " << r2[i]->toExprString();
		os << std::endl;
	}
}

/*****************************************************************************/

void RegSet::Push(Expr expr)
{
	exprstack.push_front(expr);
}

Expr RegSet::Pop()
{
	if (exprstack.empty()) {
		Diag::Error("Exprstack empty");
		return 0;
	}
	Expr e = exprstack.front();
	exprstack.pop_front();
	return e;
}

void RegSet::Debug(std::ostream& os) const
{
	os << "A Registers:" << std::endl;
	a.Dump(os);
	os << "D Registers:" << std::endl;
	d.Dump(os);
	
	os << "Stack " << exprstack.size() << std::endl;
}
