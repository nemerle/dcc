CFLAGS = -Zi -c -AL -W3 -D__MSDOS__

srchsig.exe:	srchsig.obj perfhlib.obj fixwild.obj
	link /CO srchsig perfhlib fixwild;

srchsig.obj:	srchsig.c dcc.h perfhlib.h
	cl $(CFLAGS) $*.c

perfhlib.obj:	perfhlib.c dcc.h perfhlib.h
	cl $(CFLAGS) $*.c

fixwild.obj:	fixwild.c dcc.h
	cl $(CFLAGS) $*.c

