#pragma once
/* Perfect hashing function library. Contains functions to generate perfect
    hashing functions
 * (C) Mike van Emmerik
 */
#include <stdint.h>

/* Prototypes */
void hashCleanup(void);         /* Frees memory allocated by hashParams() */
void map(void);                 /* Part 1 of creating the tables */

/* The application must provide these functions: */
void getKey(int i, uint8_t **pKeys);/* Set *keys to point to the i+1th key */
void dispKey(int i);            /* Display the key */
class PatternHasher
{
    uint16_t    *T1base, *T2base;   /* Pointers to start of T1, T2 */
    int     NumEntry;   /* Number of entries in the hash table (# keys) */
    int     EntryLen;   /* Size (bytes) of each entry (size of keys) */
    int     SetSize;    /* Size of the char set */
    char    SetMin;     /* First char in the set */
    int     NumVert;    /* c times NumEntry */
    int     *graphNode; /* The array of edges */
    int     *graphNext; /* Linked list of edges */
    int     *graphFirst;/* First edge at a vertex */
public:
    uint16_t *readT1(void);             /* Returns a pointer to the T1 table */
    uint16_t *readT2(void);             /* Returns a pointer to the T2 table */
    uint16_t *readG(void);              /* Returns a pointer to the g  table */
    void init(int _NumEntry, int _EntryLen, int _SetSize, char _SetMin,int _NumVert); /* Set the parameters for the hash table */
    void cleanup();
    int hash(unsigned char *string); //!< Hash the string to an int 0 .. NUMENTRY-1
};
extern PatternHasher g_pattern_hasher;
/* Macro reads a LH uint16_t from the image regardless of host convention */
#ifndef LH
#define LH(p)  ((int)((uint8_t *)(p))[0] + ((int)((uint8_t *)(p))[1] << 8))
#endif
