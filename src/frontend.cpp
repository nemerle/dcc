/*****************************************************************************
*			dcc project Front End module
* Loads a program into simulated main memory and builds the procedure list.
* (C) Cristina Cifuentes
****************************************************************************/

#include "dcc.h"
#include "disassem.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>			/* For malloc, free, realloc */

typedef struct {			/*        PSP structure					*/
    uint16_t int20h;			/* interrupt 20h						*/
    uint16_t eof;				/* segment, end of allocation block		*/
    uint8_t res1;				/* reserved                         	*/
    uint8_t dosDisp[5];		/* far call to DOS function dispatcher	*/
    uint8_t int22h[4];			/* vector for terminate routine			*/
    uint8_t int23h[4];			/* vector for ctrl+break routine		*/
    uint8_t int24h[4];			/* vector for error routine				*/
    uint8_t res2[22];			/* reserved								*/
    uint16_t segEnv;			/* segment address of environment block	*/
    uint8_t res3[34];			/* reserved								*/
    uint8_t int21h[6];			/* opcode for int21h and far return		*/
    uint8_t res4[6];			/* reserved								*/
    uint8_t fcb1[16];			/* default file control block 1			*/
    uint8_t fcb2[16];      	/* default file control block 2			*/
    uint8_t res5[4];			/* reserved								*/
    uint8_t cmdTail[0x80];		/* command tail and disk transfer area	*/
} PSP;

static struct {				/*      EXE file header		 	 */
    uint8_t	sigLo;			/* .EXE signature: 0x4D 0x5A	 */
    uint8_t	sigHi;
    uint16_t	lastPageSize;	/* Size of the last page		 */
    uint16_t	numPages;		/* Number of pages in the file	 */
    uint16_t	numReloc;		/* Number of relocation items	 */
    uint16_t	numParaHeader;	/* # of paragraphs in the header */
    uint16_t	minAlloc;		/* Minimum number of paragraphs	 */
    uint16_t	maxAlloc;		/* Maximum number of paragraphs	 */
    uint16_t	initSS;			/* Segment displacement of stack */
    uint16_t	initSP;			/* Contents of SP at entry       */
    uint16_t	checkSum;		/* Complemented checksum         */
    uint16_t	initIP;			/* Contents of IP at entry       */
    uint16_t	initCS;			/* Segment displacement of code  */
    uint16_t	relocTabOffset;	/* Relocation table offset       */
    uint16_t	overlayNum;		/* Overlay number                */
} header;

#define EXE_RELOCATION  0x10		/* EXE images rellocated to above PSP */

static void LoadImage(char *filename);
static void displayLoadInfo(void);
static void displayMemMap(void);

/*****************************************************************************
* FrontEnd - invokes the loader, parser, disassembler (if asm1), icode
* rewritter, and displays any useful information.
****************************************************************************/
void FrontEnd (char *filename, CALL_GRAPH * *pcallGraph)
{
    /* Load program into memory */
    LoadImage(filename);

    if (option.verbose)
        displayLoadInfo();

    /* Do depth first flow analysis building call graph and procedure list,
     * and attaching the I-code to each procedure          */
    parse (pcallGraph);

    if (option.asm1)
    {
        printf("dcc: writing assembler file %s\n", asm1_name);
    }

    /* Search through code looking for impure references and flag them */
    for(Function &f : pProcList)
    {
        f.markImpure();
        if (option.asm1)
            disassem(1, &f);
    }
    if (option.Interact)
    {
        interactDis(&pProcList.front(), 0);     /* Interactive disassembler */
    }

    /* Converts jump target addresses to icode offsets */
    for(Function &f : pProcList)
    {
        f.bindIcodeOff();
    }
    /* Print memory bitmap */
    if (option.Map)
        displayMemMap();
}


