CFLAGS = -Zi -c -AL -W3 -D__MSDOS__

dispsig.exe:	dispsig.obj perfhlib.obj
	link /CO dispsig perfhlib;

dispsig.obj:	dispsig.c dcc.h perfhlib.h
	cl $(CFLAGS) $*.c

perfhlib.obj:	perfhlib.c dcc.h perfhlib.h
	cl $(CFLAGS) $*.c

