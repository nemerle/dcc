#include "DccFrontend.h"

#include "Loaders.h"
#include "dcc.h"
#include "msvc_fixes.h"
#include "project.h"
#include "disassem.h"
#include "CallGraph.h"
#include "Command.h"
#include "chklib.h"

#include <QtCore/QFileInfo>
#include <QtCore/QDebug>
#include <cstdio>


//static void LoadImage(char *filename);
static void displayMemMap(void);
/****************************************************************************
* displayLoadInfo - Displays low level loader type info.
***************************************************************************/
void PROG::displayLoadInfo(void)
{
    int i;

    printf("File type is %s\n", (fCOM)?"COM":"EXE");
//    if (not fCOM) {
//        printf("Signature            = %02X%02X\n", header.sigLo, header.sigHi);
//        printf("File size %% 512     = %04X\n", LH(&header.lastPageSize));
//        printf("File size / 512      = %04X pages\n", LH(&header.numPages));
//        printf("# relocation items   = %04X\n", LH(&header.numReloc));
//        printf("Offset to load image = %04X paras\n", LH(&header.numParaHeader));
//        printf("Minimum allocation   = %04X paras\n", LH(&header.minAlloc));
//        printf("Maximum allocation   = %04X paras\n", LH(&header.maxAlloc));
//    }
    printf("Load image size      = %04" PRIiPTR "\n", cbImage);
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
    uint8_t i;

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

    char    c, b1[33], b2[33], b3[33];
    uint8_t i;
    int ip = 0;

    printf("\nMemory Map\n");
    while (ip < prog.cbImage)
    {
        fill(ip, b1);
        printf("%06X %s\n", ip, b1);
        ip += 16;
        for (i = 3, c = b1[1]; i < 32 and c == b1[i]; i += 2)
            ;   /* Check if all same */
        if (i > 32)
        {
            fill(ip, b2);   /* Skip until next two are not same */
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
    for(PtrFunction &f : Project::get()->pProcList)
    {
        f->markImpure();
        if (option.asm1)
        {
            ds.disassem(f);
        }
    }
    if (option.Interact)
    {
        interactDis(Project::get()->pProcList.front(), 0);     /* Interactive disassembler */
    }

    /* Converts jump target addresses to icode offsets */
    for(PtrFunction &f : Project::get()->pProcList)
    {
        f->bindIcodeOff();
    }
    /* Print memory bitmap */
    if (option.Map)
        displayMemMap();
    return(true); // we no longer own proj !
}
/*****************************************************************************
* LoadImage
****************************************************************************/

/* Parses the program, builds the call graph, and returns the list of
 * procedures found     */
void DccFrontend::parse(Project &proj)
{
    /* Set initial state */
    proj.addCommand(new MachineStateInitialization);
    proj.addCommand(new FindMain);
}

bool MachineStateInitialization::execute(CommandContext *ctx)
{
    assert(ctx && ctx->m_project);
    Project &proj(*ctx->m_project);
    const PROG &prog(proj.prog);
    proj.m_entry_state.setState(rES, 0);   /* PSP segment */
    proj.m_entry_state.setState(rDS, 0);
    proj.m_entry_state.setState(rCS, prog.initCS);
    proj.m_entry_state.setState(rSS, prog.initSS);
    proj.m_entry_state.setState(rSP, prog.initSP);
    proj.m_entry_state.IP = ((uint32_t)prog.initCS << 4) + prog.initIP;
    proj.SynthLab = SYNTHESIZED_MIN;
    return true;
}

bool FindMain::execute(CommandContext *ctx) {
    Project &proj(*ctx->m_project);
    const PROG &prog(proj.prog);

    PtrFunction start_func = proj.findByName("start");
    if(ctx->m_project->m_entry_state.IP==0) {
        ctx->recordFailure(this,"Cannot search for main func when no entry point was found");
        return false;
    }
    /* Check for special settings of initial state, based on idioms of the startup code */
    if(checkStartup(ctx->m_project->m_entry_state)) {
        proj.findByName("start")->markDoNotDecompile(); // we have main, do not decompile the start proc
        //TODO: main arguments and return values should depend on detected compiler/library
        FunctionType *main_type = FunctionType::get(Type{TYPE_WORD_SIGN},{ Type{TYPE_WORD_SIGN},Type{TYPE_PTR} },false);
        main_type->setCallingConvention(CConv::C);
        proj.addCommand(new CreateFunction("main",SegOffAddr {prog.segMain,prog.offMain},main_type));

        proj.addCommand(new LoadPatternLibrary());
    }
    return true;
}

QString CreateFunction::instanceDescription() const {
    return QString("%1 \"%2\" @ 0x%3").arg(name()).arg(m_name).arg(m_addr.addr,0,16,QChar('0'));
}

bool CreateFunction::execute(CommandContext *ctx) {
    Project &proj(*ctx->m_project);
    const PROG &prog(proj.prog);

    PtrFunction func = proj.createFunction(m_type,m_name,m_addr);
    if(m_name=="main") {
        /* In medium and large models, the segment of main may (will?) not be
                the same as the initial CS segment (of the startup code) */
        proj.m_entry_state.setState(rCS, prog.segMain);
        proj.m_entry_state.IP = prog.offMain;
        func->state = proj.m_entry_state;
    }
    if(m_name=="start") {
        proj.addCommand(new MachineStateInitialization);
        proj.addCommand(new FindMain);
        func->state = proj.m_entry_state; // just in case we fail to find main, initialize 'state'
    }

//    proj.addCommand(new ProcessFunction);
    //proj.addCommand(new FollowControl());
    /* Recursively build entire procedure list */
    //proj.callGraph->proc->FollowCtrl(proj.callGraph, &proj.m_entry_state);
    return true;
}
