#include "dcc_interface.h"
#include "dcc.h"
#include "project.h"
struct DccImpl : public IDcc {
    PtrFunction m_current_func;
    // IDcc interface
public:
    void BaseInit()
    {
        m_current_func = nullptr;
    }
    void Init(QObject *tgt)
    {
    }
    PtrFunction GetCurFuncHandle() {
        return m_current_func;
    }
    void analysis_Once()
    {
        if(m_current_func==nullptr)
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
    void prtout_asm(PtrFunction f,IStructuredTextTarget *tgt, int level) override
    {
//        if (m_current_func->nStep == 0)
//            return;

//        XmlOutPro out(iOut);
//        FuncLL the(m_Cur_Func->ll.m_asmlist);
//        the.prtout_asm(m_Cur_Func, &m_Cur_Func->m_varll, &out);
        f->toStructuredText(tgt,level);
    }
    void prtout_cpp(PtrFunction f,IStructuredTextTarget *, int level) override
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
        PtrFunction p(Project::get()->findByName(v));
        if(p!=nullptr)
            m_current_func = p;
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
