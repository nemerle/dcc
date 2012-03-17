#pragma once
#include <string>
#include <stdint.h>
#include <cassert>
#include <list>
#include <llvm/ADT/ilist.h>
#include "symtab.h"
#include "BinaryImage.h"
struct Function;
struct SourceMachine;
struct CALL_GRAPH;

typedef llvm::iplist<Function> FunctionListType;
typedef FunctionListType lFunction;
typedef lFunction::iterator ilFunction;


struct Project
{
    SYMTAB symtab;       /* Global symbol table              */

    std::string m_fname;
    FunctionListType pProcList;
    CALL_GRAPH * callGraph;	/* Pointer to the head of the call graph     */
    PROG prog;   		/* Loaded program image parameters  */
    Project() {}
    // no copies
    Project(const Project&) = delete;
    const Project &operator=(const Project & l) =delete;
    // only moves
    Project(Project && l)
    {
        m_fname  =l.m_fname;
        size_t before=l.pProcList.size();
        pProcList.splice(pProcList.end(),l.pProcList);
        callGraph=l.callGraph;
        l.m_fname.clear();
        l.pProcList.clear();
        l.callGraph=0;
        assert(before==pProcList.size());
    }
    Project &operator=(Project && l)
    {
        if(this == &l)
            return *this;
        m_fname  =l.m_fname;
        size_t before=l.pProcList.size();
        pProcList.splice(pProcList.end(),l.pProcList);
        callGraph=l.callGraph;
        l.m_fname.clear();
        l.pProcList.clear();
        l.callGraph=0;
        assert(before==pProcList.size());
        return *this;
    }

public:
    ilFunction funcIter(Function *to_find);
    ilFunction findByEntry(uint32_t entry);
    ilFunction createFunction();
    bool valid(ilFunction iter);

    int     getSymIdxByAdd(uint32_t adr);
    bool    validSymIdx(size_t idx);
    size_t  symbolSize(size_t idx);
    hlType  symbolType(size_t idx);
    const std::string &symbolName(size_t idx);
    const SYM &getSymByIdx(size_t idx) const;

    static Project *get();
    PROG * binary() {return &prog;}
    SourceMachine *machine();

protected:
    void writeGlobSymTable();
};
//extern Project g_proj;
