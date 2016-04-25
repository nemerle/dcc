#include "TPL_PatternCollector.h"

#include "msvc_fixes.h"

#include <cstring>

/** \note Fundamental problem: there seems to be no information linking the names
    in the system unit ("V" category) with their routines, except trial and
    error. I have entered a few. There is no guarantee that the same pmap
    offset will map to the same routine in all versions of turbo.tpl. They
    seem to match so far in version 4 and 5.0 */


#define roundUp(w) ((w + 0x0F) & 0xFFF0)
extern void fixWildCards(uint8_t pat[]);
void TPL_PatternCollector::enterSym(FILE *f, const char *name, uint16_t pmapOffset)
{
    uint16_t pm, cm, codeOffset, pcode;
    uint16_t j;

    /* Enter a symbol with given name */
    allocSym(count);
    strcpy(keys[count].name, name);
    pm = pmap + pmapOffset;			/* Pointer to the 4 byte pmap structure */
    fseek(f, unitBase+pm, SEEK_SET);/* Go there */
    cm = readShort(f);				/* CSeg map offset */
    codeOffset = readShort(f);		/* How far into the code segment is our rtn */
    j = cm / 8;						/* Index into the cmap array */
    pcode = csegBase+csegoffs[j]+codeOffset;
    fseek(f, unitBase+pcode, SEEK_SET);		/* Go there */
    grab(f,PATLEN);					/* Grab the pattern to buf[] */
    fixWildCards(buf);				/* Fix the wild cards */
    memcpy(keys[count].pat, buf, PATLEN);	/* Copy to the key array */
    count++;						/* Done one more */
}

void TPL_PatternCollector::allocSym(int count)
{
    keys.resize(count);
}

void TPL_PatternCollector::readCmapOffsets(FILE *f)
{
    uint16_t cumsize, csize;
    uint16_t i;

    /* Read the cmap table to find the start address of each segment */
    fseek(f, unitBase+cmap, SEEK_SET);
    cumsize = 0;
    csegIdx = 0;
    for (i=cmap; i < pmap; i+=8)
    {
        readShort(f);					/* Always 0 */
        csize = readShort(f);
        if (csize == 0xFFFF) continue;	/* Ignore the first one... unit init */
        csegoffs[csegIdx++] = cumsize;
        cumsize += csize;
        grab(f,4);
    }
}

void TPL_PatternCollector::enterSystemUnit(FILE *f)
{
    /* The system unit is special. The association between keywords and
            pmap entries is not stored in the .tpl file (as far as I can tell).
            So we hope that they are constant pmap entries.
        */

    fseek(f, 0x0C, SEEK_SET);
    cmap = readShort(f);
    pmap = readShort(f);
    fseek(f, offStCseg, SEEK_SET);
    csegBase = roundUp(readShort(f));		/* Round up to next 16 bdry */
    printf("CMAP table at %04X\n", cmap);
    printf("PMAP table at %04X\n", pmap);
    printf("Code seg base %04X\n", csegBase);

    readCmapOffsets(f);

    enterSym(f,"INITIALISE",	0x04);
    enterSym(f,"UNKNOWN008",	0x08);
    enterSym(f,"EXIT",		0x0C);
    enterSym(f,"BlockMove",	0x10);
    unknown(f,0x14, 0xC8);
    enterSym(f,"PostIO",		0xC8);
    enterSym(f,"UNKNOWN0CC",	0xCC);
    enterSym(f,"STACKCHK",	0xD0);
    enterSym(f,"UNKNOWN0D4",	0xD4);
    enterSym(f,"WriteString",	0xD8);
    enterSym(f,"WriteInt",	0xDC);
    enterSym(f,"UNKNOWN0E0",	0xE0);
    enterSym(f,"UNKNOWN0E4",	0xE4);
    enterSym(f,"CRLF",		0xE8);
    enterSym(f,"UNKNOWN0EC",	0xEC);
    enterSym(f,"UNKNOWN0F0",	0xF0);
    enterSym(f,"UNKNOWN0F4",	0xF4);
    enterSym(f,"ReadEOL", 	0xF8);
    enterSym(f,"Read",		0xFC);
    enterSym(f,"UNKNOWN100",	0x100);
    enterSym(f,"UNKNOWN104",	0x104);
    enterSym(f,"PostWrite",	0x108);
    enterSym(f,"UNKNOWN10C",	0x10C);
    enterSym(f,"Randomize",	0x110);
    unknown(f,0x114, 0x174);
    enterSym(f,"Random",		0x174);
    unknown(f,0x178, 0x1B8);
    enterSym(f,"FloatAdd",	0x1B8);		/* A guess! */
    enterSym(f,"FloatSub",	0x1BC);		/* disicx - dxbxax -> dxbxax*/
    enterSym(f,"FloatMult",	0x1C0);		/* disicx * dxbxax -> dxbxax*/
    enterSym(f,"FloatDivide",	0x1C4);		/* disicx / dxbxax -> dxbxax*/
    enterSym(f,"UNKNOWN1C8",	0x1C8);
    enterSym(f,"DoubleToFloat",0x1CC);	/* dxax to dxbxax */
    enterSym(f,"UNKNOWN1D0",	0x1D0);
    enterSym(f,"WriteFloat",	0x1DC);
    unknown(f,0x1E0, 0x200);

}

