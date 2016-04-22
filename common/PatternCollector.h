#ifndef PATTERNCOLLECTOR
#define PATTERNCOLLECTOR
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <vector>

#define SYMLEN  16          /* Number of chars in the symbol name, incl null */
#define PATLEN  23          /* Number of bytes in the pattern part */

struct HASHENTRY
{
    char name[SYMLEN];      /* The symbol name */
    uint8_t pat [PATLEN];   /* The pattern */
    uint16_t offset;        /* Offset (needed temporarily) */
};

struct PatternCollector {
    uint8_t buf[100], bufSave[7];   /* Temp buffer for reading the file */
    uint16_t readShort(FILE *f)
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

    void grab(FILE *f,int n)
    {
        if (fread(buf, 1, n, f) != (size_t)n)
        {
            printf("Could not read\n");
            exit(11);
        }
    }

    uint8_t readByte(FILE *f)
    {
        uint8_t b;

        if (fread(&b, 1, 1, f) != 1)
        {
            printf("Could not read\n");
            exit(11);
        }
        return b;
    }

    uint16_t readWord(FILE *fl)
    {
        uint8_t b1, b2;

        b1 = readByte(fl);
        b2 = readByte(fl);

        return b1 + (b2 << 8);
    }

    /* Called by map(). Return the i+1th key in *pKeys */
    uint8_t *getKey(int i)
    {
        return keys[i].pat;
    }
    /* Display key i */
    void dispKey(int i)
    {
        printf("%s", keys[i].name);
    }
    std::vector<HASHENTRY> keys; /* array of keys */
    virtual int readSyms(FILE *f)=0;
};
#endif // PATTERNCOLLECTOR

