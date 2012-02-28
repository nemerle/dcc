/*
 * Perfect hashing function library. Contains functions to generate perfect
 * hashing functions
 * (C) Mike van Emmerik
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "perfhlib.h"

/* Private data structures */

static  uint16_t    *T1, *T2;   /* Pointers to T1[i], T2[i] */
static  short   *g;         /* g[] */

static  int     numEdges;   /* An edge counter */
//static  bool    *visited;   /* Array of bools: whether visited */

/* Private prototypes */
static void initGraph(void);
static void addToGraph(int e, int v1, int v2);
static bool isCycle(void);
static void duplicateKeys(int v1, int v2);
PatternHasher g_pattern_hasher;

void
PatternHasher::init(int _NumEntry, int _EntryLen, int _SetSize, char _SetMin,
                    int _NumVert)
{
    /* These parameters are stored in statics so as to obviate the need for
        passing all these (or defererencing pointers) for every call to hash()
    */

    NumEntry = _NumEntry;
    EntryLen = _EntryLen;
    SetSize  = _SetSize;
    SetMin   = _SetMin;
    NumVert  = _NumVert;

    /* Allocate the variable sized tables etc */
    T1base = new uint16_t [EntryLen * SetSize];
    T2base = new uint16_t [EntryLen * SetSize];
    graphNode = new int [NumEntry*2 + 1];
    graphNext = new int [NumEntry*2 + 1];
    graphFirst = new int [NumVert + 1];
    g = new short [NumVert + 1];
//    visited = new bool [NumVert + 1];
    return;

BadAlloc:
    printf("Could not allocate memory\n");
    cleanup();
    exit(1);
}

void PatternHasher::cleanup(void)
{
    /* Free the storage for variable sized tables etc */
    delete [] T1base;
    delete [] T2base;
    delete [] graphNode;
    delete [] graphNext;
    delete [] graphFirst;
    delete [] g;
//    delete [] visited;
}

int PatternHasher::hash(uint8_t *string)
{
    uint16_t u, v;
    int  j;

    u = 0;
    for (j=0; j < EntryLen; j++)
    {
        T1 = T1base + j * SetSize;
        u += T1[string[j] - SetMin];
    }
    u %= NumVert;

    v = 0;
    for (j=0; j < EntryLen; j++)
    {
        T2 = T2base + j * SetSize;
        v += T2[string[j] - SetMin];
    }
    v %= NumVert;

    return (g[u] + g[v]) % NumEntry;
}

uint16_t * PatternHasher::readT1(void)
{
    return T1base;
}

uint16_t *PatternHasher::readT2(void)
{
    return T2base;
}

uint16_t * PatternHasher::readG(void)
{
    return (uint16_t *)g;
}

