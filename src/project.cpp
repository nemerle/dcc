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
Project::Project() :
    m_selected_loader(nullptr),
    callGraph(nullptr),
    m_pattern_locator(nullptr)
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
    // TODO: reset all state.
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
                             [to_find](const PtrFunction &f)->bool {return to_find->shared_from_this()==f;});
    assert(iter!=pProcList.end());
    return iter;
}

PtrFunction Project::findByEntry(uint32_t entry)
{
    /* Search procedure list for one with appropriate entry point */
    ilFunction iter= std::find_if(pProcList.begin(),pProcList.end(),
        [entry](const PtrFunction &f)  { return f->procEntry==entry; });
    if(iter==pProcList.end())
        return nullptr;
    return *iter;
}

/**
 * \brief Search procedure list for one with given name
 */
PtrFunction Project::findByName(const QString &name)
{
    ilFunction iter= std::find_if(pProcList.begin(),pProcList.end(),
        [name](const PtrFunction &f)  { return f->name==name; });
    if(iter==pProcList.end())
        return nullptr;
    return *iter;
}
PtrFunction Project::createFunction(FunctionType *f,const QString &name,SegOffAddr addr)
{
    PtrFunction func(Function::Create(f,0,name,0));
    pProcList.push_back(func);
    // FIXME: use provided segment addr !
    func->procEntry = addr.addr;
    if(!callGraph) {
        /* Set up call graph initial node */
        callGraph = new CALL_GRAPH;
        callGraph->proc = func;
        /* The entry state info is for the first procedure */
        func->state = m_entry_state;

    }

    emit newFunctionCreated(func);
    return func;
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

bool Project::addCommand(Command *cmd) {
    bool res = m_project_command_stream.add(cmd);
    emit commandListChanged();
    return res;
}

bool Project::addCommand(PtrFunction f, Command *cmd)
{
    bool res = m_function_streams[f].add(cmd);
    emit commandListChanged();
    return res;
}

bool Project::hasCommands(const PtrFunction & f)
{
    auto iter = m_function_streams.find(f);
    if(iter!=m_function_streams.end()) {
        return not iter->second.isEmpty();
    }
    return false;
}

CommandStream *Project::functionCommands(const PtrFunction & f)
{
    if(f==nullptr)
        return nullptr;
    auto iter = m_function_streams.find(f);
    if(iter!=m_function_streams.end()) {
        return &iter->second;
    }
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
    m_command_ctx.m_project = this;
    m_project_command_stream.processAll(&m_command_ctx);
    emit commandListChanged();
}
void Project::processCommands(int count) {
    m_command_ctx.m_project = this;
    while(count--) {
        if(false==m_project_command_stream.processOne(&m_command_ctx)) {
            break;
        }
    }
    emit commandListChanged();
}
void Project::processFunctionCommands(const PtrFunction &func,int count) {
    m_command_ctx.m_project = this;
    m_command_ctx.m_func    = func;
    CommandStream *cs = functionCommands(func);
    if(nullptr==cs)
        return;
    while(count--) {
        if(false==cs->processOne(&m_command_ctx)) {
            break;
        }
    }
    emit commandListChanged();
    emit functionUpdate(func);
}
void Project::resetCommandsAndErrorState()
{
    m_error_state = false;
    m_command_ctx.reset();
    m_command_ctx.m_project = this;
    m_project_command_stream.clear();
}
