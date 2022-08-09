#include "instr.h"
#include "segment.h"
#include "exprfactory.h"

void BrxXInstr::makeopc(Item op, OpWidth w)
{
	width = OPWnone;
	opstr = mnemo(op);
	opstr += LField(w);
}

/* create a synthetic branch */
BrxXInstr::BrxXInstr(int target)
	: _Instr(0x6000), typ(ITYPEjump), invop(EXOP_unknown)
{
	SetEndBB();
	arg1 = local(target);
}

BrxXInstr::BrxXInstr(Item op, ExprOp invop) 
	: _Instr(op), typ(ITYPEunknown), invop(invop)
{
	int target = addr + size;
	int lb = opcode & 0xff;
	if (lb == 0x00) {
		makeopc(op, OPWword);
		target += sword();
	} else if (lb == 0xff) {
		makeopc(op, OPWlong);
		target += slong();
	} else {
		makeopc(op, OPWshort);
		target += (int8_t)low(opcode);
	}

	enqueue(target, "BrxXInstr");

	arg1 = local(target);
	if (opcode < 0x6100)
		typ = ITYPEjump;
	else if (opcode < 0x6200) 	// XXX does this really exist?
		typ = ITYPEcall;		// XXX in compiled code?
	else {
		typ = ITYPEcond;
		SetEndBB();
	}

	expr = 0; // branches have no exprs
	// note: ITYPEcall == bsr lbl_xxx is not generated in C
}

bool BrxXInstr::IsBranch() const
{
	return typ==ITYPEcond;
}

int BrxXInstr::FixTarget()
{
	int oldtarget = arg1->Target();
	arg1 = local(addr+size);
	return oldtarget;
}

void BrxXInstr::LinkBB(BB mybb) const
{
	switch (typ) {
	case ITYPEcond:
		mybb->LinkNext(STMTif); // true path
		mybb->LinkBB(SymName()); // false path
		break;
	case ITYPEjump:
		mybb->LinkBB(SymName(), STMTgoto);
		break;
	case ITYPEcall:
		mybb->LinkNext(STMTexpr);
		break;
	default:
		FATALERROR("Default case type=" + std::to_string((int)typ));
	}
}

Expr BrxXInstr::MakeExpr(BB mybb)
{
	ExprFactory& ef = ExprFactory::Instance();
	if (typ != ITYPEcond)
		return GetExpr();
	if (invop == EXOP_unknown) // should be a standard conditional expr the compute generates
		FATALERROR("Invalid invop=" + std::to_string((int)invop));

	/* get last expr:
	 * if compare, then fix the comparison
	 * otherwise add a test for zero
	 */
	Expr prevex = mybb->LastExpr();
	if (prevex->IsOp(EXOP_cmp)) { // a compare */
		prevex->SetOp(invop);
		Invalidate();
		return 0;
	} else {
		Expr arg1 = prevex->Arg1();
		expr = ef.Op(invop, arg1, ef.Number(0));
		return expr;
	}
}

/*****************************************************************************/

/* recognize and process case type "jmp (a0)":
 Typical sequence:
		sub?.?	#9,d0		; low value if sub exists, otherweise 0
		tst.?	d0			; if no sub, this instr has a label
		blt.?	loc_def		; default case
		cmpi.?	#5,d0		; casecnt value
		bgt.?	loc_def		
		movea.l	off_xxx(pc,d0.w*4),a0 ; to start of case table
		jmp	(a0)
off_xxx:	dc.l loc_AEC	; case table low...high
*/
bool JumpInstr::make_case1()
{
	/* detect the sequence */
	if (opcode != 0x4ed0) return false; // jmp (a0)
	Instr i = seg->InstrAt(Addr()-2); // movea.l off(pc,d0.w*4),a0
	if (i->Opcode() != 0x207b) return false;
	Instr df = seg->PrevInstrAt(i->Addr());  // bgt default
	Instr nc = seg->PrevInstrAt(df->Addr());  // cmpi.l/cmpi.w #$ncases, d0
	if (nc->Opcode() != 0x0c40 && nc->Opcode() != 0x0c80) return false;
	i = seg->PrevInstrAt(nc->Addr());  // blt default
	i = seg->PrevInstrAt(i->Addr());  // tst.l/tst.w default
	if (i->Opcode() != 0x4a40 && i->Opcode() != 0x4a80) return false;
	Instr lc = seg->PrevInstrAt(i->Addr()); // optional sub? #$low, d0

	/* is obviously the pattern for a computed case */
	typ = ITYPEcase1;
	Diag::Info(DIAGcase, "Case1 at " + to_hexstring(addr));

	/* obtain number of cases from instrs */
	casecnt = nc->Value();
	Diag::Info(DIAGcase, "#Cases=" + std::to_string(casecnt));

	/* obtain base offset for case */
	caselow = 0;
	if (seg->SymbolAt(i->Addr()) == 0) { // label here?
		if (!lc)
			FATALERROR("Broken CASE1 at 0x" + to_hexstring(i->Addr()));
		caselow = lc->Value(); // no, low value from subq
	}
	Diag::Info(DIAGcase, "Case low=" + std::to_string(caselow));

	int defaddr = df->Target(); // point to default
	defcasesym = seg->SymbolAt(defaddr);
	Diag::Info(DIAGcase, "Default=" + defcasesym->Name());
	seg->Enqueue(defaddr, "JumpInstr::make_case1(1)");

	int pc = addr + 2; /* skip to table */
	local(pc);
	for (int i=0; i<= casecnt; i++) {
		Instr inst = InstrFactory::Instance().MakeCaseTgt(seg, pc + i*4);
		int tgtaddr = inst->Target();
		seg->Enqueue(tgtaddr, "JumpInstr::make_case1(2)");
	}
	return true;
}




