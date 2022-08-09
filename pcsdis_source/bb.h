#ifndef _BB_H__
#define _BB_H__

#include <bitset>

#include "common.h"
#include "symbol.h"
#include "expr.h"
#include "regset.h"
#include "bitvec.h"

#define TSROOT 0
#define NOTVISITED -1

/* flags */
#define BBF_WRITTEN	0x00000001
#define BBF_ISLOOP	0x00000002

/*declare basic block */
class _BB
{
protected:
	BB self;
	Symbol sym;
	String name;
	CProc proc;
	BBVector members;
	StmtType typ;

	BBVector succs;
	BBVector preds;

	bool defcase;
	std::vector<int> cases;

	int seq; /* numerical BB in procedural code */
	int tsnum; /* topologically sorted BB number in proc */

	InstrVector instrs;
	ExprVector exprs;
	CLoop loop;
	

//	RegSet inset;
//	RegSet outset;

	BitVec *predom;
	BitVec *postdom;
	BB ipdom;
	
	int flags;
	
	BB pre(int i) const;
	BB post(int i) const;

	const char *debug_type() const;
	void debug_dom(std::ostream& os, cString& prefix, const BitVec* v) const;
	void debug_bbs(std::ostream& os, cString& prefix, cBBVector& v) const;
	void debug_cases(std::ostream& os, cString& prefix) const;

	bool write_cases(std::ostream& os, int indent) const;
//	void write_cont_brk(std::ostream& os, int indent, cString& s) const;
	
	/* internal write statement */
	virtual void write_intern(std::ostream& os, int indent) const {
		WriteC2(os, indent, false); }

public:
	_BB(BB bb, StmtType styp); // for converting BBs into aggregate statements
	_BB(CProc proc, cString name, int bbnum);
	virtual ~_BB() {}
	virtual cString& Name() const { return name; }
//	virtual cSymbol Sym() const { return sym; }
	bool operator==(const BB& other) const;
	BB Self(BB s=0) { if (s) self=s; return self; }
	CProc Proc() const { return proc; }
	
	cBBVector& Members() const { return members; }
	
	/* instr handling */
	void AddInstr(Instr inst);
	Instr GetInstr(int i) const; // can return 0
	int InstrCnt() const { return instrs.size(); }
	Instr Last() const { return instrs.back(); }
	
	int GetFlags() const { return flags; }
	void SetFlags(int newflg) { flags = newflg; }
	void SetFlag(int addflg) { flags |= addflg; }
	void ClrFlag(int subflg) { flags &= ~subflg; }
	bool IsFlag(int flg) const { return (flags & flg) != 0; }

	/* type of BB */
	StmtType SType() const { return typ; }
	void SetSType(StmtType newtyp) { typ = newtyp; }

	/* topological sorting */
	int Seq() const { return seq; }
	void SetTSNum(int n) { tsnum = n; }
	int TSNum() const { return tsnum; }
	int Visit();
	bool Visited() const { return tsnum != NOTVISITED; }
	void Invalidate();

	// successors and predecessors
	virtual BB Succ(int i = 0) const;
	int SuccCnt() const { return succs.size(); }
	cBBVector& Succs() const { return succs; }
	bool IsSucc(BB b) const;
	void AddSucc(BB s);
	void RemoveSucc(BB b);
	BB Pred(int i) const;
	int PredCnt() const { return preds.size(); }
	cBBVector& Preds() const { return preds; }
	void AddPred(BB p);
	void RemovePred(BB b);
	
	const ExprVector& Exprs() const { return exprs; }
	int ExprCnt() const { return exprs.size(); }
	Expr LastExpr() const { return exprs.back(); }
	Expr RemoveLastExpr();
	Expr GetExpr(int i) const;
	void AddExpr(Expr ex);
	void InvertCond();

	// switch support
	void MakeCase() { defcase = true; }
	void MakeCase(int val) { cases.push_back(val); }
	bool IsDefCase() const { return defcase; }

