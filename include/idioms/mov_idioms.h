#pragma once
#include "idiom.h"
#include "icode.h"

struct Idiom14 : public Idiom
{
protected:
    iICODE m_icodes[2];
    eReg m_regL;
    eReg m_regH;
public:
    virtual ~Idiom14() {}
    Idiom14(Function *f) : Idiom(f),m_regL(rUNDEF),m_regH(rUNDEF)
    {
    }
    uint8_t minimum_match_length() {return 2;}
    bool match(iICODE pIcode);
    int action();
};

struct Idiom13 : public Idiom
{
protected:
    iICODE m_icodes[2];
    eReg m_loaded_reg;
public:
    virtual ~Idiom13() {}
    Idiom13(Function *f) : Idiom(f)
    {
    }
    uint8_t minimum_match_length() {return 2;}
    bool match(iICODE pIcode);
    int action();
};