void TPL_PatternCollector::readString(FILE *f)
{
    uint8_t len;

    len = readByte(f);
    grab(f,len);
    buf[len] = '\0';
}

void TPL_PatternCollector::unknown(FILE *f, unsigned j, unsigned k)
{
    /* Mark calls j to k (not inclusive) as unknown */
    unsigned i;

    for (i=j; i < k; i+= 4)
    {
        sprintf((char *)buf, "UNKNOWN%03X", i);
        enterSym(f,(char *)buf, i);
    }
}

void TPL_PatternCollector::nextUnit(FILE *f)
{
    /* Find the start of the next unit */

    uint16_t dsegBase, sizeSyms, sizeOther1, sizeOther2;

    fseek(f, unitBase+offStCseg, SEEK_SET);
    dsegBase = roundUp(readShort(f));
    sizeSyms = roundUp(readShort(f));
    sizeOther1 = roundUp(readShort(f));
    sizeOther2 = roundUp(readShort(f));

    unitBase += dsegBase + sizeSyms + sizeOther1 + sizeOther2;

    fseek(f, unitBase, SEEK_SET);
    if (fread(buf, 1, 4, f) == 4)
    {
        buf[4]='\0';
        printf("Start of unit: found %s\n", buf);
    }

}

void TPL_PatternCollector::setVersionSpecifics()
{

    version = buf[3];			/* The x of TPUx */

    switch (version)
    {
    case '0':				/* Version 4.0 */
        offStCseg = 0x14;	/* Offset to the LL giving the Cseg start */
        charProc = 'T';		/* Indicates a proc in the dictionary */
        charFunc = 'U';		/* Indicates a function in the dictionary */
        skipPmap = 6;		/* Bytes to skip after Func to get pmap offset */
        break;


    case '5':				/* Version 5.0 */
        offStCseg = 0x18;	/* Offset to the LL giving the Cseg start */
        charProc = 'T';		/* Indicates a proc in the dictionary */
        charFunc = 'U';		/* Indicates a function in the dictionary */
        skipPmap = 1;		/* Bytes to skip after Func to get pmap offset */
        break;

    default:
        printf("Unknown version %c!\n", version);
        exit(1);

    }

}

void TPL_PatternCollector::savePos(FILE *f)
{

    if (positionStack.size() >= 20)
    {
        printf("Overflowed filePosn array\n");
        exit(1);
    }
    positionStack.push_back(ftell(f));
}

void TPL_PatternCollector::restorePos(FILE *f)
{
    if (positionStack.empty() == 0)
    {
        printf("Underflowed filePosn array\n");
        exit(1);
    }

    fseek(f, positionStack.back(), SEEK_SET);
    positionStack.pop_back();
}

void TPL_PatternCollector::enterUnitProcs(FILE *f)
{

    uint16_t i, LL;
    uint16_t hash, hsize, dhdr, pmapOff;
    char cat;
    char name[40];

    fseek(f, unitBase+0x0C, SEEK_SET);
    cmap = readShort(f);
    pmap = readShort(f);
    fseek(f, unitBase+offStCseg, SEEK_SET);
    csegBase = roundUp(readShort(f));		/* Round up to next 16 bdry */
    printf("CMAP table at %04X\n", cmap);
    printf("PMAP table at %04X\n", pmap);
    printf("Code seg base %04X\n", csegBase);

    readCmapOffsets(f);

    fseek(f, unitBase+pmap, SEEK_SET);		/* Go to first pmap entry */
    if (readShort(f) != 0xFFFF)				/* FFFF means none */
    {
        sprintf(name, "UNIT_INIT_%d", ++unitNum);
        enterSym(f,name, 0);					/* This is the unit init code */
    }

    fseek(f, unitBase+0x0A, SEEK_SET);
    hash = readShort(f);
    //printf("Hash table at %04X\n", hash);
    fseek(f, unitBase+hash, SEEK_SET);
    hsize = readShort(f);
    //printf("Hash table size %04X\n", hsize);
    for (i=0; i <= hsize; i+= 2)
    {
        dhdr = readShort(f);
        if (dhdr)
        {
            savePos(f);
            fseek(f, unitBase+dhdr, SEEK_SET);
            do
            {
                LL = readShort(f);
                readString(f);
                strcpy(name, (char *)buf);
                cat = readByte(f);
                if ((cat == charProc) or (cat == charFunc))
                {
                    grab(f,skipPmap);		/* Skip to the pmap */
                    pmapOff = readShort(f);	/* pmap offset */
                    printf("pmap offset for %13s: %04X\n", name, pmapOff);
                    enterSym(f,name, pmapOff);
                }
                //printf("%13s %c ", name, cat);
                if (LL)
                {
                    //printf("LL seek to %04X\n", LL);
                    fseek(f, unitBase+LL, SEEK_SET);
                }
            } while (LL);
            restorePos(f);
        }
    }

}

int TPL_PatternCollector::readSyms(FILE *f)
{
    grab(f,4);
    if ((strncmp((char *)buf, "TPU0", 4) != 0) and ((strncmp((char *)buf, "TPU5", 4) != 0)))
    {
        printf("Not a Turbo Pascal version 4 or 5 library file\n");
        fclose(f);
        exit(1);
    }

    setVersionSpecifics();

    enterSystemUnit(f);
    unitBase = 0;
    do
    {
        nextUnit(f);
        if (feof(f)) break;
        enterUnitProcs(f);
    } while (1);

    return count;
}
