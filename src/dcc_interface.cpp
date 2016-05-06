#include "dcc_interface.h"
#include "dcc.h"
#include "project.h"
struct DccImpl : public IDcc {
    PtrFunction m_current_func;
    // IDcc interface
public:
    bool load(QString name)
    {
        option.filename = name;
        Project::get()->create(name);
        return Project::get()->addLoadCommands(name);
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
