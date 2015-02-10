/* Quick program to copy a named signature to a small file */

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <string.h>
#include "perfhlib.h"

/* statics */
byte buf[100];
int	numKeys;			/* Number of hash table entries (keys) */
int numVert;			/* Number of vertices in the graph (also size of g[]) */
int PatLen;				/* Size of the keys (pattern length) */
int SymLen;				/* Max size of the symbols, including null */
FILE *f;				/* File being read */
FILE *f2;				/* File being written */

static	word	*T1base, *T2base;	/* Pointers to start of T1, T2 */
static	word	*g;					/* g[] */

/* prototypes */
void grab(int n);
word readFileShort(void);
void cleanup(void);


#define SYMLEN      16
#define PATLEN      23

/* Hash table structure */
typedef struct HT_tag
{
    char    htSym[SYMLEN];
    byte    htPat[PATLEN];
} HT;

HT  ht;                 /* One hash table entry */

void
main(int argc, char *argv[])
{
	word w, len;
	int i;

	if (argc <= 3)
	{
		printf("Usage: dispsig <SigFilename> <FunctionName> <BinFileName>\n");
		printf("Example: dispsig dccm8s.sig printf printf.bin\n");
		exit(1);
	}

	if ((f = fopen(argv[1], "rb")) == NULL)
	{
		printf("Cannot open %s\n", argv[1]);
		exit(2);
	}

	if ((f2 = fopen(argv[3], "wb")) == NULL)
	{
		printf("Cannot write to %s\n", argv[3]);
		exit(2);
	}


	/* Read the parameters */
	grab(4);
	if (memcmp("dccs", buf, 4) != 0)
	{
		printf("Not a dccs file!\n");
		exit(3);
	}
	numKeys = readFileShort();
	numVert = readFileShort();
	PatLen = readFileShort();
	SymLen = readFileShort();

	/* Initialise the perfhlib stuff. Also allocates T1, T2, g, etc */
	hashParams(						/* Set the parameters for the hash table */
		numKeys,					/* The number of symbols */
		PatLen,						/* The length of the pattern to be hashed */
		256,						/* The character set of the pattern (0-FF) */
		0,							/* Minimum pattern character value */
		numVert);					/* Specifies C, the sparseness of the graph.
										See Czech, Havas and Majewski for details
									*/

	T1base	= readT1();
	T2base	= readT2();
	g		= readG();

	/* Read T1 and T2 tables */
	grab(2);
	if (memcmp("T1", buf, 2) != 0)
	{
		printf("Expected 'T1'\n");
		exit(3);
	}
	len = PatLen * 256 * sizeof(word);
	w = readFileShort();
	if (w != len)
	{
		printf("Problem with size of T1: file %d, calc %d\n", w, len);
		exit(4);
	}
	if (fread(T1base, 1, len, f) != len)
	{
		printf("Could not read T1\n");
		exit(5);
	}

	grab(2);
	if (memcmp("T2", buf, 2) != 0)
	{
		printf("Expected 'T2'\n");
		exit(3);
	}
	w = readFileShort();
	if (w != len)
	{
		printf("Problem with size of T2: file %d, calc %d\n", w, len);
		exit(4);
	}
	if (fread(T2base, 1, len, f) != len)
	{
		printf("Could not read T2\n");
		exit(5);
	}

	/* Now read the function g[] */
	grab(2);
	if (memcmp("gg", buf, 2) != 0)
	{
		printf("Expected 'gg'\n");
		exit(3);
	}
	len = numVert * sizeof(word);
	w = readFileShort();
	if (w != len)
	{
		printf("Problem with size of g[]: file %d, calc %d\n", w, len);
		exit(4);
	}
	if (fread(g, 1, len, f) != len)
	{
		printf("Could not read T2\n");
		exit(5);
	}


	/* This is now the hash table */
	grab(2);
	if (memcmp("ht", buf, 2) != 0)
	{
		printf("Expected 'ht'\n");
		exit(3);
	}
	w = readFileShort();
	if (w != numKeys * (SymLen + PatLen + sizeof(word)))
	{
		printf("Problem with size of hash table: file %d, calc %d\n", w, len);
		exit(6);
	}


	for (i=0; i < numKeys; i++)
	{
        if (fread(&ht, 1, SymLen + PatLen, f) != (size_t)(SymLen + PatLen))
        {
            printf("Could not read pattern %d from %s\n", i, argv[1]);
            exit(7);
        }
        if (stricmp(ht.htSym, argv[2]) == 0)
        {
            /* Found it! */
            break;
        }

	}
    fclose(f);
    if (i == numKeys)
    {
        printf("Function %s not found!\n", argv[2]);
        exit(2);
    }

    printf("Function %s index %d\n", ht.htSym, i);
    for (i=0; i < PatLen; i++)
    {
        printf("%02X ", ht.htPat[i]);
    }

    fwrite(ht.htPat, 1, PatLen, f2);
    fclose(f2);

    printf("\n");


}


void
cleanup(void)
{
	/* Free the storage for variable sized tables etc */
	if (T1base)	free(T1base);
	if (T2base)	free(T2base);
	if (g) free(g);
}

void grab(int n)
{
	if (fread(buf, 1, n, f) != (size_t)n)
	{
		printf("Could not read\n");
		exit(11);
	}
}

word
readFileShort(void)
{
	byte b1, b2;

	if (fread(&b1, 1, 1, f) != 1)
	{
		printf("Could not read\n");
		exit(11);
	}
	if (fread(&b2, 1, 1, f) != 1)
	{
		printf("Could not read\n");
		exit(11);
	}
	return (b2 << 8) + b1;
}

/* Following two functions not needed unless creating tables */

void getKey(int i, byte **keys)
{
}

/* Display key i */
void
dispKey(int i)
{
}

