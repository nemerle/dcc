/*
 * Code to check for library functions. If found, replaces procNNNN with the
 * library function name. Also checks startup code for correct DS, and the
 * address of main()
 * (C) Mike van Emmerik
*/
#include "chklib.h"

#include "Enums.h"
#include "dcc.h"
#include "msvc_fixes.h"
#include "project.h"
#include "perfhlib.h"
#include "dcc_interface.h"

#include <QtCore/QDir>
#include <QtCore/QString>
#include <QtCore/QDebug>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <string.h>
#include "Command.h"

#define  NIL   -1                   /* Used like NULL, but 0 is valid */

/* Hash table structure */
struct HT
{
    char    htSym[SYMLEN];
    uint8_t htPat[PATLEN];
};

/* Structure of the prototypes table. Same as the struct in parsehdr.h,
    except here we don't need the "next" index (the elements are already
    sorted by function name) */
struct PH_FUNC_STRUCT
{
    char    name[SYMLEN];               /* Name of function or arg */
    hlType  typ;                        /* Return type */
    int     numArg;                     /* Number of args */
    int     firstArg;                   /* Index of first arg in chain */
    //  int     next;                       /* Index of next function in chain */
    bool    bVararg;                    /* True if variable arguements */
} ;


#define NUM_PLIST   64              	/* Number of entries to increase allocation by */

/* statics */
static char buf[100];          				/* A general purpose buffer */

#define DCCLIBS "dcclibs.dat"           /* Name of the prototypes data file */

/* prototypes */
static bool grab(int n, QFile & _file);
static uint16_t readFileShort(FILE *_file);
static void readFileSection(uint16_t* p, int len, QFile & _file);
static void cleanup(void);
static void readProtoFile(void);
static int  searchPList(char *name);
static void checkHeap(char *msg);              /* For debugging */

void fixWildCards(uint8_t pat[]);			/* In fixwild.c */

static bool locatePattern(const uint8_t *source, int iMin, int iMax, uint8_t *pattern,
                           int iPatLen, int *index);

/*  *   *   *   *   *   *   *   *   *   *   *   *   *   *   *\
*                                                            *
*   S t a r t   P a t t e r n s   ( V e n d o r    i d )     *
*                                                            *
\*  *   *   *   *   *   *   *   *   *   *   *   *   *   *   */
static uint8_t pattMsC5Start[] =
{
    0xB4, 0x30,         /* Mov ah, 30 */
    0xCD, 0x21,         /* int 21 (dos version number) */
    0x3C, 0x02,         /* cmp al, 2 */
    0x73, 0x02,         /* jnb $+4 */
    0xCD, 0x20,         /* int 20 (exit) */
    0xBF                /* Mov di, DSEG */
};
static uint8_t pattMsC8Start[] =
{
    0xB4, 0x30,         /* Mov ah, 30 */
    0xCD, 0x21,         /* int 21 */
    0x3C, 0x02,         /* cmp al,2 */
    0x73, 0x05,         /* jnb $+7 */
    0x33, 0xC0,         /* xor ax, ax */
    0x06, 0x50,         /* push es:ax */
    0xCB,               /* retf */
    0xBF                /* mov di, DSEG */
};
static uint8_t pattMsC8ComStart[] =
{
    0xB4, 0x30,         /* Mov ah, 30 */
    0xCD, 0x21,         /* int 21 (dos version number) */
    0x3C, 0x02,         /* cmp al, 2 */
    0x73, 0x01,         /* jnb $+3 */
    0xC3,               /* ret */
    0x8C, 0xDF          /* Mov di, ds */
};
static uint8_t pattBorl2Start[] =
{
    0xBA, WILD, WILD,       /* Mov dx, dseg */
    0x2E, 0x89, 0x16,       /* mov cs:[], dx */
    WILD, WILD,
    0xB4, 0x30,             /* mov ah, 30 */
    0xCD, 0x21,             /* int 21 (dos version number) */
    0x8B, 0x2E, 0x02, 0,    /* mov bp, [2] */
    0x8B, 0x1E, 0x2C, 0,    /* mov bx, [2C] */
    0x8E, 0xDA,             /* mov ds, dx */
    0xA3, WILD, WILD,       /* mov [xx], ax */
    0x8C, 0x06, WILD, WILD, /* mov [xx], es */
    0x89, 0x1E, WILD, WILD, /* mov [xx], bx */
    0x89, 0x2E, WILD, WILD, /* mov [xx], bp */
    0xC7                    /* mov [xx], -1 */
};
static uint8_t pattBorl3Start[] =
{
    0xBA, WILD, WILD,   	/* Mov dx, dseg */
    0x2E, 0x89, 0x16,   	/* mov cs:[], dx */
    WILD, WILD,
    0xB4, 0x30,         	/* mov ah, 30 */
    0xCD, 0x21,         	/* int 21 (dos version number) */
    0x8B, 0x2E, 0x02, 0,	/* mov bp, [2] */
    0x8B, 0x1E, 0x2C, 0,	/* mov bx, [2C] */
    0x8E, 0xDA,         	/* mov ds, dx */
    0xA3, WILD, WILD,       /* mov [xx], ax */
    0x8C, 0x06, WILD, WILD, /* mov [xx], es */
    0x89, 0x1E, WILD, WILD, /* mov [xx], bx */
    0x89, 0x2E, WILD, WILD, /* mov [xx], bp */
    0xE8                    /* call ... */
};

