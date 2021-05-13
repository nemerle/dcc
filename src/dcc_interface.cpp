#include "dcc_interface.h"
#include "dcc.h"
#include "project.h"
struct DccImpl : public IDcc {
    ilFunction m_current_func;
    // IDcc interface
public:
    void BaseInit() override
    {
        m_current_func = Project::get()->functions().end();
    }
    void Init(QObject *tgt) override
    {
    }
    ilFunction GetFirstFuncHandle() override
    {
        return Project::get()->functions().begin();
    }
    ilFunction GetCurFuncHandle() override
    {
        return m_current_func;
    }
    void analysis_Once() override
    {
    }
    void load(QString name) override
    {
        option.filename = name;
        Project::get()->create(name);
    }
    void prtout_asm(IXmlTarget *, int level) override
    {
    }
    void prtout_cpp(IXmlTarget *, int level) override
    {
    }
    size_t getFuncCount() override
    {
        return Project::get()->functions().size();
    }
    const lFunction &validFunctions() const override
    {
        return Project::get()->functions();
    }
    void SetCurFunc_by_Name(QString v) override
    {
        lFunction & funcs(Project::get()->functions());
        for(auto iter=funcs.begin(),fin=funcs.end(); iter!=fin; ++iter) {
            if(iter->name==v) {
                m_current_func = iter;
                return;
            }
        }
    }
    QDir installDir() override {
        return QDir(".");
    }
    QDir dataDir(QString kind) override { // return directory containing decompilation helper data -> signatures/includes/etc.
        QDir res(installDir());
        res.cd(kind);
        return res;
    }
};

IDcc* IDcc::get() {
    static IDcc *v=nullptr;
    if(nullptr == v)
        v = new DccImpl;
    return v;
}
