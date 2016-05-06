#pragma once
#include "Procedure.h"

#include <QtCore/QObject>
#include <QtCore/QDir>
#include <llvm/ADT/ilist.h>

class IStructuredTextTarget;

struct IDcc {
    static IDcc *get();
    virtual void BaseInit()=0;
    virtual bool load(QString name)=0; // load and preprocess -> find entry point
    virtual const lFunction &validFunctions() const =0;
    virtual QDir installDir()=0;
    virtual QDir dataDir(QString kind)=0;
};
