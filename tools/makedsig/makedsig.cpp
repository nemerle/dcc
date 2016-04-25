/* Program for making the DCC signature file */

#include "LIB_PatternCollector.h"
#include "TPL_PatternCollector.h"
#include "perfhlib.h"		/* Symbol table prototypes */
#include "msvc_fixes.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QStringList>

#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <string.h>
#include <algorithm>

/* Symbol table constnts */
#define C 2.2 /* Sparseness of graph. See Czech, Havas and Majewski for details */

/* prototypes */

void saveFile(FILE *fl, const PerfectHash &p_hash, PatternCollector *coll);		/* Save the info */

int	 numKeys;				/* Number of useful codeview symbols */


static void printUsage(bool longusage) {
    if(longusage)
        printf(
                    "This program is to make 'signatures' of known c and tpl library calls for the dcc program.\n"
                    "It needs as the first arg the name of a library file, and as the second arg, the name "
                    "of the signature file to be generated.\n"
                    "Example: makedsig CL.LIB dccb3l.sig\n"
                    "      or makedsig turbo.tpl dcct4p.sig\n"
                    );
    else
        printf("Usage: makedsig <libname> <signame>\n"
               "or makedsig -h for help\n");
}
int main(int argc, char *argv[])
{
    QCoreApplication app(argc,argv);
    FILE *f2; // output file
    FILE *srcfile; // .lib file
    int s;
    if(app.arguments().size()<2) {
        printUsage(false);
        return 0;
    }
    QString arg2 = app.arguments()[1];
    if (arg2.startsWith("-h") or arg2.startsWith("-?"))
    {
        printUsage(true);
        return 0;
    }
    PatternCollector *collector;
    if(arg2.endsWith("tpl")) {
        collector = new TPL_PatternCollector;
    } else if(arg2.endsWith(".lib")) {
        collector = new LIB_PatternCollector;
    }
    if ((srcfile = fopen(argv[1], "rb")) == NULL)
    {
        printf("Cannot read %s\n", argv[1]);
        exit(2);
    }

    if ((f2 = fopen(argv[2], "wb")) == NULL)
    {
        printf("Cannot write %s\n", argv[2]);
        exit(2);
    }

    fprintf(stderr, "Seed: ");
    scanf("%d", &s);
    srand(s);

    PerfectHash p_hash;
    numKeys = collector->readSyms(srcfile);			/* Read the keys (symbols) */

    printf("Num keys: %d; vertices: %d\n", numKeys, (int)(numKeys*C));
    /* Set the parameters for the hash table */
    p_hash.setHashParams(   numKeys,					/* The number of symbols */
                PATLEN,						/* The length of the pattern to be hashed */
                256,						/* The character set of the pattern (0-FF) */
                0,							/* Minimum pattern character value */
                            numKeys*C);			/* C is the sparseness of the graph. See Czech,
                                        Havas and Majewski for details */

    /* The following two functions are in perfhlib.c */
    p_hash.map(collector);     /* Perform the mapping. This will call getKey() repeatedly */
    p_hash.assign();						/* Generate the function g */

    saveFile(f2,p_hash,collector);     /* Save the resultant information */

    fclose(srcfile);
    fclose(f2);

}

/*	*	*	*	*	*	*	*	*	*	*	*  *\
*												*
*		S a v e   t h e   s i g   f i l e		*
*												*
\*	*	*	*	*	*	*	*	*	*	*	*  */


void writeFile(FILE *fl,const char *buffer, int len)
{
    if ((int)fwrite(buffer, 1, len, fl) != len)
    {
        printf("Could not write to file\n");
        exit(1);
    }
}

void writeFileShort(FILE *fl,uint16_t w)
{
    uint8_t b;

    b = (uint8_t)(w & 0xFF);
    writeFile(fl,(char *)&b, 1);		/* Write a short little endian */
    b = (uint8_t)(w>>8);
    writeFile(fl,(char *)&b, 1);
}

void saveFile(FILE *fl, const PerfectHash &p_hash, PatternCollector *coll)
{
    int i, len;
    const uint16_t *pTable;

    writeFile(fl,"dccs", 4);					/* Signature */
    writeFileShort(fl,numKeys);				/* Number of keys */
    writeFileShort(fl,(short)(numKeys * C));	/* Number of vertices */
    writeFileShort(fl,PATLEN);					/* Length of key part of entries */
    writeFileShort(fl,SYMLEN);					/* Length of symbol part of entries */

    /* Write out the tables T1 and T2, with their sig and byte lengths in front */
    writeFile(fl,"T1", 2);						/* "Signature" */
    pTable = p_hash.readT1();
    len = PATLEN * 256;
    writeFileShort(fl,len * sizeof(uint16_t));
    for (i=0; i < len; i++)
    {
        writeFileShort(fl,pTable[i]);
    }
    writeFile(fl,"T2", 2);
    pTable = p_hash.readT2();
    writeFileShort(fl,len * sizeof(uint16_t));
    for (i=0; i < len; i++)
    {
        writeFileShort(fl,pTable[i]);
    }

    /* Write out g[] */
    writeFile(fl,"gg", 2);			  			/* "Signature" */
    pTable = p_hash.readG();
    len = (short)(numKeys * C);
    writeFileShort(fl,len * sizeof(uint16_t));
    for (i=0; i < len; i++)
    {
        writeFileShort(fl,pTable[i]);
    }

    /* Now the hash table itself */
    writeFile(fl,"ht ", 2);			  			/* "Signature" */
    writeFileShort(fl,numKeys * (SYMLEN + PATLEN + sizeof(uint16_t)));	/* byte len */
    for (i=0; i < numKeys; i++)
    {
        writeFile(fl,(char *)&coll->keys[i], SYMLEN + PATLEN);
    }
}



