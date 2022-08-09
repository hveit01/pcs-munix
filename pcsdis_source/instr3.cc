#include "instr.h"
#include "segment.h"
#include "exprfactory.h"

ImmInstr::ImmInstr(Item op, ExprOp exop)
	: _Instr(op)
{
	ExprFactory& ef = ExprFactory::Instance();
	width = width67();
	opstr += LField(width);

	arg1 = imm(width);
	arg2 = ea(opcode);

	expr = ef.Op(exop, arg2->asExpr(), arg1->asExpr());
}

Expr ImmInstr::MakeExpr(BB mybb)
{
	return expr;
}

BitiInstr::BitiInstr(Item op)
	: _Instr(op)
{
	width = width67();

	arg1 = imm(width);
	arg2 = ea(opcode);
}

MoveInstr::MoveInstr(Item op, OpWidth sz)
	: _Instr(op)
{
	width = sz;
	opstr += LField(sz);
	
	arg1 = ea(opcode & 0x3f, sz);
	arg2 = ea((ry() | (r8(opcode)<<3)) & 0x3f, OPWnone);
	
	ExprFactory& ef = ExprFactory::Instance();

	//move eas, ead
	if (arg2->IsStkpush())
		expr = ef.Op(EXOP_push, arg1->asExpr());
	else
		expr = ef.Op(EXOP_ASN, arg2->asExpr(), arg1->asExpr());
}

Expr MoveInstr::MakeExpr(BB mybb)
{
	return expr;
}

ExtXInstr::ExtXInstr(Item op, ExprOp exop)
	: _Instr(op)
{
	ExprFactory& ef = ExprFactory::Instance();
	width = width6();
	opstr += LField(width);
	arg1 = new RegArg(rx(), REGdata);
	if (width == 4) exop = EXOP_CAST4;
	expr = ef.Op(exop, arg1->asExpr());
}

Expr ExtXInstr::MakeExpr(BB mybb)
{
	return expr;
}

LinkXInstr::LinkXInstr(Item op)
	: _Instr(op)
{
	bool szl = opcode < 0x4e00;
	opstr = "link" + LField(szl ? OPWlong : OPWword);

	arg1 = new RegArg(rx(), REGptr);
	arg2 = new NumArg(szl ? slong() : sword(), "#");
	
	expr = 0; // has no expr
}

Expr LinkXInstr::MakeExpr(BB curbb)
{
	// TODO process variables
	Diag::Trace(DIAGexpr, "MakeExpr: link - process variables");

	if (!IsValid()) return 0;
	Diag::Trace(DIAGproc, "MakeExpr " + toAsmString());
	return 0;
}

MonopInstr::MonopInstr(Item op, ExprOp exop)
	: _Instr(op)
{
	arg1 = ea(opcode);
	makeexpr(exop);
}

void MonopInstr::makeexpr(ExprOp exop)
{
	ExprFactory& ef = ExprFactory::Instance();
	Expr e0;
	expr = arg1->asExpr();
	switch (exop) {
	case EXOP_clr:
		e0 = ef.Number(0);
		expr = arg1->IsStkpush() ?
			ef.Op(EXOP_push, e0) :
			ef.Op(EXOP_ASN, expr, e0);
		break;
	case EXOP_tst:
		e0 = ef.Number(0);
		expr = ef.Op(EXOP_cmp, expr, e0);
		break;
	case EXOP_func:
		expr = ef.Function(expr);
		break;
	default:
		if (arg1->IsStkpush())
			expr = ef.Op(EXOP_push, expr);
		else
			expr = ef.Op(EXOP_ASN, expr, ef.Op(exop, expr));
	}
}

