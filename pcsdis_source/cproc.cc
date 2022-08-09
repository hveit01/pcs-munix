#include "instr.h"
#include "segment.h"
#include "cloop.h"
#include "cproc.h"

_CProc::_CProc(Symbol sym)
	: self(0), seg(0), name(sym->Name()), bbs(), tsbbs(), /*exprs(),*/ loops(),
	  isstatic(sym->IsStatic()), rootbb(0), curbb(0), regs(0)
{
	seg = SegmentFactory::Instance()->Text();
	Diag::Trace(DIAGproc, "New Proc " + name);
}

CProc _CProc::MakeProc(Symbol s)
{
	CProc p = std::make_shared<_CProc>(s);
	p->self = p;
	return p;
}

BB _CProc::BBbyName(cString name) const
{
	for (auto bb : bbs) {
		if (bb->Name() == name)
			return bb;
	}
	return 0;
}

BB _CProc::BBbySeq(int seq) const
{
	try { return bbs.at(seq); }
	catch (const std::out_of_range& oor) {
		FATALERROR("Invalid seq=" + std::to_string(seq));
	}
	return 0;
}

BB _CProc::BBbyTSNum(int num) const
{
	try { return tsbbs.at(num); }
	catch (const std::out_of_range& oor) {
		FATALERROR("Invalid tsnum=" + std::to_string(num));
	}
	return 0;
}

BB _CProc::addbb(cString name)
{
	BB b = StmtFactory::Instance().Block(Self(), name, bbs.size());
	if (!rootbb) rootbb = b;
	bbs.push_back(b);
	return b;
}

/*
 * Collects instrs into tempinst vector until a new symbol is found or
 * a BB regularly ends
 */
void _CProc::AddInstr(Instr in)
{
	if (in==0) FATALERROR("Instr is 0");
	
	Diag::Info(DIAGproc, 
		"AddInstr at " + to_hexstring(in->Addr()) +
			": " + in->toAsmString() + 
			(in->IsEndBB() ? " ; endbb" : ""));
			
	inst.push_back(in);

	cSymbol sym = seg->SymbolAt(in->Addr());
	if (!curbb || sym)
		curbb = addbb(sym->Name());

	/* put instr into BB */
	curbb->AddInstr(in);
	/* does instr end a BB? */
	if (in->IsEndBB())
		curbb = 0; /* mark current BB as finished, so the next instr
					* will create a new one */
}

void _CProc::debug(std::ostream& os) const
{
#if 0
	bool bflg = Diag::IsDiag(DIAGbb);
	bool lflg = Diag::IsDiag(DIAGloop);
	
	if (bflg) {
		os << std::endl << std::endl << std::endl
		   << "#if 0" << std::endl << "/* DEBUG: " << Name() << " */" << std::endl;
//		for (auto bb : bbs) bb->Debug(os);
	}
	if (lflg)
		for (auto lp : loops) lp->Debug(os);

	if (bflg || lflg)
		os << "#endif" << std::endl << std::endl;
#endif
}

void _CProc::WriteC(std::ostream& os, int indent) const
{
	debug(os);
	
//	os << std::endl << "/* C function */" << std::endl;
	if (isstatic) os << "static ";
	/* XXX dump return type */

	os << name << "(";

	/* dump parameters */
	bool first=true;
	for (auto p : params) {
		if (!first) os << ",";
		os << "arg" << std::to_string(p);
		first=false;
	}	
	os << ")" << std::endl;

	os << "{" << std::endl;
	os << "#if ISNOTC" << std::endl;
	
	/* dump registers and vars */
	int rm = 0x8000;
	for (int i=7; i >= 0; i--, rm >>= 1) {
		if (regs & rm)
			os << "\tregister ? *a" << std::dec << i << ";" << std::endl;
	}	
	for (int i=7; i >= 0; i--, rm >>= 1) {
		if (regs & rm)
			os << "\tregister ? d" << std::dec << i << ";" << std::endl;
	}		
	for (auto v : vars)
		os << "\t? var" << std::to_string(v) << ";" << std::endl;
	os << std::endl << std::endl;

	/* dump BBs */
	for (auto bb : bbs) {
		bb->Debug(os);
	}

#if 0
	BB bb = rootbb;
	while (bb) {
//		os << "/* BB: " << bb->Name() << " */" << std::endl;
		bb->WriteC(os, indent+2);
		bb = GetNextStmt(bb);
	}
#endif	


	os << "#endif /*ISNOTC*/" << std::endl;
	os << "}" << std::endl << std::endl;
}

void _CProc::linkbbs()
{
	for (auto cur : bbs)
		cur->Link();
}

bool cmpIntAsc(const int s1, const int s2) { return s1 < s2; }

void _CProc::collect_p_v_r()
{
	for (auto i : inst) {
		regs |= i->getRegs();
		int p = i->getParam();
		if (p >= 0 && 
			std::find(params.begin(), params.end(), p) == params.end())
			params.push_back(p);
		int v = i->getVar();
		if (v >= 0 && 
			std::find(vars.begin(), vars.end(), v) == vars.end())
			vars.push_back(v);
	}
	std::sort(params.begin(),params.end(),cmpIntAsc);
	std::sort(vars.begin(),vars.end(),cmpIntAsc);
}

