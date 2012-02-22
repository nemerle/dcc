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
typedef std::vector<std::string> strTable;

struct bundle
{
public:
    void appendCode(const char *format, ...);
    void appendDecl(const char *format, ...);
    strTable    decl;   /* Declarations */
    strTable    code;   /* C code       */
};


#define lineSize	360		/* 3 lines in the mean time */

void    newBundle (bundle *procCode);
Int     nextBundleIdx (strTable *strTab);
void	addLabelBundle (strTable &strTab, Int idx, Int label);
void    writeBundle (std::ostream &ios, bundle procCode);
void    freeBundle (bundle *procCode);

