#include "instr.h"
#include "segment.h"
#include "cloop.h"
#include "cproc.h"

_BB::_BB(CProc proc, cString nam, int seq)
	: self(0), name(nam), proc(proc), members(), 
	  typ(STMTnone), 
	  succs(), preds(), 
	  defcase(false), cases(0), 
	  seq(seq), tsnum(NOTVISITED), 
	  instrs(), exprs(),
	  loop(0),
	  predom(0), postdom(0), ipdom(0),
	  flags(0)
{}

_BB::_BB(BB bb, StmtType styp)
	: self(0), name(bb->Name()), proc(bb->Proc()), members(),
	  typ(styp),
	  succs(), preds(), 
	  defcase(bb->IsDefCase()), cases(0), 
	  seq(bb->Seq()), tsnum(bb->TSNum()),
	  instrs(), exprs(),
	  loop(0),
	  predom(0), postdom(0), ipdom(0),
	  flags(0)
{}

bool _BB::operator==(const BB& other) const
{
	return proc == other->proc && seq == other->seq;
}

BB _BB::Succ(int n) const
{
	try { return succs.at(n); }
	catch (const std::out_of_range& oor) { return 0; }
}

bool _BB::IsSucc(BB b) const
{
	std::cout << "Issucc " << b->Name() << std::endl;
	for (auto s : succs) {
		std::cout << "Check: " << s->Name() << " seqs=" << s->seq << "," << b->seq << std::endl;
		if (s->seq == b->seq) return true;
	}
	std::cout << "no succ" << std::endl;
	return false;
}

/* only add if not duplicate (multiple case clauses) */
void _BB::AddSucc(BB s)
{
	/* compatibility C++-11 */
	for (auto bb : succs) {
		if (bb->Seq() == s->Seq()) return;
	}
	succs.push_back(s);
}

void _BB::RemoveSucc(BB b)
{
	/* compatibility C++-11 */
	for (auto it = succs.begin(); it != succs.end(); ++it) {
		if ((*it)->Seq() == b->seq) {
			succs.erase(it); return;
		}
	}
}

BB _BB::Pred(int n) const
{
	try { return preds.at(n); }
	catch (const std::out_of_range& oor) { return 0; }
}

void _BB::AddPred(BB p)
{
	/* only add if not duplicate (multiple case clauses) */
	for (auto bb : preds) {
		if (bb->Name() == p->Name()) return;
	}
	preds.push_back(p);
}

void _BB::RemovePred(BB b)
{
	for (auto it = preds.begin(); it != preds.end(); ++it) {
		if ((*it)->Seq() == b->seq) {
			preds.erase(it); return;
		}
	}
}

void _BB::Invalidate()
{
	succs.erase(succs.begin(), succs.end());
	preds.erase(preds.begin(), preds.end());
	tsnum = -1;
	delete predom; predom = 0;
	delete postdom; postdom = 0;
	typ = STMTnone;
}

const char* _BB::debug_type() const
{
	switch (typ) {
	default:
	case STMTnone: return "NONE";
	case STMTif: return "IF";
	case STMTdowhile: return "DOWHILE";
	case STMTreturn: return "RETURN";
	case STMTexpr: return "EXPR";
	case STMT_case1: return "CASE1(temp)";
	case STMTswitch: return "SWITCH";
	case STMT_case2: return "CASE2(temp)";
	case STMTgoto: return "GOTO";
	case STMT_table: return "TABLE(temp)";
	case STMT_null: return "NULL";
	case STMTcontinue: return "CONTINUE";
	case STMTbreak: return "BREAK";
	}
}

void _BB::debug_dom(std::ostream& os, cString& prefix, const BitVec* v) const
{
	os << prefix;
	if (tsnum != NOTVISITED && v != 0) {
		for (int i=0; i < v->Size(); i++) {
			if (v->Get(i)) os << proc->BBbyTSNum(i)->Name() << "(" << i << ") ";
		}
	} else
		os << "null ";
	os << std::endl;
}

void _BB::debug_bbs(std::ostream& os, cString& prefix, const std::vector<BB>& v) const
{
	os << prefix;
	if (v.size())
		for (auto it : v) os << it->Name() << " ";
	else os << "null ";
	os << std::endl;
}

void _BB::debug_cases(std::ostream& os, cString& prefix) const
{
	if (cases.size() == 0) return;
	os << prefix;
	for (auto it : cases) os << it << " ";
	os << std::endl;
}

