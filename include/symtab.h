/*
 * Symbol table prototypes
 * (C) Mike van Emmerik
*/
#pragma once
#include <string>
#include <stdint.h>
#include "Enums.h"
#include "types.h"
struct Expr;
struct AstIdent;
struct TypeContainer;
/* * * * * * * * * * * * * * * * * */
/* Symbol table structs and protos */
/* * * * * * * * * * * * * * * * * */
struct SymbolCommon
{
    std::string name;   /* New name for this variable/symbol/argument */
    int         size;   /* Size/maximum size                */
    hlType      type;       /* probable type                */
    eDuVal      duVal;      /* DEF, USE, VAL    						*/
    SymbolCommon() : size(0),type(TYPE_UNKNOWN)
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
    Expr	*actual=0;	/* Expression tree of actual parameter 		*/
    AstIdent 	*regs=0;		/* For register arguments only				*/
    tLabel      label=0;        /* Immediate off from BP (+:args, -:params) */
    uint8_t     regOff=0;     /* Offset is a register (e.g. SI, DI)       */
    bool        hasMacro=false;	/* This type needs a macro					*/
    std::string macro;  	/* Macro name								*/
    bool        invalid=false;	/* Boolean: invalid entry in formal arg list*/
    void setArgName(int i)
    {
        char buf[32];
        sprintf (buf, "arg%d", i);
        name = buf;
    }
};
template<class T>
class SymbolTableCommon : public std::vector<T>
{
public:
    typedef typename std::vector<T>::iterator iterator;
    typedef typename std::vector<T>::const_iterator const_iterator;
    iterator findByLabel(typename T::tLabel lab)
    {
        auto iter = std::find_if(this->begin(),this->end(),
                                 [lab](T &s)->bool {return s.label==lab;});
        return iter;
    }
    const_iterator findByLabel(typename T::tLabel lab) const
    {
        auto iter = std::find_if(this->begin(),this->end(),
                                 [lab](const T &s)->bool {return s.label==lab;});
        return iter;
    }

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
        return (symOff==other.symOff) && symProc==(other.symProc);
    }
};

enum tableType                     /* The table types */
{
    Label=0,                        /* The label table */
    Comment                        /* The comment table */
};
constexpr int NUM_TABLE_TYPES = int(Comment)+1; /* Number of entries: must be last */

void    createSymTables(void);
void    destroySymTables(void);
bool    readVal (std::ostringstream &symName, uint32_t   symOff, Function *symProc);
void    selectTable(tableType);     /* Select a particular table */

