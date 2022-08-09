#ifndef __CLOOP_H__
#define __CLOOP_H__

#include "common.h"
#include "symbol.h"
#include "bb.h"
#include "expr.h"
#include "regset.h"

class _CLoop {
protected:
	BB head;
	BBVector members;
public:
	_CLoop(BB head);
	~_CLoop() {}
	cString Name() const { return head->Name(); }
	
	BB Head() const { return head; }
	BB Last() const { return members.back(); }
	cBBVector& Members() const { return members; }
//	BB Member(int n) const;
	void Add(BB bb) { members.push_back(bb); }
	bool HasMember(const BB bb) const;
	bool HasMember(const CLoop lp) const;
	
	void Debug(std::ostream& os) const;
};

#endif
