#pragma once

#include "PatternCollector.h"

struct LIB_PatternCollector : public PatternCollector
{
protected:
    unsigned long offset;
    uint8_t lnum = 0;				/* Count of LNAMES  so far */
    uint8_t segnum = 0;			/* Count of SEGDEFs so far */
    uint8_t codeLNAMES;			/* Index of the LNAMES for "CODE" class */
    uint8_t codeSEGDEF;			/* Index of the first SEGDEF that has class CODE */
    #define NONE 0xFF			/* Improbable segment index */
    uint8_t *leData;            /* Pointer to 64K of alloc'd data. Some .lib files
                                   have the symbols (PUBDEFs) *after* the data
                                   (LEDATA), so you need to keep the data here */
    uint16_t maxLeData;			/* How much data we have in there */
    /* read a length then string to buf[]; make it an asciiz string */
    void readString( FILE *fl);

public:
    /* Read the .lib file, and put the keys into the array *keys[]. Returns the count */
    int readSyms(FILE *fl);

};
