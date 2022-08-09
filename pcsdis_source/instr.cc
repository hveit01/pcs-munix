#include "common.h"
#include "instr.h"
#include "segment.h"
#include "exprfactory.h"

Segment *_Instr::curseg = 0;

_Instr::_Instr(int addr)
	: self(0), _Item(addr), words(), seg(curseg), size(0), end(false), endbb(false),
	  opstr(), opcode(0), arg1(0), arg2(0), arg3(0), expr(0), width(OPWnone),
	  valid(true)
{
}

_Instr::_Instr(Item op)
	: _Item(op->Addr()), words(), seg(curseg), size(2), end(false), endbb(false),
	  opstr(), opcode(op->Value()), arg1(0), arg2(0), arg3(0), expr(0), width(OPWnone),
	  valid(true)
{
	opstr = mnemo(op);
	words.push_back(op);
}

String _Instr::mnemo(Item op) const
{
	return InstrFactory::Instance().GetMnemo(op);
}

/* convert a width to an assembler length field (.b, .w ... */
cString _Instr::LField(OpWidth w)
{
	switch (w) {
	default:
		FATALERROR("Instr::LField: default case w=" + std::to_string(w));
	case OPWshort: return ".s"; // branch only
	case OPWbyte:  return ".b";
	case OPWword:  return ".w";
	case OPWlong:  return ".l";
	case OPWext:   return ".x";
	}
}

/* convert an opcode length field in bit 7-6 into a width */
OpWidth _Instr::width67() const
{
	switch (opcode & 0x00c0) {
	default:
		FATALERROR("Instr::width67: at " + to_hexstring(Addr()) + 
			" default case width=" + to_hexstring(opcode & 0x00c0));
	case 0x0000: return OPWbyte;
	case 0x0040: return OPWword;
	case 0x0080: return OPWlong;
	}
}

/* convert an opcode length field in bit 10-9 into a width */
OpWidth _Instr::width109() const
{
	switch (opcode & 0x0600) {
	default:
		FATALERROR("Instr::width109: at " + to_hexstring(Addr()) + 
			" default case w=" + to_hexstring(opcode & 0x0600));
	case 0x0000: return OPWbyte;
	case 0x0400: return OPWword;
	case 0x0800: return OPWlong;
	}
}

cString _Instr::toExprString() const
{
	Expr e = GetExpr();
	if (!IsValid() || expr->IsNull()) {
		String s("/* ");
		s += toAsmString();
		s += " */";
		return s;
	} else
		return e->toExprString();
}


Expr _Instr::GetExpr() const
{
	if (!expr)  // per default just emit the instruction words */
		const_cast<_Instr*>(this)->expr =
			ExprFactory::Instance().Asm(self, true);

	return expr;
}

/* Catchall for Instrs not yet handled */
Expr _Instr::MakeExpr(BB curbb)
{
	FATALERROR("Instr::MakeExpr: instr=" + toAsmString());
	
	if (!IsValid()) return 0;
	Diag::Trace(DIAGproc, "MakeExpr " + toAsmString());
	return GetExpr();
}

_Instr::~_Instr()
{
	delete arg1;
	delete arg2;
	delete arg3;
}

void _Instr::DumpInstr(std::ostream& os) const
{
	os << toAsmString();
}

bool _Instr::operator==(const Instr& other) const
{
	if (opcode != other->Opcode()) return false;
	return std::equal(words.begin(), words.end(), other->Words().begin());
}	

/* This emits a line of instruction: address, opcode, label, instr */
void _Instr::WriteAsm(std::ostream& os) const
{
	int abs = seg->Rel2Abs(addr);
	Symbol sym = seg->SymbolAt(addr);
	if (sym) {
		cString lbl = sym->Name();
		if (sym->IsGlobal())
			os << std::endl << "\t\t\tglobal\t" << lbl << std::endl;
		else if (sym->IsStatic())
			os << std::endl << "\t\t\tstatic\t" << lbl << std::endl;

		os << "      " << lbl << ":" << std::endl;
	}

	os << "\t\t" << toAsmString();
	os << std::endl;
}

void _Instr::enqueue(int addr, cString& where) const
{
	seg->Enqueue(addr, where);
}

int _Instr::uword()
{
	ItemWord iw = seg->WordAt(addr+size);
	words.push_back(iw);
//	std::cout << "uword("<<std::hex<<(addr+size)<<")="<<w<<std::endl;
	size += 2;
	return iw->Value();
}

int _Instr::ulong()
{
	ItemLong il = seg->LongAt(addr+size);
	words.push_back(il);
//	std::cout << "ulong("<<std::hex<<(addr+size)<<")="<<w<<std::endl;
	size += 4;
	return il->Value();
}

int _Instr::sword()
{
	ItemWord iw = seg->SWordAt(addr+size);
	words.push_back(iw);
//	std::cout << "sword("<<std::hex<<(addr+size)<<")="<<w<<std::endl;
	size +=2;
	return iw->Value();
}

int _Instr::slong()
{
	ItemLong il = seg->SLongAt(addr+size);
	words.push_back(il);
//	std::cout << "slong("<<std::hex<<(addr+size)<<")="<<w<<std::endl;
	size +=4;
	return il->Value();
}

cString _Instr::toAsmString() const
{
	String astr(opstr);
	if (arg1) {
		astr += "\t";
		astr += arg1->toAsmString();
	}
	if (arg2) {
		astr += ", ";
		astr += arg2->toAsmString();
	}
	if (arg3) {
		astr += ", ";
		astr += arg3->toAsmString();
	}
	return astr;
}

