#pragma once
#include <vector>
#include "idiom.h"
#include "icode.h"
#include <deque>

struct Idiom21 : public Idiom
{
protected:
    iICODE m_icodes[2];
public:
    virtual ~Idiom21() {}
    Idiom21(Function *f) : Idiom(f)
    {
    }
    uint8_t minimum_match_length() {return 2;}
    bool match(iICODE pIcode);
    int action();
};

struct Idiom7 : public Idiom
{
protected:
    iICODE m_icode;
public:
    virtual ~Idiom7() {}
    Idiom7(Function *f) : Idiom(f)
    {
    }
    uint8_t minimum_match_length() {return 1;}
    bool match(iICODE pIcode);
    int action();
};
struct Idiom10 : public Idiom
{
protected:
    iICODE m_icodes[2];
public:
    virtual ~Idiom10() {}
    Idiom10(Function *f) : Idiom(f)
    {
    }
    uint8_t minimum_match_length() {return 1;}
    bool match(iICODE pIcode);
    int action();
};
