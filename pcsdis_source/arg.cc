#include "arg.h"
#include "instr.h"
#include "exprfactory.h"

Arg::Arg()
	: astr()
{
}		

String Arg::asHex(int val, bool null) const
{
	String s("");
	if (val<0)
		s = "-$" + to_hexstring(-val); 
	else if (val > 0 && val < 10)
		s = std::to_string(val);
	else if (val >= 10)
		s = "$" + to_hexstring(val);
	else if (null)
		s = "0";
	return s;
}

Expr Arg::MakeOff(Expr baseex, Expr offex)
{
	ExprFactory& ef = ExprFactory::Instance();

	/* convert (*ax).off to ax->off */
	ExprOp exop = EXOP_OFF;

	if (baseex->IsOp(EXOP_DEREF)) {
		Expr e1 = baseex->Arg1();
		if (e1->IsA()==EXPRareg) {
			baseex = e1;
			exop = EXOP_PTROFF;
		}
	}
	return ef.Op(exop, baseex, offex);
}

RegArg::RegArg(int reg, RegType regtyp)
	: Arg(), reg1(reg & 07), regtype(regtyp), ispc(false)
{
	ExprFactory& ef = ExprFactory::Instance();
	if (regtype == REGdata) {
		astr = "d" + std::to_string(reg1);
		expr = ef.DReg(reg1);
	} else {
		if (reg < 0) {
			astr = "pc";
			expr = ef.AReg(reg);
			ispc = true;
		} else {
			astr = "a" + std::to_string(reg1);
			expr = ef.AReg(reg1);
		}
	}
}

RegArg::RegArg(int reg, const char* pre, const char* post)
	: Arg(), reg1(reg & 07), regtype(REGptr), ispc(false)
{
	ExprFactory& ef = ExprFactory::Instance();
	astr = pre;
	
	if (reg < 0) {
		astr += "pc";
		expr = ef.AReg(reg);
		ispc = true;
	} else {
		if (reg1==7)
			astr += "sp";
		else
			astr += "a" + std::to_string(reg1);
		expr = ef.AReg(reg1);
	}
	astr += post;
}

NumArg::NumArg(int ival, cString& pre, cString& post)
	: Arg(), ival(ival)
{
	astr = pre + asHex(ival, true) + post;
	expr = ExprFactory::Instance().Number(ival);
}

XnArg::XnArg(int xreg, OpWidth w, int scale1)
	: Arg(), xreg1(xreg), width(w), scale1(scale1)
{
	ExprFactory& ef = ExprFactory::Instance();
	bool isareg = xreg1 & 010; // A register 
	int r = xreg1 & 07;
	if (isareg) {
		astr = "a" + std::to_string(xreg1 & 07);
		expr = ef.AReg(r);
	} else {
		astr = "d" + std::to_string(xreg1 & 07);
		expr = ef.DReg(r);
	}
	if (w != OPWnone) astr += _Instr::LField(width);

	if (scale1 != 1) {
		astr += "*" + std::to_string(scale1);
		expr = ef.Op(EXOP_UMUL, expr, ef.Number(scale1));
	}
}

LblArg::LblArg(Symbol sym, int target)
	: Arg(), sym(sym), target(target)
{
	expr = ExprFactory::Instance().Name(sym->Name());
}

SpecArg::SpecArg(int reg1)
	: RegArg(reg1, REGdata)
{
	const char *name;
	switch(reg1 & 0xfff) {
	case CCR:	name = "ccr"; break;
	case SR:	name = "sr"; break;
	case 0:		name = "sfc"; break;
	case 1:		name = "dfc"; break;
	case 2:		name = "cacr"; break;
	case 3:		name = "tc"; break;
	case 4:		name = "itt0"; break;
	case 5:		name = "itt1"; break;
	case 6:		name = "dtt0"; break;
	case 7:		name = "dtt1"; break;
	
	case USP:	name = "usp"; regtype = REGptr; break;
	case 0x801: name = "vbr"; regtype = REGptr; break;
	case 0x802:	name = "caar"; break;
	case 0x803: name = "msp"; regtype = REGptr; break;
	case 0x804:	name = "isp"; regtype = REGptr; break;
	case 0x805:	name = "mmusr"; break;
	case 0x806:	name = "urp"; regtype = REGptr; break;
	case 0x807: name = "srp"; regtype = REGptr; break;
	default:
		astr = "cr" + to_hexstring(reg1);
		return;
	}
	astr = name;
	expr = ExprFactory::Instance().CStr("<SpecArg>");
}

AindArg::AindArg(int reg)
	 : RegArg(reg, "(", ")")
{
	expr = ExprFactory::Instance().Op(EXOP_DEREF, expr); // expr set in parent ctor
}

ApdArg::ApdArg(int reg) : RegArg(reg, "-(", ")")
{
	if (IsStkpush())
		expr = 0;	// needs to be checked by MoveInstr and others
	else
		expr = ExprFactory::Instance().Op(EXOP_LDEC, expr); // expr set in parent ctor
}

Aidx2Arg::Aidx2Arg(int reg, int disp)
	: RegArg(reg), disp(disp), param(-1), var(-1)
{
	ExprFactory& ef = ExprFactory::Instance();
	astr = asHex(disp);
	switch (reg) {
	case 7:
		astr += "(sp)";
		expr = ef.CStr("<SP>");
		break;
	case 6:
		if (disp >= 0) {
			param = disp-8;
			astr = "arg" + std::to_string(param) + "(a6)";
			expr = ef.Arg(param);
		} else {
			var = -disp;
			astr = "var" + std::to_string(var);
			expr = ef.Name(astr);
			astr += "(a6)";
		}
		break;
	default:
		astr += "(a" + std::to_string(reg) + ")";
		if (disp == 0)
			expr = ef.Op(EXOP_DEREF, ef.AReg(reg));
		else
			expr = ef.Op(EXOP_PTROFF, ef.AReg(reg), ef.Number(disp));
	}
}

