#ifndef _CPROC_H__
#define _CPROC_H__

#include "common.h"
#include "symbol.h"
#include "bb.h"
#include "expr.h"
#include "regset.h"

/* Effectively, this stores a CFG (tree of BBs) */
class _CProc
{
private:
	InstrVector inst;
protected:
	CProc self;
	Segment *seg;
	String name;
	BBVector bbs;
	BBVector tsbbs;
	CLoopVector loops;
	
	BB rootbb;
	std::map<String,BB> stmts;
	bool isstatic;
	BB curbb;
	BB addbb(cString name);
	void addstmt(BB stmt);
	void debug(std::ostream& os) const;
	
	std::vector<int>params;
	std::vector<int>vars;
	int regs;
	
	void linkbbs();
	void collect_p_v_r();
	void make_pre_dominators();
	void make_post_dominators();
	CLoop find_loop(BB head, BB tail);
	void find_all_loops();
	void find_ipdoms();
	void structure_cases();
	void structure_loops();
	void structure_cont_brk(BB stmt, BB cont, BB brk);
	void structure_ifs();
	bool structure_ifelse(BB stmt);

public:
	_CProc(Symbol s);
	~_CProc() {}
	bool IsStatic() const { return isstatic; }
	cString& Name() const { return name; }
	CProc Self(CProc s=0) { if (s) self = s; return self; }
	
	static CProc MakeProc(Symbol s);

	BB Root() const { return bbs.front(); }
	BB Exit() const { return bbs.back(); }
	const CLoopVector& Loops() { return loops; }
	int BBCnt() const { return bbs.size(); }

	void AddInstr(Instr inst);
	BB BBbyName(cString nam) const; // get by symbol name
	BB BBbySeq(int seq) const; // get by sequence number
	BB BBbyTSNum(int tsnum) const;	// get by tsort number
	int TSortBBs(BB cur=0, int num=-1);

	BB GetStmt(const BB bb) const;
	BB GetNextStmt(const BB bb) const;

	bool Decompile(); // do transformations on procs
	
	void WriteC(std::ostream& os, int indent) const;
};

#endif