/* calculate the topologically sorted order of BBs in procedure */
int _CProc::TSortBBs(BB cur, int tsnum)
{
	int num;
	if (!cur) {
		num = TSortBBs(bbs[0], 0);
		Diag::Info(DIAGproc, "CProc " + name + 
			" bbs=" + std::to_string(bbs.size()) +
			" visited=" + std::to_string(num+1));
	} else {
		cur->SetTSNum(tsnum++);
		tsbbs.push_back(cur);
		num = cur->Visit();
	}
	return num;
}

/* calculate the forward dominators of each BB 
 * classical iterative algorithm of Allen & Cocke 1972
*/
void _CProc::make_pre_dominators()
{
	int n = tsbbs.size();
	if (n == 0) return; /* no BBs, empty procedure */

	for (auto bb : tsbbs) /* initialize all dominators set */
		bb->InitPreDom(n);
	
	BB root = Root();
	root->PreDom()->ClearAll();
	root->PreDom()->Set(root->TSNum());

	bool changed;
	do {
		changed = false;
		for (auto bb : tsbbs) {
			if (!bb->Visited() || bb==root) continue;

			for (auto pred : bb->Preds()) {
				if (!pred->Visited()) {
					std::cerr << "bb="<<bb->Name()<<" pred="<<pred->Name()<< std::endl;
					continue;
				}
				BitVec *d = bb->PreDom();
				BitVec temp(d);
				d->And(pred->PreDom());
				d->Set(bb->TSNum());
				if (temp != d)
					changed = true;
			}
		}
	} while (changed);
}

/* calculate the post dominators of each BB */
void _CProc::make_post_dominators()
{
	/* Effectively, this traverses the CFG in reverse order from end to begin.
	 * But something is different here compared to make_pre_dominators:
	 *
	 * there may be function with an endless loop, such as swtch()
	 * so the exit node is unconnected.
	 * In this case, we can't traverse the tree backwards from the end, but 
	 * use the already found outer loop head as new exit node: this mus have an entry.
     */
	int n = tsbbs.size();
	if (n == 0) return; /* no BBs, empty procedure */

	for (auto bb : tsbbs) /* initialize all dominators set */
		bb->InitPostDom(n);

	BB tail = Exit();
	if (tail->TSNum() == NOTVISITED) 
		FATALERROR("Unreachable Exit");	// for now emergency exit
	tail->PostDom()->ClearAll();
	tail->PostDom()->Set(tail->TSNum());

	bool changed;
	do {
		changed = false;
		for (auto it = tsbbs.rbegin(); it != tsbbs.rend(); it++) {
			BB bb = *it;
			if (!bb->Visited() || bb==tail) continue;
			
			for (auto succ : bb->Succs()) {
				BitVec *d = bb->PostDom();
				BitVec temp(d);
				d->And(succ->PostDom());
				d->Set(bb->TSNum());
				if (temp != d)
					changed = true;
			}
		}
	} while (changed);
}


bool _CProc::Decompile()
{
	linkbbs(); /* build linkage */
	collect_p_v_r(); /* find all params, vars, register vars */
	TSortBBs(); /* topologically sort BBs of proc */
	make_pre_dominators(); /* calculate dominators of nodes */
	find_all_loops(); /* find natural loops */
	make_post_dominators(); /* calculate dominators of nodes */
	find_ipdoms(); /* find immediate post dominators */
//	structure_cases(); /* recombine cases */
//	structure_loops(); /* recombine loops */
//	structure_ifs(); /* combine if-then-else */
	return true;
}

CLoop _CProc::find_loop(BB head, BB tail)
{
	std::deque<BB> stk;
	CLoop lp = std::make_shared<_CLoop>(head);
	if (head != tail) {
		lp->Add(tail);
		stk.push_front(tail);
	}
	while (!stk.empty()) {
		BB cur = stk.front(); stk.pop_front();
		for (auto p : cur->Preds()) {
			if (!lp->HasMember(p)) {
				lp->Add(p);
				stk.push_front(p);
			}
		}
	}
	head->SetLoop(lp);
	return lp;
}

bool loopsort(CLoop i, CLoop j)
{
	return j->HasMember(i);
}

void _CProc::find_all_loops()
{
	loops.clear();
	for (auto b : tsbbs) {
		if (b->TSNum() == 0) continue;
		for (auto s : b->Succs()) {
			if (b->IsPreDom(s))
				loops.push_back(find_loop(s, b));
		}
	}
}

void _CProc::find_ipdoms()
{
	for (auto it=tsbbs.rbegin(); it != tsbbs.rend(); it++) { // for all nodes except Exit()
		BB cur = *it;
		if (cur->Succs().size()==0) continue; // exclude exit node
		BitVec* postdom = cur->PostDom(); // list of post dominators
		for (auto s : cur->Succs()) {
			if (cur->IsPostDom(s)) {	 // successor is a post dominator
				if (!cur->SetIPDom(s)) { // s is immediate dominator of cur
					FATALERROR("Multiple IPDom in S=" + s->Name() + " Cur=" + cur->Name() +
							   " is " + cur->ImmPostDom()->Name());
				}
			}
		}
		cur->SetIPDom(); // fix unset
	}
}