void _BB::Debug(std::ostream& os) const
{
	os << "/* BB(" << tsnum << "): " << Name();
	if (IsFlag(BBF_ISLOOP))
		os << " LOOP";
	os << " type=" << debug_type() << std::endl;
	if (IsFlag(BBF_ISLOOP)) {
		debug_bbs(os, "\t  loop=", loop->Members());
	}

	BB ipd = ImmPostDom();
	os << "\t  ipdom=" << (ipd ? ipd->Name() : "null") << std::endl;
//		debug_dom(os,   "\t  predom=", predom);
//		debug_dom(os,   "\t  postdom=", postdom);
	if (succs.size()==2)
		os << "\t  succs: f=" << succs[0]->Name() << " t=" << succs[1]->Name() << std::endl;
	else
		debug_bbs(os,   "\t  succs=", succs);
//		debug_bbs(os,   "\t  preds=", preds);
	debug_cases(os, "\t  cases=");

	if (Diag::IsDiag(DIAGinstr)) {
		for (auto it : instrs) 
			if (it->IsValid()) Diag::Info("\t  " + it->toAsmString());
	}
	os << "*/" << std::endl;

	if (Diag::IsDiag(DIAGexpr))
		WriteExprs(os, 8);

	os << std::endl;
}

void _BB::WriteExprs(std::ostream& os, int indent) const
{
	for (auto ex : exprs) {
		if (ex->IsExcluded())
			Indent(os, indent, "/* " + ex->toExprString() + "; */");
		else
			Indent(os, indent, ex->toExprString() + ";");
	}
}

void _BB::WriteTitle(std::ostream& os, int indent) const
{
	String c(Name() + " "); c += debug_type();
	BB t = True(); if (t) c += " t=" + t->Name();
	BB f = False(); if (f) c += " f=" + f->Name();
	Indent(os, indent, Comment(c));
}

/* write a statement with or without curly brackets */
void _BB::WriteC2(std::ostream& os, int indent, bool curly) const
{
	if (IsFlag(BBF_WRITTEN)) return;

	if (SType()==STMTnone) {
		Indent(os, indent, Comment("dead BB "+Name()));
		return;
	}

	/* process curly brackets:
	 * not written if not requested, no case labels or no exprs */
	int ecnt = ExprCnt();
	curly &= write_cases(os, indent-2);
	curly &= (ecnt>0);
	if (curly) { 
		Indent(os, indent, "{");
		indent += 2;
	}

	switch (typ) { /* do not use SType()! */
	case STMTcontinue:
		write_cont_brk(os, indent, "continue;");
		break;
	case STMTbreak:
		write_cont_brk(os, indent, "break;");
		break;
	case STMTif:
		break;
	case STMTgoto:
		WriteExprs(os, indent);
		Indent(os, indent, "goto " + Succ(0)->Name() + ";");
		break;
	case STMTreturn:
		Indent(os, indent, "return;"); // TODO: return value
		break;
	case STMTexpr:
	case STMT_case2:
	case STMT_case1:
	case STMTswitch:
	case STMT_table:
		WriteExprs(os, indent);
		break;
	default:
		FATALERROR("Default case in " + Name() + " SType=" + debug_type());
	}
	if (curly) Indent(os, indent-2, "}");
}

void _BB::write_cont_brk(std::ostream& os, int indent, cString& s) const
{
	if (SuccCnt() > 1) { // was an IF before
		BB s = const_cast<_BB*>(this)->Self();
		IfStmt ifs = StmtFactory::Instance().If(s, 0, 0);
		ifs->WriteC(os, indent);
		indent += 2;
	} else { // was an EXPR/GOTO before
		/* write leading expressions */
		WriteExprs(os, indent);
	}
	Indent(os, indent, s);
}

bool _BB::write_cases(std::ostream& os, int indent) const
{
	bool iscase = cases.size() > 0 || defcase;
	if (!iscase) return true;

	if (defcase)
		Indent(os, indent, "default:");

	for (auto c : cases) {
		String s("case "); s += std::to_string(c); s+= ":";
		Indent(os, indent, s);
	}
	return false;
}