	/* navigation */
	BB True() const { return Succ(0); }
	BB False() const { return Succ(1); }
	BB BBbyName(cString name) const;
	BB GetNext() const;
	BB GetPrev() const;

	/* dominators of BB */
	BitVec* PreDom() { return predom; } // dominating predecessors
	BitVec* PostDom() { return postdom; } // dominating successors
	void InitPreDom(int n);
	void InitPostDom(int n);
	bool IsPreDom(BB b) const;
	bool IsPostDom(BB b) const;
	BB ImmPostDom() const { return ipdom; }
	bool SetIPDom(BB newipdom=0); // false if already set
	
	/* for linkage of BBs */
	void LinkNext(StmtType styp);
	void LinkBB(BB target);
	void LinkBB(cString name, StmtType styp=STMTnone);
	void Link(); // do linkage
	
	/* remember it is a loop head */
	void SetLoop(CLoop lp);

	/* output stuff */
	void WriteC(std::ostream& os, int indent) const;

	/* pure write, no stmt indirection */
	void WriteC2(std::ostream& os, int indent, bool curly) const; // pure write
	virtual void WriteTitle(std::ostream& os, int indent) const;
	void WriteExprs(std::ostream& os, int indent) const;
	void write_cont_brk(std::ostream& os, int indent, cString& s) const;
	virtual void Debug(std::ostream& os) const;
};

/*****************************************************************************/

/* A statement is a BasicBlock with special properties
 * usually it is a container
 */

class _LoopStmt : public _BB
{
protected:
	BB head;
	Expr cond;
	BBVector members;
	BB breakbb;
	BB contbb;

	BB before() const;
	void copy_members(cBBVector&);
	
	void write_do(std::ostream& os, int indent) const;
	void write_intern(std::ostream& os, int indent) const;
	
public:
	_LoopStmt(CLoop lp, StmtType typ);
	~_LoopStmt() {}
	
	BB Succ(int i=0) const { return breakbb; }

	Expr Cond() const { return cond; }
	void SetCond(Expr newc) { cond = newc; }
	cBBVector& Members() const { return members; }
	BB Cont() const { return contbb; }
	void SetCont(BB newc) { contbb = newc; }
	BB Break() const { return breakbb; }

	void WriteTitle(std::ostream& os, int indent) const;
	
};
typedef std::shared_ptr<_LoopStmt> LoopStmt;

class _SwitchStmt : public _BB
{
protected:
	BB head;
	BB breakbb;
	void write_intern(std::ostream& os, int indent) const;

public:
	_SwitchStmt(BB head, BB switchbb);
	~_SwitchStmt() {}

	BB Succ(int i=0) const { return breakbb; }
	BB Break() const { return breakbb; }
	void WriteTitle(std::ostream& os, int indent) const;
};
typedef std::shared_ptr<_SwitchStmt> SwitchStmt;

class _IfStmt : public _BB
{
protected:
	BB head;
	Expr cond;
	BB tblk;
	BB fblk;
	void write_intern(std::ostream& os, int indent) const;
public:
	_IfStmt(BB head, BB t, BB f);
	~_IfStmt() {}
	void WriteTitle(std::ostream& os, int indent) const;
	static void WriteIf(BB bb, std::ostream& os, int indent);
};
typedef std::shared_ptr<_IfStmt> IfStmt;

class StmtFactory {
private:
	StmtFactory() {}
public:
	static StmtFactory& Instance() {
		static StmtFactory instance;
		return instance;
	}
	StmtFactory(StmtFactory const&) = delete;
	void operator=(StmtFactory const&) = delete;
	
	// generate a standard BB block,
	// dependung on last instruction
	BB Block(CProc proc, cString name, int bbnum) const;
	
	LoopStmt Loop(CLoop lp, StmtType typ) const;
	SwitchStmt Switch(BB head, BB switchbb) const;
	IfStmt If(BB first, BB t, BB f) const;
};

#endif

