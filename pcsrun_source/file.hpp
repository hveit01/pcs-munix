#ifndef __FILEENTRY__
#define __FILEENTRY__


class FILEENT {
private:
	char* name;
	int fd;
	int mode;	
	bool unlk;
	bool istty;
	
public:
	FILEENT();
	~FILEENT();
	
	void Clone(const FILEENT& orig);

	void Set(const char *n, int f, int m, bool i);
	int Close();
	
	bool IsNamed(const char* name) const;
	void DeleteOnClose() { unlk = true; }
	static void Delete(const char* name);
		
	bool IsClosed() const { return fd == -1; }
	
	const char* Name() const { return name; }
	int RealFd() const { return fd; }
	int Mode() const { return mode; }
	bool IsTty() const { return istty; }

	void DebugRWbuf(bool isread, unsigned int addr, int size);

};


#endif
