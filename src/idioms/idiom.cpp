#include "idiom.h"
#include "Procedure.h"

Idiom::Idiom(Function *f) : m_func(f),m_end(f->Icode.entries.end())
{
}
