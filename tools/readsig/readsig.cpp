/* Quick program to read the output from makedsig */

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <string.h>
#include "perfhlib.h"

/* statics */
uint8_t buf[100];
int	numKeys;			/* Number of hash table entries (keys) */
int numVert;			/* Number of vertices in the graph (also size of g[]) */
int PatLen;				/* Size of the keys (pattern length) */
int SymLen;				/* Max size of the symbols, including null */
FILE *f;				/* File being read */

/* prototypes */
void grab(int n);
uint16_t readFileShort(void);
void cleanup(void);

static bool bDispAll = false;
static PerfectHash g_pattern_hasher;
static uint16_t *T1base;
static uint16_t *T2base;
static uint16_t *g;

int main(int argc, char *argv[])
{
    uint16_t w, len;
    int h, i, j;
    long filePos;

    if (argc <= 1)
    {
        printf("Usage: readsig [-a] <SigFilename>\n");
        printf("-a for all symbols (else just duplicates)\n");
        exit(1);
    }

    i = 1;

    if (strcmp(argv[i], "-a") == 0)
    {
        i++;
        bDispAll = true;
    }
    if ((f = fopen(argv[i], "rb")) == NULL)
    {
        printf("Cannot open %s\n", argv[i]);
        exit(2);
    }

    /* Read the parameters */
    grab(4);
    if (memcmp("dccs", buf, 4) != 0)
    {
        printf("Not a dccs file!\n");
        exit(3);
    }
    numKeys = readFileShort();
    numVert = readFileShort();
    PatLen = readFileShort();
    SymLen = readFileShort();

    /* Initialise the perfhlib stuff. Also allocates T1, T2, g, etc */
    g_pattern_hasher.setHashParams(
                    numKeys,                /* The number of symbols */
                    PatLen,                 /* The length of the pattern to be hashed */
                    256,                    /* The character set of the pattern (0-FF) */
                    0,                      /* Minimum pattern character value */
                    numVert);               /* Specifies C, the sparseness of the graph. See Czech, Havas and Majewski for details */

    T1base  = g_pattern_hasher.readT1();
    T2base  = g_pattern_hasher.readT2();
    g       = g_pattern_hasher.readG();

    /* Read T1 and T2 tables */
    grab(2);
    if (memcmp("T1", buf, 2) != 0)
    {
        printf("Expected 'T1'\n");
        exit(3);
    }
    len = PatLen * 256 * sizeof(uint16_t);
    w = readFileShort();
    if (w != len)
    {
        printf("Problem with size of T1: file %d, calc %d\n", w, len);
        exit(4);
    }
    if (fread(T1base, 1, len, f) != len)
    {
        printf("Could not read T1\n");
        exit(5);
    }

    grab(2);
    if (memcmp("T2", buf, 2) != 0)
    {
        printf("Expected 'T2'\n");
        exit(3);
    }
    w = readFileShort();
    if (w != len)
    {
        printf("Problem with size of T2: file %d, calc %d\n", w, len);
        exit(4);
    }
    if (fread(T2base, 1, len, f) != len)
    {
        printf("Could not read T2\n");
        exit(5);
    }

    /* Now read the function g[] */
    grab(2);
    if (memcmp("gg", buf, 2) != 0)
    {
        printf("Expected 'gg'\n");
        exit(3);
    }
    len = numVert * sizeof(uint16_t);
    w = readFileShort();
    if (w != len)
    {
        printf("Problem with size of g[]: file %d, calc %d\n", w, len);
        exit(4);
    }
    if (fread(g, 1, len, f) != len)
    {
        printf("Could not read T2\n");
        exit(5);
    }


    /* This is now the hash table */
    grab(2);
    if (memcmp("ht", buf, 2) != 0)
    {
        printf("Expected 'ht'\n");
        exit(3);
    }
    w = readFileShort();
    if (w != numKeys * (SymLen + PatLen + sizeof(uint16_t)))
    {
        printf("Problem with size of hash table: file %d, calc %d\n", w, len);
        exit(6);
    }

    if (bDispAll)
    {
        filePos = ftell(f);
        for (i=0; i < numKeys; i++)
        {
            grab(SymLen + PatLen);

            printf("%16s ", buf);
            for (j=0; j < PatLen; j++)
            {
                printf("%02X", buf[SymLen+j]);
                if ((j%4) == 3) printf(" ");
            }
            printf("\n");
        }
        printf("\n\n\n");
        fseek(f, filePos, SEEK_SET);
    }

    for (i=0; i < numKeys; i++)
    {
        grab(SymLen + PatLen);

        h = g_pattern_hasher.hash(&buf[SymLen]);
        if (h != i)
        {
            printf("Symbol %16s (index %3d) hashed to %d\n", buf, i, h);
        }
    }

    printf("Done!\n");
    fclose(f);
    return 0;
}


void
cleanup(void)
{
    /* Free the storage for variable sized tables etc */
    if (T1base)	free(T1base);
    if (T2base)	free(T2base);
    if (g) free(g);
}

void grab(int n)
{
    if (fread(buf, 1, n, f) != (size_t)n)
    {
        printf("Could not read\n");
        exit(11);
    }
}

uint16_t readFileShort(void)
{
    uint8_t b1, b2;

    if (fread(&b1, 1, 1, f) != 1)
    {
        printf("Could not read\n");
        exit(11);
    }
    if (fread(&b2, 1, 1, f) != 1)
    {
        printf("Could not read\n");
        exit(11);
    }
    return (b2 << 8) + b1;
}
