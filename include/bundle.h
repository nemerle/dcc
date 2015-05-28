/*****************************************************************************
 * Project: dcc
 * File:    bundle.h
 * Purpose: Module to handle the bundle type (array of pointers to strings).
 * (C) Cristina Cifuentes
 ****************************************************************************/

#include <stdio.h>

typedef struct {
    Int     numLines;   /* Number of lines in the table   */
    Int     allocLines; /* Number of lines allocated in the table */
    char    **str;      /* Table of strings */
} strTable;


typedef struct {
    strTable    decl;   /* Declarations */
    strTable    code;   /* C code       */
} bundle;


#define lineSize	360		/* 3 lines in the mean time */

void    newBundle (bundle *procCode);
void    appendStrTab (strTable *strTab, char *format, ...);
Int		nextBundleIdx (strTable *strTab);
void	addLabelBundle (strTable *strTab, Int idx, Int label);
void    writeBundle (FILE *fp, bundle procCode);
void    freeBundle (bundle *procCode);

