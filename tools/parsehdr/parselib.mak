CFLAGS = -Zi -c -AS -W3 -D__MSDOS__

parselib.exe:	parselib.obj
	link /CO parselib;

parselib.obj:	parselib.c
	cl $(CFLAGS) $*.c

