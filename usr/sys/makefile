include kcommondefs

LIBS= lib.os lib.io lib.fs lib.munet lib.bmt lib.sock
NEWLIBS= lib.du
OBJS = l.o c.o linesw.o table.o name.o

all: unix

unix:   $(OBJS) $(LIBS)
	ed CURversion.c < version.ed >/dev/null
	$(CC) -c $(CFLAGS) CURversion.c
	$(LD) -o /newunix -x param $(OBJS) CURversion.o $(LIBS)
		mcs -d /newunix
	@echo new unix in /newunix

lunix:  $(OBJS) $(LIBS)
	ed CURversion.c < version.ed >/dev/null
	$(CC) -c $(CFLAGS) CURversion.c
	$(LD) -o lunix -x param $(OBJS) CURversion.o $(LIBS)
	@echo diskless node unix in lunix
	touch lunix
	sync

aunix:
	./sav my
	./get aunix
	-$(MAKE) allunix
	./get my

allunix: $(OBJS) $(LIBS) lib.nosup lib.startup
	ed CURversion.c < version.ed >/dev/null
	$(CC) -c $(CFLAGS) CURversion.c
	$(LD) -o /aunix -x param $(OBJS) CURversion.o lib.nosup $(LIBS) lib.startup
		mcs -d /aunix
	@echo new aunix in /aunix

libs:
	-cd ml; $(MAKE)
	-cd os; $(MAKE)
	-cd io; $(MAKE)
	-cd io/nosup; $(MAKE)
	-cd nudnix; $(MAKE)
	-cd fs/s5; $(MAKE)
	-cd fs/du; $(MAKE)
	-cd startup; $(MAKE)
	-cd munet; $(MAKE)
	-cd bmt; $(MAKE)
	-cd sock; $(MAKE)

c.o: conf.h $(INC)/space.h $(INC)/opt.h $(INC)/init.h
l.o: conf.h
linesw.o: conf.h
table.o: conf.h

clean:
	rm -f *.o os/*.o io/*.o io/nosup/*.o ml/*.o nudnix/*.o
	rm -f fs/*/*.o startup/*.o munet/*.o bmt/*.o bmt/*.oo
	rm -f sock/*/*.o sock/*/libsock
