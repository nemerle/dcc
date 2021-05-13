/*
 * Symbol table prototypes
 * (C) Mike van Emmerik
*/
#pragma once
#include "Enums.h"
#include "types.h"
#include "msvc_fixes.h"

#include <QtCore/QString>
#include <string>
#include <stdint.h>
#include <vector>

struct Expr;
struct AstIdent;
struct TypeContainer;
class QTextStream;

/* * * * * * * * * * * * * * * * * */
/* Symbol table structs and protos */
/* * * * * * * * * * * * * * * * * */
struct SymbolCommon
{
    QString     name;   /* New name for this variable/symbol/argument */
    int         size;   /* Size/maximum size                */
    hlType      type;       /* probable type                */
    eDuVal      duVal;      /* DEF, USE, VAL    						*/
    SymbolCommon(hlType t=TYPE_UNKNOWN) : size(0),type(t)
    {}
};
struct SYM : public SymbolCommon
{
    typedef uint32_t tLabel;
    SYM() : label(0),flg(0)
    {

    }
    uint32_t    label;      /* physical address (20 bit)    */
    uint32_t     flg;       /* SEG_IMMED, IMPURE, WORD_OFF  */
};
/* STACK FRAME */
struct STKSYM : public SymbolCommon
{
    typedef int16_t tLabel;
    Expr	*   actual=0;       /* Expression tree of actual parameter 		*/
    AstIdent *  regs=0;         /* For register arguments only				*/
    tLabel      label=0;        /* Immediate off from BP (+:args, -:params) */
    uint8_t     regOff=0;       /* Offset is a register (e.g. SI, DI)       */
    bool        hasMacro=false;	/* This type needs a macro					*/
    QString     macro;          /* Macro name								*/
    bool        invalid=false;	/* Boolean: invalid entry in formal arg list*/
    void setArgName(int i)
    {
        char buf[32];
        sprintf (buf, "arg%d", i);
        name = buf;
    }
    STKSYM(hlType t=TYPE_UNKNOWN) : SymbolCommon(t) {}
};
template<class T>
class SymbolTableCommon
{
    std::vector<T> storage;
public:
    typedef typename std::vector<T>::iterator iterator;
    typedef typename std::vector<T>::const_iterator const_iterator;

    iterator findByLabel(typename T::tLabel lab)
    {
        auto iter = std::find_if(storage.begin(),storage.end(),
                                 [lab](T &s)->bool {return s.label==lab;});
        return iter;
    }
    const_iterator findByLabel(typename T::tLabel lab) const
    {
        auto iter = std::find_if(storage.begin(),storage.end(),
                                 [lab](const T &s)->bool {return s.label==lab;});
        return iter;
    }
    const T& operator[](size_t idx) const {
        return storage[idx];
    }
    T& operator[](size_t idx) {
        return storage[idx];
    }

    void push_back(const T &entry) {
        storage.push_back(entry);
    }
    iterator begin() { return storage.begin(); }
    const_iterator begin() const { return storage.cbegin(); }
    iterator end() { return storage.end(); }
    const_iterator end() const { return storage.cend(); }

    T &back() { return storage.back(); }

    size_t size() const { return storage.size(); }

    bool empty() const { return storage.empty(); }

};
/* SYMBOL TABLE */
class SYMTAB : public SymbolTableCommon<SYM>
{

public:
    void updateSymType(uint32_t symbol, const TypeContainer &tc);
    SYM *updateGlobSym(uint32_t operand, int size, uint16_t duFlag, bool &inserted_new);
};
struct Function;
struct SYMTABLE
{
    std::string pSymName;              /* Ptr to symbolic name or comment */
    uint32_t   symOff;                 /* Symbol image offset */
    Function *symProc;             /* Procedure pointer */
    SYMTABLE() : symOff(0),symProc(0) {}
    SYMTABLE(uint32_t _sym,Function *_proc) : symOff(_sym),symProc(_proc)
    {}
    bool operator == (const SYMTABLE &other) const
    {
        // does not yse pSymName, to ease finding by symOff/symProc combo
        // in map<SYMTABLE,X>
        return (symOff==other.symOff) and symProc==(other.symProc);
    }
};

enum tableType                     /* The table types */
{
    Label=0,                        /* The label table */
    Comment                        /* The comment table */
};
constexpr int NUM_TABLE_TYPES = int(Comment)+1; /* Number of entries: must be last */

void    createSymTables();
void    destroySymTables();
bool    readVal (QTextStream & symName, uint32_t   symOff, Function *symProc);
void    selectTable(tableType);     /* Select a particular table */

