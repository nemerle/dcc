#include <stdint.h>
/** Perfect hashing function library. Contains functions to generate perfect
    hashing functions */
struct PatternCollector;
struct PerfectHash {
    uint16_t    *T1base;
    uint16_t    *T2base;   /* Pointers to start of T1, T2 */
    short   *g;         /* g[] */

    int     NumEntry;   /* Number of entries in the hash table (# keys) */
    int     EntryLen;   /* Size (bytes) of each entry (size of keys) */
    int     SetSize;    /* Size of the char set */
    char    SetMin;     /* First char in the set */
    int     NumVert;    /* c times NumEntry */
    /** Set the parameters for the hash table */
    void setHashParams(int _numEntry, int _entryLen, int _setSize, char _setMin, int _numVert);

public:
    void map(PatternCollector * collector); /* Part 1 of creating the tables */
    void hashCleanup(); /* Frees memory allocated by setHashParams() */
    void assign(); /* Part 2 of creating the tables */
    int hash(uint8_t *string); /* Hash the string to an int 0 .. NUMENTRY-1 */
    const uint16_t *readT1(void) const { return T1base; }
    const uint16_t *readT2(void) const { return T2base; }
    const uint16_t *readG(void) const  { return (uint16_t *)g; }
    uint16_t *readT1(void){ return T1base; }
    uint16_t *readT2(void){ return T2base; }
    uint16_t *readG(void) { return (uint16_t *)g; }
private:
    void initGraph();
    void addToGraph(int e, int v1, int v2);
    bool isCycle();
    bool DFS(int parentE, int v);
    void traverse(int u);
    PatternCollector *m_collector; /* used to retrieve the keys */

};
