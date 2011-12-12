/* Perfect hashing function library. Contains functions to generate perfect
    hashing functions
 * (C) Mike van Emmerik
 */


#define TRUE 1
#define FALSE 0
#define bool unsigned char
#define byte unsigned char
#define word unsigned short

/* Prototypes */
void hashCleanup(void);         /* Frees memory allocated by hashParams() */
void map(void);                 /* Part 1 of creating the tables */

/* The application must provide these functions: */
void getKey(int i, byte **pKeys);/* Set *keys to point to the i+1th key */
void dispKey(int i);            /* Display the key */
class PatternHasher
{
    word    *T1base, *T2base;   /* Pointers to start of T1, T2 */
    int     NumEntry;   /* Number of entries in the hash table (# keys) */
    int     EntryLen;   /* Size (bytes) of each entry (size of keys) */
    int     SetSize;    /* Size of the char set */
    char    SetMin;     /* First char in the set */
    int     NumVert;    /* c times NumEntry */
    int     *graphNode; /* The array of edges */
    int     *graphNext; /* Linked list of edges */
    int     *graphFirst;/* First edge at a vertex */
public:
    word *readT1(void);             /* Returns a pointer to the T1 table */
    word *readT2(void);             /* Returns a pointer to the T2 table */
    word *readG(void);              /* Returns a pointer to the g  table */
    void init(int _NumEntry, int _EntryLen, int _SetSize, char _SetMin,int _NumVert); /* Set the parameters for the hash table */
    void cleanup();
    int hash(unsigned char *string); //!< Hash the string to an int 0 .. NUMENTRY-1
};
extern PatternHasher g_pattern_hasher;
/* Macro reads a LH word from the image regardless of host convention */
#ifndef LH
#define LH(p)  ((int)((byte *)(p))[0] + ((int)((byte *)(p))[1] << 8))
#endif
