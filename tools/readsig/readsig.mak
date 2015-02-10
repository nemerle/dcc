CFLAGS = -Zi -c -AL -W3 -D__MSDOS__

readsig.exe:	readsig.obj perfhlib.obj
	link /CO readsig perfhlib;

readsig.obj:	readsig.c dcc.h perfhlib.h
	cl $(CFLAGS) $*.c

perfhlib.obj:	perfhlib.c dcc.h perfhlib.h
	cl $(CFLAGS) $*.c

