#pragma once
#include "Procedure.h"

#include <QtCore/QObject>
#include <QtCore/QDir>
#include <llvm/ADT/ilist.h>

class IXmlTarget;

struct IDcc {
    static IDcc *get();
    virtual void BaseInit()=0;
    virtual void Init(QObject *tgt)=0;
    virtual lFunction::iterator GetFirstFuncHandle()=0;
    virtual lFunction::iterator GetCurFuncHandle()=0;
    virtual void analysis_Once()=0;
    virtual void load(QString name)=0; // load and preprocess -> find entry point
    virtual void prtout_asm(IXmlTarget *,int level=0)=0;
    virtual void prtout_cpp(IXmlTarget *,int level=0)=0;
    virtual size_t getFuncCount()=0;
    virtual const lFunction &validFunctions() const =0;
    virtual void SetCurFunc_by_Name(QString )=0;
    virtual QDir installDir()=0;
    virtual QDir dataDir(QString kind)=0;
};
