#pragma once
#include <string>
#include <stdint.h>
#include <cassert>
#include <list>
#include <boost/icl/interval.hpp>
#include <boost/icl/interval_map.hpp>
#include <boost/icl/split_interval_map.hpp>
#include <unordered_set>
#include <QtCore/QString>
#include "symtab.h"
#include "BinaryImage.h"
#include "Procedure.h"
class QString;
class SourceMachine;
struct CALL_GRAPH;
class IProject
{
    virtual PROG *binary()=0;
    virtual const QString & project_name() const =0;
    virtual const QString & binary_path() const =0;
};
class Project : public IProject
{
    static  Project *s_instance;
            QString     m_fname;
            QString     m_project_name;
            QString     m_output_path;
public:

    typedef std::list<Function> FunctionListType;
    typedef FunctionListType lFunction;
    typedef FunctionListType::iterator ilFunction;

            SYMTAB      symtab;       /* Global symbol table              */
            FunctionListType pProcList;
            CALL_GRAPH * callGraph;	/* Pointer to the head of the call graph     */
            PROG        prog;   		/* Loaded program image parameters  */
                        // no copies
                        Project(const Project&) = delete;
    const   Project &   operator=(const Project & l) =delete;
                        // only moves
                        Project(); // default constructor,

public:
            void        create(const QString &a);
            bool        load();
    const   QString &   output_path() const {return m_output_path; }
            void        set_output_path(const QString &path) { m_output_path=path; }
    const   QString &   project_name() const {return m_project_name;}
    const   QString &   binary_path() const {return m_fname;}
            QString     output_name(const char *ext);
            ilFunction  funcIter(Function *to_find);
            ilFunction  findByEntry(uint32_t entry);
            ilFunction  createFunction(FunctionType *f, const QString & name);
            bool        valid(ilFunction iter);

            int         getSymIdxByAddr(uint32_t adr);
            bool        validSymIdx(size_t idx);
            size_t      symbolSize(size_t idx);
            hlType      symbolType(size_t idx);
    const   QString &   symbolName(size_t idx);
    const   SYM &       getSymByIdx(size_t idx) const;

    static  Project *   get();
            PROG *      binary() {return &prog;}
            SourceMachine *machine();

    const   FunctionListType &functions() const { return pProcList; }
            FunctionListType &functions()       { return pProcList; }
protected:
            void        initialize();
            void        writeGlobSymTable();
};
//extern Project g_proj;
