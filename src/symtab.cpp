/*
 * (C) Mike van Emmerik
 * These could probably be replaced by functions from libg++
 */

/* * * * * * * * * * * * * * * * * * * * * * * * * * * *\
*                                                       *
*       S y m b o l   t a b l e   F u n c t i o n s     *
*                                                       *
\* * * * * * * * * * * * * * * * * * * * * * * * * * * */

/* This file implements a symbol table with a symbolic name, a symbol value
    (word), and a procedure number. Two tables are maintained, to be able to
    look up by name or by value. Pointers are used for the duplicated symbolic
    name to save space. Both tables have the same structure.
    The hash tables automatically expand when they get 90% full; they are
    never compressed. Expanding the tables could take some time, since about
    half of the entries have to be moved on average.
    Linear probing is used, due to the difficulty of implementing (e.g.)
    quadratic probing with a variable table size.
*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "dcc.h"
#include "symtab.h"

#define TABLESIZE 16                /* Number of entries added each expansion */
                                    /* Probably has to be a power of 2 */
#define STRTABSIZE 256              /* Size string table is inc'd by */
#define NIL ((word)-1)



static word numEntry;               /* Number of entries in this table */
static word tableSize;              /* Size of the table (entries) */
static SYMTABLE *symTab;            /* Pointer to the symbol hashed table */
static SYMTABLE *valTab;            /* Pointer to the value  hashed table */

static  char *pStrTab;              /* Pointer to the current string table */
static  int   strTabNext;           /* Next free index into pStrTab */

static  tableType curTableType; /* Which table is current */
typedef struct _tableInfo
{
    SYMTABLE *symTab;
    SYMTABLE *valTab;
    word      numEntry;
    word      tableSize;
} TABLEINFO_TYPE;

TABLEINFO_TYPE tableInfo[NUM_TABLE_TYPES];   /* Array of info about tables */


/* Local prototypes */
static  void    expandSym(void);

/* Create a new symbol table. Returns "handle" */
void createSymTables(void)
{
    /* Initilise the comment table */
    /* NB - there is no symbol hashed comment table */

    numEntry  = 0;
    tableSize = TABLESIZE;
    valTab = (SYMTABLE*)allocMem(sizeof(SYMTABLE) * TABLESIZE);
    memset(valTab, 0, sizeof(SYMTABLE) * TABLESIZE);

    tableInfo[Comment].symTab = 0;
    tableInfo[Comment].valTab = valTab;
    tableInfo[Comment].numEntry = numEntry;
    tableInfo[Comment].tableSize = tableSize;
    
    /* Initialise the label table */
    numEntry  = 0;
    tableSize = TABLESIZE;
    symTab = (SYMTABLE*)allocMem(sizeof(SYMTABLE) * TABLESIZE);
    memset(symTab, 0, sizeof(SYMTABLE) * TABLESIZE);

    valTab = (SYMTABLE*)allocMem(sizeof(SYMTABLE) * TABLESIZE);
    memset(valTab, 0, sizeof(SYMTABLE) * TABLESIZE);

    tableInfo[Label].symTab = symTab;
    tableInfo[Label].valTab = valTab;
    tableInfo[Label].numEntry = numEntry;
    tableInfo[Label].tableSize = tableSize;
    curTableType = Label;

    /* Now the string table */
    strTabNext = 0;
    pStrTab = (char *)allocMem(STRTABSIZE);

    tableInfo[Label].symTab = symTab;
    tableInfo[Label].valTab = valTab;
    tableInfo[Label].numEntry = numEntry;
    tableInfo[Label].tableSize = tableSize;
    curTableType = Label;

}

void
selectTable(tableType tt)
{
    if (curTableType == tt) return; /* Nothing to do */

    symTab   = tableInfo[tt].symTab;
    valTab   = tableInfo[tt].valTab;
    numEntry = tableInfo[tt].numEntry;
    tableSize= tableInfo[tt].tableSize;
    curTableType = tt;
}

void destroySymTables(void)
{
    selectTable(Label);
    free(symTab);                   /* The symbol hashed label table */
    free(valTab);                   /* And the value hashed label table */
    selectTable(Comment);
    free(valTab);                   /* And the value hashed comment table */
}


/* Hash the symbolic name */
word symHash(char *name, word *pre)
{
    int i;
    word h = 0;
    char ch;

    for (i=0; i < (int)strlen(name); i++)
    {
        ch = name[i];
        h = (h << 2) ^ ch;
        h += (ch >> 2) + (ch << 5);
    }

    *pre = h;                       /* Pre modulo hash value */
    return h % tableSize;           /* Post modulo hash value */
}