/* recognize and process case type "jmp lbinlswitch" 
 Typical sequence:
loc_B8:
		move.w	#$11,d1 ; number of cases
		lea	off_C6,a0
		jmp	(lbinlswitch).l
off_C6:		dc.l $30 	; first value
		dc.l loc_56		; first target
		dc.l $31
		dc.l loc_56
		dc.l $32
	    ...
		dc.l 'x'		; last case (0x10)
		dc.l loc_86
		dc.l loc_152 	; default case
*/
bool JumpInstr::make_case2()
{
	Instr inst;
	Symbol sym = arg1->Sym();
	if (!sym) return false;
	cString symname = sym->Name();
	int csz;
	if (symname == "lbinlswitch") csz = 4;
	else if (symname == "lbinswitch") csz = 2;
	else return false;
	
	typ = ITYPEcase2;
	Diag::Info(DIAGcase, "Case2 at " + to_hexstring(addr));

	InstrFactory& ifi = InstrFactory::Instance();
	Instr ii = seg->InstrAt(addr-8);
	casecnt = seg->InstrAt(addr-8)->Value(); // point to number of cases
	Diag::Info(DIAGcase, "#Cases=" + std::to_string(casecnt));
	
	int pc = addr + 6;	// point to case table
	local(pc);
	for (int i=0; i < casecnt; i++, pc += (csz+4)) {
		inst = ifi.MakeCaseVal(seg, pc, csz); // these are dc.w or dc.l
		inst = ifi.MakeCaseTgt(seg, pc+csz);  // these are dc.l lbl_xxx
		int tgtaddr = inst->Target();
		seg->Enqueue(tgtaddr, "JumpInstr::make_case2(1)");
	}
	inst = ifi.MakeCaseTgt(seg, pc);	// this is dc.l lbl_xxx
	int defaddr = inst->Target();
	defcasesym = seg->SymbolAt(defaddr);
	Diag::Info(DIAGcase, "Default=" + defcasesym->Name());
	seg->Enqueue(defaddr, "JumpInstr::make_case2(2)");

	return true;
}

void JumpInstr::link_case1(BB switchbb) const
{
	/* case type 1 is a sequence of three BBs reached by a case headerbb:
	 *
	 * headerbb:
	 *   ...
	 *   goto case1a
	 *
	 * case1a:
	 *   d0 = d0 - lowvalue
	 *   if (d0 < 0) goto defcase
	 * case1b:
	 *   if (d0 > highvalue) goto defcase
	 * switchbb:
	 *   jump *table[d0]
	 */

	Diag::Trace(DIAGcase, "Link case1 "+ switchbb->Name());

	BB case1b = switchbb->Pred(0); // point to case1b
	BB defbb = case1b->False(); // point to default case
	defbb->MakeCase(); // mark default case
	switchbb->LinkBB(defbb->Name(), STMT_case1); // set my BB type

	/* successors of switchbb are the case BBs */
	BB tblbb = switchbb->GetNext();
	for (int i=0; i<=casecnt; i++) {
		Instr inst = tblbb->GetInstr(i); // get the dc.l lbl_xxx
		BB tgt = switchbb->BBbyName(inst->SymName());
		switchbb->LinkBB(tgt); // switchbb->Succ(i) = BB(lbl_xxx)
		int caseval = i + caselow;
		tgt->MakeCase(caseval);
	}
}

void JumpInstr::link_case2(BB switchbb) const
{
	Instr inst;
	Diag::Trace(DIAGcase, "Link case2 " + switchbb->Name());

	cString defname = defcasesym->Name();
	BB defbb = switchbb->BBbyName(defname); // get default case from sym
	defbb->MakeCase(); // mark default case
	switchbb->LinkBB(defname, STMT_case2); // set my BB type

	/* collect the case values from the table */
	BB tblbb = switchbb->GetNext();
	for (int i=0; i<casecnt; i++) {
		inst =  tblbb->GetInstr(2*i+1);
		BB tgt = switchbb->BBbyName(inst->SymName());
		switchbb->LinkBB(tgt);
		int caseval = tblbb->GetInstr(2*i)->Value();
		tgt->MakeCase(caseval);
	}
}

void JumpInstr::LinkBB(BB mybb) const
{
	switch(typ) {
	case ITYPEjump:
		mybb->LinkBB(SymName(), STMTgoto);
		break;
	case ITYPEcase1:
		link_case1(mybb);
		break;
	case ITYPEcase2:
		link_case2(mybb);
		break;
	default:
		FATALERROR("Default case type=" + std::to_string((int)typ));
	}
}

JumpInstr::JumpInstr(Item op)
	: _Instr(op), typ(ITYPEjump), casecnt(-1), defcasesym(0)
{
	arg1 = ea(opcode);

	SetEnd();	// ends instruction tracking
	SetEndBB(); // ends a BB

	/* recognize PCS case helper code */
	if (make_case2()) return;
	if (make_case1()) return;
}

Expr JumpInstr::MakeExpr(BB mybb)
{
	return 0;
}
