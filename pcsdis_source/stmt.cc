#include "instr.h"
#include "segment.h"
#include "cloop.h"
#include "cproc.h"
#include "stmt.h"

/* a loop looks like
 *
 *   ...
 *   goto head;
 * body:
 *   stmt()...
 * head: // == continue
 *   if (cond) goto body;
 *   if (cond2) goto body; // cond || cond2
 *   ...
 * breakbb:
 *   ...
 */
_LoopStmt::_LoopStmt(CLoop lp, StmtType typ)
	: _BB(lp->Head(), typ), head(lp->Head()),
	  cond(head->RemoveLastExpr())
{
	copy_members(lp->Members());
	
	/* break is the node at exit of the loop */
	breakbb = head->ImmPostDom();

	/* continue is the previous block in loop before cond */
	contbb = before();
	if (!contbb) contbb = head;
}

/* The BB just before the head
 * This must be a member of the loop nodes
 * which has head as its immediate dominator.
 */
BB _LoopStmt::before() const
{
	for (auto m : Members()) {
		if (m->ImmPostDom() == head)
			return m;
	}
	return 0;
}

bool lmsort(BB i, BB j)
{
	return i->Name() < j->Name();
}

void _LoopStmt::copy_members(cBBVector& mems)
{
	for (auto m : mems)
		members.push_back(m);
	std::sort(members.begin(), members.end(), lmsort);
}

void _LoopStmt::write_intern(std::ostream& os, int indent) const
{
	write_do(os, indent);
}	

void _LoopStmt::WriteTitle(std::ostream& os, int indent) const
{
	String title("dowhile "); title += Name();
	Indent(os, indent, Comment(title));
}

void _LoopStmt::write_do(std::ostream& os, int indent) const
{
	Indent(os, indent, "do {");
	for (auto m : members) {
		/* for now, write head as a break condition */
		if (m->Seq() == head->Seq()) {
			static_cast<BB>(m)->WriteTitle(os, indent+2);
//			m->write_cont_brk(os, indent+2, "break;");
			m->WriteExprs(os, indent+2);
		} else
			m->WriteC(os, indent+2);
	}

	/* cannot print while cond for now, because
	 * complex conditions are not yet combined
	 */
#if 0
	IndentNoCR(os, indent, "} while (");
	os << cond->toExprString() << ");" << std::endl;
#else
	IndentNoCR(os, indent, "} while (1);"); os << std::endl;
#endif
}

/*****************************************************************************/
/* there are 2 types of switch statements recognized
 * case1: a switch with a consecutive range of cases
 *        starting with a low value, ending with a high value +
 *        a default case + a jump tabe. This is accomplished by 
 *        the consecutive BBs case1a, case1b, case1
 *        casehead:
 *			BB with some expressions which jumps to case1a
 *        .... some other code
 *        case1a:
 *          subtract low value from D0 to make d0 offset 0-based
 *          check if < 0, if so, goto default case
 *        case1b:
 *          check if > casecnt, if so, goto default case
 *        case1:
 *          indirect jump to case via following table block
 *        table:
 *          offset to case low+0
 *          offset to case low+1
 *          ...
 *          offset to case low+casecnt-1
 *        end of switch (break target)
 *
 * case2: a switch with a sparse jump table, this uses a compiler
 *        help "lbinlswitch" - the 32 bit code exclusively uses this.
 *        casehead:
 *          BB with some expressions which jums to case2:
 *        .... some other code
 *        case2:
 *          load casecnt into d1, d0 is the actual case expr result
 *          direct jump to lbinlswitch
 *        table:
 *          case value1
 *          offset to case for value1
 *          case value2
 *          offset to case for value2
 *          ....
 *          offset to default case
 *        end of switch (break target)
 *
 * instr4.cc already sets the successors of case1, case2 to the various cases
 *
 */

_SwitchStmt::_SwitchStmt(BB head, BB switchbb)
	: _BB(head, STMTswitch), head(head)
{
	/* head calculates the switch expression
	 * the list of successors of switchbb are the cases
	 */
	members = switchbb->Succs();
	head->SetSType(STMTswitch); // becomes a regular expression
	
	BB table = switchbb->GetNext();
	breakbb = table->GetNext();
	table->Invalidate(); // table is no longer needed
	
	switchbb->Invalidate(); // switchbb is now dead
}

void _SwitchStmt::WriteTitle(std::ostream& os, int indent) const
{
	String title("switch "); title += Name();
	Indent(os, indent, Comment(title));
}

void _SwitchStmt::write_intern(std::ostream& os, int indent) const
{
//	WriteTitle(os, indent);
	head->WriteC2(os, indent, false); // write the expressions
	Indent(os, indent, "switch (d0) {"); // switch head
	for (auto m : members) {			// write all cases
		if (m->Seq() == head->Seq()) continue;
		m->WriteC2(os, indent+2, false);
	}
	Indent(os, indent, "}");
}

/*****************************************************************************/

_IfStmt::_IfStmt(BB head, BB t, BB f)
	: _BB(head, STMTif),
	  head(head), tblk(t), fblk(f)
{
	cond = head->RemoveLastExpr();
}

void _IfStmt::WriteTitle(std::ostream& os, int indent) const
{
	String title("if "); title += Name();
	Indent(os, indent, Comment(title));
}

void _IfStmt::write_intern(std::ostream& os, int indent) const
{
	os << "if:write_intern" << std::endl;
	
	/* write leading expressions */
//	head->WriteC2(os, indent, false);
	WriteExprs(os, indent);
	String s("if (");
	s += cond->toExprString();
	s += ")";
	Indent(os, indent, s);
	if (tblk) {
		s = "{";
		Indent(os, indent, s);
		tblk->WriteC(os, indent+2);
	}
	if (fblk) {
		Indent(os, indent, "} else {");
		fblk->WriteC(os, indent+2);
	}
	if (tblk || fblk)
		Indent(os, indent, "}");
}

/*****************************************************************************/

SwitchStmt StmtFactory::Switch(BB head, BB switchbb) const 
{
	SwitchStmt sstmt = std::make_shared<_SwitchStmt>(head, switchbb);
	sstmt->Self(sstmt);
	return sstmt;
}

LoopStmt StmtFactory::Loop(CLoop lp, StmtType typ) const 
{
	LoopStmt lstmt = std::make_shared<_LoopStmt>(lp, typ);
	lstmt->Self(lstmt);
	return lstmt;
}

IfStmt StmtFactory::If(BB head, BB t, BB f) const
{
	IfStmt istmt = std::make_shared<_IfStmt>(head, t, f);
	istmt->Self(istmt);
	return istmt;
}

BB StmtFactory::Block(CProc proc, cString name, int bbnum) const
{
	BB newbb = std::make_shared<_BB>(proc, name, bbnum);
	return newbb->Self(newbb);
}

/*****************************************************************************/

