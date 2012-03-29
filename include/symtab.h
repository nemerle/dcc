/*
 * Symbol table prototypes
 * (C) Mike van Emmerik
*/
#pragma once
#include <string>
#include <stdint.h>
#include "Enums.h"
#include "types.h"
struct COND_EXPR;
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
    SYM() : label(0),flg(0)
    {

    }
    uint32_t    label;      /* physical address (20 bit)    */
    uint32_t     flg;       /* SEG_IMMED, IMPURE, WORD_OFF  */
};
/* STACK FRAME */
struct STKSYM : public SymbolCommon
{
    COND_EXPR	*actual;	/* Expression tree of actual parameter 		*/
    COND_EXPR 	*regs;		/* For register arguments only				*/
    int16_t      label;        /* Immediate off from BP (+:args, -:params) */
    uint8_t     regOff;     /* Offset is a register (e.g. SI, DI)       */
    bool        hasMacro;	/* This type needs a macro					*/
    std::string macro;  	/* Macro name								*/
    bool        invalid;	/* Boolean: invalid entry in formal arg list*/
    STKSYM()
    {
        actual=regs=0;
        label=0;
        regOff=0;
        invalid=hasMacro = false;
    }
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
    iterator findByLabel(uint32_t lab)
    {
        auto iter = std::find_if(this->begin(),this->end(),
                                 [lab](T &s)->bool {return s.label==lab;});
        return iter;
    }
    const_iterator findByLabel(uint32_t lab) const
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
boolT   readVal (std::ostringstream &symName, uint32_t   symOff, Function *symProc);
void    selectTable(tableType);     /* Select a particular table */