/* Hash the symOff and symProc fields */
/* Note: for the time being, there no use is made of the symProc field */
word
valHash(dword symOff, PPROC symProc, word *pre)
{
    word h = 0;

    h = (word)(symOff ^ (symOff >> 8));

    *pre = h;                       /* Pre modulo hash value */
    return h % tableSize;           /* Post modulo hash value */
}


void
enterSym(char *symName, dword symOff, PPROC symProc, boolT bSymToo)
{
    word h, pre, j;

    if ((numEntry / 9 * 10) >= tableSize)
    {
        /* Table is full. Expand it */
        expandSym();
    }

    /* Enter it into the value hashed table first */
    h = valHash(symOff, symProc, &pre);     /* Ideal spot for this entry */
    if (valTab[h].symProc == 0)             /* Collision? */
    {
        /* No. Just insert here */
        valTab[h].pSymName= symName;        /* Symbol name ptr */
        valTab[h].symOff    = symOff;       /* Offset of the symbol */
        valTab[h].symProc   = symProc;      /* Symbol's proc num */
        valTab[h].preHash = pre;            /* Pre modulo hash value */
        valTab[h].postHash= h;              /* Post modulo hash value */
        valTab[h].nextOvf = NIL;            /* No overflow */
        valTab[h].prevOvf = NIL;            /* No back link */
    }
    else
    {
        /* Linear probing, for now */
        j = (h+1) % tableSize;
        while (j != h)
        {
            if (valTab[j].symProc == 0)
            {
                /* Insert here */
                valTab[j].pSymName= symName;    /* Symbol name ptr */
                valTab[j].symOff  = symOff;     /* Offset of the symbol */
                valTab[j].symProc = symProc;    /* Symbol's proc num */
                valTab[j].preHash = pre;        /* Pre modulo hash value */
                valTab[j].postHash= h;          /* Post modulo hash value */
                /* Insert after the primary entry in the table */
                valTab[j].nextOvf = valTab[h].nextOvf;
                valTab[h].nextOvf = j;
                valTab[j].prevOvf = h;          /* The backlink */
                break;
            }
            else
            {
                /* Probe further */
                j = (j+1) % tableSize;
            }
        }
        if (j == h)
        {
            printf("enterSym: val table overflow!\n");
            exit(1);
        }
    }

    /* Now enter into the symbol hashed table as well, if reqd */
    if (!bSymToo) return;
    h = symHash(symName, &pre);             /* Ideal spot for this entry */
    if (symTab[h].pSymName == 0)            /* Collision? */
    {
        /* No. Just insert here */
        symTab[h].pSymName= symName;        /* Symbol name ptr */
        symTab[h].symOff  = symOff;         /* Offset of the symbol */
        symTab[h].symProc = symProc;        /* Symbol's proc num */
        symTab[h].preHash = pre;            /* Pre modulo hash value */
        symTab[h].postHash= h;              /* Post modulo hash value */
        symTab[h].nextOvf = NIL;            /* No overflow */
        symTab[h].prevOvf = NIL;            /* No back link */
    }
    else
    {
        /* Linear probing, for now */
        j = (h+1) % tableSize;
        while (j != h)
        {
            if (symTab[j].pSymName == 0)
            {
                /* Insert here */
                symTab[j].pSymName= symName;    /* Symbol name ptr */
                symTab[j].symOff  = symOff;     /* Offset of the symbol */
                symTab[j].symProc = symProc;    /* Symbol's proc num */
                symTab[j].preHash = pre;        /* Pre modulo hash value */
                symTab[j].postHash= h;          /* Post modulo hash value */
                /* Insert after the primary entry in the table */
                symTab[j].nextOvf = symTab[h].nextOvf;
                symTab[h].nextOvf = j;
                symTab[j].prevOvf = h;          /* The backlink */
                break;
            }
            else
            {
                /* Probe further */
                j = (j+1) % tableSize;
            }
        }
        if (j == h)
        {
            printf("enterSym: sym table overflow!\n");
            exit(1);
        }
    }

}


boolT
findSym(char *symName, word *pIndex)
{
    word h, j, pre;

    h = symHash(symName, &pre);
    j = h;
    do
    {
        if (symTab[j].pSymName == 0)
        {
            return FALSE;                   /* No entry at all */
        }
        if (strcmp(symName, symTab[j].pSymName) == 0)
        {
            *pIndex = j;
            return TRUE;                    /* Symbol found */
        }
        j = symTab[j].nextOvf;              /* Follow the chain */
    }
    while (j != NIL);
    return FALSE;                           /* End of chain */
}

