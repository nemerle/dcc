/*****************************************************************************
 * File: bundle.c
 * Module that handles the bundle type (array of pointers to strings).
 * (C) Cristina Cifuentes
 ****************************************************************************/

#include "dcc.h"
#include <stdarg.h>
#include <iostream>
#include <memory.h>
#include <stdlib.h>
#include <string.h>
#include <QtCore/QIODevice>
#define deltaProcLines  20

using namespace std;
/* Allocates memory for a new bundle and initializes it to zero.    */


/* Adds the given label to the start of the line strTab[idx].  The first
 * tab is removed and replaced by this label */
void strTable::addLabelBundle (int idx, int label)
{
    QString &processedLine(entries.at(idx));
    QString s = QString("l%1: ").arg(label);
    if(processedLine.size()<4)
        processedLine = s;
    else
        processedLine = s+processedLine.mid(4);
}


/* Writes the contents of the string table on the file fp.  */
static void writeStrTab (QIODevice &ios, strTable &strTab)
{
    for (const QString & entr : strTab.entries)
        ios.write(entr.toLatin1());
}


/* Writes the contents of the bundle (procedure code and declaration) to
 * a file.          */
void writeBundle (QIODevice &ios, bundle procCode)
{
    writeStrTab (ios, procCode.decl);
    writeStrTab (ios, procCode.code);
}


/* Frees the storage allocated by the string table. */
static void freeStrTab (strTable &strTab)
{
    strTab.entries.clear();
}


/* Deallocates the space taken by the bundle procCode */
void freeBundle (bundle *procCode)
{
    freeStrTab (procCode->decl);
    freeStrTab (procCode->code);
}

void bundle::appendCode(const char *format,...)
{
    va_list args;
    char buf[lineSize]={0};
    va_start (args, format);
    vsprintf (buf, format, args);
    code.entries.push_back(buf);
    va_end (args);
}
void bundle::appendCode(const QString & s)
{
    code.entries.push_back(s);
}

void bundle::appendDecl(const char *format,...)
{
    va_list args;
    char buf[lineSize]={0};
    va_start (args, format);
    vsprintf (buf, format, args);
    decl.entries.push_back(buf);
    va_end (args);
}

void bundle::appendDecl(const QString &v)
{
    decl.entries.push_back(v);
}



