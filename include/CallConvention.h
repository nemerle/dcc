#pragma once
#include "ast.h"

class QTextStream;

struct CConv {
    enum CC_Type {
        UNKNOWN=0,
        C,
        PASCAL
    };
    virtual void processHLI(Function *func, Expr *_exp, iICODE picode)=0;
    //! given return and argument types fill Function's STKFRAME and return locations
    virtual void calculateStackLayout(Function *func)=0;
    virtual void writeComments(QTextStream &)=0;
    static CConv * create(CC_Type v);
protected:

};

struct C_CallingConvention : public CConv {
    virtual void processHLI(Function *func, Expr *_exp, iICODE picode) override;
    virtual void writeComments(QTextStream &) override;
    void calculateStackLayout(Function *func) override;

private:
    int processCArg(Function *callee, Function *pProc, ICODE *picode, size_t numArgs);
};
struct Pascal_CallingConvention : public CConv {
    virtual void processHLI(Function *func, Expr *_exp, iICODE picode) override;
    virtual void writeComments(QTextStream &) override;
    void calculateStackLayout(Function *func) override;
};
struct Unknown_CallingConvention : public CConv {
    void processHLI(Function *func, Expr *_exp, iICODE picode) override {}
    void calculateStackLayout(Function *func) override;
    virtual void writeComments(QTextStream &) override;
};
