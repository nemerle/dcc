#pragma once
#include "idiom.h"
#include "icode.h"

struct Idiom5 : public Idiom
{
protected:
    iICODE m_icodes[2];
public:
    virtual ~Idiom5() {}
    Idiom5(Function *f) : Idiom(f)
    {
    }
    uint8_t minimum_match_length() {return 2;}
    bool match(iICODE pIcode);
    int action();
};

struct Idiom6 : public Idiom
{
protected:
    iICODE m_icodes[2];
public:
    virtual ~Idiom6() {}
    Idiom6(Function *f) : Idiom(f)
    {
    }
    uint8_t minimum_match_length() {return 2;}
    bool match(iICODE pIcode);
    int action();
};

struct Idiom18 : public Idiom
{
protected:
    iICODE m_icodes[4];
    bool m_is_dec;
    /* type of variable: 1 = reg-var, 2 = local */
    int m_idiom_type;
public:
    Idiom18(Function *f) : Idiom(f)
    {
    }
    uint8_t minimum_match_length() {return 4;}
    bool match(iICODE picode);
    int action();
};

struct Idiom19 : public Idiom
{
protected:
    iICODE m_icodes[2];
    bool m_is_dec;
public:
    Idiom19(Function *f) : Idiom(f)
    {
    }
    uint8_t minimum_match_length() {return 2;}
    bool match(iICODE picode);
    int action();
};

struct Idiom20 : public Idiom
{
protected:
    iICODE m_icodes[4];
    condNodeType m_is_dec;
public:
    Idiom20(Function *f) : Idiom(f)
    {
    }
    uint8_t minimum_match_length() {return 4;}
    bool match(iICODE picode);
    int action();
};
