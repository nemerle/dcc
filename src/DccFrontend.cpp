#include "DccFrontend.h"

#include "dcc.h"
#include "msvc_fixes.h"
#include "project.h"
#include "disassem.h"
#include "CallGraph.h"

#include <QtCore/QFileInfo>
#include <QtCore/QDebug>
#include <cstdio>


class Loader
{
    bool loadIntoProject(IProject *);
};

struct PSP {			/*        PSP structure					*/
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
};

static struct MZHeader {				/*      EXE file header		 	 */
    uint8_t     sigLo;			/* .EXE signature: 0x4D 0x5A	 */
    uint8_t     sigHi;
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

//static void LoadImage(char *filename);
static void displayMemMap(void);
/****************************************************************************
* displayLoadInfo - Displays low level loader type info.
***************************************************************************/
void PROG::displayLoadInfo(void)
{
    int	i;

    printf("File type is %s\n", (fCOM)?"COM":"EXE");
    if (not fCOM) {
        printf("Signature            = %02X%02X\n", header.sigLo, header.sigHi);
        printf("File size %% 512     = %04X\n", LH(&header.lastPageSize));
        printf("File size / 512      = %04X pages\n", LH(&header.numPages));
        printf("# relocation items   = %04X\n", LH(&header.numReloc));
        printf("Offset to load image = %04X paras\n", LH(&header.numParaHeader));
        printf("Minimum allocation   = %04X paras\n", LH(&header.minAlloc));
        printf("Maximum allocation   = %04X paras\n", LH(&header.maxAlloc));
    }
    printf("Load image size      = %08lX\n", cbImage - sizeof(PSP));
    printf("Initial SS:SP        = %04X:%04X\n", initSS, initSP);
    printf("Initial CS:IP        = %04X:%04X\n", initCS, initIP);

    if (option.VeryVerbose and cReloc)
    {
        printf("\nRelocation Table\n");
        for (i = 0; i < cReloc; i++)
        {
            printf("%06X -> [%04X]\n", relocTable[i],LH(image() + relocTable[i]));
        }
    }
    printf("\n");
}

/*****************************************************************************
* fill - Fills line for displayMemMap()
****************************************************************************/
static void fill(int ip, char *bf)
{
    PROG &prog(Project::get()->prog);
    static uint8_t type[4] = {'.', 'd', 'c', 'x'};
    uint8_t	i;

    for (i = 0; i < 16; i++, ip++)
    {
        *bf++ = ' ';
        *bf++ = (ip < prog.cbImage)? type[(prog.map[ip >> 2] >> ((ip & 3) * 2)) & 3]: ' ';
    }
    *bf = '\0';
}


/*****************************************************************************
* displayMemMap - Displays the memory bitmap
****************************************************************************/
static void displayMemMap(void)
{
    PROG &prog(Project::get()->prog);

    char	c, b1[33], b2[33], b3[33];
    uint8_t i;
    int ip = 0;

    printf("\nMemory Map\n");
    while (ip < prog.cbImage)
    {
        fill(ip, b1);
        printf("%06X %s\n", ip, b1);
        ip += 16;
        for (i = 3, c = b1[1]; i < 32 and c == b1[i]; i += 2)
            ;		/* Check if all same */
        if (i > 32)
        {
            fill(ip, b2);	/* Skip until next two are not same */
            fill(ip+16, b3);
            if (not (strcmp(b1, b2) || strcmp(b1, b3)))
            {
                printf("                   :\n");
                do
                {
                    ip += 16;
                    fill(ip+16, b1);
                } while (0==strcmp(b1, b2));
            }
        }
    }
    printf("\n");
}
DccFrontend::DccFrontend(QObject *parent) :
    QObject(parent)
{
}

/*****************************************************************************
* FrontEnd - invokes the loader, parser, disassembler (if asm1), icode
* rewritter, and displays any useful information.
****************************************************************************/
bool DccFrontend::FrontEnd ()
{

    /* Do depth first flow analysis building call graph and procedure list,
     * and attaching the I-code to each procedure          */
    parse (*Project::get());

    if (option.asm1)
    {
        qWarning() << "dcc: writing assembler file "<<asm1_name<<'\n';
    }

    /* Search through code looking for impure references and flag them */
    Disassembler ds(1);
    for(Function &f : Project::get()->pProcList)
    {
        f.markImpure();
        if (option.asm1)
        {
            ds.disassem(&f);
        }
    }
    if (option.Interact)
    {
        interactDis(&Project::get()->pProcList.front(), 0);     /* Interactive disassembler */
    }

    /* Converts jump target addresses to icode offsets */
    for(Function &f : Project::get()->pProcList)
    {
        f.bindIcodeOff();
    }
    /* Print memory bitmap */
    if (option.Map)
        displayMemMap();
    return(true); // we no longer own proj !
}
struct DosLoader {
protected:
    void prepareImage(PROG &prog,size_t sz,QFile &fp) {
        /* Allocate a block of memory for the program. */
        prog.cbImage  = sz + sizeof(PSP);
        prog.Imagez    = new uint8_t [prog.cbImage];
        prog.Imagez[0] = 0xCD;		/* Fill in PSP int 20h location */
        prog.Imagez[1] = 0x20;		/* for termination checking     */
        /* Read in the image past where a PSP would go */
        if (sz != fp.read((char *)prog.Imagez + sizeof(PSP),sz))
            fatalError(CANNOT_READ, fp.fileName().toLocal8Bit().data());
    }
};
struct ComLoader : public DosLoader {
    bool canLoad(QFile &fp) {
        fp.seek(0);
        char sig[2];
        if(2==fp.read(sig,2)) {
            return not (sig[0] == 0x4D and sig[1] == 0x5A);
        }
        return false;
    }
    bool load(PROG &prog,QFile &fp) {
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
};
struct ExeLoader : public DosLoader {
    bool canLoad(QFile &fp) {
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
    bool load(PROG &prog,QFile &fp) {
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
};
/*****************************************************************************
* LoadImage
****************************************************************************/
bool Project::load()
{
    // addTask(loaderSelection,PreCond(BinaryImage))
    // addTask(applyLoader,PreCond(Loader))
    const char *fname = binary_path().toLocal8Bit().data();
    QFile finfo(binary_path());
    /* Open the input file */
    if(not finfo.open(QFile::ReadOnly)) {
        fatalError(CANNOT_OPEN, fname);
    }
    /* Read in first 2 bytes to check EXE signature */
    if (finfo.size()<=2)
    {
        fatalError(CANNOT_READ, fname);
    }
    ComLoader com_loader;
    ExeLoader exe_loader;
    if(exe_loader.canLoad(finfo)) {
        prog.fCOM = false;
        return exe_loader.load(prog,finfo);
    }
    if(com_loader.canLoad(finfo)) {
        prog.fCOM = true;
        return com_loader.load(prog,finfo);
    }
    return false;
}
uint32_t SynthLab;
/* Parses the program, builds the call graph, and returns the list of
 * procedures found     */
void DccFrontend::parse(Project &proj)
{
    PROG &prog(proj.prog);
    STATE state;

    /* Set initial state */
    state.setState(rES, 0);   /* PSP segment */
    state.setState(rDS, 0);
    state.setState(rCS, prog.initCS);
    state.setState(rSS, prog.initSS);
    state.setState(rSP, prog.initSP);
    state.IP = ((uint32_t)prog.initCS << 4) + prog.initIP;
    SynthLab = SYNTHESIZED_MIN;

    /* Check for special settings of initial state, based on idioms of the
          startup code */
    state.checkStartup();
    ilFunction start_proc;
    /* Make a struct for the initial procedure */
    if (prog.offMain != -1)
    {
        start_proc = proj.createFunction(0,"main");
        start_proc->retVal.loc = REG_FRAME;
        start_proc->retVal.type = TYPE_WORD_SIGN;
        start_proc->retVal.id.regi = rAX;
        /* We know where main() is. Start the flow of control from there */
        start_proc->procEntry = prog.offMain;
        /* In medium and large models, the segment of main may (will?) not be
            the same as the initial CS segment (of the startup code) */
        state.setState(rCS, prog.segMain);
        state.IP = prog.offMain;
    }
    else
    {
        start_proc = proj.createFunction(0,"start");
        /* Create initial procedure at program start address */
        start_proc->procEntry = (uint32_t)state.IP;
    }

    /* The state info is for the first procedure */
    start_proc->state = state;

    /* Set up call graph initial node */
    proj.callGraph = new CALL_GRAPH;
    proj.callGraph->proc = start_proc;

    /* This proc needs to be called to set things up for LibCheck(), which
       checks a proc to see if it is a know C (etc) library */
    prog.bSigs = SetupLibCheck();
    //BUG:  proj and g_proj are 'live' at this point !

    /* Recursively build entire procedure list */
    start_proc->FollowCtrl(proj.callGraph, &state);

    /* This proc needs to be called to clean things up from SetupLibCheck() */
    CleanupLibCheck();
}