/****************************************************************************
* displayLoadInfo - Displays low level loader type info.
***************************************************************************/
static void displayLoadInfo(void)
{
    int	i;

    printf("File type is %s\n", (prog.fCOM)?"COM":"EXE");
    if (! prog.fCOM) {
        printf("Signature            = %02X%02X\n", header.sigLo, header.sigHi);
        printf("File size %% 512     = %04X\n", LH(&header.lastPageSize));
        printf("File size / 512      = %04X pages\n", LH(&header.numPages));
        printf("# relocation items   = %04X\n", LH(&header.numReloc));
        printf("Offset to load image = %04X paras\n", LH(&header.numParaHeader));
        printf("Minimum allocation   = %04X paras\n", LH(&header.minAlloc));
        printf("Maximum allocation   = %04X paras\n", LH(&header.maxAlloc));
    }
    printf("Load image size      = %04X\n", prog.cbImage - sizeof(PSP));
    printf("Initial SS:SP        = %04X:%04X\n", prog.initSS, prog.initSP);
    printf("Initial CS:IP        = %04X:%04X\n", prog.initCS, prog.initIP);

    if (option.VeryVerbose && prog.cReloc)
    {
        printf("\nRelocation Table\n");
        for (i = 0; i < prog.cReloc; i++)
        {
            printf("%06X -> [%04X]\n", prog.relocTable[i],LH(prog.Image + prog.relocTable[i]));
        }
    }
    printf("\n");
}


/*****************************************************************************
* fill - Fills line for displayMemMap()
****************************************************************************/
static void fill(int ip, char *bf)
{
    static uint8_t type[4] = {'.', 'd', 'c', 'x'};
    uint8_t	i;

    for (i = 0; i < 16; i++, ip++)
    {
        *bf++ = ' ';
        *bf++ = (ip < prog.cbImage)?
                    type[(prog.map[ip >> 2] >> ((ip & 3) * 2)) & 3]: ' ';
    }
    *bf = '\0';
}


/*****************************************************************************
* displayMemMap - Displays the memory bitmap
****************************************************************************/
static void displayMemMap(void)
{
    char	c, b1[33], b2[33], b3[33];
    uint8_t i;
    int ip = 0;

    printf("\nMemory Map\n");
    while (ip < prog.cbImage)
    {
        fill(ip, b1);
        printf("%06X %s\n", ip, b1);
        ip += 16;
        for (i = 3, c = b1[1]; i < 32 && c == b1[i]; i += 2)
            ;		/* Check if all same */
        if (i > 32)
        {
            fill(ip, b2);	/* Skip until next two are not same */
            fill(ip+16, b3);
            if (! (strcmp(b1, b2) || strcmp(b1, b3)))
            {
                printf("                   :\n");
                do
                {
                    ip += 16;
                    fill(ip+16, b1);
                } while (! strcmp(b1, b2));
            }
        }
    }
    printf("\n");
}


