EXE=.exe
CFLAGS = -I. -g -Wno-write-strings -std=c++11
#-Wall
CC = gcc
CCC = g++

CFILES = main.cc cofffile.cc reloc.cc segment.cc binfile.cc symbol.cc\
	 util.cc instr.cc instr2.cc instr3.cc arg.cc bb.cc\
	 instfactory.cc expr.cc regset.cc exprfactory.cc cproc.cc\
	 stmt.cc instr4.cc instr5.cc cloop.cc
#OFILES = main.o cofffile.o reloc.o segment.o binfile.o symbol.o\
#	  util.o instr.o instr2.o instr3.o arg.o bb.o\
#	  instfactory.o expr.o regset.o exprfactory.o cproc.o\
#	  stmt.o instr4.o instr5.o
OFILES = $(CFILES:.cc=.o)

all:	dis$(EXE)

clean:
	rm *.o dis$(EXE)
	
dis$(EXE): $(OFILES)
	$(CCC) -o $@ $(OFILES)

%.o: %.c
	$(CC) $(CFLAGS) -c $<
	
%.o: %.cc
	$(CCC) $(CFLAGS) -c $<
