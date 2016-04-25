#include "dcc_interface.h"
#include "dcc.h"
#include "project.h"
struct DccImpl : public IDcc{


    // IDcc interface
public:
    void BaseInit()
    {
    }
    void Init(QObject *tgt)
    {
    }
    ilFunction GetFirstFuncHandle()
    {
    }
    ilFunction GetCurFuncHandle()
    {
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
    }
    const lFunction &validFunctions() const
    {
        return Project::get()->functions();
    }
    void SetCurFunc_by_Name(QString)
    {
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
