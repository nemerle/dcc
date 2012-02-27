#pragma once
#include <vector>
#include "idiom.h"
#include "icode.h"
#include <deque>
struct CallIdiom : public Idiom
{
protected:
    int m_param_count;
public:
    virtual ~CallIdiom() {}
    CallIdiom(Function *f) : Idiom(f)
    {
    }

};
struct Idiom3 : public CallIdiom
{
protected:
    iICODE m_icodes[2];
public:
    virtual ~Idiom3() {}
    Idiom3(Function *f) : CallIdiom(f)
    {
    }
    uint8_t minimum_match_length() {return 2;}
    bool match(iICODE pIcode);
    int action();
};
struct Idiom17 : public CallIdiom
{
protected:
    std::vector<iICODE> m_icodes;
public:
    virtual ~Idiom17() {}
    Idiom17(Function *f) : CallIdiom(f)
    {
    }
    uint8_t minimum_match_length() {return 2;}
    bool match(iICODE pIcode);
    int action();
};
