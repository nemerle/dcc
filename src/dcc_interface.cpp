#include "dcc_interface.h"
#include "dcc.h"
#include "project.h"
struct DccImpl : public IDcc {
    ilFunction m_current_func;
    // IDcc interface
public:
    void BaseInit()
    {
        m_current_func = Project::get()->functions().end();
    }
    void Init(QObject *tgt)
    {
    }
    ilFunction GetFirstFuncHandle()
    {
        return Project::get()->functions().begin();
    }
    ilFunction GetNextFuncHandle(ilFunction iter)
    {
        if(iter!=Project::get()->functions().end())
            ++iter;
        return iter;
    }
    ilFunction GetCurFuncHandle()
    {
        return m_current_func;
    }
    void analysis_Once()
    {
        if(m_current_func==Project::get()->functions().end())
            return;
        if(m_current_func->nStep==0) { // unscanned function
        }
    }
    bool load(QString name)
    {
        option.filename = name;
        Project::get()->create(name);
        return Project::get()->addLoadCommands(name);
    }
    void prtout_asm(IStructuredTextTarget *, int level)
    {
//        if (m_Cur_Func->m_nStep == 0)
//            return;

//        XmlOutPro out(iOut);
//        FuncLL the(m_Cur_Func->ll.m_asmlist);
//        the.prtout_asm(m_Cur_Func, &m_Cur_Func->m_varll, &out);
    }
    void prtout_cpp(IStructuredTextTarget *, int level)
    {
    }
    bool isValidFuncHandle(ilFunction f) {
        return f != Project::get()->functions().end();
    }
    size_t getFuncCount()
    {
        return Project::get()->functions().size();
    }
    const lFunction &validFunctions() const
    {
        return Project::get()->functions();
    }
    void SetCurFunc_by_Name(QString v)
    {
        lFunction & funcs(Project::get()->functions());
        for(auto iter=funcs.begin(),fin=funcs.end(); iter!=fin; ++iter) {
            if(iter->name==v) {
                m_current_func = iter;
                return;
            }
        }
    }
    QDir installDir() {
        return QDir(".");
    }
    QDir dataDir(QString kind) { // return directory containing decompilation helper data -> signatures/includes/etc.
        QDir res(installDir());
        res.cd(kind);
        return res;
    }
};

IDcc* IDcc::get() {
    static IDcc *v=0;
    if(nullptr == v)
        v = new DccImpl;
    return v;
}
