#pragma once
#include "idiom.h"
#include "icode.h"

struct Idiom8 : public Idiom
{
protected:
    iICODE m_icodes[2];
    uint8_t m_loaded_reg;
public:
    virtual ~Idiom8() {}
    Idiom8(Function *f) : Idiom(f)
    {
    }
    uint8_t minimum_match_length() {return 2;}
    bool match(iICODE pIcode);
    int action();
};

struct Idiom15 : public Idiom
{
protected:
    std::vector<iICODE> m_icodes;
public:
    virtual ~Idiom15() {}
    Idiom15(Function *f) : Idiom(f)
    {
    }
    uint8_t minimum_match_length() {return 2;}
    bool match(iICODE pIcode);
    int action();
};

struct Idiom12 : public Idiom
{
protected:
    iICODE m_icodes[2];
    uint8_t m_loaded_reg;
public:
    virtual ~Idiom12() {}
    Idiom12(Function *f) : Idiom(f)
    {
    }
    uint8_t minimum_match_length() {return 2;}
    bool match(iICODE pIcode);
    int action();
};


struct Idiom9 : public Idiom
{
protected:
    iICODE m_icodes[2];
    uint8_t m_loaded_reg;
public:
    virtual ~Idiom9() {}
    Idiom9(Function *f) : Idiom(f)
    {
    }
    uint8_t minimum_match_length() {return 2;}
    bool match(iICODE pIcode);
    int action();
};

