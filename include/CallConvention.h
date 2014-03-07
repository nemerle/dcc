#pragma once
#include "ast.h"

struct CConv {
    enum Type {
        UNKNOWN=0,
        C,
        PASCAL
    };
    virtual void processHLI(Function *func, Expr *_exp, iICODE picode)=0;
    virtual void writeComments(std::ostream &)=0;
    static CConv * create(Type v);
protected:

};

struct C_CallingConvention : public CConv {
    virtual void processHLI(Function *func, Expr *_exp, iICODE picode);
    virtual void writeComments(std::ostream &);

private:
    int processCArg(Function *callee, Function *pProc, ICODE *picode, size_t numArgs);
};
struct Pascal_CallingConvention : public CConv {
    virtual void processHLI(Function *func, Expr *_exp, iICODE picode);
    virtual void writeComments(std::ostream &);
};
struct Unknown_CallingConvention : public CConv {
    void processHLI(Function *func, Expr *_exp, iICODE picode) {}
    virtual void writeComments(std::ostream &);
};