Expr MonopInstr::MakeExpr(BB mybb)
{
/* function arg processing will be handled in Add op routine */
#if 0
	/* for calls get number of arguments from following stack adjusting add */
	if (expr->IsOp(EXOP_func)) {
		Instr next = mybb->GetInstr(myidx+1);
		if (next) {
//			Diag::Info(DIAGinstr, "Function argcnt: next=" + next->toAsmString());
			Expr nex = next->GetExpr();
			Arg *a1 = next->Arg1();
			Arg *a2 = next->Arg2();
			int stkinc;
			if (nex->IsOp(EXOP_ADDASN) && 
			    a2 && a2->RegNum()==15 && a1 && // 15 is A7 
				(stkinc=a1->Value()) > 0) {
					expr->SetArgcnt(stkinc / 4);
					Diag::Info(DIAGinstr, "Function at " + to_hexstring(Addr()) +
						" argcnt=" + std::to_string(stkinc/4));
					/* TODO consume from BB stack */
					next->Invalidate();
			}
		}
	}
#endif
	return expr;
}

MonopXInstr::MonopXInstr(Item op, ExprOp exop)
	: MonopInstr(op, exop)
{
	width = width67();
	opstr += LField(width);
}

PeaInstr::PeaInstr(Item op)
	: _Instr(op)
{
	ExprFactory& ef = ExprFactory::Instance();
	arg1 = ea(opcode, OPWlong);
	switch (arg1->IsA()) {
	case ARG_NumArg:
		expr = arg1->asExpr();
		break;
	default:
	case ARG_RelocArg: // an internal or external reference
		expr = ef.Op(EXOP_ADDR, arg1->asExpr());
		break;
	}
	expr = ef.Op(EXOP_push, expr);
}

Expr PeaInstr::MakeExpr(BB mybb)
{
	return expr;
}

ImpliedInstr::ImpliedInstr(Item op, ExprOp exop)
	:_Instr(op)
{
	if (exop==EXOP_null) Invalidate();
}

void ImpliedInstr::LinkBB(BB mybb) const
{
	mybb->SetSType(IType()==ITYPEexit ? STMTreturn : STMTnone);
}

Expr ImpliedInstr::MakeExpr(BB mybb)
{
	return expr;
}

MulDivLInstr::MulDivLInstr(Item op, ExprOp exop)
	: _Instr(op), expr2(0)
{
	/* note: exop is EXOP__MUL or EXOP__DIV
	 * needs to adjusted to become unsigned or signed or ASN
	 */
	ExprOp eop;
	ExprFactory& ef = ExprFactory::Instance();

	int extw = uword();
	opstr = mnemo(op);
	if (op11(extw)) {
		eop = toSAOp(exop); // make it a signed op
		opstr += "s";
	} else {
		eop = toUAOp(exop); // make it an unsigned op
		opstr += "u";
	}
	opstr += LField(OPWlong);

	int q = r15(extw);
	int r = rx(extw);

	arg1 = ea(opcode, OPWlong); // source
	if (pair10(extw)) { // 64bit in dr:dq
		arg2 = new RegArg(r, REGdata);	// arg2 stores remainder or high product
		arg3 = new RegArg(q, REGdata);	// arg3 stores quotient or low product
		if (exop==EXOP__MUL) FATALERROR("64 bit multiplication!");
	} else { // 32 bit op
		arg3 = new RegArg(q, REGdata);
		if (r != q)
			arg2 = new RegArg(r, REGdata);
	}
	
	expr = ef.Op(eop, arg3->asExpr(), arg1->asExpr());
	if (arg2 && exop==EXOP__DIV) {
		expr2 = ef.Op(divToMod(eop), arg2->asExpr(), arg1->asExpr());
	}

//	std::cout << "exop=" << exop << " eop=" << eop << " asm=" << toAsmString() << " expr=" << expr->toExprString() << std::endl;

}

cString MulDivLInstr::toAsmString() const
{
	String astr(opstr);
	astr += "\t";
	astr += arg1->toAsmString();
	astr += ", ";
	if (arg2) {
		astr += arg2->toAsmString();
		astr += ":";
	}
	astr += arg3->toAsmString();
	return astr;
}

Expr MulDivLInstr::MakeExpr(BB mybb)
{
	/* return the regular result */
	return expr;
}

