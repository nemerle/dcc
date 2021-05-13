/*****************************************************************************
 * Project: dcc
 * File:    bundle.h
 * Purpose: Module to handle the bundle type (array of pointers to strings).
 * (C) Cristina Cifuentes
 ****************************************************************************/
#pragma once

#include <QtCore/QString>
#include <stdio.h>
#include <vector>

class QIODevice;

struct strTable
{
    /* Returns the next available index into the table */
    size_t nextIdx() {return entries.size();}
public:
    void addLabelBundle(int idx, int label);
    std::vector<QString> entries;
};

struct bundle
{
public:
    void appendCode(const char *format, ...);
    void appendCode(const QString &s);
    void appendDecl(const char *format, ...);
    void appendDecl(const QString &);
    void init()
    {
        decl.entries.clear();
        code.entries.clear();
    }
    strTable    decl;   /* Declarations */
    strTable    code;   /* C code       */
    int current_indent;
};

extern bundle cCode;
#define lineSize	360		/* 3 lines in the mean time */

//void    newBundle (bundle *procCode);
void    writeBundle (QIODevice & ios, bundle procCode);
void    freeBundle (bundle *procCode);

