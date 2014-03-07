#pragma once
#include <string>
#include <stdint.h>
#include <cassert>
#include <list>
#include <llvm/ADT/ilist.h>
#include <boost/icl/interval.hpp>
#include <boost/icl/interval_map.hpp>
#include <boost/icl/split_interval_map.hpp>
#include <unordered_set>
#include "symtab.h"
#include "BinaryImage.h"
#include "Procedure.h"
class SourceMachine;
struct CALL_GRAPH;
class IProject
{
    virtual PROG *binary()=0;
    virtual const std::string & project_name() const =0;
    virtual const std::string & binary_path() const =0;
};
class Project : public IProject
{
    static Project *s_instance;
    std::string m_fname;
    std::string m_project_name;
public:

typedef llvm::iplist<Function> FunctionListType;
typedef FunctionListType lFunction;
    typedef FunctionListType::iterator ilFunction;
    SYMTAB symtab;       /* Global symbol table              */

    FunctionListType pProcList;
    CALL_GRAPH * callGraph;	/* Pointer to the head of the call graph     */
    PROG prog;   		/* Loaded program image parameters  */
    // no copies
    Project(const Project&) = delete;
    const Project &operator=(const Project & l) =delete;
    // only moves
    Project(); // default constructor,

public:
    void create(const std::string & a);
    const std::string &project_name() const {return m_project_name;}
    const std::string &binary_path() const {return m_fname;}
    ilFunction funcIter(Function *to_find);
    ilFunction findByEntry(uint32_t entry);
    ilFunction createFunction(FunctionType *f,const std::string &name);
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
    void initialize();
    void writeGlobSymTable();
};
//extern Project g_proj;