UnlkInstr::UnlkInstr(Item op)
	: _Instr(op)
{
	arg1 = new RegArg(rx(), REGptr);
	Invalidate();
}

IArgInstr::IArgInstr(Item op)
	: _Instr(op)
{
	arg1 = new NumArg(uword(), "#");
}

IArgInstr::IArgInstr(Item op, int len)
	: _Instr(op)
{
	bool szl = len==2;
//	opstr = mnemo(op);
	opstr += LField(szl ? OPWlong : OPWword);
	arg1 = new NumArg(szl ? ulong() : uword(), "#");
}

MovemInstr::MovemInstr(Item op)
	: _Instr(op)
{
	width = width6();
	opstr += LField(width);
	int extw = uword();
	arg1 = new NumArg(extw, "#");
	arg2 = ea(opcode);
	int d = dir10();
	if (d==0) regs = arg1->Value();
	swapargs(d);
	Invalidate();
}
Expr MovemInstr::MakeExpr(BB mybb)
{
	return 0;
}

LeaInstr::LeaInstr(Item op)
	: _Instr(op)
{
	ExprFactory& ef = ExprFactory::Instance();
	arg1 = ea(opcode, OPWlong);
	arg2 = new RegArg(ry(), REGptr);

	switch (arg1->IsA()) {
	case ARG_NumArg:
		expr = arg1->asExpr();
		break;
	default:
	case ARG_RelocArg: // an internal or external reference
		expr = ef.Op(EXOP_ADDR, arg1->asExpr());
		break;
	}
	expr = ef.Op(EXOP_ASN, arg2->asExpr(), expr);
}

Expr LeaInstr::MakeExpr(BB mybb)
{
	return expr;
}

QuickInstr::QuickInstr(Item op, ExprOp exop)
	: _Instr(op)
{
	ExprFactory& ef = ExprFactory::Instance();
	width = width67();
	opstr += LField(width);
	int n = ry(); 
	arg1 = new NumArg( n ? n : 8, "#");
	arg2 = ea(opcode);
	
	expr = ef.Op(exop, arg2->asExpr(), ef.Number(arg1->Value()));
}

Expr QuickInstr::MakeExpr(BB mybb)
{
	return expr;
}


DbccInstr::DbccInstr(Item op)
	: _Instr(op)
{
	int target = addr + size;
	enqueue(target += sword() /*, "DbccInstr"*/);
	arg1 = new RegArg(rx(), REGdata);
	arg2 = local(target);
//	SetEnd();
//	SetEndBB();
}

void DbccInstr::LinkBB(BB mybb) const
{
	mybb->LinkNext(STMTif); // true path
	mybb->LinkBB(Sym()->Name()); // false path
}

SccInstr::SccInstr(Item op, ExprOp exop)
	: _Instr(op)
{
	arg1 = ea(opcode);
	
	ExprFactory& ef = ExprFactory::Instance();
	expr = ef.Op(exop, arg1->asExpr(), ef.Number(0));
}

Expr SccInstr::MakeExpr(BB mybb)
{
	return expr;
}

MoveqInstr::MoveqInstr(Item op)
	: _Instr(op)
{
	arg1 = new NumArg(low(opcode), "#");
	arg2 = new RegArg(ry(), REGdata);
	
	ExprFactory& ef = ExprFactory::Instance();
	expr = ef.Op(EXOP_ASN, arg2->asExpr(), arg1->asExpr());
}

Expr MoveqInstr::MakeExpr(BB mybb)
{
	return expr;
}

MulDivWInstr::MulDivWInstr(Item op, ExprOp exop)
	: _Instr(op)
{
	opstr += LField(OPWword);

	arg1 = ea(opcode);
	arg2 = new RegArg(ry(), REGdata);
	
	ExprFactory& ef = ExprFactory::Instance();

	expr = ef.Op(exop, arg2->asExpr(), arg1->asExpr());
}

Expr MulDivWInstr::MakeExpr(BB mybb)
{
	return expr;
}