/* Find symbol by value */
boolT
findVal(dword symOff, PPROC symProc, word *pIndex)
{
    word h, j, pre;

    h = valHash(symOff, symProc, &pre);
    j = h;
    do
    {
        if (valTab[j].symProc == 0)
        {
            return FALSE;                   /* No entry at all */
        }
        if ((valTab[j].symOff == symOff)
            /*&& (valTab[j].symProc == symProc)*/)
        {
            *pIndex = j;
            return TRUE;                    /* Symbol found */
        }
        j = valTab[j].nextOvf;              /* Follow the chain */
    }
    while (j != NIL);
    return FALSE;                           /* End of chain */
}

word
findBlankSym(char *symName)
{
    word h, j, pre;

    h = symHash(symName, &pre);
    j = h;
    do
    {
        if (symTab[j].pSymName == 0)
        {
            return j;                   /* Empty entry. Terminate probing */
        }
        j = (++j) % tableSize;              /* Linear probing */
    }
    while (j != h);
    printf("Could not find blank entry in table! Num entries is %ld of %ld\n",
        (long)numEntry, (long)tableSize);
	return 0;
}

/* Using the symbolic name, read the value */
boolT
readSym(char *symName, dword *pSymOff, PPROC *pSymProc)
{
    word i;

    if (!findSym(symName, &i))
    {
        return FALSE;
    }
    *pSymOff = symTab[i].symOff;
    *pSymProc= symTab[i].symProc;
    return TRUE;
}

/* Using the value, read the symbolic name */
boolT
readVal(char *symName, dword symOff, PPROC symProc)
{
    word i;

    if (!findVal(symOff, symProc, &i))
    {
        return FALSE;
    }
    strcpy(symName, valTab[i].pSymName);
    return TRUE;
}



/*  A doubly linked list of entries belonging to the same hash bucket is
    maintained, to prevent the need for many entries to be moved when deleting
    an entry. It is implemented with indexes, and is not an open hashing system.
    Symbols are deleted from both hash tables.
*/

/* Known limitation: strings are never deleted from the string table */

void
deleteSym(char *symName)
{
    word i, j, back;
    dword symOff;
    PPROC symProc;

    /* Delete from symbol hashed table first */
    if (!findSym(symName, &i))
    {
        printf("Could not delete non existant symbol name %s\n", symName);
        exit(1);
    }
    symOff = symTab[i].symOff;              /* Remember these for valTab */
    symProc= symTab[i].symProc;
    j = symTab[i].nextOvf;                  /* Look at next overflowed entry */
    
    if (j == NIL)                           /* Any overflows? */
    {
        /* No, so we just wipe out this record. Must NIL the pointer of
            the previous record, however */
        symTab[symTab[i].prevOvf].nextOvf = NIL;
        j = i;                              /* So we wipe out the current name */
    }
    else 
    {
        /* Yes, move this entry to this vacated spot. Note that the nextOvf
            field will still point to the next record in the overflow chain,
            but we need to preserve the backlink for adjusting the current
            item's backlink */
        back = symTab[j].prevOvf;
        memcpy(&symTab[i], &symTab[j], sizeof(SYMTABLE));
        symTab[i].prevOvf = back;
    }
    /* And now mark the vacated record as empty */
    symTab[j].pSymName = 0;             /* Rub out the name */

    
    /* Delete from value hashed table */
    if (!findVal(symOff, symProc, &i))
    {
        printf("Could not delete non existant symbol off %04X proc %d\n",
            symOff, symProc);
        exit(1);
    }
    j = valTab[i].nextOvf;              /* Look at next overflowed entry */
    
    if (j == NIL)                       /* Any overflows? */
    {
        /* No, so we just wipe out this record. Must NIL the pointer of
            the previous record, however */
        valTab[valTab[i].prevOvf].nextOvf = NIL;
        j = i;                          /* So we wipe out the current entry */
    }
    else 
    {
        /* Yes, move this entry to this vacated spot. Note that the nextOvf
            field will still point to the next record in the overflow chain,
            but we need to preserve the backlink for adjusting the current
            item's backlink */
        back = valTab[j].prevOvf;
        memcpy(&valTab[i], &valTab[j], sizeof(SYMTABLE));
        valTab[i].prevOvf = back;
    }
    /* And now mark the vacated record as empty */
    valTab[j].symProc = 0;          /* Rub out the entry */
}


