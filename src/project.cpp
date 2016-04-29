#include "dcc.h"
#include "CallGraph.h"
#include "project.h"
#include "Procedure.h"

#include <QtCore/QString>
#include <QtCore/QDir>
#include <QtCore/QDebug>
#include <utility>

using namespace std;
QString asm1_name, asm2_name;     /* Assembler output filenames     */
SYMTAB  symtab;             /* Global symbol table                  */
STATS   stats;              /* cfg statistics                       */
//PROG    prog;               /* programs fields                    */
OPTION  option;             /* Command line options                 */
Project *Project::s_instance = nullptr;
Project::Project() : callGraph(nullptr)
{
    m_project_command_stream.setMaximumCommandCount(10);
    connect(&m_project_command_stream,SIGNAL(streamCompleted(bool)),SLOT(onCommandStreamFinished(bool)));
    memset(&prog,0,sizeof(prog));
}
void Project::initialize()
{
    resetCommandsAndErrorState();
    delete callGraph;
    callGraph = nullptr;
}
void Project::create(const QString &a)
{
    initialize();
    QFileInfo fi(a);
    m_fname=a;
    m_project_name = fi.completeBaseName();
    m_output_path = fi.path();
}

QString Project::output_name(const char *ext) {
    return m_output_path+QDir::separator()+m_project_name+"."+ext;
}
bool Project::valid(ilFunction iter)
{
    return iter!=pProcList.end();
}
ilFunction Project::funcIter(Function *to_find)
{
    auto iter=std::find_if(pProcList.begin(),pProcList.end(),
                             [to_find](const Function &f)->bool {return to_find==&f;});
    assert(iter!=pProcList.end());
    return iter;
}

ilFunction Project::findByEntry(uint32_t entry)
{
    /* Search procedure list for one with appropriate entry point */
    ilFunction iter= std::find_if(pProcList.begin(),pProcList.end(),
        [entry](const Function &f)  { return f.procEntry==entry; });
return iter;
}
ilFunction Project::createFunction(FunctionType *f,const QString &name,SegOffAddr addr)
{
    pProcList.push_back(Function::Create(f,0,name,0));
    ilFunction iter = (++pProcList.rbegin()).base();
    // FIXME: use provided segment addr !
    iter->procEntry = addr.addr;
    if(!callGraph) {
        /* Set up call graph initial node */
        callGraph = new CALL_GRAPH;
        callGraph->proc = iter;
        /* The entry state info is for the first procedure */
        iter->state = m_entry_state;

    }

    emit newFunctionCreated(*iter);
    return iter;
}

int Project::getSymIdxByAdd(uint32_t adr)
{
    size_t i;
    for (i = 0; i < symtab.size(); i++)
        if (symtab[i].label == adr)
            break;
    return i;
}
bool Project::validSymIdx(size_t idx)
{
    return idx<symtab.size();
}
const SYM &Project::getSymByIdx(size_t idx) const
{
    return symtab[idx];
}
size_t Project::symbolSize(size_t idx)
{
    assert(validSymIdx(idx));
    return symtab[idx].size;
}
hlType Project::symbolType(size_t idx)
{
    assert(validSymIdx(idx));
    return symtab[idx].type;
}

const QString &Project::symbolName(size_t idx)
{
    assert(validSymIdx(idx));
    return symtab[idx].name;
}
Project *Project::get()
{
    //WARNING: poor man's singleton, not thread safe
    if(s_instance==nullptr)
        s_instance=new Project;
    return s_instance;
}


SourceMachine *Project::machine()
{
    return nullptr;
}

void Project::onCommandStreamFinished(bool state)
{
    if(false==state) {
        m_error_state = true;
    }
}
void Project::dumpAllErrors() {
    for(QPair<Command *,QString> v : m_command_ctx.m_failures) {
        qDebug() << QString("%1 command failed with : %2").arg(v.first->name()).arg(v.second);
    }
}

void Project::setLoader(DosLoader * ldr)
{
    m_selected_loader = ldr;
    emit loaderSelected();
}
bool Project::addLoadCommands(QString fname)
{
    if(!addCommand(new LoaderSelection(fname)))
        return false;
    if(!addCommand(new LoaderApplication(fname))) {
        return false;
    }
    return true;
}

void Project::processAllCommands()
{
    m_command_ctx.proj = this;
    m_project_command_stream.processAll(&m_command_ctx);

}

void Project::resetCommandsAndErrorState()
{
    m_error_state = false;
    m_command_ctx.reset();
    m_command_ctx.proj = this;
    m_project_command_stream.clear();
}