/* return the recognized statement if BB was replaced by a stmt. */
BB _CProc::GetStmt(const BB bb) const
{
	auto stmt = stmts.find(bb->Name());
	return stmt != stmts.end() ? stmt->second : bb;
}

/* return the recognized statement if BB was replaced by a stmt. */
BB _CProc::GetNextStmt(const BB bb) const
{
	return GetStmt(bb)->Succ(0);
}

void _CProc::addstmt(BB stmt)
{
	stmts[stmt->Name()] = stmt;
}

/* recognize and process the switch statement
 * head calculates the switch expression into D0
 * bb is the actual table jump.
 * destroy the dead BBs case1a and case1b
 */
void _CProc::structure_cases()
{
	BB head, case1a, case1b;
	for (auto switchbb : bbs) {
		switch (switchbb->SType()) {
		case STMT_case1: 
			/* traverse to the actual expression head */
			case1b = switchbb->Pred(0);
			case1a = case1b->Pred(0);
			head = case1a->Pred(0);

			/* kill the helper BBs, they are no longer needed */
			case1b->Invalidate();
			case1a->Invalidate();
			break;
		case STMT_case2: // lbinlswitch base
			/* head is simply the predecessor of the case2 bb */
			head = switchbb->Pred(0);
			break;
		default:
			continue;
		}
		
		/* found a switch */
		std::cout << "STRUCTURE CASE " << head->Name() << std::endl;
		StmtFactory& sf = StmtFactory::Instance();
		SwitchStmt stmt = sf.Switch(head, switchbb);
		addstmt(stmt);
		structure_cont_brk(stmt, 0, stmt->Break());
	}
}

/* The loops PCC generates jump to the condition first */
void _CProc::structure_loops()
{
	/* sort loops from inner to outer */
	std::sort(loops.begin(), loops.end(), loopsort); 

	StmtFactory& sf = StmtFactory::Instance();

	/* recombine loops */
	for (auto lp : loops) {
//		std::cout << "STRUCTURE LOOP " << lp->Name() << std::endl;
		BB head = lp->Head();
		
		LoopStmt lstmt = sf.Loop(lp, STMTdowhile);
		addstmt(lstmt);
		
//		if (lstmt->Cont())
//			std::cout << "cont=" << lstmt->Cont()->Name() << std::endl;
//		if (lstmt->Break())
//			std::cout << "break=" << lstmt->Break()->Name() << std::endl << std::endl;
		for (auto lm : lstmt->Members()) {
//			std::cout << "Member: " << lm->Name() << std::endl;
			structure_cont_brk(lm, lstmt->Cont(), lstmt->Break());
		}
//		std::cout << "END STRUCTURE LOOP " << lp->Name() 
//			<< std::endl << std::endl;
	}
}

/* convert the case parts to "case xxx: statements; break;" */
void _CProc::structure_cont_brk(BB stmt, BB cont, BB brk)
{
	BB b;
	
//	std::cout << "SCB: " << stmt->Name() << 
//		" type=" << stmt->SType() << std::endl;
	switch (stmt->SType()) {
	case STMTexpr:
	case STMTgoto:
		std::cout << "scb goto/expr: " << stmt->Name() << ": ";
		if (stmt->Succ()==cont) {
			stmt->SetSType(STMTcontinue);
//			std::cout << " make continue" << std::endl;
		} else if (stmt->Succ()==brk) {
			stmt->SetSType(STMTbreak);
//			std::cout << " make break" << std::endl;
		} else {
//			std::cout << " nothing" << std::endl;
//			std::cout << std::endl;
		}
		break;
	case STMTif:
//		std::cout << "scb if: " << stmt->Name()	<< ": ";
		b = stmt->False();
		if (b==cont || b==brk)
			stmt->InvertCond();
		
		b = stmt->True();
		if (b == cont) stmt->SetSType(STMTcontinue);
		else if (b == brk) stmt->SetSType(STMTbreak);
		else {
			for (auto s : stmt->Succs())
				structure_cont_brk(s, cont, brk);
		}
		break;
//	case STMTswitch:
	default:
		break;
	}
}

void _CProc::structure_ifs()
{
	bool more;
	do {
		more = false;
		for (auto b : bbs)
			more |= structure_ifelse(b);
	} while (more);
	
}

bool _CProc::structure_ifelse(BB stmt)
{
	if (stmt->SType() != STMTif) return false;

	BB t = stmt->True();
	BB f = stmt->False();
	if (t->SuccCnt() != 1 || f->SuccCnt() != 1) return false;
	BB ts = t->Succ(0)->ImmPostDom();
	BB fs = f->Succ(0)->ImmPostDom();

	if (ts != fs)
		return false;
	
	IfStmt istmt = StmtFactory::Instance().If(stmt, t, f);
//	stmt->Replace(istmt);
	return true;
}
