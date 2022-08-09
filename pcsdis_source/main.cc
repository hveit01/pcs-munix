
#include <getopt.h>
#include "cofffile.h"

int usage(const char* prog)
{
	DIAGstream << "Usage: " << prog << " [-dX] [-i in][-a asmout][-c cout] [in]" << std::endl
		<< "\tin\tInput file (PCS COFF)" << std::endl
		<< "\t-i in\tAlternative input file" << std::endl
		<< "\t-e X..\tEnable debug output" << std::endl
		<< "\t-d X..\tDisable debug output" << std::endl
		<< "\t-a asmout\tDump Assembler output ('-a-' dumps to std out)" << std::endl
		<< "\t-c cout\tDump C output ('-c-' dumps to std out)" << std::endl
		<< std::endl;
	return 1;
}

int main(int argc, char* argv[])
{
	/* input file, assembler output, decompiler output */
	String infile, asmout, decout;
	COFFFile df;
	int c;
	
	while ((c=getopt(argc,argv,"a:c:d:e:i:h")) != -1)
		switch (c) {
		case 'a':
			asmout = optarg;
			break;
		case 'd': // debugging
		case 'e': // debugging
			while (optarg[0]) {
				Diag::Set((DiagFlag)optarg[0], c=='e');
				optarg++;
			}
			break;
		case 'c':
			decout = optarg;
			break;
		case 'i':
			infile = optarg;
			break;
		default:
		case 'h':
		case '?':
			return usage(argv[0]);
		}
	if (infile.empty()) {
		if (optind < argc)
			infile = argv[optind];
		else {
			return usage(argv[0]);
		}
	}
	
	if (!df.Open(infile))
		FATALERROR("Open file " + infile);

	if (!df.Read())
		FATALERROR("Read file " + infile);

	Diag::Trace(DIAGfileread, "Success loading file " + infile);

	if (!asmout.empty() || !decout.empty()) {
		if (!df.Disassemble())
			FATALERROR("Error disassembling file " + infile);

		if (!asmout.empty()) {
			if (asmout[0]=='-')
				df.WriteAsm(std::cout);
			else {
				std::ofstream of(asmout);
				df.WriteAsm(of);
			}
		}
	}

	if (!decout.empty()) {
		if (!df.Decompile())
			FATALERROR("Error decompiling file " + infile);

		if (decout[0]=='-')
			df.WriteC(std::cout);
		else {
			std::ofstream of(decout);
			df.WriteC(of);
		}
	}
	
	return 0;
}

