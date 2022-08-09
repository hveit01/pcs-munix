#ifndef _DBYTE_H_
#define _DBYTE_H_

#include "item.h"
#include "symbol.h"

#define DB_IHEAD	0x0001	/* start of instruction or data */
#define DB_ISUCC	0x0002	/* successor of instruction or data */

class _DByte : public _Item
{
protected:
	int flags;
	Symbol sym;
	int value;

public:
	_DByte(int addr, int value)
		: _Item(addr), flags(0), sym(0), value(value) {}
	virtual ~_DByte() {}
	ItemType IsA() const { return ITEMbyte; }

	int Size() const { return 1; }

	Symbol Sym() const { return sym; }
	void SetSym(Symbol newsym) { sym = newsym; }

	void AddFlag(int f) { flags |= f; }
	bool IsFlag(int f) const { return flags & f; }

	int Value() const { return value; }
	void SetValue(int newval) { value = newval; }
};

typedef std::shared_ptr<_DByte> DByte;


#endif
