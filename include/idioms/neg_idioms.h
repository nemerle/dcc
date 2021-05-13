#pragma once
#include "idiom.h"
#include "icode.h"

struct Idiom11 : public Idiom
{
protected:
    iICODE m_icodes[3];
public:
    virtual ~Idiom11() {}
    Idiom11(Function *f) : Idiom(f)
    {
    }
    uint8_t minimum_match_length() {return 3;}
    bool match(iICODE pIcode);
    int action();
};

struct Idiom16 : public Idiom
{
protected:
    iICODE m_icodes[3];
public:
    virtual ~Idiom16() {}
    Idiom16(Function *f) : Idiom(f)
    {
    }
    uint8_t minimum_match_length() {return 3;}
    bool match(iICODE pIcode);
    int action();
};