/*****************************************************************************
* LoadImage
****************************************************************************/
static void LoadImage(char *filename)
{
    FILE   *fp;
    int		i, cb;
    uint8_t	buf[4];

    /* Open the input file */
    if ((fp = fopen(filename, "rb")) == NULL)
    {
        fatalError(CANNOT_OPEN, filename);
    }

    /* Read in first 2 bytes to check EXE signature */
    if (fread(&header, 1, 2, fp) != 2)
    {
        fatalError(CANNOT_READ, filename);
    }

    if (! (prog.fCOM = (boolT)(header.sigLo != 0x4D || header.sigHi != 0x5A))) {
        /* Read rest of header */
        fseek(fp, 0, SEEK_SET);
        if (fread(&header, sizeof(header), 1, fp) != 1)
        {
            fatalError(CANNOT_READ, filename);
        }

        /* This is a typical DOS kludge! */
        if (LH(&header.relocTabOffset) == 0x40)
        {
            fatalError(NEWEXE_FORMAT);
        }

        /* Calculate the load module size.
        * This is the number of pages in the file
        * less the length of the header and reloc table
        * less the number of bytes unused on last page
        */
        cb = (uint32_t)LH(&header.numPages) * 512 - (uint32_t)LH(&header.numParaHeader) * 16;
        if (header.lastPageSize)
        {
            cb -= 512 - LH(&header.lastPageSize);
        }

        /* We quietly ignore minAlloc and maxAlloc since for our
* purposes it doesn't really matter where in real memory
* the program would end up.  EXE programs can't really rely on
* their load location so setting the PSP segment to 0 is fine.
* Certainly programs that prod around in DOS or BIOS are going
* to have to load DS from a constant so it'll be pretty
* obvious.
*/
        prog.initCS = (int16_t)LH(&header.initCS) + EXE_RELOCATION;
        prog.initIP = (int16_t)LH(&header.initIP);
        prog.initSS = (int16_t)LH(&header.initSS) + EXE_RELOCATION;
        prog.initSP = (int16_t)LH(&header.initSP);
        prog.cReloc = (int16_t)LH(&header.numReloc);

        /* Allocate the relocation table */
        if (prog.cReloc)
        {
            prog.relocTable = new uint32_t [prog.cReloc];
            fseek(fp, LH(&header.relocTabOffset), SEEK_SET);

            /* Read in seg:offset pairs and convert to Image ptrs */
            for (i = 0; i < prog.cReloc; i++)
            {
                fread(buf, 1, 4, fp);
                prog.relocTable[i] = LH(buf) +
                        (((int)LH(buf+2) + EXE_RELOCATION)<<4);
            }
        }
        /* Seek to start of image */
        fseek(fp, (int)LH(&header.numParaHeader) * 16, SEEK_SET);
    }
    else
    {	/* COM file
         * In this case the load module size is just the file length
         */
        fseek(fp, 0, SEEK_END);
        cb = ftell(fp);

        /* COM programs start off with an ORG 100H (to leave room for a PSP)
         * This is also the implied start address so if we load the image
         * at offset 100H addresses should all line up properly again.
         */
        prog.initCS = 0;
        prog.initIP = 0x100;
        prog.initSS = 0;
        prog.initSP = 0xFFFE;
        prog.cReloc = 0;

        fseek(fp, 0, SEEK_SET);
    }

    /* Allocate a block of memory for the program. */
    prog.cbImage  = cb + sizeof(PSP);
    prog.Image    = new uint8_t [prog.cbImage];
    prog.Image[0] = 0xCD;		/* Fill in PSP int 20h location */
    prog.Image[1] = 0x20;		/* for termination checking     */

    /* Read in the image past where a PSP would go */
#ifdef __DOSWIN__
    if (cb > 0xFFFF)
    {
        printf("Image size of %ld bytes too large for fread!\n", cb);
        fatalError(CANNOT_READ, filename);
    }
#endif
    if (cb != (int)fread(prog.Image + sizeof(PSP), 1, (size_t)cb, fp))
    {
        fatalError(CANNOT_READ, filename);
    }

    /* Set up memory map */
    cb = (prog.cbImage + 3) / 4;
    prog.map = (uint8_t *)malloc(cb);
    memset(prog.map, BM_UNKNOWN, (size_t)cb);

    /* Relocate segment constants */
    if (prog.cReloc)
    {
        for (i = 0; i < prog.cReloc; i++)
        {
            uint8_t *p = &prog.Image[prog.relocTable[i]];
            uint16_t  w = (uint16_t)LH(p) + EXE_RELOCATION;
            *p++    = (uint8_t)(w & 0x00FF);
            *p      = (uint8_t)((w & 0xFF00) >> 8);
        }
    }

    fclose(fp);
}


/*****************************************************************************
* allocMem - malloc with failure test
****************************************************************************/
void *allocMem(int cb)
{
    uint8_t *p;

    //printf("Attempt to allocMem %5ld bytes\n", cb);

    if (! (p = (uint8_t*)malloc((size_t)cb)))
        /*	if (! (p = (uint8_t*)calloc((size_t)cb, 1)))	*/
    {
        fatalError(MALLOC_FAILED, cb);
    }
    // printf("allocMem: %p\n", p);
    return p;
}
