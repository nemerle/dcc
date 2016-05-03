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
#include <unordered_map>
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
enum CompilerVendor {
    eUnknownVendor=0,
    eBorland,
    eMicrosoft,
    eLogitech,
};
enum CompilerLanguage {
    eUnknownLanguage=0,
    eAnsiCorCPP,
    ePascal,
    eModula2
};
enum CompilerMemoryModel {
    eUnknownMemoryModel=0,
    eTiny,
    eSmall,
    eCompact,
    eMedium,
    eLarge
};
struct LoaderMetadata {
    CompilerVendor compiler_vendor;
    CompilerLanguage compiler_language;
    CompilerMemoryModel compiler_memory_model;
    int compiler_version;
    QString compilerId() const {
        switch(compiler_vendor) {
            case eBorland:
                switch(compiler_language) {
                case eUnknownLanguage:
                    return QString("bx") + codeModelChar();
                case eAnsiCorCPP:
                    return QString("b%1%2").arg(compiler_version).arg(codeModelChar());
                case ePascal:
                    return QString("tp%1").arg(compiler_version);
                default:
                    return "xxx";
                }
        case eMicrosoft:
            assert(compiler_language==eAnsiCorCPP);
            return QString("m%1%2").arg(compiler_version).arg(codeModelChar());
        case eLogitech:
            assert(compiler_language==eModula2);
            return QString("l%1%2").arg(compiler_version).arg(codeModelChar());
        case eUnknownVendor:
            return "xxx";
        }
        return "xxx";
    }
    QChar codeModelChar() const {
        switch(compiler_memory_model) {
        case eUnknownMemoryModel: return 'x';
        case eTiny: return 't';
        case eSmall: return 's';
        case eCompact: return 'c';
        case eMedium: return 'm';
        case eLarge: return 'l';
        }
    }
};
class Project : public QObject
{
    Q_OBJECT
public:
            DosLoader * m_selected_loader;
            bool m_metadata_available=false;
            LoaderMetadata m_loader_data;
            uint32_t    SynthLab;       //!< Last snthetic lab idx
            SYMTAB      symtab;         //!< Global symbol table
            FunctionListType pProcList; //!< List of located functions
            CALL_GRAPH * callGraph;     //!< Pointer to the head of the call graph
            STATE       m_entry_state;  //!< Machine state at program load

            PROG        prog;       /* Loaded program image parameters  */
            CommandStream m_project_command_stream;
            std::unordered_map<PtrFunction,CommandStream> m_function_streams;

            bool        m_error_state;
            struct PatternLocator *m_pattern_locator;
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
            ilFunction funcIter(Function *to_find);
            PtrFunction  findByEntry(uint32_t entry);
            PtrFunction findByName(const QString &name);
            PtrFunction createFunction(FunctionType *f, const QString & name, SegOffAddr addr);
            bool        valid(ilFunction iter);

            int         getSymIdxByAdd(uint32_t adr);
            bool        validSymIdx(size_t idx);
            size_t      symbolSize(size_t idx);
            hlType      symbolType(size_t idx);
    const   QString &   symbolName(size_t idx);
    const   SYM &       getSymByIdx(size_t idx) const;

            LoaderMetadata &getLoaderMetadata() { assert(m_metadata_available); return m_loader_data; }
            void setLoaderMetadata(LoaderMetadata d)  { m_loader_data = d; m_metadata_available=true;}
    static  Project *   get();
            PROG *      binary() {return &prog;}
            SourceMachine *machine();

    const   FunctionListType &functions() const { return pProcList; }
            FunctionListType &functions()       { return pProcList; }

            bool        addCommand(Command *cmd);
            bool        addCommand(PtrFunction f, Command *cmd); // Add function level command
            void        dumpAllErrors();
            void        setLoader(DosLoader *ins);
            void        processCommands(int count=1);
public slots:
            void        onCommandStreamFinished(bool state);
signals:
            void        newFunctionCreated(PtrFunction);
            void        loaderSelected();
            void        commandListChanged();
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
