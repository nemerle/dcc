#pragma once

#include <list>
#include <stdint.h>

struct ICODE;
struct Function;

using iICODE=std::list<ICODE>::iterator;

struct Idiom
{
protected:
    Function *m_func;
    iICODE m_end;
public:
    Idiom(Function *f);
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

