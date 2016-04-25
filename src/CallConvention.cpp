#include <ostream>
#include <cassert>
#include "CallConvention.h"
#include <QtCore/QTextStream>

CConv *CConv::create(Type v)
{
    static C_CallingConvention *c_call      = nullptr;
    static Pascal_CallingConvention *p_call = nullptr;
    static Unknown_CallingConvention *u_call= nullptr;
    if(nullptr==c_call)
        c_call = new C_CallingConvention;
    if(nullptr==p_call)
        p_call = new Pascal_CallingConvention;
    if(nullptr==u_call)
        u_call = new Unknown_CallingConvention;
    switch(v) {
    case UNKNOWN: return u_call;
    case C: return c_call;
    case PASCAL: return p_call;
    }
    assert(false);
    return nullptr;
}

void C_CallingConvention::writeComments(QTextStream & ostr)
{
    ostr << " * C calling convention.\n";
}
void Pascal_CallingConvention::writeComments(QTextStream & ostr)
{
    ostr << " * Pascal calling convention.\n";
}
void Unknown_CallingConvention::writeComments(QTextStream & ostr)
{
    ostr << " * Unknown calling convention.\n";
}
