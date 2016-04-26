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
    ilFunction GetCurFuncHandle()
    {
        return m_current_func;
    }
    void analysis_Once()
    {
    }
    void load(QString name)
    {
        option.filename = name;
        Project::get()->create(name);
    }
    void prtout_asm(IXmlTarget *, int level)
    {
    }
    void prtout_cpp(IXmlTarget *, int level)
    {
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
