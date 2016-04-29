#pragma once

#include "symtab.h"
#include "BinaryImage.h"
#include "Procedure.h"
#include "state.h"
#include "src/Command.h"

#include <llvm/ADT/ilist.h>
#include <boost/icl/interval.hpp>
#include <boost/icl/interval_map.hpp>
#include <boost/icl/split_interval_map.hpp>
#include <QtCore/QString>
#include <list>
#include <unordered_set>
#include <string>
#include <stdint.h>
#include <assert.h>

class QString;
class SourceMachine;
struct CALL_GRAPH;
struct DosLoader;
struct SegOffAddr {
    uint16_t seg;
    uint32_t addr;
};
class Project : public QObject
{
    Q_OBJECT
public:
    typedef llvm::iplist<Function> FunctionListType;
    typedef FunctionListType lFunction;
    typedef FunctionListType::iterator ilFunction;

    DosLoader * m_selected_loader;
public:
            uint32_t    SynthLab;       //!< Last snthetic lab idx
            SYMTAB      symtab;         //!< Global symbol table
            FunctionListType pProcList; //!< List of located functions
            CALL_GRAPH * callGraph;     //!< Pointer to the head of the call graph
            STATE       m_entry_state;  //!< Machine state at program load

            PROG        prog;       /* Loaded program image parameters  */
            CommandStream m_project_command_stream;
            bool        m_error_state;
public:
                        // prevent Project instance copying
                        Project(const Project&) = delete;
    const   Project &   operator=(const Project & l) =delete;
                        // only moves
                        Project(); // default constructor,

            void        create(const QString &a);

            bool        addLoadCommands(QString fname);
            void        processAllCommands();
            void        resetCommandsAndErrorState();


    const   QString &   output_path() const {return m_output_path;}
    const   QString &   project_name() const {return m_project_name;}
    const   QString &   binary_path() const {return m_fname;}
            QString     output_name(const char *ext);
            ilFunction  funcIter(Function *to_find);
            ilFunction  findByEntry(uint32_t entry);
            ilFunction  createFunction(FunctionType *f, const QString & name, SegOffAddr addr);
            bool        valid(ilFunction iter);

            int         getSymIdxByAdd(uint32_t adr);
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

            bool        addCommand(Command *cmd) { return m_project_command_stream.add(cmd); }
            void        dumpAllErrors();
            void        setLoader(DosLoader *ins);
public slots:
            void        onCommandStreamFinished(bool state);
signals:
            void        newFunctionCreated(Function &);
            void        loaderSelected();
protected:
            void        initialize();
            void        writeGlobSymTable();

protected:
    static  Project *   s_instance;
            QString     m_fname;
            QString     m_project_name;
            QString     m_output_path;
            CommandContext m_command_ctx;

};
