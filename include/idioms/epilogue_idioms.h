#pragma once
#include "idiom.h"
#include "icode.h"

#include <deque>

struct EpilogIdiom : public Idiom
{
protected:
    std::deque<iICODE> m_icodes; // deque to push_front optional icodes from popStkVars
    void popStkVars (iICODE pIcode);
public:
    virtual ~EpilogIdiom() {}
    EpilogIdiom(Function *f) : Idiom(f)
    {
    }

};
struct Idiom2 : public EpilogIdiom
{
    virtual ~Idiom2() {}
    Idiom2(Function *f) : EpilogIdiom(f)
    {
    }
    uint8_t minimum_match_length() {return 3;}
    bool match(iICODE pIcode);
    int action();
};
struct Idiom4 : public EpilogIdiom
{
protected:
    int m_param_count;
public:
    virtual ~Idiom4() {}
    Idiom4(Function *f) : EpilogIdiom(f)
    {
    }
    uint8_t minimum_match_length() {return 1;}
    bool match(iICODE pIcode);
    int action();
};