static uint8_t pattBorl4on[] =
{
    0x9A, 0, 0, WILD, WILD	/* Call init (offset always 0) */
};

static uint8_t pattBorl4Init[] =
{
    0xBA, WILD, WILD,		/* Mov dx, dseg */
    0x8E, 0xDA,         	/* mov ds, dx */
    0x8C, 0x06, WILD, WILD, /* mov [xx], es */
    0x8B, 0xC4,				/* mov ax, sp */
    0x05, 0x13, 0,			/* add ax, 13h */
    0xB1, 0x04,				/* mov cl, 4 */
    0xD3, 0xE8,				/* shr ax, cl */
    0x8C, 0xD2				/* mov dx, ss */
};

static uint8_t pattBorl5Init[] =
{
    0xBA, WILD, WILD,		/* Mov dx, dseg */
    0x8E, 0xDA,         	/* mov ds, dx */
    0x8C, 0x06, 0x30, 0,	/* mov [0030], es */
    0x33, 0xED,				/* xor bp, bp <----- */
    0x8B, 0xC4,				/* mov ax, sp */
    0x05, 0x13, 0,			/* add ax, 13h */
    0xB1, 0x04,				/* mov cl, 4 */
    0xD3, 0xE8,				/* shr ax, cl */
    0x8C, 0xD2				/* mov dx, ss */
};

static uint8_t pattBorl7Init[] =
{
    0xBA, WILD, WILD,		/* Mov dx, dseg */
    0x8E, 0xDA,         	/* mov ds, dx */
    0x8C, 0x06, 0x30, 0,	/* mov [0030], es */
    0xE8, WILD, WILD,		/* call xxxx */
    0xE8, WILD, WILD,		/* call xxxx... offset always 00A0? */
    0x8B, 0xC4,				/* mov ax, sp */
    0x05, 0x13, 0,			/* add ax, 13h */
    0xB1, 0x04,				/* mov cl, 4 */
    0xD3, 0xE8,				/* shr ax, cl */
    0x8C, 0xD2				/* mov dx, ss */
};


static uint8_t pattLogiStart[] =
{
    0xEB, 0x04,         /* jmp short $+6 */
    WILD, WILD,         /* Don't know what this is */
    WILD, WILD,         /* Don't know what this is */
    0xB8, WILD, WILD,   /* mov ax, dseg */
    0x8E, 0xD8          /* mov ds, ax */
};

static uint8_t pattTPasStart[] =
{
    0xE9, 0x79, 0x2C    /* Jmp 2D7C - Turbo pascal 3.0 */
};



/*  *   *   *   *   *   *   *   *   *   *   *   *   *   *   *\
*                                                            *
*       M a i n   P a t t e r n s   ( M o d e l    i d )     *
*                                                            *
\*  *   *   *   *   *   *   *   *   *   *   *   *   *   *   */


/* This pattern works for MS and Borland, small and tiny model */
static uint8_t pattMainSmall[] =
{
    0xFF, 0x36, WILD, WILD,                 /* Push environment pointer */
    0xFF, 0x36, WILD, WILD,                 /* Push argv */
    0xFF, 0x36, WILD, WILD,                 /* Push argc */
    0xE8, WILD, WILD						/* call _main */
    //  0x50,                                   /* push ax... not in Borland V3 */
    //  0xE8                                    /* call _exit */
};
/* Num bytes from start pattern to the relative offset of main() */
#define OFFMAINSMALL 13

