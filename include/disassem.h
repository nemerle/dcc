/*
***************************************************************************
   dcc project disassembler header
  (C) Mike van Emmerik
***************************************************************************
*/
#pragma once
#include "bundle.h"

#include <fstream>
#include <vector>
#include <QString>
#include <QTextStream>
struct LLInst;
struct Function;
struct Disassembler
{
protected:
    int pass;
    int g_lab;
    //bundle &cCode;
    QIODevice *m_disassembly_target;
    QTextStream m_fp;
    std::vector<std::string> m_decls;
    std::vector<std::string> m_code;

public:
    Disassembler(int _p) : pass(_p)
    {
        g_lab=0;
    }
public:
    void disassem(Function *ppProc);
    void disassem(Function *ppProc, int i);
    void dis1Line(LLInst &inst, int loc_ip, int pass);
};
