/*
***************************************************************************
   dcc project disassembler header
  (C) Mike van Emmerik
***************************************************************************
*/
#pragma once
#include "bundle.h"

#include <memory>
#include <fstream>
#include <vector>
#include <QString>
#include <QTextStream>

struct LLInst;
struct Function;
typedef std::shared_ptr<Function> PtrFunction;

struct Disassembler
{
protected:
    int pass=0;
    int g_lab=0;
    QIODevice *m_disassembly_target=nullptr;
    QTextStream m_fp;
    std::vector<std::string> m_decls;
    std::vector<std::string> m_code;

public:
    Disassembler(int _p) : pass(_p)
    {
    }

    void disassem(PtrFunction ppProc);
    void disassem(PtrFunction ppProc, int i);
    void dis1Line(LLInst &inst, int loc_ip, int pass);
};
/* Definitions for extended keys (first key is zero) */

#define EXT			0x100		/* "Extended" flag */

#ifdef __UNIX__
#define KEY_DOWN	EXT+'B'
#define KEY_LEFT	EXT+'D'
#define KEY_UP		EXT+'A'
#define KEY_RIGHT	EXT+'C'
#define KEY_NPAGE	EXT+'J'		/* Enter correct value! */
#define KEY_PPAGE	EXT+'K'		/* Another guess! */
#endif

/* "Attributes" */
#define A_NORMAL	'N'			/* For Dos/Unix */
#define A_REVERSE	'I'
#define A_BOLD		'B'

#define LINES 24
#define COLS 80