/* This pattern works for MS and Borland, medium model */
static uint8_t pattMainMedium[] =
{
    0xFF, 0x36, WILD, WILD,                 /* Push environment pointer */
    0xFF, 0x36, WILD, WILD,                 /* Push argv */
    0xFF, 0x36, WILD, WILD,                 /* Push argc */
    0x9A, WILD, WILD, WILD, WILD            /* call far _main */
    //  0x50                                /* push ax */
    //  0x0E,                               /* push cs NB not tested Borland */
    //  0xE8                                /* call _exit */
};
/* Num bytes from start pattern to the relative offset of main() */
#define OFFMAINMEDIUM 13

/* This pattern works for MS and Borland, compact model */
static uint8_t pattMainCompact[] =
{
    0xFF, 0x36, WILD, WILD,                 /* Push environment pointer lo */
    0xFF, 0x36, WILD, WILD,                 /* Push environment pointer hi */
    0xFF, 0x36, WILD, WILD,                 /* Push argv lo */
    0xFF, 0x36, WILD, WILD,                 /* Push argv hi */
    0xFF, 0x36, WILD, WILD,                 /* Push argc */
    0xE8, WILD, WILD,                       /* call _main */
    //  0x50,                                   /* push ax */
    //  0xE8                                    /* call _exit */
};
/* Num bytes from start pattern to the relative offset of main() */
#define OFFMAINCOMPACT 21

/* This pattern works for MS and Borland, large model */
static uint8_t pattMainLarge[] =
{
    0xFF, 0x36, WILD, WILD,                 /* Push environment pointer lo */
    0xFF, 0x36, WILD, WILD,                 /* Push environment pointer hi */
    0xFF, 0x36, WILD, WILD,                 /* Push argv lo */
    0xFF, 0x36, WILD, WILD,                 /* Push argv hi */
    0xFF, 0x36, WILD, WILD,                 /* Push argc */
    0x9A, WILD, WILD, WILD, WILD            /* call far _main */
    //  0x50                                    /* push ax */
    //  0x0E,                                   /* push cs */
    //  0xE8                                    /* call _exit */
};
/* Num bytes from start pattern to the relative offset of main() */
#define OFFMAINLARGE 21


/*  *   *   *   *   *   *   *   *   *   *   *   *   *   *   *\
*                                                            *
*       	M i s c e l l a n e o u s    P a t t e r n s	 *
*                                                            *
\*  *   *   *   *   *   *   *   *   *   *   *   *   *   *   */

/* This pattern is for the stack check code in Microsoft compilers */
static uint8_t pattMsChkstk[] =
{
    0x59,					/* pop cx		*/
    0x8B, 0xDC,          	/* mov bx, sp	*/
    0x2B, 0xD8,				/* sub bx, ax	*/
    0x72, 0x0A,				/* jb bad		*/
    0x3B, 0x1E, WILD, WILD,	/* cmp bx, XXXX */
    0x72, 0x04,				/* jb bad		*/
    0x8B, 0xE3,				/* mov sp, bx	*/
    0xFF, 0xE1,				/* jmp [cx]		*/
    0x33, 0xC0,				/* xor ax, ax	*/
    0xE9					/* jmp XXXX		*/
};

