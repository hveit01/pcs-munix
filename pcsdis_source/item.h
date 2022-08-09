#ifndef __ITEM_H__
#define __ITEM_H__

#include "common.h"

/* base class */ 
class _Item
{
protected:
	int addr;
public:
	_Item(int addr) : addr(addr) {}
	virtual ~_Item() {}
	
	int Addr() const { return addr; }
	void SetAddr(int newaddr) { addr = newaddr; }

	virtual ItemType IsA() const { return ITEMunknown; }
	virtual int Size() const { return 0; } // # of bytes

	/* storage of scalar data */
	virtual int Value() const { return 0; }
	virtual int16_t Value16() const { return Value(); }
	virtual void SetValue(int newval) {}
};
typedef std::shared_ptr<_Item> Item;
typedef std::vector<Item> ItemVector;

/* DByte is derived from Item, see dbyte.h */

/* stores a word, either signed or unsigned */
class _ItemWord : public _Item
{
protected:
	int value;
public:
	_ItemWord(int addr, int value) : _Item(addr), value(value) {}
	virtual ~_ItemWord() {}

	ItemType IsA() const { return ITEMword; }
	int Size() const { return 2; } // # of bytes
	int Value() const { return value; }
	int16_t Value16() const { return (int16_t)value; }
	void SetValue(int newval) { value = newval; }
};
typedef std::shared_ptr<_ItemWord> ItemWord;

class _ItemLong : public _Item
{
protected:
	int value;
public:
	_ItemLong(int addr, int value) : _Item(addr), value(value) {}
	virtual ~_ItemLong() {}
	ItemType IsA() const { return ITEMlong; }
	int Size() const { return 4; } // # of bytes

	int Value() const { return value; }
	void SetValue(int newval) { value = newval; }
};
typedef std::shared_ptr<_ItemLong> ItemLong;

#endif
