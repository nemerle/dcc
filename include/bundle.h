/*****************************************************************************
 * Project: dcc
 * File:    bundle.h
 * Purpose: Module to handle the bundle type (array of pointers to strings).
 * (C) Cristina Cifuentes
 ****************************************************************************/
#pragma once
#include <stdio.h>
#include <vector>
#include <string>
struct strTable : std::vector<std::string>
{
    /* Returns the next available index into the table */
    size_t nextIdx() {return size();}
public:
    void addLabelBundle(int idx, int label);
};

struct bundle
{
public:
    void appendCode(const char *format, ...);
    void appendCode(const std::string &s);
    void appendDecl(const char *format, ...);
    void appendDecl(const std::string &);
    void init()
    {
        decl.clear();
        code.clear();
    }
    strTable    decl;   /* Declarations */
    strTable    code;   /* C code       */
    int current_indent;
};

extern bundle cCode;
#define lineSize	360		/* 3 lines in the mean time */

//void    newBundle (bundle *procCode);
void    writeBundle (std::ostream &ios, bundle procCode);
void    freeBundle (bundle *procCode);

