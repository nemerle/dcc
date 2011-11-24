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
#include <cstdio>
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <unordered_map>
#include "dcc.h"
#include "symtab.h"

#define TABLESIZE 16                /* Number of entries added each expansion */
                                    /* Probably has to be a power of 2 */
#define STRTABSIZE 256              /* Size string table is inc'd by */
#define NIL ((word)-1)
using namespace std;
static  char *pStrTab;              /* Pointer to the current string table */
static  int   strTabNext;           /* Next free index into pStrTab */
namespace std
{
template<>
struct hash<SYMTABLE> : public unary_function<const SYMTABLE &,size_t>
{
    size_t operator()(const SYMTABLE & key) const
    {
        word h = 0;
        h = (word)(key.symOff ^ (key.symOff >> 8));
        return h;
    }

};
}
static  tableType curTableType; /* Which table is current */
struct TABLEINFO_TYPE
{
    void    deleteVal(dword symOff, Function *symProc, boolT bSymToo);
    void    enterSym(const char *symName, dword symOff, Function *symProc, boolT bSymToo);
    std::string findVal(dword symOff, Function *symProc, word &pIndex);
    void create(tableType type);
    void destroy();
private:
    void    deleteSym(char *symName);
    boolT   findSym(const char *symName, word &pIndex);
    boolT   readSym(char *symName, dword *pSymOff, Function **pSymProc);
    void    expandSym(void);
    word    findBlankSym(const std::string &symName);
    word    symHash(const char *name, word *pre);
    word    valHash(dword symOff, Function *symProc, word *pre);

    SYMTABLE *symTab;   /* Pointer to the symbol hashed table */
    SYMTABLE *valTab;   /* Pointer to the value  hashed table */
    word      numEntry; /* Number of entries in this table */
    word      tableSize;/* Size of the table (entries) */
    unordered_map<string,SYMTABLE> z;
    unordered_map<SYMTABLE,string> z2;
};

TABLEINFO_TYPE tableInfo[NUM_TABLE_TYPES];   /* Array of info about tables */
TABLEINFO_TYPE currentTabInfo;

/* Create a new symbol table. Returns "handle" */
void TABLEINFO_TYPE::create(tableType type)
{
    switch(type)
    {
        case Comment:
            numEntry  = 0;
            tableSize = TABLESIZE;
            valTab = (SYMTABLE*)allocMem(sizeof(SYMTABLE) * TABLESIZE);
            symTab = 0;
            memset(valTab, 0, sizeof(SYMTABLE) * TABLESIZE);
            break;
        case Label:
            currentTabInfo.numEntry  = 0;
            currentTabInfo.tableSize = TABLESIZE;
            currentTabInfo.symTab = (SYMTABLE*)allocMem(sizeof(SYMTABLE) * TABLESIZE);
            memset(currentTabInfo.symTab, 0, sizeof(SYMTABLE) * TABLESIZE);

            currentTabInfo.valTab = (SYMTABLE*)allocMem(sizeof(SYMTABLE) * TABLESIZE);
            memset(currentTabInfo.valTab, 0, sizeof(SYMTABLE) * TABLESIZE);
            break;
    }

}

void createSymTables(void)
{
    /* Initilise the comment table */
    /* NB - there is no symbol hashed comment table */
    currentTabInfo.create(Comment);
    tableInfo[Comment] = currentTabInfo;

    /* Initialise the label table */
    currentTabInfo.create(Label);

    tableInfo[Label] = currentTabInfo;
    curTableType = Label;

    /* Now the string table */
    strTabNext = 0;
    pStrTab = (char *)allocMem(STRTABSIZE);

//    tableInfo[Label].symTab = currentTabInfo.symTab;
//    tableInfo[Label].valTab = currentTabInfo.valTab;
//    tableInfo[Label].numEntry = currentTabInfo.numEntry;
//    tableInfo[Label].tableSize = currentTabInfo.tableSize;
    curTableType = Label;

}

void selectTable(tableType tt)
{
    if (curTableType == tt)
        return; /* Nothing to do */
    currentTabInfo = tableInfo[tt];
    curTableType = tt;
}
void TABLEINFO_TYPE::destroy()
{
    if(symTab)
        free(symTab);  // The symbol hashed label table
    if(valTab)
        free(valTab);  // And the value hashed label table

}
void destroySymTables(void)
{
    selectTable(Label);
    currentTabInfo.destroy();
    selectTable(Comment);
    currentTabInfo.destroy();
}


