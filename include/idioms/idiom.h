#pragma once
#include "icode.h"
#include "Procedure.h"
struct Idiom
{
protected:
    Function *m_func;
    iICODE m_end;
public:
    Idiom(Function *f) : m_func(f),m_end(f->Icode.end())
    {
    }
    virtual uint8_t minimum_match_length()=0;
    virtual bool match(iICODE at)=0;
    virtual int action()=0;
    int operator ()(iICODE at)
    {
        if(match(at))
            return action();
        return 1;
    }
};
