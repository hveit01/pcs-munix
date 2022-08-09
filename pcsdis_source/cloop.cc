#include "instr.h"
#include "segment.h"
#include "cloop.h"

_CLoop::_CLoop(BB head)
	:head(head), members()
{
	Add(head);
}

bool _CLoop::HasMember(const BB bb) const
{
	for (const auto m : members)
		if (m == bb) return true;
	return false;
}

bool _CLoop::HasMember(const CLoop lp) const
{
	return HasMember(lp->Head());
}

void _CLoop::Debug(std::ostream& os) const
{
	os << "\t/* Loop " << head->Name() << std::endl 
	   << "\t *  members=";
	for (auto m : members) os << m->Name() << " ";
	os << std::endl << "\t */" << std::endl;
}