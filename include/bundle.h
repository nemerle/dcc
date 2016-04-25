/*****************************************************************************
 * Project: dcc
 * File:    bundle.h
 * Purpose: Module to handle the bundle type (array of pointers to strings).
 * (C) Cristina Cifuentes
 ****************************************************************************/
#pragma once
#include <stdio.h>
#include <vector>
#include <QtCore/QString>
#include <QtCore/QIODevice>

struct strTable : std::vector<QString>
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
    void appendCode(const QString &s);
    void appendDecl(const char *format, ...);
    void appendDecl(const QString &);
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
void    writeBundle (QIODevice & ios, bundle procCode);
void    freeBundle (bundle *procCode);

