#include "LIB_PatternCollector.h"

#include "msvc_fixes.h"

#include <cstring>
#include <cstring>
/** \note there is an untested assumption that the *first* segment definition
    with class CODE will be the one containing all useful functions in the
    LEDATA records. Functions such as _exit() have more than one segment
    declared with class CODE (MSC8 libraries) */

extern void fixWildCards(uint8_t pat[]);
void readNN(int n, FILE *fl)
{
    if (fseek(fl, (long)n, SEEK_CUR) != 0)
    {
        printf("Could not seek file\n");
        exit(2);
    }
}

void LIB_PatternCollector::readString(FILE *fl)
{
    uint8_t len;

    len = readByte(fl);
    if (fread(buf, 1, len, fl) != len)
    {
        printf("Could not read string len %d\n", len);
        exit(2);
    }
    buf[len] = '\0';
    offset += len;
}

int LIB_PatternCollector::readSyms(FILE *fl)
{
    int i;
    int count = 0;
    int	firstSym = 0;			/* First symbol this module */
    uint8_t b, c, type;
    uint16_t w, len;

    codeLNAMES = NONE;			/* Invalidate indexes for code segment */
    codeSEGDEF = NONE;			/* Else won't be assigned */

    offset = 0;                 /* For diagnostics, really */

    if ((leData = (uint8_t *)malloc(0xFF80)) == 0)
    {
        printf("Could not malloc 64k bytes for LEDATA\n");
        exit(10);
    }

    while (!feof(fl))
    {
        type = readByte(fl);
        len = readWord(fl);
        /* Note: uncommenting the following generates a *lot* of output */
        /*printf("Offset %05lX: type %02X len %d\n", offset-3, type, len);//*/
        switch (type)
        {

        case 0x96:				/* LNAMES */
            while (len > 1)
            {
                readString(fl);
                ++lnum;
                if (strcmp((char *)buf, "CODE") == 0)
                {
                    /* This is the class name we're looking for */
                    codeLNAMES= lnum;
                }
                len -= strlen((char *)buf)+1;
            }
            b = readByte(fl);		/* Checksum */
            break;

        case 0x98:				/* Segment definition */
            b = readByte(fl);		/* Segment attributes */
            if ((b & 0xE0) == 0)
            {
                /* Alignment field is zero. Frame and offset follow */
                readWord(fl);
                readByte(fl);
            }

            w = readWord(fl);		/* Segment length */

            b = readByte(fl);		/* Segment name index */
            ++segnum;

            b = readByte(fl);		/* Class name index */
            if ((b == codeLNAMES) and (codeSEGDEF == NONE))
            {
                /* This is the segment defining the code class */
                codeSEGDEF = segnum;
            }

            b = readByte(fl);		/* Overlay index */
            b = readByte(fl);		/* Checksum */
            break;

        case 0x90:				/* PUBDEF: public symbols */
            b = readByte(fl);		/* Base group */
            c = readByte(fl);		/* Base segment */
            len -= 2;
            if (c == 0)
            {
                w = readWord(fl);
                len -= 2;
            }
            while (len > 1)
            {
                readString(fl);
                w = readWord(fl);		/* Offset */
                b = readByte(fl);		/* Type index */
                if (c == codeSEGDEF)
                {
                    char *p;
                    HASHENTRY entry;
                    p = (char *)buf;
                    if (buf[0] == '_')	/* Leading underscore? */
                    {
                        p++; 			/* Yes, remove it*/
                    }
                    i = std::min(size_t(SYMLEN-1), strlen(p));
                    memcpy(entry.name, p, i);
                    entry.name[i] = '\0';
                    entry.offset = w;
                    /*printf("%04X: %s is sym #%d\n", w, keys[count].name, count);//*/
                    keys.push_back(entry);
                    count++;
                }
                len -= strlen((char *)buf) + 1 + 2 + 1;
            }
            b = readByte(fl);		/* Checksum */
            break;


        case 0xA0:				/* LEDATA */
        {
            b = readByte(fl);		/* Segment index */
            w = readWord(fl);		/* Offset */
            len -= 3;
            /*printf("LEDATA seg %d off %02X len %Xh, looking for %d\n", b, w, len-1, codeSEGDEF);//*/

            if (b != codeSEGDEF)
            {
                readNN(len,fl);	/* Skip the data */
                break;			/* Next record */
            }


            if (fread(&leData[w], 1, len-1, fl) != len-1)
            {
                printf("Could not read LEDATA length %d\n", len-1);
                exit(2);
            }
            offset += len-1;
            maxLeData = std::max<uint16_t>(maxLeData, w+len-1);

            readByte(fl);				/* Checksum */
            break;
        }

        default:
            readNN(len,fl);			/* Just skip the lot */

            if (type == 0x8A)	/* Mod end */
            {
                /* Now find all the patterns for public code symbols that
                        we have found */
                for (i=firstSym; i < count; i++)
                {
                    uint16_t off = keys[i].offset;
                    if (off == (uint16_t)-1)
                    {
                        continue;			/* Ignore if already done */
                    }
                    if (keys[i].offset > maxLeData)
                    {
                        printf(
                                    "Warning: no LEDATA for symbol #%d %s "
                                    "(offset %04X, max %04X)\n",
                                    i, keys[i].name, off, maxLeData);
                        /* To make things consistant, we set the pattern for
                                    this symbol to nulls */
                        memset(&keys[i].pat, 0, PATLEN);
                        continue;
                    }
                    /* Copy to temp buffer so don't overrun later patterns.
                                (e.g. when chopping a short pattern).
                                Beware of short patterns! */
                    if (off+PATLEN <= maxLeData)
                    {
                        /* Available pattern is >= PATLEN */
                        memcpy(buf, &leData[off], PATLEN);
                    }
                    else
                    {
                        /* Short! Only copy what is available (and malloced!) */
                        memcpy(buf, &leData[off], maxLeData-off);
                        /* Set rest to zeroes */
                        memset(&buf[maxLeData-off], 0, PATLEN-(maxLeData-off));
                    }
                    fixWildCards((uint8_t *)buf);
                    /* Save into the hash entry. */
                    memcpy(keys[i].pat, buf, PATLEN);
                    keys[i].offset = (uint16_t)-1;	// Flag it as done
                    //printf("Saved pattern for %s\n", keys[i].name);
                }


                while (readByte(fl) == 0);
                readNN(-1,fl);			/* Unget the last byte (= type) */
                lnum = 0;			/* Reset index into lnames */
                segnum = 0;			/* Reset index into snames */
                firstSym = count;	/* Remember index of first sym this mod */
                codeLNAMES = NONE;	/* Invalidate indexes for code segment */
                codeSEGDEF = NONE;
                memset(leData, 0, maxLeData);	/* Clear out old junk */
                maxLeData = 0;		/* No data read this module */
            }

            else if (type == 0xF1)
            {
                /* Library end record */
                return count;
            }

        }
    }


    free(leData);
    keys.clear();

    return count;
}