AluInstr::AluInstr(Item op, ExprOp exop)
	: _Instr(op)
{
	ExprFactory& ef = ExprFactory::Instance();
	width = width67();
	opstr += LField(width);

	arg1 = ea(opcode);
	arg2 = new RegArg(ry(), REGdata);
	swapargs(dir8());
	
	expr = ef.Op(exop, arg2->asExpr(), arg1->asExpr());
}

Expr AluInstr::MakeExpr(BB mybb)
{
	return expr;
}

AluAInstr::AluAInstr(Item op, ExprOp exop)
	: _Instr(op)
{
	ExprFactory& ef = ExprFactory::Instance();
	width = width8();
	opstr += LField(width);
	
	arg1 = ea(opcode, width);
	arg2 = new RegArg(ry(), REGptr);
	
	expr = ef.Op(exop, arg2->asExpr(), arg1->asExpr());
}

Expr AluAInstr::MakeExpr(BB mybb)
{
	return expr;
}

DcInstr::DcInstr(Item op)
	: _Instr(op)
{
	arg1 = new NumArg(opcode, "#");
}

ShftInstr::ShftInstr(Item op, ExprOp exop)
	: _Instr(op)
{
	ExprFactory& ef = ExprFactory::Instance();
	opstr = mnemo(op);
	arg1 = ea(opcode);
	
	expr = ef.Op(exop, arg1->asExpr(), ef.Number(1));
}

Expr ShftInstr::MakeExpr(BB mybb)
{
	return expr;
}

ShftXInstr::ShftXInstr(Item op, ExprOp exop)
	: _Instr(op)
{
	ExprFactory& ef = ExprFactory::Instance();
	int sh = ((opcode & 0x100)>>6) | ((opcode & 030)>>3);
	width = width67();
	opstr = mnemo(op) + LField(width);
	int n = ry();
	arg1 = IR() ? new RegArg(ry(), REGdata) : (Arg*)new NumArg( n ? n : 8, "#");
	arg2 = new RegArg(rx(), REGdata);

	expr = ef.Op(exop, arg2->asExpr(), arg1->asExpr());
}

Expr ShftXInstr::MakeExpr(BB mybb)
{
	return expr;
}




/* bfset, bfclr , set/clr bitfield */
BfInstr::BfInstr(Item op, ExprOp exop)
	: _Instr(op)
{
	ExprFactory& ef = ExprFactory::Instance();
	
	arg1 = bfarg(uword());
	expr = arg1->asExpr(); // is PTR(ea, BIT(off, size)
	Expr e2 = expr->Arg2(); // is BIT(off,size)
//	int off = e2->Arg1()->asNumber(); 
	int siz = e2->Arg2()->asNumber();
	int bits = exop==EXOP_ones ? makebits(siz) : 0;
	expr = ef.Op(EXOP_ASN, expr, ef.Number(bits));
}

int BfInstr::makebits(int sz)
{
	return (1 << sz) - 1;
}

Expr BfInstr::MakeExpr(BB mybb)
{
	return expr;
}

// bfextu, extract bit field
BfdInstr::BfdInstr(Item op)
	: _Instr(op)
{
	ExprFactory& ef = ExprFactory::Instance();

	int extw = uword();
	arg1 = bfarg(extw);
	arg2 = new RegArg(r15(extw), REGdata);
	
	expr = ef.Op(EXOP_ASN, arg2->asExpr(), arg1->asExpr());
}

Expr BfdInstr::MakeExpr(BB mybb)
{
	return expr;
}

// bfins, inject into bit field
BfsInstr::BfsInstr(Item op)
	: _Instr(op)
{
	ExprFactory& ef = ExprFactory::Instance();

	int extw = uword();
	arg2 = bfarg(extw);
	arg1 = new RegArg(r15(extw), REGdata);

	expr = ef.Op(EXOP_ASN, arg2->asExpr(), arg1->asExpr());
}

Expr BfsInstr::MakeExpr(BB mybb)
{
	return expr;
}
