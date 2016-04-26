#include "Loaders.h"

#include "dcc.h"

#include <QtCore/QDebug>

#define EXE_RELOCATION  0x10		/* EXE images rellocated to above PSP */

struct PSP {			/*        PSP structure		*/
    uint16_t int20h;			/* interrupt 20h			*/
    uint16_t eof;			/* segment, end of allocation block	*/
    uint8_t res1;			/* reserved                         	*/
    uint8_t dosDisp[5];		/* far call to DOS function dispatcher	*/
    uint8_t int22h[4];		/* vector for terminate routine		*/
    uint8_t int23h[4];		/* vector for ctrl+break routine		*/
    uint8_t int24h[4];		/* vector for error routine		*/
    uint8_t res2[22];			/* reserved			*/
    uint16_t segEnv;			/* segment address of environment block	*/
    uint8_t res3[34];			/* reserved			*/
    uint8_t int21h[6];		/* opcode for int21h and far return	*/
    uint8_t res4[6];			/* reserved			*/
    uint8_t fcb1[16];			/* default file control block 1		*/
    uint8_t fcb2[16];                       /* default file control block 2		*/
    uint8_t res5[4];			/* reserved			*/
    uint8_t cmdTail[0x80];		/* command tail and disk transfer area	*/
};

static struct MZHeader {                    /*      EXE file header		 	 */
    uint8_t     sigLo;		/* .EXE signature: 0x4D 0x5A	 */
    uint8_t     sigHi;
    uint16_t	lastPageSize;	/* Size of the last page		 */
    uint16_t	numPages;		/* Number of pages in the file	 */
    uint16_t	numReloc;		/* Number of relocation items	 */
    uint16_t	numParaHeader;	/* # of paragraphs in the header */
    uint16_t	minAlloc;		/* Minimum number of paragraphs	 */
    uint16_t	maxAlloc;		/* Maximum number of paragraphs	 */
    uint16_t	initSS;		/* Segment displacement of stack */
    uint16_t	initSP;		/* Contents of SP at entry       */
    uint16_t	checkSum;		/* Complemented checksum         */
    uint16_t	initIP;		/* Contents of IP at entry       */
    uint16_t	initCS;		/* Segment displacement of code  */
    uint16_t	relocTabOffset;	/* Relocation table offset       */
    uint16_t	overlayNum;	/* Overlay number                */
} header;

void DosLoader::prepareImage(PROG & prog, size_t sz, QFile & fp) {
    /* Allocate a block of memory for the program. */
    prog.cbImage  = sz + sizeof(PSP);
    prog.Imagez    = new uint8_t [prog.cbImage];
    prog.Imagez[0] = 0xCD;		/* Fill in PSP int 20h location */
    prog.Imagez[1] = 0x20;		/* for termination checking     */
    /* Read in the image past where a PSP would go */
    if (sz != fp.read((char *)prog.Imagez + sizeof(PSP),sz))
        fatalError(CANNOT_READ, fp.fileName().toLocal8Bit().data());
}

bool ComLoader::canLoad(QFile & fp) {
    fp.seek(0);
    char sig[2];
    if(2==fp.read(sig,2)) {
        return not (sig[0] == 0x4D and sig[1] == 0x5A);
    }
    return false;
}

bool ComLoader::load(PROG & prog, QFile & fp) {
    prog.fCOM = true;
    fp.seek(0);
    /* COM file
         * In this case the load module size is just the file length
         */
    auto cb = fp.size();

    /* COM programs start off with an ORG 100H (to leave room for a PSP)
         * This is also the implied start address so if we load the image
         * at offset 100H addresses should all line up properly again.
         */
    prog.initCS = 0;
    prog.initIP = 0x100;
    prog.initSS = 0;
    prog.initSP = 0xFFFE;
    prog.cReloc = 0;

    prepareImage(prog,cb,fp);


    /* Set up memory map */
    cb = (prog.cbImage + 3) / 4;
    prog.map = (uint8_t *)malloc(cb);
    memset(prog.map, BM_UNKNOWN, (size_t)cb);
    return true;
}

bool ExeLoader::canLoad(QFile & fp) {
    if(fp.size()<sizeof(header))
        return false;
    MZHeader tmp_header;
    fp.seek(0);
    fp.read((char *)&tmp_header, sizeof(header));
    if(not (tmp_header.sigLo == 0x4D and tmp_header.sigHi == 0x5A))
        return false;

    /* This is a typical DOS kludge! */
    if (LH(&header.relocTabOffset) == 0x40)
    {
        qDebug() << "Don't understand new EXE format";
        return false;
    }
    return true;
}

bool ExeLoader::load(PROG & prog, QFile & fp) {
    prog.fCOM = false;
    /* Read rest of header */
    fp.seek(0);
    if (fp.read((char *)&header, sizeof(header)) != sizeof(header))
        return false;

    /* Calculate the load module size.
        * This is the number of pages in the file
        * less the length of the header and reloc table
        * less the number of bytes unused on last page
        */
    uint32_t cb = (uint32_t)LH(&header.numPages) * 512 - (uint32_t)LH(&header.numParaHeader) * 16;
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
        prog.relocTable.resize(prog.cReloc);
        fp.seek(LH(&header.relocTabOffset));

        /* Read in seg:offset pairs and convert to Image ptrs */
        uint8_t	buf[4];
        for (int i = 0; i < prog.cReloc; i++)
        {
            fp.read((char *)buf,4);
            prog.relocTable[i] = LH(buf) + (((int)LH(buf+2) + EXE_RELOCATION)<<4);
        }
    }
    /* Seek to start of image */
    uint32_t start_of_image= LH(&header.numParaHeader) * 16;
    fp.seek(start_of_image);
    /* Allocate a block of memory for the program. */
    prepareImage(prog,cb,fp);

    /* Set up memory map */
    cb = (prog.cbImage + 3) / 4;
    prog.map = (uint8_t *)malloc(cb);
    memset(prog.map, BM_UNKNOWN, (size_t)cb);

    /* Relocate segment constants */
    for(uint32_t v : prog.relocTable) {
        uint8_t *p = &prog.Imagez[v];
        uint16_t  w = (uint16_t)LH(p) + EXE_RELOCATION;
        *p++    = (uint8_t)(w & 0x00FF);
        *p      = (uint8_t)((w & 0xFF00) >> 8);
    }
    return true;
}
