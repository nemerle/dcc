#include <utility>
#include "dcc.h"
#include "project.h"
#include "Procedure.h"
using namespace std;
//Project g_proj;
char    *asm1_name, *asm2_name;     /* Assembler output filenames     */
SYMTAB  symtab;             /* Global symbol table      			  */
STATS   stats;              /* cfg statistics       				  */
//PROG    prog;               /* programs fields      				  */
OPTION  option;             /* Command line options     			  */
Project *Project::s_instance = 0;
Project::Project() : callGraph(nullptr)
{

}
void Project::initialize()
{
    delete callGraph;
    callGraph = nullptr;
}
void Project::create(const string &a)
{
    m_fname=a;
    string::size_type ext_loc=a.find_last_of('.');
    string::size_type slash_loc=a.find_last_of('/',ext_loc);
    if(slash_loc==string::npos)
        slash_loc=0;
    else
        slash_loc++;
    if(ext_loc!=string::npos)
        m_project_name = a.substr(slash_loc,(ext_loc-slash_loc));
    else
        m_project_name = a.substr(slash_loc);
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
        [entry](const Function &f) ->
            bool { return f.procEntry==entry; });
return iter;
}

ilFunction Project::createFunction()
{
    pProcList.push_back(Function::Create());
    return (++pProcList.rbegin()).base();
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

const std::string &Project::symbolName(size_t idx)
{
    assert(validSymIdx(idx));
    return symtab[idx].name;
}
Project *Project::get()
{
    //WARNING: poor man's singleton, not thread safe
    if(s_instance==0)
        s_instance=new Project;
    return s_instance;
}


SourceMachine *Project::machine()
{
    return nullptr;
}

