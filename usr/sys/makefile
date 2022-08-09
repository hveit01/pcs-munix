SHELL=/bin/sh
CC=cc
B=/bin
LD=$B/ld
A68=a68
BMTLIB=lib.bmt
.s.o:
	/lib/cpp -P -DPCS $*.s >x.s
	$(A68) x.s
	mv x.o $*.o
	rm x.s

unix:   l.o c.o linesw.o name.o lib.os lib.io lib.munet lib.bmt
	ed CURversion.c < version.ed >/dev/null
	$(CC) -c $(CFLAGS) CURversion.c
	$(LD) -o /newunix -x param CURversion.o l.o c.o linesw.o name.o lib.os lib.io lib.munet lib.bmt lib.tcpip
	@echo new unix in /newunix

lunix:  l.o c.o linesw.o name.o lib.os lib.io lib.munet lib.bmt
	ed CURversion.c < version.ed >/dev/null
	$(CC) -c $(CFLAGS) CURversion.c
	$(LD) -o lunix -x param CURversion.o l.o c.o linesw.o name.o lib.os lib.io lib.munet lib.bmt
	@echo diskless node unix in lunix
	touch lunix
	sync

aunix:
	get aunix
	-make allunix

allunix:l.o c.o linesw.o name.o lib.os lib.io lib.startup lib.munet lib.nosup lib.bmt
	ed CURversion.c < version.ed >/dev/null
	$(CC) -c $(CFLAGS) CURversion.c
	$(LD) -o /aunix -x param CURversion.o l.o c.o linesw.o name.o lib.startup lib.os lib.nosup lib.io lib.munet lib.bmt lib.tcpip

libs:
	-cd os; $(MAKE)
	-cd io; $(MAKE)
	-cd io/nosup; $(MAKE)
	-cd ml; $(MAKE)
	-cd startup; $(MAKE)
	-cd munet; $(MAKE)
	-cd tcpsock; $(MAKE)
	-cd bmt; $(MAKE)

c.o: conf.h
l.o: conf.h
linesw.o: conf.h

clean:
	rm -f *.o os/*.o io/*.o io/nosup/*.o ml/*.o \
		  startup/*.o munet/*.o bmt/*.o tcpsock/*.o

