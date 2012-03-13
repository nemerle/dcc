#include <utility>
#include "project.h"
#include "Procedure.h"
Project g_proj;
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
Project *get()
{
    return &g_proj;
}