/* Hash the symbolic name */
word TABLEINFO_TYPE::symHash(const char *name, word *pre)
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
word TABLEINFO_TYPE::valHash(dword symOff, Function * symProc, word *pre)
{
    word h = 0;

    h = (word)(symOff ^ (symOff >> 8));

    *pre = h;                       /* Pre modulo hash value */
    return h % tableSize;           /* Post modulo hash value */
}
void TABLEINFO_TYPE::enterSym(const char *symName, dword symOff, Function * symProc, boolT bSymToo)
{
    word h, pre, j;
    SYMTABLE entry;
    entry.pSymName= symName;        /* Symbol name ptr */
    entry.symOff    = symOff;       /* Offset of the symbol */
    entry.symProc   = symProc;      /* Symbol's proc num */
    entry.preHash = pre;            /* Pre modulo hash value */
    entry.postHash= h;              /* Post modulo hash value */
    entry.nextOvf = NIL;            /* No overflow */
    entry.prevOvf = NIL;            /* No back link */
    z[symName] = entry;
    z2[entry] = symName;
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
    h = symHash(symName, &pre);  /* Ideal spot for this entry */
    if (symTab[h].pSymName.empty())            /* Collision? */
    {
        /* No. Just insert here */
        symTab[h].pSymName= symName; /* Symbol name ptr */
        symTab[h].symOff  = symOff;  /* Offset of the symbol */
        symTab[h].symProc = symProc; /* Symbol's proc num */
        symTab[h].preHash = pre;     /* Pre modulo hash value */
        symTab[h].postHash= h;       /* Post modulo hash value */
        symTab[h].nextOvf = NIL;     /* No overflow */
        symTab[h].prevOvf = NIL;     /* No back link */
    }
    else
    {
        /* Linear probing, for now */
        j = (h+1) % tableSize;
        while (j != h)
        {
            if (symTab[j].pSymName.empty())
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


void enterSym(char *symName, dword symOff, Function * symProc, boolT bSymToo)
{
    currentTabInfo.enterSym(symName,symOff,symProc,bSymToo);
}

boolT TABLEINFO_TYPE::findSym(const char *symName, word &pIndex)
{
    word h, j, pre;

    h = symHash(symName, &pre);
    j = h;
    bool found=false;
    do
    {
        if (symTab[j].pSymName.empty())
        {
            return FALSE;                   /* No entry at all */
        }
        if (strcmp(symName, symTab[j].pSymName.c_str()) == 0)
        {
            pIndex = j;
            found=true;
            break;                    /* Symbol found */
        }
        j = symTab[j].nextOvf;              /* Follow the chain */
    }
    while (j != NIL);
    auto iter = z.find(symName);
    if(iter!=z.end())
    {
        assert(iter->second==symTab[j]);
    }

    return found;                           /* End of chain */
}
/* Find symbol by value */
std::string TABLEINFO_TYPE::findVal(dword symOff, Function * symProc, word &pIndex)
{
    word h, j, pre;
    std::string res="";
    h = valHash(symOff, symProc, &pre);
    j = h;
    do
    {
        if (valTab[j].symProc == 0)
            break;                   /* No entry at all */

        if ((valTab[j].symOff == symOff) /*&& (valTab[j].symProc == symProc)*/)
        {
            pIndex = j;
            res=valTab[j].pSymName;
            break;                    /* Symbol found */
        }
        j = valTab[j].nextOvf;              /* Follow the chain */
    }
    while (j != NIL);
    auto iter = z2.find(SYMTABLE(symOff,symProc));
    if(iter!=z2.end())
    {
        assert(iter->second==res);
    }
    return res;                           /* End of chain */
}

word TABLEINFO_TYPE::findBlankSym(const std::string &symName)
{
    word h, j, pre;

    h = symHash(symName.c_str(), &pre);
    j = h;
    do
    {
        if (symTab[j].pSymName.empty())
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
boolT TABLEINFO_TYPE::readSym(char *symName, dword *pSymOff, Function * *pSymProc)
{
    word i;

    if (!findSym(symName, i))
    {
        return FALSE;
    }
    *pSymOff = symTab[i].symOff;
    *pSymProc= symTab[i].symProc;
    return TRUE;
}




/*  A doubly linked list of entries belonging to the same hash bucket is
    maintained, to prevent the need for many entries to be moved when deleting
    an entry. It is implemented with indexes, and is not an open hashing system.
    Symbols are deleted from both hash tables.
*/

/* Known limitation: strings are never deleted from the string table */

void TABLEINFO_TYPE::deleteSym(char *symName)
{
    word i, j, back;
    dword symOff;
    Function * symProc;

    /* Delete from symbol hashed table first */
    if (!findSym(symName, i))
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
        symTab[i] = symTab[j];
        symTab[i].prevOvf = back;
    }
    /* And now mark the vacated record as empty */
    symTab[j].pSymName.clear();             /* Rub out the name */


    /* Delete from value hashed table */
    if (findVal(symOff, symProc, i).empty())
    {
        printf("Could not delete non existant symbol off %04X proc %d\n",symOff, symProc);
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
        valTab[i]= valTab[j];
        valTab[i].prevOvf = back;
    }
    /* And now mark the vacated record as empty */
    valTab[j].symProc = 0;          /* Rub out the entry */
}
void TABLEINFO_TYPE::deleteVal(dword symOff, Function * symProc, boolT bSymToo)
{
    word i, j, back;
    std::string symName;

    /* Delete from value hashed table */
    if (findVal(symOff, symProc, i).empty())
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
    if (!findSym(symName.c_str(), i))
    {
        printf("Could not delete non existant symbol name %s\n", symName.c_str());
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
        symTab[i] = symTab[j];
        symTab[i].prevOvf = back;
    }
    /* And now mark the vacated record as empty */
    symTab[j].pSymName.clear();             /* Rub out the name */

}

void TABLEINFO_TYPE::expandSym(void)
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
            symTab[i].pSymName.clear();
        }
    }
}

/* This function adds to the string table. At this stage, strings are not
    deleted */
char * addStrTbl(char *pStr)
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
void deleteVal(dword symOff, Function * symProc, boolT bSymToo)
{
    currentTabInfo.deleteVal(symOff,symProc,bSymToo);
}
std::string findVal(dword symOff, Function * symProc, word *pIndex)
{
    return currentTabInfo.findVal(symOff,symProc,*pIndex);
}
/* Using the value, read the symbolic name */
boolT readVal(char *symName, dword symOff, Function * symProc)
{
    word i;
    std::string r=currentTabInfo.findVal(symOff, symProc, i);
    if (r.empty())
    {
        return false;
    }
    strcpy(symName, r.c_str());
    return true;
}