/* Check this function to see if it is a library function. Return true if
    it is, and copy its name to pProc->name
*/
bool PatternLocator::LibCheck(Function & pProc)
{
    PROG &prog(Project::get()->prog);
    long fileOffset;
    int h, i, j, arg;
    int Idx;
    uint8_t pat[PATLEN];

    fileOffset = pProc.procEntry;              /* Offset into the image */
    if (fileOffset == prog.offMain)
    {
        /* Easy - this function is called main! */
        pProc.name = "main";
        return false;
    }
    if(fileOffset + PATLEN > prog.cbImage)
        return false;
    memcpy(pat, &prog.image()[fileOffset], PATLEN);
    //memmove(pat, &prog.image()[fileOffset], PATLEN);
    fixWildCards(pat);                  /* Fix wild cards in the copy */
    h = g_pattern_hasher.hash(pat);                      /* Hash the found proc */
    /* We always have to compare keys, because the hash function will always return a valid index */
    if (memcmp(ht[h].htPat, pat, PATLEN) == 0)
    {
        /* We have a match. Save the name, if not already set */
        if (pProc.name.isEmpty() )     /* Don't overwrite existing name */
        {
            /* Give proc the new name */
            pProc.name = ht[h].htSym;
        }
        /* But is it a real library function? */
        i = NIL;
        if ((numFunc == 0) or (i=searchPList(ht[h].htSym)) != NIL)
        {
            pProc.flg |= PROC_ISLIB; 		/* It's a lib function */
            pProc.callingConv(CConv::C);
            if (i != NIL)
            {
                PH_FUNC_STRUCT &phfunc(pFunc[i]);
                /* Allocate space for the arg struct, and copy the hlType to
                    the appropriate field */
                arg = phfunc.firstArg;
                pProc.args.numArgs = phfunc.numArg;
                pProc.args.resize(phfunc.numArg);
                pProc.getFunctionType()->clearArguments();
                for (j=0; j < phfunc.numArg; j++)
                {
                    pProc.getFunctionType()->addArgument(pArg[arg]);
                    pProc.args[j].type = pArg[arg++];
                }
                if (phfunc.typ != TYPE_UNKNOWN)
                {
                    pProc.type->setReturnType(phfunc.typ);
                    switch (pProc.getReturnType()) {
                    case TYPE_LONG_SIGN:
                    case TYPE_LONG_UNSIGN:
                            pProc.liveOut.setReg(rDX).addReg(rAX);
                            break;
                    case TYPE_WORD_SIGN:
                    case TYPE_WORD_UNSIGN:
                            pProc.liveOut.setReg(rAX);
                            break;
                    case TYPE_BYTE_SIGN:
                    case TYPE_BYTE_UNSIGN:
                            pProc.liveOut.setReg(rAL);
                            break;
                        case TYPE_STR:
                        case TYPE_PTR:
                            fprintf(stderr,"Warning assuming Large memory model\n");
                            pProc.liveOut.setReg(rAX).addReg(rDS);
                            break;
                        default:
                            qCritical() << QString("Unknown retval type %1 for %2 in LibCheck")
                                       .arg(pProc.getReturnType()).arg(pProc.name);
                            /*** other types are not considered yet ***/
                    }
                }
                pProc.getFunctionType()->m_call_conv->calculateStackLayout(&pProc);
                pProc.getFunctionType()->m_vararg = pFunc[i].bVararg;
            }
        }
        else if (i == NIL)
        {
            /* Have a symbol for it, but does not appear in a header file.
                Treat it as if it is not a library function */
            pProc.flg |= PROC_RUNTIME;		/* => is a runtime routine */
        }
    }
    if (locatePattern(prog.image(), pProc.procEntry,
                      pProc.procEntry+sizeof(pattMsChkstk),
                      pattMsChkstk, sizeof(pattMsChkstk), &Idx))
    {
        /* Found _chkstk */
        pProc.name = "chkstk";
        pProc.flg |= PROC_ISLIB; 		/* We'll say its a lib function */
        pProc.args.numArgs = 0;		/* With no args */
    }

    return (bool)((pProc.flg & PROC_ISLIB) != 0);
}



bool grab(int n, QFile &_file)
{

    if (_file.read(buf,n) != n)
    {
        qCritical() << "File read failed";
        return false;
    }
    return true;
}

uint16_t readFileShort(QFile &f)
{
    uint16_t tgt;
    if(sizeof(tgt)!=f.read((char *)&tgt,sizeof(tgt))) {
        qCritical() << "File read failed";
        return false;
    }
    return tgt;
}

// Read a section of the file, considering endian issues
static void readFileSection(uint16_t* p, int len, QFile & f)
{
    for (int i=0; i < len; i += 2)
    {
        *p++ = readFileShort(f);
    }
}

/* Search the source array between limits iMin and iMax for the pattern (length
    iPatLen). The pattern can contain wild bytes; if you really want to match
    for the pattern that is used up by the WILD uint8_t, tough - it will match with
    everything else as well. */