void
deleteVal(dword symOff, PPROC symProc, boolT bSymToo)
{
    word i, j, back;
    char *symName;

    /* Delete from value hashed table */
    if (!findVal(symOff, symProc, &i))
    {
        printf("Could not delete non existant symbol off %04X proc %p\n",
            symOff, symProc);
        exit(1);
    }
    symName = symTab[i].pSymName;       /* Remember this for symTab */
    j = valTab[i].nextOvf;              /* Look at next overflowed entry */
    
    if (j == NIL)                       /* Any overflows? */
    {
        /* No, so we just wipe out this record. Must NIL the pointer of
            the previous record, however */
        valTab[valTab[i].prevOvf].nextOvf = NIL;
        j = i;                          /* So we wipe out the current entry */
    }
    else 
    {
        /* Yes, move this entry to this vacated spot. Note that the nextOvf
            field will still point to the next record in the overflow chain,
            but we need to preserve the backlink for adjusting the current
            item's backlink */
        back = valTab[j].prevOvf;
        memcpy(&valTab[i], &valTab[j], sizeof(SYMTABLE));
        valTab[i].prevOvf = back;
    }
    /* And now mark the vacated record as empty */
    valTab[j].symProc = 0;          /* Rub out the entry */

    /* If requested, delete from symbol hashed table now */
    if (!bSymToo) return;
    if (!findSym(symName, &i))
    {
        printf("Could not delete non existant symbol name %s\n", symName);
        exit(1);
    }
    j = symTab[i].nextOvf;                  /* Look at next overflowed entry */
    
    if (j == NIL)                           /* Any overflows? */
    {
        /* No, so we just wipe out this record. Must NIL the pointer of
            the previous record, however */
        symTab[symTab[i].prevOvf].nextOvf = NIL;
        j = i;                              /* So we wipe out the current name */
    }
    else 
    {
        /* Yes, move this entry to this vacated spot. Note that the nextOvf
            field will still point to the next record in the overflow chain,
            but we need to preserve the backlink for adjusting the current
            item's backlink */
        back = symTab[j].prevOvf;
        memcpy(&symTab[i], &symTab[j], sizeof(SYMTABLE));
        symTab[i].prevOvf = back;
    }
    /* And now mark the vacated record as empty */
    symTab[j].pSymName = 0;             /* Rub out the name */
    
}

static void
expandSym(void)
{
    word i, j, n, newPost;

    printf("\nResizing table...\r");
    /* We double the table size each time, so on average only half of the
        entries move to the new half. This works because we are effectively
        shifting the "binary point" of the hash value to the left each time,
        thereby leaving the number unchanged or adding an MSBit of 1. */
    tableSize <<= 2;
    symTab = (SYMTABLE*)reallocVar(symTab, tableSize * sizeof(SYMTABLE));
    memset (&symTab[tableSize/2], 0, (tableSize/2) * sizeof(SYMTABLE));

    /* Now we have to move some of the entries to take advantage of the extra
        space */

    for (i=0; i < numEntry; i++)
    {
        newPost = symTab[i].preHash % tableSize;
        if (newPost != symTab[i].postHash)
        {
            /* This entry is now in the wrong place. Copy it to the new position,
                then delete it. */
            j = findBlankSym(symTab[i].pSymName);
            memcpy(&symTab[j], &symTab[i], sizeof(SYMTABLE));
            /* Correct the post hash value */
            symTab[j].postHash = newPost;

            /* Now adjust links */
            n = symTab[j].prevOvf;
            if (n != NIL)
            {
                symTab[n].nextOvf = j;
            }

            n = symTab[j].nextOvf;
            if (n != NIL)
            {
                symTab[n].prevOvf = j;
            }
            
            /* Mark old position as deleted */
            symTab[i].pSymName = 0;
        }
    }
}

/* This function adds to the string table. At this stage, strings are not
    deleted */
char *
addStrTbl(char *pStr)
{
    char *p;

    if ((strTabNext + strlen(pStr) + 1) >= STRTABSIZE)
    {
        /* We can't realloc the old string table pointer, since that will
            potentially move the string table, and pointers will be invalid.
            So we realloc this one to its present usage (hopefully it won't
            move), and allocate a new one */
        if (reallocVar((void *)pStrTab, strTabNext) != pStrTab)
		{
			printf("Damn it! String table moved on shrinking!\n");
			exit(1);
		}
        pStrTab = (char *)allocMem(STRTABSIZE);
        strTabNext = 0;
    }
    p = strcpy(&pStrTab[strTabNext], pStr);
    strTabNext += strlen(pStr) +1;
    return p;
}
