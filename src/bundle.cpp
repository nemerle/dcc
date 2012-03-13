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

#define deltaProcLines  20

using namespace std;
/* Allocates memory for a new bundle and initializes it to zero.    */


/* Adds the given label to the start of the line strTab[idx].  The first
 * tab is removed and replaced by this label */
void strTable::addLabelBundle (int idx, int label)
{
    char s[16];
    sprintf (s, "l%d: ", label);
    at(idx) = string(s)+at(idx).substr(4);
}


/* Writes the contents of the string table on the file fp.  */
static void writeStrTab (std::ostream &ios, strTable &strTab)
{
    for (size_t i = 0; i < strTab.size(); i++)
        ios << strTab[i];
}


/* Writes the contents of the bundle (procedure code and declaration) to
 * a file.          */
void writeBundle (std::ostream &ios, bundle procCode)
{
    writeStrTab (ios, procCode.decl);
    writeStrTab (ios, procCode.code);
}


/* Frees the storage allocated by the string table. */
static void freeStrTab (strTable &strTab)
{
    strTab.clear();
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
    code.push_back(buf);
    va_end (args);
}
void bundle::appendCode(const std::string &s)
{
    code.push_back(s);
}

void bundle::appendDecl(const char *format,...)
{
    va_list args;
    char buf[lineSize]={0};
    va_start (args, format);
    vsprintf (buf, format, args);
    decl.push_back(buf);
    va_end (args);
}

void bundle::appendDecl(const std::string &v)
{
    decl.push_back(v);
}



