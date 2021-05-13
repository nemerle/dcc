/* Quick program to see if a pattern is in a sig file. Pattern is supplied
    in a small .bin or .com style file */

#include "perfhlib.h"

#include <memory.h>
#include <stdio.h>
#include <stdlib.h>

/* statics */
uint8_t buf[100];
int numKeys; /* Number of hash table entries (keys) */
int numVert; /* Number of vertices in the graph (also size of g[]) */
int PatLen;  /* Size of the keys (pattern length) */
int SymLen;  /* Max size of the symbols, including null */
FILE *f;     /* Sig file being read */
FILE *fpat;  /* Pattern file being read */

static uint16_t *T1base, *T2base; /* Pointers to start of T1, T2 */
static uint16_t *g;               /* g[] */

#define SYMLEN 16
#define PATLEN 23

struct HT {
    /* Hash table structure */
    char htSym[SYMLEN];
    uint8_t htPat[PATLEN];
};

HT *ht; /* Declare a pointer to a hash table */

/* prototypes */
void grab(int n);
uint16_t readFileShort(void);
void cleanup(void);
extern void fixWildCards(uint8_t pat[]); /* In fixwild.c */
void pattSearch(void);
PerfectHash g_pattern_hasher;

int main(int argc, char *argv[]) {
    uint16_t w, len;
    int h, i;
    int patlen;

    if (argc <= 2) {
        printf("Usage: srchsig <SigFilename> <PattFilename>\n");
        printf("Searches the signature file for the given pattern\n");
        printf("e.g. %s dccm8s.sig mypatt.bin\n", argv[0]);
        exit(1);
    }

    if ((f = fopen(argv[1], "rb")) == NULL) {
        printf("Cannot open signature file %s\n", argv[1]);
        exit(2);
    }

    if ((fpat = fopen(argv[2], "rb")) == NULL) {
        printf("Cannot open pattern file %s\n", argv[2]);
        exit(2);
    }

    /* Read the parameters */
    grab(4);
    if (memcmp("dccs", buf, 4) != 0) {
        printf("Not a dccs file!\n");
        exit(3);
    }
    numKeys = readFileShort();
    numVert = readFileShort();
    PatLen = readFileShort();
    SymLen = readFileShort();

    /* Initialise the perfhlib stuff. Also allocates T1, T2, g, etc */
    g_pattern_hasher.setHashParams(
                numKeys,  /* The number of symbols */
                PatLen,   /* The length of the pattern to be hashed */
                256,      /* The character set of the pattern (0-FF) */
                0,        /* Minimum pattern character value */
                numVert); /* Specifies C, the sparseness of the graph. See Czech, Havas
                   and Majewski for details */

    T1base = g_pattern_hasher.readT1();
    T2base = g_pattern_hasher.readT2();
    g = g_pattern_hasher.readG();

    /* Read T1 and T2 tables */
    grab(2);
    if (memcmp("T1", buf, 2) != 0) {
        printf("Expected 'T1'\n");
        exit(3);
    }
    len = PatLen * 256 * sizeof(uint16_t);
    w = readFileShort();
    if (w != len) {
        printf("Problem with size of T1: file %d, calc %d\n", w, len);
        exit(4);
    }
    if (fread(T1base, 1, len, f) != len) {
        printf("Could not read T1\n");
        exit(5);
    }

    grab(2);
    if (memcmp("T2", buf, 2) != 0) {
        printf("Expected 'T2'\n");
        exit(3);
    }
    w = readFileShort();
    if (w != len) {
        printf("Problem with size of T2: file %d, calc %d\n", w, len);
        exit(4);
    }
    if (fread(T2base, 1, len, f) != len) {
        printf("Could not read T2\n");
        exit(5);
    }

    /* Now read the function g[] */
    grab(2);
    if (memcmp("gg", buf, 2) != 0) {
        printf("Expected 'gg'\n");
        exit(3);
    }
    len = numVert * sizeof(uint16_t);
    w = readFileShort();
    if (w != len) {
        printf("Problem with size of g[]: file %d, calc %d\n", w, len);
        exit(4);
    }
    if (fread(g, 1, len, f) != len) {
        printf("Could not read T2\n");
        exit(5);
    }

    /* This is now the hash table */
    /* First allocate space for the table */
    if ((ht = (HT *)malloc(numKeys * sizeof(HT))) == 0) {
        printf("Could not allocate hash table\n");
        exit(1);
    }
    grab(2);
    if (memcmp("ht", buf, 2) != 0) {
        printf("Expected 'ht'\n");
        exit(3);
    }
    w = readFileShort();
    if (w != numKeys * (SymLen + PatLen + sizeof(uint16_t))) {
        printf("Problem with size of hash table: file %d, calc %d\n", w, len);
        exit(6);
    }

    for (i = 0; i < numKeys; i++) {
        if ((int)fread(&ht[i], 1, SymLen + PatLen, f) != SymLen + PatLen) {
            printf("Could not read\n");
            exit(11);
        }
    }

    /* Read the pattern to buf */
    if ((patlen = fread(buf, 1, 100, fpat)) == 0) {
        printf("Could not read pattern\n");
        exit(11);
    }
    if (patlen != PATLEN) {
        printf("Error: pattern length is %d, should be %d\n", patlen, PATLEN);
        exit(12);
    }

    /* Fix the wildcards */
    fixWildCards(buf);

    printf("Pattern:\n");
    for (i = 0; i < PATLEN; i++)
        printf("%02X ", buf[i]);
    printf("\n");

    h = g_pattern_hasher.hash(buf);
    printf("Pattern hashed to %d (0x%X), symbol %s\n", h, h, ht[h].htSym);
    if (memcmp(ht[h].htPat, buf, PATLEN) == 0) {
        printf("Pattern matched");
    } else {
        printf("Pattern mismatch: found following pattern\n");
        for (i = 0; i < PATLEN; i++)
            printf("%02X ", ht[h].htPat[i]);
        printf("\n");
        pattSearch(); /* Look for it the hard way */
    }
    cleanup();
    free(ht);
    fclose(f);
    fclose(fpat);
    return 0;
}

void pattSearch(void) {
    int i;

    for (i = 0; i < numKeys; i++) {
        if ((i % 100) == 0)
            printf("\r%d ", i);
        if (memcmp(ht[i].htPat, buf, PATLEN) == 0) {
            printf("\nPattern matched offset %d (0x%X)\n", i, i);
        }
    }
    printf("\n");
}

void cleanup(void) {
    /* Free the storage for variable sized tables etc */
    if (T1base)
        free(T1base);
    if (T2base)
        free(T2base);
    if (g)
        free(g);
}

void grab(int n) {
    if (fread(buf, 1, n, f) != (size_t)n) {
        printf("Could not read\n");
        exit(11);
    }
}

uint16_t readFileShort(void) {
    uint8_t b1, b2;

    if (fread(&b1, 1, 1, f) != 1) {
        printf("Could not read\n");
        exit(11);
    }
    if (fread(&b2, 1, 1, f) != 1) {
        printf("Could not read\n");
        exit(11);
    }
    return (b2 << 8) + b1;
}

/* Following two functions not needed unless creating tables */

void getKey(int i, uint8_t **keys) {}

/* Display key i */
void dispKey(int i) {}
