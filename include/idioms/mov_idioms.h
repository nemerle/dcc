#pragma once
#include <vector>
#include "idiom.h"
#include "icode.h"
#include <deque>

struct Idiom14 : public Idiom
{
protected:
    iICODE m_icodes[2];
    byte m_regL;
    byte m_regH;
public:
    virtual ~Idiom14() {}
    Idiom14(Function *f) : Idiom(f),m_regL(0),m_regH(0)
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
    byte m_loaded_reg;
public:
    virtual ~Idiom13() {}
    Idiom13(Function *f) : Idiom(f)
    {
    }
    uint8_t minimum_match_length() {return 2;}
    bool match(iICODE pIcode);
    int action();
};