/* write as statement if it is a statement */
void _BB::WriteC(std::ostream& os, int indent) const
{
	/* don't write twice */
	if (IsFlag(BBF_WRITTEN)) return;
	
	const BB stmt = proc->GetStmt(self);
	stmt->WriteTitle(os, indent);
	stmt->write_intern(os, indent);

	/* mark BB as written */
	const_cast<_BB*>(this)->SetFlag(BBF_WRITTEN);
}

BB _BB::GetNext() const
{
	return proc->BBbySeq(seq+1);
}

BB _BB::GetPrev() const
{
	if (seq <= 0)
		FATALERROR("No previous BB for " + Name());
	return proc->BBbySeq(seq-1);
}

void _BB::LinkNext(StmtType styp)
{
	SetSType(styp);
	BB next = GetNext();
	if (next) {
		AddSucc(next);
		next->AddPred(self);
	}
}

/* Add submitted instrs.
 * Convert them directly into exprs
 * Process them in order to aggregate them.
 */
void _BB::AddInstr(Instr inst)
{
	instrs.push_back(inst);
	
	Expr ex = inst->MakeExpr(self);
	AddExpr(ex);
	ex = inst->GetExpr2();
	if (ex) AddExpr(ex);
}

/* aggregate an expression with info from registers */
void _BB::AddExpr(Expr expr)
{
	if (!expr) return;

//	Diag::Info(DIAGexpr, " AddExpr at " + Name() + ": " + expr->toExprString());
	exprs.push_back(expr);
}

/* invert a condition for an IF BB */
void _BB::InvertCond()
{
	if (SType() != STMTif) return;
	Expr ex = LastExpr();
	ex->InvertCond();
	BB t = True();
	BB f = False();
	succs[1] = t;
	succs[0] = f;
}

BB _BB::BBbyName(cString name) const
{
	return proc->BBbyName(name);
}

void _BB::Link()
{
	Instr last = instrs.back(); // last instr
	if (last)
		last->LinkBB(self);
}

Expr _BB::GetExpr(int n) const
{
	try { return exprs.at(n); }
	catch (const std::out_of_range& oor) { return 0; }
}

Expr _BB::RemoveLastExpr()
{
	Expr last = LastExpr();
//	exprs.pop_back();
	last->Exclude(); // debugonly
	return last;
}

void _BB::LinkBB(BB target)
{
	if (target) {
		AddSucc(target);
		target->AddPred(self);
	}
}

void _BB::LinkBB(cString name, StmtType styp)
{
	BB target = BBbyName(name);
	LinkBB(target);
	if (styp != STMTnone)
		SetSType(styp);
}

Instr _BB::GetInstr(int i) const
{
	try {
		return instrs.at(i);
	}
	catch (const std::out_of_range& oor) {
		return 0;
	}
}

int _BB::Visit()
{
	int num = TSNum();
	for (auto s : succs) {
		if (!s->Visited())
			num = proc->TSortBBs(s, num+1);
	}
	return num;
}

void _BB::InitPreDom(int n)
{
	predom = new BitVec(n, true); /* all set */
}

void _BB::InitPostDom(int n)
{
	postdom = new BitVec(n, true); /* all set */
}

bool _BB::IsPreDom(BB b) const
{
	return predom->Get(b->TSNum());
}

bool _BB::IsPostDom(BB b) const
{
	return postdom->Get(b->TSNum());
}

/* sets the immediate post dominator, if
 * 1. not this node itself
 * 2. and not already set
 * unless newipdom is 0:
 *   this will try to fix the IP, if
 * 1. it is not the exit node
 * 2. and there is one other dominator except the node itself
 *    (ipdom is this node then).
 * Can there be more than one dominator then?
 *   Sure, but there can be only one which is the immediate one.
 *   Since the CFG is topologically sorted, so is the list of post dominators
 *   and o the first one found in the list is the immediate one.
 */
bool _BB::SetIPDom(BB newipdom)
{
	if (newipdom) {
		if (newipdom->tsnum == tsnum) return true; // ignore attempt to set oneself
		if (ipdom) return false; // already set
		ipdom = newipdom;
	} else if (Visited() && succs.size() > 0) { // fix unset, if it is not exit and a regular node
		int idx = postdom->FirstNotEqual(tsnum);
		if (idx != -1)
			ipdom = proc->BBbyTSNum(idx);
		else
			FATALERROR("No IPDom for " + Name());
	}
	return true;
}

void _BB::SetLoop(CLoop lp)
{
	loop = lp;
	SetFlag(BBF_ISLOOP);
}