static bool locatePattern(const uint8_t *source, int iMin, int iMax, uint8_t *pattern, int iPatLen, int *index)
{
    int i, j;
    const uint8_t *pSrc;                             /* Pointer to start of considered source */
    int iLast;

    iLast = iMax - iPatLen;                 /* Last source uint8_t to consider */

    for (i=iMin; i <= iLast; i++)
    {
        pSrc = &source[i];                  /* Start of current part of source */
        /* i is the index of the start of the moving pattern */
        for (j=0; j < iPatLen; j++)
        {
            /* j is the index of the uint8_t being considered in the pattern. */
            if ((*pSrc != pattern[j]) and (pattern[j] != WILD))
            {
                /* A definite mismatch */
                break;                      /* Break to outer loop */
            }
            pSrc++;
        }
        if (j >= iPatLen)
        {
            /* Pattern has been found */
            *index = i;                     /* Pass start of pattern */
            return 1;                       /* Indicate success */
        }
        /* Else just try next value of i */
    }
    /* Pattern was not found */
    *index = -1;                            /* Invalidate index */
    return 0;                               /* Indicate failure */
}


/**
 * This function checks the startup code for various compilers' way of
 * loading DS. If found, it sets DS. This may not be needed in the future if
 * pushing and popping of registers is implemented.
 * Also sets prog.offMain and prog.segMain if possible
 */
