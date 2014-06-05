I've fixed many issues in this codebase, among other things - memory reallocation during decompilation.
To reflect those fixes, I've edited the original readme a bit.

-----------------------------
Original readme:

			   dcc Distribution

The code provided in this distribution is (C) by their authors:
	Cristina Cifuentes (most of dcc code)
	Mike van Emmerik (signatures and prototype code)
	Jeff Ledermann (some disassembly code)
and is provided "as is".

The following files are included in the dccoo.tar.gz distribution:
- dcc.zip (dcc.exe DOS program, 1995)
- dccsrc.zip (source code *.c, *.h for dcc, 1993-1994)
- dcc32.zip (dcc_oo.exe 32 bit console (Win95/Win-NT) program, 1997)
- dccsrcoo.zip (source code *.cpp, *.h for "oo" dcc, 1993-1997)
- dccbsig.zip (library signatures for Borland C compilers, 1994)
- dccmsig.zip (library signatures for Microsoft C compilers, 1994)
- dcctpsig.zip (library signatures for Turbo Pascal compilers, 1994)
- dcclibs.dat (prototype file for C headers, 1994)
- test.zip (sample test files: *.c *.exe *.b, 1993-1996)
- makedsig.zip (creates a .sig file from a .lib C file, 1994)
- makedstp.zip (creates a .sig file from a Pascal library file, 1994)
- readsig.zip (reads signatures in a .sig file, 1994)
- dispsrch.zip (displays the name of a function given a signature, 1994)
- parsehdr.zip (generates a prototype file (dcclibs.dat) from C *.h files, 1994)

Note that the dcc_oo.exe program (in dcc32.zip) is a 32 bit program,
so it won't work under Windows 3.1. Also, it is a console mode program,
meaning that it has to be run in the "Command Prompt" window (sometimes
known as the "Dos Box"). It is not a GUI program.

The following files are included in the test.zip file:  fibo,
benchsho, benchlng, benchfn, benchmul, byteops, intops, longops,
max, testlong, matrixmu, strlen, dhamp.
The version of dcc included in this distribution (dccsrcoo.zip and
dcc32.exe) is a bit better than the first release, but it is still
broken in some cases, and we do not have the time to work in this
project at present so we cannot provide any changes.
Comments on individual files:
- fibo (fibonacci): the small model (fibos.exe) decompiles correctly,
  the large model (fibol.exe) expects an extra argument for scanf().
  This argument is the segment and is not displayed.
- benchsho: the first scanf() takes loc0 as an argument.  This is
  part of a long variable, but dcc does not have any clue at that
  stage that the stack offset pushed on the stack is to be used
  as a long variable rather than an integer variable.
- benchlng: as part of the main() code, LO(loc1) | HI(loc1) should
  be displayed instead of loc3 | loc9.  These two integer variables
  are equivalent to the one long loc1 variable.
- benchfn: see benchsho.
- benchmul: see benchsho.
- byteops: decompiles correctly.
- intops: the du analysis for DIV and MOD is broken.  dcc currently
  generates code for a long and an integer temporary register that
  were used as part of the analysis.
- longops: decompiles correctly.
- max: decompiles correctly.
- testlong: this example decompiles correctly given the algorithms
  implemented in dcc.  However, it shows that when long variables
  are defined and used as integers (or long) without giving dcc
  any hint that this is happening, the variable will be treated as
  two integer variables.  This is due to the fact that the assembly
  code is in terms of integer registers, and long registers are not
  available in 80286, so a long variable is equivalent to two integer
  registers.  dcc only knows of this through idioms such as add two
  long variables.
- matrixmu: decompiles correctly.  Shows that arrays are not supported
  in dcc.
- strlen: decompiles correctly.  Shows that pointers are partially
  supported by dcc.
- dhamp: this program has far more data types than what dcc recognizes
  at present.

Our thanks to Gary Shaffstall for some debugging work.  Current bugs
are:
- if the code generated in the one line is too long, the (static)
  buffer used for that line is clobbered.  Solution: make the buffer
  larger (currently 200 chars).
- the large memory model problem & scanf()
- dcc's error message shows a p option available which doesn't
  exist, and doesn't show an i option which exists.

<del>- there is a nasty problem whereby some arrays can get reallocated to
  a new address, and some pointers can become invalid. This mainly
  tends to happen to larger executable files. A major rewrite will
  probably be required to fix this.</del>

For more information refer to the thesis "Reverse Compilation
Techniques" by Cristina Cifuentes, Queensland University of
Technology, 1994, and the dcc home page:
	http://www.it.uq.edu.au/groups/csm/dcc_readme.html

Please note that the executable version of dcc provided in this
distribution does not necessarily match the source code provided,
some changes were done without us keeping track of every change.

Using dcc

Here is a very brief summary of switches for dcc: 

*   a1, a2: assembler output, before and after re-ordering of input code 
*   c: Attempt to follow control through indirect call instructions 
*   i: Enter interactive disassembler 
*   m: Memory map 
*   s: Statistics summary 
*   v, V: verbose (and Very verbose) 
*   o filename: Use filename as assembler output file 

If dcc encounters illegal instructions, it will attempt to enter the so called
interactive disassembler. The idea of this was to allow commands to fix the
problem so that dcc could continue, but no such changes are implemented
as yet. (Note: the Unix versions do not have the interactive disassembler). If
you get into this, you can get out of it by pressing ^X (control-X). Once dcc
has entered the interactive disassembler, however, there is little chance that
it will recover and produce useful output.

If dcc loads the signature file dccxxx.sig, this means that it has not
recognised the compiler library used. You can place the signatures in a
different direcory to where you are working if you set the DCC environment
variable to point to their path. Note that if dcc can't find its signature files, it
will be severely handicapped. 