BfArg::BfArg(Arg *d, Arg *o, Arg *w)
	: Arg(), disp(d), of(o), wf(w)
{
	// ea{off:w}
	astr = disp->toAsmString();
	astr += "{";
	astr += of->toAsmString();
	astr += ":";
	astr += wf->toAsmString();
	astr += "}";

	ExprFactory& ef = ExprFactory::Instance();
	expr = MakeOff(disp->asExpr(), 
			ef.Op(EXOP_bit, of->asExpr(), wf->asExpr()));
}

BfArg::~BfArg()
{
	delete disp;
	delete of;
	delete wf;
}

RelocArg::RelocArg(Reloc rel, int off)
	: Arg(), rel(rel), off(off)
{}

void RelocArg::makeasm()
{
	astr = "(";
	const char *rname = relname();
	if (rname) {
		astr += rname;
		if (off != 0) {
			astr += "+" + std::to_string(off);
		}
	} else {
		astr += asHex(off, true);
	}
	astr += ")" + _Instr::LField(OPWlong);
}

void RelocArg::makeexpr(ExprOpt opt)
{
	ExprFactory& ef = ExprFactory::Instance();
	const char *rname = relname();
	if (rname) {
		expr = ef.Name(rname);
		if (off != 0)
			expr = MakeOff(expr, ef.Number(off));
	} else {
		expr = ef.Number(off);
	}
}

const char *RelocArg::relname() const
{
	return rel ? rel->Name().c_str() : 0;
}

cString& RelocArg::toAsmString()
{
	if (astr.length() == 0) makeasm();
	return astr;
}

Expr RelocArg::asExpr(ExprOpt opt)
{
	if (expr == 0) makeexpr(opt);
	return expr;
}

Symbol RelocArg::Sym() const 
{
	return rel->Sym();
}

int RelocArg::Target() const
{
	int tgt = rel ? rel->Target() : 0;
	return tgt + off;
}

IndexArg::IndexArg(int disp, Arg* areg, Arg *xreg)
	: Arg(), disp(disp), var(-1),
	  areg(areg), xreg(xreg)
{
	ExprFactory& ef = ExprFactory::Instance();
	
	astr = asHex(disp) + "(" + areg->toAsmString() + "," + xreg->toAsmString() + ")";
	
	// an[xn].off
	if (areg->Areg()==14) { /* SP */
		var = -disp;
		cString s("var" + to_hexstring(var));
		expr = ef.Op(EXOP_BRACKET, ef.Name(s), xreg->asExpr()); // var_nn[xn]
	} else {
		expr = ef.Op(EXOP_BRACKET, areg->asExpr(), xreg->asExpr()); // [xn]
		expr = MakeOff(expr, ef.Number(disp));
	}
}
				
IndexPreArg::IndexPreArg(int base, Arg *areg, Arg* xreg, int outer)
	: Arg(), base(base), outer(outer), areg(areg), xreg(xreg)
{
	astr = "([" + asHex(base,true);
	if (areg) {
		astr += ",";
		astr += areg->toAsmString();
	}
	if (xreg) {
		astr += ",";
		astr += xreg->toAsmString();
	}
	astr += "]," + asHex(outer, true);
	
	/* expr not yet */
}

IndexPostArg::IndexPostArg(int base, Arg *areg, Arg* xreg, int outer)
	: Arg(), base(base), outer(outer), areg(areg), xreg(xreg)
{
	astr = "([" + asHex(base, true);
	if (areg) {
		astr += ",";
		astr += areg->toAsmString();
	}
	astr += "]";
	if (xreg) {
		astr += ",";
		astr += xreg->toAsmString();
	}
	astr += "," + asHex(outer, true) + ")";

	/* expr not yet */
}

IndexBaseArg::IndexBaseArg(int base, Arg *areg, Arg* xreg)
	: Arg(), base(base), areg(areg), xreg(xreg)
{
	if (areg) {
		astr = asHex(base) + "(" + areg->toAsmString();
		if (xreg) {
			astr += ",";
			astr += xreg->toAsmString();
		}
		astr += ")";
	} else {
		if (xreg)
			astr = "([" + asHex(base, true) + "]," + xreg->toAsmString() + ")";
		else
			astr = "(" + asHex(base, true) + ")";
	}

	/* expr not yet */
}

StringArg::StringArg(const char *s)
{
	astr = "";
	bool inquote = false;
	for (const char* p = s; p[0]; p++) {
		inquote = accumulate(p[0], inquote);
	}
	accumulate(0, inquote);
	
	expr = ExprFactory::Instance().CStr(astr);
}

bool StringArg::accumulate(char ch, bool inquote)
{
	int len = astr.length();
	if (isprint(ch) || ch=='\n' || ch == '\t' || ch == '\r' || ch == 7) {
		if (!inquote) {
			if (len > 0) astr += ",";
			astr += "\'";
		}
		switch (ch) {
		case '\n':
			astr += "\\n"; break;
		case '\r':
			astr += "\\r"; break;
		case '\t':
			astr += "\\t"; break;
		case 7:
			astr += "\\007"; break;
		default:
			astr += ch;
		}
		return true;
	}
	
	/* other not printable */
	if (len > 0) { // not at the beginning
		if (inquote) astr += "\'";
		astr += ",";
	}
	astr += asHex(ch, true);
	return false;
}