void checkStartup(STATE &state)
{
    Project &   proj(*Project::get());
    PROG &      prog(Project::get()->prog);


    int startOff;       /* Offset into the Image of the initial CS:IP */
    int i, rel, para, init;

    startOff = ((uint32_t)prog.initCS << 4) + prog.initIP;

    /* Check the Turbo Pascal signatures first, since they involve only the
                first 3 bytes, and false positives may be found with the others later */
    if (locatePattern(prog.image(), startOff, startOff+5, pattBorl4on,sizeof(pattBorl4on), &i))
    {
        /* The first 5 bytes are a far call. Follow that call and
                        determine the version from that */
        rel = LH(&prog.image()[startOff+1]);  	 /* This is abs off of init */
        para= LH(&prog.image()[startOff+3]);/* This is abs seg of init */
        init = ((uint32_t)para << 4) + rel;
        if (locatePattern(prog.image(), init, init+26, pattBorl4Init, sizeof(pattBorl4Init), &i))
        {

            state.setState(rDS, LH(&prog.image()[i+1]));
            printf("Borland Turbo Pascal v4 detected\n");
            proj.setLoaderMetadata({eBorland,ePascal,eUnknownMemoryModel,4});
            prog.offMain = startOff;            /* Code starts immediately */
            prog.segMain = prog.initCS;			/* At the 5 uint8_t jump */
            return;                     /* Already have vendor */
        }
        else if (locatePattern(prog.image(), init, init+26, pattBorl5Init, sizeof(pattBorl5Init), &i))
        {

            state.setState( rDS, LH(&prog.image()[i+1]));
            printf("Borland Turbo Pascal v5.0 detected\n");
            proj.setLoaderMetadata({eBorland,ePascal,eUnknownMemoryModel,5});
            prog.offMain = startOff;            /* Code starts immediately */
            prog.segMain = prog.initCS;
            return;                     /* Already have vendor */
        }
        else if (locatePattern(prog.image(), init, init+26, pattBorl7Init,
                               sizeof(pattBorl7Init), &i))
        {

            state.setState( rDS, LH(&prog.image()[i+1]));
            printf("Borland Pascal v7 detected\n");
            proj.setLoaderMetadata({eBorland,ePascal,eUnknownMemoryModel,7});
            prog.offMain = startOff;            /* Code starts immediately */
            prog.segMain = prog.initCS;
            return;                     /* Already have vendor */
        }
    }


    LoaderMetadata metadata;
    /* Search for the call to main pattern. This is compiler independant,
        but decides the model required. Note: must do the far data models
        (large and compact) before the others, since they are the same pattern
        as near data, just more pushes at the start. */
    if(prog.cbImage>int(startOff+0x180+sizeof(pattMainLarge)))
    {
        if (locatePattern(prog.image(), startOff, startOff+0x180, pattMainLarge,sizeof(pattMainLarge), &i))
        {
            rel = LH(&prog.image()[i+OFFMAINLARGE]);  /* This is abs off of main */
            para= LH(&prog.image()[i+OFFMAINLARGE+2]);/* This is abs seg of main */
            /* Save absolute image offset */
            prog.offMain = ((uint32_t)para << 4) + rel;
            prog.segMain = (uint16_t)para;
            metadata.compiler_memory_model = eLarge;
        }
        else if (locatePattern(prog.image(), startOff, startOff+0x180, pattMainCompact,
                               sizeof(pattMainCompact), &i))
        {
            rel = LH_SIGNED(&prog.image()[i+OFFMAINCOMPACT]);/* This is the rel addr of main */
            prog.offMain = i+OFFMAINCOMPACT+2+rel;  /* Save absolute image offset */
            prog.segMain = prog.initCS;
            metadata.compiler_memory_model = eCompact;
        }
        else if (locatePattern(prog.image(), startOff, startOff+0x180, pattMainMedium,
                               sizeof(pattMainMedium), &i))
        {
            rel = LH(&prog.image()[i+OFFMAINMEDIUM]);  /* This is abs off of main */
            para= LH(&prog.image()[i+OFFMAINMEDIUM+2]);/* This is abs seg of main */
            prog.offMain = ((uint32_t)para << 4) + rel;
            prog.segMain = (uint16_t)para;
            metadata.compiler_memory_model = eMedium;
        }
        else if (locatePattern(prog.image(), startOff, startOff+0x180, pattMainSmall,
                               sizeof(pattMainSmall), &i))
        {
            rel = LH_SIGNED(&prog.image()[i+OFFMAINSMALL]); /* This is rel addr of main */
            prog.offMain = i+OFFMAINSMALL+2+rel;    /* Save absolute image offset */
            prog.segMain = prog.initCS;
            metadata.compiler_memory_model = eSmall;
        }
        else if (memcmp(&prog.image()[startOff], pattTPasStart, sizeof(pattTPasStart)) == 0)
        {
            rel = LH_SIGNED(&prog.image()[startOff+1]);     /* Get the jump offset */
            prog.offMain = rel+startOff+3;          /* Save absolute image offset */
            prog.offMain += 0x20;                   /* These first 32 bytes are setting up */
            prog.segMain = prog.initCS;
            proj.setLoaderMetadata({eBorland,ePascal,eUnknownMemoryModel,3});
            printf("Turbo Pascal 3.0 detected\n");
            printf("Main at %04X\n", prog.offMain);
            return;                         /* Already have vendor */
        }
        else
        {
            printf("Main could not be located!\n");
            prog.offMain = -1;
        }
    }
    else
    {
        printf("Main could not be located!\n");
        prog.offMain = -1;
    }
    //printf("Model: %c\n", chModel);

    /* Now decide the compiler vendor and version number */
    if (memcmp(&prog.image()[startOff], pattMsC5Start, sizeof(pattMsC5Start)) == 0)
    {
        /* Yes, this is Microsoft startup code. The DS is sitting right here
            in the next 2 bytes */
        state.setState( rDS, LH(&prog.image()[startOff+sizeof(pattMsC5Start)]));
        metadata.compiler_vendor    = eMicrosoft;
        metadata.compiler_language   = eAnsiCorCPP;
        metadata.compiler_version   = 5;
        printf("MSC 5 detected\n");
    }

    /* The C8 startup pattern is different from C5's */
    else if (memcmp(&prog.image()[startOff], pattMsC8Start, sizeof(pattMsC8Start)) == 0)
    {
        state.setState( rDS, LH(&prog.image()[startOff+sizeof(pattMsC8Start)]));
        printf("MSC 8 detected\n");
        metadata.compiler_vendor    = eMicrosoft;
        metadata.compiler_language   = eAnsiCorCPP;
        metadata.compiler_version   = 8;
    }

    /* The C8 .com startup pattern is different again! */
    else if (memcmp(&prog.image()[startOff], pattMsC8ComStart,
                    sizeof(pattMsC8ComStart)) == 0)
    {
        printf("MSC 8 .com detected\n");
        metadata.compiler_vendor    = eMicrosoft;
        metadata.compiler_language   = eAnsiCorCPP;
        metadata.compiler_version   = 8;
    }
    else if (locatePattern(prog.image(), startOff, startOff+0x30, pattBorl2Start, sizeof(pattBorl2Start), &i))
    {
        /* Borland startup. DS is at the second uint8_t (offset 1) */
        state.setState( rDS, LH(&prog.image()[i+1]));
        printf("Borland v2 detected\n");
        metadata.compiler_vendor    = eBorland;
        metadata.compiler_language  = eAnsiCorCPP;
        metadata.compiler_version   = 2;
    }
    else if (locatePattern(prog.image(), startOff, startOff+0x30, pattBorl3Start, sizeof(pattBorl3Start), &i))
    {
        /* Borland startup. DS is at the second uint8_t (offset 1) */
        state.setState( rDS, LH(&prog.image()[i+1]));
        printf("Borland v3 detected\n");
        metadata.compiler_vendor    = eBorland;
        metadata.compiler_language  = eAnsiCorCPP;
        metadata.compiler_version   = 3;
    }
    else if (locatePattern(prog.image(), startOff, startOff+0x30, pattLogiStart, sizeof(pattLogiStart), &i))
    {
        /* Logitech modula startup. DS is 0, despite appearances */
        printf("Logitech modula detected\n");
        metadata.compiler_vendor    = eLogitech;
        metadata.compiler_language  = eModula2;
        metadata.compiler_version   = 1;
    }

    /* Other startup idioms would go here */
    else
    {
        printf("Warning - compiler not recognised\n");
    }
    proj.setLoaderMetadata(metadata);
}

