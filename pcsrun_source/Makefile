EXE=.exe
CFLAGS = -I. -g -Wno-write-strings
CC = gcc
CCC = g++

MUSASHIC = m68kcpu.cc m68kdasm.cc m68kopac.cc\
	m68kopdm.cc m68kopnz.cc m68kops.cc\
	fsincos.cc fyl2x.cc softfloat.cc
MUSASHIO = m68kcpu.o m68kdasm.o m68kopac.o\
	m68kopdm.o m68kopnz.o m68kops.o\
	fsincos.o fyl2x.o softfloat.o

CFILES = main.cc diag.cc file.cc mem.cc proc.cc syscall.cc util.cc\
	map.cc $(MUSASHIC)
OFILES = main.o diag.o file.o mem.o proc.o syscall.o util.o\
	map.o $(MUSASHIO)

all:	pcsrun$(EXE)

clean:
	rm *.o pcsrun$(EXE)
	
pcsrun$(EXE): $(OFILES)
	$(CCC) -o $@ $(OFILES)
	cp $@ ..

m68kmake$(EXE): m68kmake.c
	$(CC) -o $@ m68kmake.c

%.o: %.c
	$(CC) $(CFLAGS) -c $<
	
%.o: %.cc
	$(CCC) $(CFLAGS) -c $<

m68kopac.cc m68kopdm.cc m68kopnz.cc m68kops.cc m68kops.h: m68kmake$(EXE) m68k_in.c
	./m68kmake
	
diag.cc: pcsrun.h debug.h
file.cc: pcsrun.h file.hpp
mem.cc: pcsrun.h mem.hpp m68k.h debug.h
proc.cc: pcsrun.h proc.hpp m68k.h
main.cc: pcsrun.h m68k.h
syscall.cc: pcsrun.h m68k.h
util.cc: pcsrun.h
pcsrun.h: debug.h coff.h mem.hpp file.hpp proc.hpp

m68kcpu.cc: m68kops.h m68kcpu.h m68kfpu.h
m68kcpu.h: m68k.h
m68kdasm.cc: m68k.h
m68k.h: m68kconf.h milieu.h softfloat.h
m68kopac.cc: m68kcpu.h m68k_in.c
m68kopdm.cc: m68kcpu.h m68k_in.c
m68kopnz.cc: m68kcpu.h m68k_in.c
m68kops.cc: m68kcpu.h m68k_in.c
m68kopac.h: m68k_in.cc

fsincos.cc: mamesf.h softfloat.h fpu_constant.h
fyl2x.cc: mamesf.h softfloat.h fpu_constant.h
softfloat.cc: softfloat.h milieu.h softfloat-specialize
softfloat.h: softfloat-macros
milieu.h: mamesf.h
