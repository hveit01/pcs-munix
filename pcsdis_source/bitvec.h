#ifndef __BITVEC_H__
#define __BITVEC_H__

/* there is <bitset> but this is static sized
 * There is also vector<bool> which I use as a base to add the required
 * operations I need */

class BitVec {
protected:
	std::vector<bool> bvec;
public:
	BitVec(int sz, bool initval=false);
	BitVec(const BitVec* other);
	~BitVec() {}
	
	int  Size() const { return bvec.size(); }
	void SetAll();
	void ClearAll();
	bool Set(int n); // true if success
	bool Clear(int n); // true if success
	bool Get(int n) const;
	
	void Copy(const BitVec* other);
	bool operator==(const BitVec* other) const;
	bool operator!=(const BitVec* other) const;
	void And(const BitVec* other);
	void Or(const BitVec* other);
	void Dump(std::ostream& os) const;
	
	int FirstNotEqual(int n) const; // return first index != n
};

#endif