/* DCCLIBS.DAT is a data file sorted on function name containing names and
    return types of functions found in include files, and the names and types
    of arguements. Only functions in this list will be considered library
    functions; others (like LXMUL@) are helper files, and need to be analysed
    by dcc, rather than considered as known functions. When a prototype is
    found (in searchPList()), the parameter info is written to the proc struct.
*/
bool PatternLocator::readProtoFile(void)
{
    IDcc *dcc = IDcc::get();
    QString szProFName = dcc->dataDir("prototypes").absoluteFilePath(DCCLIBS); /* Full name of dclibs.lst */

    QFile fProto(szProFName);

    if (not fProto.open(QFile::ReadOnly))
    {
        printf("Warning: cannot open library prototype data file %s\n", qPrintable(szProFName));
        return false;
    }

    grab(4, fProto);
    if (strncmp(buf, "dccp", 4) != 0)
    {
        printf("%s is not a dcc prototype file\n", qPrintable(szProFName));
        return false;
    }

    grab(2, fProto);
    if (strncmp(buf, "FN", 2) != 0)
    {
        printf("FN (Function Name) subsection expected in %s\n", qPrintable(szProFName));
        return false;
    }

    numFunc = readFileShort(fProto);     /* Num of entries to allocate */

    /* Allocate exactly correct # entries */
    pFunc = new PH_FUNC_STRUCT[numFunc];

    for (int i=0; i < numFunc; i++)
    {
        size_t read_size=fProto.read((char *)&pFunc[i], SYMLEN);
        assert(read_size==SYMLEN);
        if(read_size!=SYMLEN)
            break;
        pFunc[i].typ      = (hlType)readFileShort(fProto);
        pFunc[i].numArg   = readFileShort(fProto);
        pFunc[i].firstArg = readFileShort(fProto);
        if(fProto.atEnd())
            break;
        char isvararg;
        fProto.read(&isvararg,1);
        pFunc[i].bVararg = (isvararg!=0);
    }

    grab(2, fProto);
    if (strncmp(buf, "PM", 2) != 0)
    {
        printf("PM (Parameter) subsection expected in %s\n", qPrintable(szProFName));
        return false;
    }

    numArg = readFileShort(fProto);     /* Num of entries to allocate */

    /* Allocate exactly correct # entries */
    pArg.clear();
    pArg.reserve(numArg);

    for (int i=0; i < numArg; i++)
    {
        //      fread(&pArg[i], 1, SYMLEN, fProto);     /* No names to read as yet */
        pArg.push_back((hlType) readFileShort(fProto));
    }
}