Arg *_Instr::imm(OpWidth w)
{
	switch (w) {
	default:
		FATALERROR("Default case w=" + std::to_string((int)w));
	case OPWbyte:
		return new NumArg(low(uword()), "#");
	case OPWword:
		return new NumArg(uword(), "#");
	case OPWlong:
		return new NumArg(ulong(), "#");
	}
}

int _Instr::scale(int extw) const
{
	switch (extw & 0x600) {
	default:
		FATALERROR("Instr::scale: internal error");
	case 0x000: return 1;
	case 0x200: return 2;
	case 0x400: return 4;
	case 0x600: return 8;
	}
}

int _Instr::offarg(int arg)
{
	switch (arg & 03) {
	default:
	case 1:
		return 0;
	case 2:
		return uword();
	case 3:
		return slong();
	}
}

Arg *_Instr::eaext(int ea, uint16_t extw)
{
//	std::cout << "At " << to_hexstring(Addr()) << " EA=" << to_octstring(ea) <<
//		" extw=" << to_hexstring(extw) << std::endl;
	
	Arg *areg, *xreg;
	OpWidth w = width11(extw);

	if (idx68000(extw)) { // classic 68000 index mode
//		std::cout << "Classic 68000 index mode" << std::endl;
		areg = new RegArg(ea, REGptr);
		xreg = new XnArg(r15(extw), w, scale(extw));
		int disp = (int8_t)low(extw);
		if (areg->IsPC()) {
			disp += Addr();
//			enqueue(disp);
		}
		return new IndexArg(disp, areg, xreg);
	}
	
//	std::cout << "New 68020 index mode" << std::endl;
	areg = BS(extw) ? 0 : new RegArg(ea, REGptr);
	xreg = IS(extw) ? 0 : 
		new XnArg(r15(extw), w, scale(extw));

	// new 68020+ indexed ops
	int base = offarg(extw >> 4);
	int outer = offarg(extw);
	
	switch (extw & 0307) {
	case 0000: // !bs,bd*,!is,noind
	case 0100: // !bs,bd*,--,mind*
	case 0200: // --,bd*,!is,noind
	case 0300: // -- bd*,--,mind*
		return new IndexBaseArg(base, areg, xreg);
	default:
		switch (extw & 0304) {
		case 0000: // !bs,bd*,!is,indpre*
		case 0100: // !bs,bd*, --,mind*
		case 0200: //  --,bd*,!is,indpre*
		case 0300: //  --,bd*, --,mind*
			return new IndexPreArg(base, areg, xreg, outer);
		case 0004: // !bs,bd*,!is,indpost*
		case 0204: //  --,bd*,!is,indpost*
			return new IndexPostArg(base, areg, xreg, outer);
		default:
			FATALERROR("Default case extw=" + to_hexstring(extw));
		}
	}
	return 0;
}

Arg *_Instr::ea(uint16_t op, OpWidth isz)
{
//	Diag::Info(DIAGinstr, "EA=" + to_octstring(op));
	
	Reloc r;
	int reg = op & 07;

	switch (op & 0070) {
	case 000:
		return new RegArg(reg, REGdata);
	case 010:
		return new RegArg(reg, REGptr);
	case 020:
		return new AindArg(reg);
	case 030:
		return new ApiArg(reg);
	case 040:
		return new ApdArg(reg);
	case 050:
		return new Aidx2Arg(reg, sword());
	case 060:
		return eaext(op, uword());
	default:
	case 070: 
		switch (op & 07) {
		case 002:
			return local(addr + size + sword());
		case 003:
			return eaext(-1, uword());
		case 000:
			return new NumArg(uword(), "(", ").w");
		case 001:
			/* there may be a named reloc+offset, but maybe it is an absolute value */
			r = seg->RelocByAddr(addr + size);
			return new RelocArg(r, ulong()); /* make arg and consume reloc offset */
		case 004:
			return imm(isz);
		default:
			FATALERROR("Default case ea=" + to_octstring(op & 077));
		}
	}
	return 0;
}

Arg *_Instr::local(int target) const
{
	Symbol sym = seg->MakeLocal(target);
	return new LblArg(sym, target);
}

void _Instr::invalidinstr()
{
	opstr = "db.w";
	arg1 = new NumArg(Opcode());
	SetEnd();
	FATALERROR("Invalid instr= " + to_hexstring(Opcode()));
}

void _Instr::swapargs(bool sw)
{
	if (sw) {
		Arg *x = arg1;
		arg1 = arg2;
		arg2 = x;
	}
}

Arg *_Instr::bfarg(int extw)
{
	Arg *disp = ea(opcode);
	int off = OFFSET(extw);
	int wid = WIDTH(extw);
	Arg *o = DO(extw) ? new RegArg(off, REGdata) : (Arg*)new NumArg(off);
	Arg *w = DW(extw) ? new RegArg(wid, REGdata) : (Arg*)new NumArg(wid);
	return new BfArg(disp, o, w);
}

cString& _Instr::shftop(int n) const
{
	static cString sops[] = {
		"asr", "lsr", "roxr", "ror", "asl", "lsl", "roxl", "rol"
	};
	return sops[n];
}

void _Instr::linkbb(BB mybb, StmtType stype) const
{
	mybb->LinkNext(stype);
}

void _Instr::LinkBB(BB mybb) const
{
	mybb->SetSType(STMTnone);
}

Symbol _Instr::Sym() const
{
	return seg->SymbolAt(Addr());
}

cString _Instr::SymName() const
{
	Symbol sym = Sym();
	return sym ? sym->Name() : "-----";
}

int _Instr::getParam() const
{
	int p = -1;
	if (arg1) p = arg1->Param();
	if (p == -1 && arg2) p = arg2->Param();
	return p;
}

int _Instr::getVar() const
{
	int v = -1;
	if (arg1) v = arg1->Var();
	if (v == -1 && arg2) v = arg2->Var();
	return v;
}
