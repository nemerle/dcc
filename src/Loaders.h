#ifndef LOADERS_H
#define LOADERS_H

#include "BinaryImage.h"
#include <QtCore/QFile>
#include <stdlib.h>

struct DosLoader {
protected:
    void prepareImage(PROG &prog,size_t sz,QFile &fp);
public:
    virtual bool canLoad(QFile &fp)=0;
    virtual QString loaderName() const =0;
    virtual bool load(PROG &prog,QFile &fp)=0;
};
struct ComLoader : public DosLoader {
    virtual ~ComLoader() {}

    bool canLoad(QFile &fp) override;
    bool load(PROG &prog,QFile &fp) override;
    QString loaderName() const override { return "16-bit DOS - COM loader"; }
};
struct ExeLoader : public DosLoader {
    virtual ~ExeLoader() {}

    bool canLoad(QFile &fp) override;
    bool load(PROG &prog,QFile &fp) override;
    QString loaderName() const override { return "16-bit DOS - EXE loader"; }
};

#endif // LOADERS_H