int PatternLocator::searchPList(const char *name)
{
    /* Search through the symbol names for the name */
    /* Use binary search */
    int mx, mn, i, res;

    mx = numFunc;
    mn = 0;

    while (mn < mx)
    {
        i = (mn + mx) /2;
        res = strcmp(pFunc[i].name, name);
        if (res == 0)
            return i;            /* Found! */

        if (res < 0)
            mn = i+1;
        else
            mx = i-1;
    }

    /* Still could be the case that mn == mx == required record */
    res = strcmp(pFunc[mn].name, name);
    if (res == 0)
        return mn;            /* Found! */

    return NIL;
}
/* This procedure is called to initialise the library check code */
bool PatternLocator::load()
{
    uint16_t w, len;
    int i;
    IDcc *dcc = IDcc::get();
    QString fpath = dcc->dataDir("sigs").absoluteFilePath(pattern_id);
    QFile g_file(fpath);
    if (not g_file.open(QFile::ReadOnly))
    {
        printf("Warning: cannot open signature file %s\n", qPrintable(fpath));
        return false;
    }

    readProtoFile();


    /* Read the parameters */
    grab(4, g_file);
    if (memcmp("dccs", buf, 4) != 0)
    {
        printf("Not a dcc signature file!\n");
        return false;
    }
    numKeys = readFileShort(g_file);
    numVert = readFileShort(g_file);
    PatLen  = readFileShort(g_file);
    SymLen  = readFileShort(g_file);
    if ((PatLen != PATLEN) or (SymLen != SYMLEN))
    {
        printf("Sorry! Compiled for sym and pattern lengths of %d and %d\n", SYMLEN, PATLEN);
        return false;
    }

    /* Initialise the perfhlib stuff. Also allocates T1, T2, g, etc */
    /* Set the parameters for the hash table */
    g_pattern_hasher.setHashParams(
                    numKeys,                /* The number of symbols */
                    PatLen,                 /* The length of the pattern to be hashed */
                    256,                    /* The character set of the pattern (0-FF) */
                    0,                      /* Minimum pattern character value */
                    numVert);               /* Specifies c, the sparseness of the graph. See Czech, Havas and Majewski for details */
    T1base  = g_pattern_hasher.readT1();
    T2base  = g_pattern_hasher.readT2();
    g       = g_pattern_hasher.readG();

    /* Read T1 and T2 tables */
    grab(2, g_file);
    if (memcmp("T1", buf, 2) != 0)
    {
        printf("Expected 'T1'\n");
        exit(3);
    }
    len = (uint16_t) (PatLen * 256 * sizeof(uint16_t));
    w = readFileShort(g_file);
    if (w != len)
    {
        printf("Problem with size of T1: file %d, calc %d\n", w, len);
        return false;
    }
    readFileSection(T1base, len, g_file);

    grab(2, g_file);
    if (memcmp("T2", buf, 2) != 0)
    {
        printf("Expected 'T2'\n");
        return false;
    }
    w = readFileShort(g_file);
    if (w != len)
    {
        printf("Problem with size of T2: file %d, calc %d\n", w, len);
        return false;
    }
    readFileSection(T2base, len, g_file);

    /* Now read the function g[] */
    grab(2, g_file);
    if (memcmp("gg", buf, 2) != 0)
    {
        printf("Expected 'gg'\n");
        return false;
    }
    len = (uint16_t)(numVert * sizeof(uint16_t));
    w = readFileShort(g_file);
    if (w != len)
    {
        printf("Problem with size of g[]: file %d, calc %d\n", w, len);
        return false;
    }
    readFileSection(g, len, g_file);


    /* This is now the hash table */
    /* First allocate space for the table */
    ht = new HT[numKeys];
    if ( nullptr == ht)
    {
        printf("Could not allocate hash table\n");
        return false;
    }
    grab(2, g_file);
    if (memcmp("ht", buf, 2) != 0)
    {
        printf("Expected 'ht'\n");
        return false;
    }
    w = readFileShort(g_file);
    if (w != numKeys * (SymLen + PatLen + sizeof(uint16_t)))
    {
        printf("Problem with size of hash table: file %d, calc %d\n", w, len);
        return false;
    }


    for (i=0; i < numKeys; i++)
    {
        if (g_file.read((char *)&ht[i], SymLen + PatLen) != SymLen + PatLen)
        {
            printf("Could not read signature\n");
            return false;
        }
    }
    return true;
}
PatternLocator::~PatternLocator()
{
    /* Deallocate all the stuff allocated in SetupLibCheck() */
    delete [] ht;
    delete [] pFunc;
    delete [] T1base;
    delete [] T2base;
    delete [] g;
}

bool LoadPatternLibrary::execute(CommandContext * ctx)
{
    if(nullptr==ctx->m_project) {
        ctx->recordFailure(this,"Project was not created yet.");
        return false;
    }
    Project & project( *ctx->m_project) ;
    if(!project.m_metadata_available) {
        ctx->recordFailure(this,"Loader metadata was not set. FindMain command should be run before LoadPatternLibrary");
        return false;
    }
    PatternLocator *loc = new PatternLocator(project.getLoaderMetadata().compilerId());
    if(!loc->load()) {
        qDebug()<< " No library patterns found for" << project.getLoaderMetadata().compilerId();
        delete loc;
        return true; // not an error, just no pattern locator available
    }
    project.m_pattern_locator = loc;
    return true;
}
