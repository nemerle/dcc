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
    (uint16_t), and a procedure number. Two tables are maintained, to be able to
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
#define NIL ((uint16_t)-1)
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
        uint16_t h = 0;
        h = (uint16_t)(key.symOff ^ (key.symOff >> 8));
        return h;
    }

};
}
static  tableType curTableType; /* Which table is current */
struct TABLEINFO_TYPE
{
    TABLEINFO_TYPE()
    {
        symTab=valTab=0;
    }
    //void deleteVal(uint32_t symOff, Function *symProc, boolT bSymToo);
    void create(tableType type);
    void destroy();
private:

    SYMTABLE *symTab;   /* Pointer to the symbol hashed table */
    SYMTABLE *valTab;   /* Pointer to the value  hashed table */
    uint16_t      numEntry; /* Number of entries in this table */
    uint16_t      tableSize;/* Size of the table (entries) */
    unordered_map<string,SYMTABLE> z;
    unordered_map<SYMTABLE,string> z2;
};

static TABLEINFO_TYPE tableInfo[NUM_TABLE_TYPES];   /* Array of info about tables */
static TABLEINFO_TYPE currentTabInfo;

/* Create a new symbol table. Returns "handle" */
void TABLEINFO_TYPE::create(tableType type)
{
    switch(type)
    {
        case Comment:
            numEntry  = 0;
            tableSize = TABLESIZE;
            valTab = new SYMTABLE [TABLESIZE];
            symTab = 0;
            break;
        case Label:
            currentTabInfo.numEntry  = 0;
            currentTabInfo.tableSize = TABLESIZE;
            currentTabInfo.symTab = new SYMTABLE [TABLESIZE];
            currentTabInfo.valTab = new SYMTABLE [TABLESIZE];
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
    delete [] symTab; // The symbol hashed label table
    delete [] valTab; // And the value hashed label table
}
void destroySymTables(void)
{
    selectTable(Label);
    currentTabInfo.destroy();
    selectTable(Comment);
    currentTabInfo.destroy();
}

/* Using the value, read the symbolic name */
boolT readVal(std::ostringstream &symName, uint32_t symOff, Function * symProc)
{
    return false; // no symbolic names for now
}
