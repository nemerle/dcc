/*****************************************************************************
 *			dcc project scanner module
 * Implements a simple state driven scanner to convert 8086 machine code into
 * I-code
 * (C) Cristina Cifuentes, Jeff Ledermann
 ****************************************************************************/

#include <cstring>
#include "dcc.h"
#include "scanner.h"
#include "project.h"
/*  Parser flags  */
#define TO_REG      0x000100    /* rm is source  */
#define S_EXT       0x000200    /* sign extend   */
#define OP386       0x000400    /* 386 op-code   */
#define NSP         0x000800    /* NOT_HLL if SP is src or dst */
// defined in Enums.h #define ICODEMASK   0xFF00FF    /* Masks off parser flags */

static void rm(int i);
static void modrm(int i);
static void segrm(int i);
static void data1(int i);
static void data2(int i);
static void regop(int i);
static void segop(int i);
static void strop(int i);
static void escop(int i);
static void axImp(int i);
static void alImp(int i);
static void axSrcIm(int i);
static void memImp(int i);
static void memReg0(int i);
static void memOnly(int i);
static void dispM(int i);
static void dispS(int i);
static void dispN(int i);
static void dispF(int i);
static void prefix(int i);
static void immed(int i);
static void shift(int i);
static void arith(int i);
static void trans(int i);
static void const1(int i);
static void const3(int i);
static void none1(int i);
static void none2(int i);
static void checkInt(int i);

#define iZERO (llIcode)0    // For neatness
#define IC	  llIcode

static struct {
    void (*state1)(int);
    void (*state2)(int);
    uint32_t flg;
    llIcode opcode;
} stateTable[] = {
    {  modrm,   none2, B                        , iADD	},  /* 00 */
    {  modrm,   none2, 0                        , iADD	},  /* 01 */
    {  modrm,   none2, TO_REG | B               , iADD	},  /* 02 */
    {  modrm,   none2, TO_REG 			, iADD	},  /* 03 */
    {  data1,   axImp, B                        , iADD	},  /* 04 */
    {  data2,   axImp, 0                        , iADD	},  /* 05 */
    {  segop,   none2, NO_SRC                   , iPUSH	},  /* 06 */
    {  segop,   none2, NO_SRC 			, iPOP	},  /* 07 */
    {  modrm,   none2, B                        , iOR	},  /* 08 */
    {  modrm,   none2, NSP                      , iOR	},  /* 09 */
    {  modrm,   none2, TO_REG | B               , iOR	},  /* 0A */
    {  modrm,   none2, TO_REG | NSP		, iOR	},  /* 0B */
    {  data1,   axImp, B                        , iOR	},  /* 0C */
    {  data2,   axImp, 0                        , iOR	},	/* 0D */
    {  segop,   none2, NO_SRC                   , iPUSH	},	/* 0E */
    {  none1,   none2, OP386                    , iZERO },	/* 0F */
    {  modrm,   none2, B                        , iADC	},	/* 10 */
    {  modrm,   none2, NSP                      , iADC	},	/* 11 */
    {  modrm,   none2, TO_REG | B               , iADC	},	/* 12 */
    {  modrm,   none2, TO_REG | NSP		, iADC	},	/* 13 */
    {  data1,   axImp, B                        , iADC	},	/* 14 */
    {  data2,   axImp, 0			, iADC	},	/* 15 */
    {  segop,   none2, NOT_HLL | NO_SRC         , iPUSH	},	/* 16 */
    {  segop,   none2, NOT_HLL | NO_SRC         , iPOP	},	/* 17 */
    {  modrm,   none2, B			, iSBB	},	/* 18 */
    {  modrm,   none2, NSP                      , iSBB	},	/* 19 */
    {  modrm,   none2, TO_REG | B               , iSBB	},	/* 1A */
    {  modrm,   none2, TO_REG | NSP		, iSBB	},	/* 1B */
    {  data1,   axImp, B            		, iSBB	},	/* 1C */
    {  data2,   axImp, 0                        , iSBB  },	/* 1D */
    {  segop,   none2, NO_SRC                   , iPUSH	},	/* 1E */
    {  segop,   none2, NO_SRC                   , iPOP	},	/* 1F */
    {  modrm,   none2, B                        , iAND	},	/* 20 */
    {  modrm,   none2, NSP                      , iAND	},	/* 21 */
    {  modrm,   none2, TO_REG | B               , iAND	},	/* 22 */
    {  modrm,   none2, TO_REG | NSP             , iAND	},	/* 23 */
    {  data1,   axImp, B                        , iAND	},	/* 24 */
    {  data2,   axImp, 0                        , iAND	},	/* 25 */
    { prefix,   none2, 0                        , (IC)rES},	/* 26 */
    {  none1,   axImp, NOT_HLL | B|NO_SRC	, iDAA	},	/* 27 */
    {  modrm,   none2, B                        , iSUB	},	/* 28 */
    {  modrm,   none2, 0                        , iSUB	},	/* 29 */
    {  modrm,   none2, TO_REG | B               , iSUB	},	/* 2A */
    {  modrm,   none2, TO_REG                   , iSUB	},	/* 2B */
    {  data1,   axImp, B                        , iSUB	},	/* 2C */
    {  data2,   axImp, 0                        , iSUB	},	/* 2D */
    { prefix,   none2, 0                        , (IC)rCS},	/* 2E */
    {  none1,   axImp, NOT_HLL | B|NO_SRC       , iDAS  },	/* 2F */
    {  modrm,   none2, B                        , iXOR  },	/* 30 */
    {  modrm,   none2, NSP                      , iXOR  },	/* 31 */
    {  modrm,   none2, TO_REG | B               , iXOR  },	/* 32 */
    {  modrm,   none2, TO_REG | NSP             , iXOR  },	/* 33 */
    {  data1,   axImp, B                        , iXOR	},	/* 34 */
    {  data2,   axImp, 0                        , iXOR	},	/* 35 */
    { prefix,   none2, 0                        , (IC)rSS},	/* 36 */
    {  none1,   axImp, NOT_HLL | NO_SRC         , iAAA  },	/* 37 */
    {  modrm,   none2, B                        , iCMP	},	/* 38 */
    {  modrm,   none2, NSP                      , iCMP	},	/* 39 */
    {  modrm,   none2, TO_REG | B               , iCMP	},	/* 3A */
    {  modrm,   none2, TO_REG | NSP             , iCMP	},	/* 3B */
    {  data1,   axImp, B                        , iCMP	},	/* 3C */
    {  data2,   axImp, 0                        , iCMP	},	/* 3D */
    { prefix,   none2, 0                        , (IC)rDS},	/* 3E */
    {  none1,   axImp, NOT_HLL | NO_SRC         , iAAS  },	/* 3F */
    {  regop,   none2, 0                        , iINC	},	/* 40 */
    {  regop,   none2, 0                        , iINC	},	/* 41 */
    {  regop,   none2, 0                        , iINC	},	/* 42 */
    {  regop,   none2, 0                        , iINC	},	/* 43 */
    {  regop,   none2, NOT_HLL			, iINC	},	/* 44 */
    {  regop,   none2, 0                        , iINC	},	/* 45 */
    {  regop,   none2, 0                        , iINC	},	/* 46 */
    {  regop,   none2, 0                        , iINC	},	/* 47 */
    {  regop,   none2, 0                        , iDEC	},	/* 48 */
    {  regop,   none2, 0                        , iDEC	},	/* 49 */
    {  regop,   none2, 0                        , iDEC	},	/* 4A */
    {  regop,   none2, 0                        , iDEC	},	/* 4B */
    {  regop,   none2, NOT_HLL			, iDEC	},	/* 4C */
    {  regop,   none2, 0                        , iDEC	},	/* 4D */
    {  regop,   none2, 0                        , iDEC	},	/* 4E */
    {  regop,   none2, 0                        , iDEC	},	/* 4F */
    {  regop,   none2, NO_SRC                   , iPUSH },	/* 50 */
    {  regop,   none2, NO_SRC                   , iPUSH },	/* 51 */
    {  regop,   none2, NO_SRC                   , iPUSH },	/* 52 */
    {  regop,   none2, NO_SRC                   , iPUSH },	/* 53 */
    {  regop,   none2, NOT_HLL | NO_SRC         , iPUSH },	/* 54 */
    {  regop,   none2, NO_SRC                   , iPUSH },	/* 55 */
    {  regop,   none2, NO_SRC                   , iPUSH },	/* 56 */
    {  regop,   none2, NO_SRC                   , iPUSH },	/* 57 */
    {  regop,   none2, NO_SRC                   , iPOP  },	/* 58 */
    {  regop,   none2, NO_SRC                   , iPOP  },	/* 59 */
    {  regop,   none2, NO_SRC 			, iPOP  },	/* 5A */
    {  regop,   none2, NO_SRC                   , iPOP	},	/* 5B */
    {  regop,   none2, NOT_HLL | NO_SRC         , iPOP  },	/* 5C */
    {  regop,   none2, NO_SRC                   , iPOP  },	/* 5D */
    {  regop,   none2, NO_SRC                   , iPOP  },	/* 5E */
    {  regop,   none2, NO_SRC                   , iPOP  },	/* 5F */
    {  none1,   none2, NOT_HLL | NO_OPS         , iPUSHA},	/* 60 */
    {  none1,   none2, NOT_HLL | NO_OPS         , iPOPA },	/* 61 */
    { memOnly,  modrm, TO_REG | NSP		, iBOUND},	/* 62 */
    {  none1,   none2, OP386                    , iZERO },	/* 63 */
    {  none1,   none2, OP386                    , iZERO },	/* 64 */
    {  none1,   none2, OP386                    , iZERO },	/* 65 */
    {  none1,   none2, OP386                    , iZERO },	/* 66 */
    {  none1,   none2, OP386                    , iZERO },	/* 67 */
    {  data2,   none2, NO_SRC                   , iPUSH },	/* 68 */
    {  modrm,   data2, TO_REG | NSP             , iIMUL },	/* 69 */
    {  data1,   none2, S_EXT | NO_SRC           , iPUSH },	/* 6A */
    {  modrm,   data1, TO_REG | NSP | S_EXT	, iIMUL },	/* 6B */
    {  strop,  memImp, NOT_HLL | B|IM_OPS       , iINS  },	/* 6C */
    {  strop,  memImp, NOT_HLL | IM_OPS         , iINS  },	/* 6D */
    {  strop,  memImp, NOT_HLL | B|IM_OPS       , iOUTS },	/* 6E */
    {  strop,  memImp, NOT_HLL | IM_OPS         , iOUTS },	/* 6F */
    {  dispS,   none2, NOT_HLL			, iJO	},	/* 70 */
    {  dispS,   none2, NOT_HLL			, iJNO	},	/* 71 */
    {  dispS,   none2, 0                        , iJB	},	/* 72 */
    {  dispS,   none2, 0                        , iJAE	},	/* 73 */
    {  dispS,   none2, 0                        , iJE	},	/* 74 */
    {  dispS,   none2, 0                        , iJNE	},	/* 75 */
    {  dispS,   none2, 0                        , iJBE	},	/* 76 */
    {  dispS,   none2, 0                        , iJA	},	/* 77 */
    {  dispS,   none2, 0                        , iJS	},	/* 78 */
    {  dispS,   none2, 0                        , iJNS	},	/* 79 */
    {  dispS,   none2, NOT_HLL			, iJP	},	/* 7A */
    {  dispS,   none2, NOT_HLL			, iJNP	},	/* 7B */
    {  dispS,   none2, 0                        , iJL	},	/* 7C */
    {  dispS,   none2, 0                        , iJGE	},	/* 7D */
    {  dispS,   none2, 0                        , iJLE	},	/* 7E */
    {  dispS,   none2, 0                        , iJG	},	/* 7F */
    {  immed,   data1, B                        , iZERO	},	/* 80 */
    {  immed,   data2, NSP                      , iZERO	},	/* 81 */
    {  immed,   data1, B                        , iZERO	},	/* 82 */ /* ?? */
    {  immed,   data1, NSP | S_EXT		, iZERO	},	/* 83 */
    {  modrm,   none2, TO_REG | B		, iTEST	},	/* 84 */
    {  modrm,   none2, TO_REG | NSP		, iTEST	},	/* 85 */
    {  modrm,   none2, TO_REG | B               , iXCHG	},	/* 86 */
    {  modrm,   none2, TO_REG | NSP		, iXCHG	},	/* 87 */
    {  modrm,   none2, B                        , iMOV	},	/* 88 */
    {  modrm,   none2, 0            		, iMOV	},	/* 89 */
    {  modrm,   none2, TO_REG | B               , iMOV	},	/* 8A */
    {  modrm,   none2, TO_REG 			, iMOV	},	/* 8B */
    {  segrm,   none2, NSP                      , iMOV	},	/* 8C */
    { memOnly,  modrm, TO_REG | NSP		, iLEA	},	/* 8D */
    {  segrm,   none2, TO_REG | NSP		, iMOV	},	/* 8E */
    { memReg0,  none2, NO_SRC                   , iPOP	},	/* 8F */
    {   none1,  none2, NO_OPS                   , iNOP	},	/* 90 */
    {  regop,   axImp, 0                        , iXCHG	},	/* 91 */
    {  regop,   axImp, 0                        , iXCHG	},	/* 92 */
    {  regop,   axImp, 0                        , iXCHG	},	/* 93 */
    {  regop,   axImp, NOT_HLL			, iXCHG	},	/* 94 */
    {  regop,   axImp, 0                        , iXCHG	},	/* 95 */
    {  regop,   axImp, 0                        , iXCHG	},	/* 96 */
    {  regop,   axImp, 0                        , iXCHG	},	/* 97 */
    {  alImp,   axImp, SRC_B | S_EXT            , iSIGNEX},	/* 98 */
    {axSrcIm,   axImp, IM_DST | S_EXT           , iSIGNEX},	/* 99 */
    {  dispF,   none2, 0                        , iCALLF },	/* 9A */
    {  none1,   none2, FLOAT_OP| NO_OPS         , iWAIT	},	/* 9B */
    {  none1,   none2, NOT_HLL | NO_OPS         , iPUSHF},	/* 9C */
    {  none1,   none2, NOT_HLL | NO_OPS         , iPOPF	},	/* 9D */
    {  none1,   none2, NOT_HLL | NO_OPS         , iSAHF	},	/* 9E */
    {  none1,   none2, NOT_HLL | NO_OPS         , iLAHF	},	/* 9F */
    {  dispM,   axImp, B                        , iMOV	},	/* A0 */
    {  dispM,   axImp, 0                        , iMOV	},	/* A1 */
    {  dispM,   axImp, TO_REG | B               , iMOV	},	/* A2 */
    {  dispM,   axImp, TO_REG 			, iMOV	},	/* A3 */
    {  strop,  memImp, B | IM_OPS               , iMOVS	},	/* A4 */
    {  strop,  memImp, IM_OPS                   , iMOVS	},	/* A5 */
    {  strop,  memImp, B | IM_OPS               , iCMPS	},	/* A6 */
    {  strop,  memImp, IM_OPS                   , iCMPS	},	/* A7 */
    {  data1,   axImp, B                        , iTEST	},	/* A8 */
    {  data2,   axImp, 0                        , iTEST	},	/* A9 */
    {  strop,  memImp, B | IM_OPS               , iSTOS	},	/* AA */
    {  strop,  memImp, IM_OPS                   , iSTOS	},	/* AB */
    {  strop,  memImp, B | IM_OPS               , iLODS	},	/* AC */
    {  strop,  memImp, IM_OPS                   , iLODS	},	/* AD */
    {  strop,  memImp, B | IM_OPS               , iSCAS	},	/* AE */
    {  strop,  memImp, IM_OPS                   , iSCAS	},	/* AF */
    {  regop,   data1, B                        , iMOV	},	/* B0 */
    {  regop,   data1, B                        , iMOV	},	/* B1 */
    {  regop,   data1, B                        , iMOV	},	/* B2 */
    {  regop,   data1, B                        , iMOV	},	/* B3 */
    {  regop,   data1, B                        , iMOV	},	/* B4 */
    {  regop,   data1, B                        , iMOV	},	/* B5 */
    {  regop,   data1, B                        , iMOV	},	/* B6 */
    {  regop,   data1, B                        , iMOV	},	/* B7 */
    {  regop,   data2, 0                        , iMOV	},	/* B8 */
    {  regop,   data2, 0                        , iMOV	},	/* B9 */
    {  regop,   data2, 0                        , iMOV	},	/* BA */
    {  regop,   data2, 0                        , iMOV	},	/* BB */
    {  regop,   data2, NOT_HLL			, iMOV	},	/* BC */
    {  regop,   data2, 0                        , iMOV	},	/* BD */
    {  regop,   data2, 0                        , iMOV	},	/* BE */
    {  regop,   data2, 0                        , iMOV	},	/* BF */
    {  shift,   data1, B                        , iZERO	},	/* C0 */
    {  shift,   data1, NSP | SRC_B		, iZERO	},	/* C1 */
    {  data2,   none2, 0                        , iRET	},	/* C2 */
    {  none1,   none2, NO_OPS                   , iRET	},	/* C3 */
    { memOnly,  modrm, TO_REG | NSP		, iLES	},	/* C4 */
    { memOnly,  modrm, TO_REG | NSP		, iLDS	},	/* C5 */
    { memReg0,  data1, B                        , iMOV	},	/* C6 */
    { memReg0,  data2, 0			, iMOV	},	/* C7 */
    {  data2,   data1, 0                        , iENTER},	/* C8 */
    {  none1,   none2, NO_OPS                   , iLEAVE},	/* C9 */
    {  data2,   none2, 0                        , iRETF	},	/* CA */
    {  none1,   none2, NO_OPS                   , iRETF	},	/* CB */
    { const3,   none2, NOT_HLL			, iINT	},	/* CC */
    {  data1,checkInt, NOT_HLL			, iINT	},	/* CD */
    {  none1,   none2, NOT_HLL | NO_OPS         , iINTO	},	/* CE */
    {  none1,   none2, NOT_HLL | NO_OPS         , iIRET	},	/* Cf */
    {  shift,  const1, B                        , iZERO	},	/* D0 */
    {  shift,  const1, SRC_B                    , iZERO	},	/* D1 */
    {  shift,   none1, B                        , iZERO	},	/* D2 */
    {  shift,   none1, SRC_B                    , iZERO	},	/* D3 */
    {  data1,   axImp, NOT_HLL			, iAAM	},	/* D4 */
    {  data1,   axImp, NOT_HLL			, iAAD	},	/* D5 */
    {  none1,   none2, 0                        , iZERO	},	/* D6 */
    { memImp,   axImp, NOT_HLL | B| IM_OPS      , iXLAT	},	/* D7 */
    {  escop,   none2, FLOAT_OP			, iESC	},	/* D8 */
    {  escop,   none2, FLOAT_OP			, iESC	},	/* D9 */
    {  escop,   none2, FLOAT_OP			, iESC	},	/* DA */
    {  escop,   none2, FLOAT_OP			, iESC	},	/* DB */
    {  escop,   none2, FLOAT_OP			, iESC	},	/* DC */
    {  escop,   none2, FLOAT_OP			, iESC	},	/* DD */
    {  escop,   none2, FLOAT_OP			, iESC	},	/* DE */
    {  escop,   none2, FLOAT_OP			, iESC	},	/* Df */
    {  dispS,   none2, 0                        , iLOOPNE},	/* E0 */
    {  dispS,   none2, 0                        , iLOOPE},	/* E1 */
    {  dispS,   none2, 0                        , iLOOP	},	/* E2 */
    {  dispS,   none2, 0                        , iJCXZ	},	/* E3 */
    {  data1,   axImp, NOT_HLL | B|NO_SRC       , iIN	},	/* E4 */
    {  data1,   axImp, NOT_HLL | NO_SRC         , iIN	},	/* E5 */
    {  data1,   axImp, NOT_HLL | B|NO_SRC       , iOUT	},	/* E6 */
    {  data1,   axImp, NOT_HLL | NO_SRC         , iOUT	},	/* E7 */
    {  dispN,   none2, 0                        , iCALL	},	/* E8 */
    {  dispN,   none2, 0                        , iJMP	},	/* E9 */
    {  dispF,   none2, 0                        , iJMPF	},	/* EA */
    {  dispS,   none2, 0                        , iJMP	},	/* EB */
    {  none1,   axImp, NOT_HLL | B|NO_SRC       , iIN	},	/* EC */
    {  none1,   axImp, NOT_HLL | NO_SRC         , iIN	},	/* ED */
    {  none1,   axImp, NOT_HLL | B|NO_SRC       , iOUT	},	/* EE */
    {  none1,   axImp, NOT_HLL | NO_SRC         , iOUT	},	/* EF */
    {  none1,   none2, NOT_HLL | NO_OPS         , iLOCK	},	/* F0 */
    {  none1,   none2, 0                        , iZERO	},	/* F1 */
    { prefix,   none2, 0                        , iREPNE},	/* F2 */
    { prefix,   none2, 0                        , iREPE	},	/* F3 */
    {  none1,   none2, NOT_HLL | NO_OPS         , iHLT	},	/* F4 */
    {  none1,   none2, NO_OPS                   , iCMC	},	/* F5 */
    {  arith,   none1, B                        , iZERO	},	/* F6 */
    {  arith,   none1, NSP			, iZERO	},	/* F7 */
    {  none1,   none2, NO_OPS                   , iCLC	},	/* F8 */
    {  none1,   none2, NO_OPS                   , iSTC	},	/* F9 */
    {  none1,   none2, NOT_HLL | NO_OPS         , iCLI	},	/* FA */
    {  none1,   none2, NOT_HLL | NO_OPS         , iSTI	},	/* FB */
    {  none1,   none2, NO_OPS                   , iCLD	},	/* FC */
    {  none1,   none2, NO_OPS                   , iSTD	},	/* FD */
    {  trans,   none1, B                        , iZERO	},	/* FE */
    {  trans,   none1, NSP                      , iZERO	}	/* FF */
} ;

static uint16_t    SegPrefix, RepPrefix;
static const uint8_t  *pInst;		/* Ptr. to current uint8_t of instruction */
static ICODE * pIcode;		/* Ptr to Icode record filled in by scan() */


static void decodeBranchTgt(x86_insn_t &insn)
{
    x86_op_t *tgt_op = insn.x86_get_branch_target();
    if(tgt_op->type==op_expression)
        return; // unhandled for now
    if(tgt_op->type==op_register)
        return; // unhandled for now
    int32_t addr = tgt_op->getAddress();
    if(tgt_op->is_relative())
    {
        addr +=  insn.addr+insn.size;
    }
    pIcode->ll()->replaceSrc((uint32_t)addr);
    pIcode->ll()->setFlags(I);
    //    PROG &prog(Project::get()->prog);
    //    long off = (short)getWord();	/* Signed displacement */
    //    assert(addr==(uint32_t)(off + (unsigned)(pInst - prog.image())));

}

static void convertUsedFlags(x86_insn_t &from,ICODE &to)
{
    to.ll()->flagDU.d=0;
    to.ll()->flagDU.u=0;
    if(from.containsFlag(insn_eflag_carry,from.flags_set))
        to.ll()->flagDU.d |= Cf;
    if(from.containsFlag(insn_eflag_sign,from.flags_set))
        to.ll()->flagDU.d |= Sf;
    if(from.containsFlag(insn_eflag_zero,from.flags_set))
        to.ll()->flagDU.d |= Zf;
    if(from.containsFlag(insn_eflag_direction,from.flags_set))
        to.ll()->flagDU.d |= Df;

    if(from.containsFlag(insn_eflag_carry,from.flags_tested))
        to.ll()->flagDU.u |= Cf;
    if(from.containsFlag(insn_eflag_sign,from.flags_tested))
        to.ll()->flagDU.u |= Sf;
    if(from.containsFlag(insn_eflag_zero,from.flags_tested))
        to.ll()->flagDU.u |= Zf;
    if(from.containsFlag(insn_eflag_direction,from.flags_tested))
        to.ll()->flagDU.u |= Df;
}
static void convertPrefix(x86_insn_prefix prefix,ICODE &to)
{
    if(prefix ==insn_no_prefix)
        return;
    // insn_lock - no need to handle
    RepPrefix = (uint16_t)prefix & ~insn_lock;
}
/****************************************************************************
 Checks for int 34 to int 3B - if so, converts to ESC nn instruction
 ****************************************************************************/
static void fixFloatEmulation(x86_insn_t &insn)
{
    if(insn.operand_count==0)
        return;
    if(insn.group!=x86_insn_t::insn_interrupt)
        return;
    PROG &prog(Project::get()->prog);
    uint16_t wOp=insn.x86_get_imm()->data.word;
    if ((wOp < 0x34) || (wOp > 0x3B))
        return;
    uint8_t buf[16];
    /* This is a Borland/Microsoft floating point emulation instruction. Treat as if it is an ESC opcode */

    int actual_valid_bytes=std::min(16U,prog.cbImage-insn.offset);
    memcpy(buf,prog.image()+insn.offset,actual_valid_bytes);
    X86_Disasm ds(opt_16_bit);
    x86_insn_t patched_insn;
    //patch actual instruction into buffer;
    buf[1] = wOp-0x34+0xD8;
    ds.x86_disasm(buf,actual_valid_bytes,0,1,&patched_insn);
    patched_insn.addr   = insn.addr; // actual address
    patched_insn.offset = insn.offset; // actual offset
    insn = patched_insn;
    insn.size += 1; // to account for emulator call INT
}

int disassembleOneLibDisasm(uint32_t ip,x86_insn_t &l)
{
    PROG &prog(Project::get()->prog);
    X86_Disasm ds(opt_16_bit);
    int cnt=ds.x86_disasm(prog.image(),prog.cbImage,0,ip,&l);
    if(cnt && l.is_valid())
    {
        fixFloatEmulation(l); //can change 'l'
    }
    if(l.is_valid())
        return l.size;
    return 0;
}
eReg convertRegister(const x86_reg_t &reg)
{

    eReg regmap[]={ rUNDEF,
                    rUNDEF,rUNDEF,rUNDEF,rUNDEF,   //eax ecx ebx edx
                    rSP,rUNDEF,rUNDEF,rUNDEF,   //esp ebp esi edi
                    rAX,rCX,rDX,rBX,
                    rSP,rBP,rSI,rDI,
                    rAL,rCL,rDL,rBL,
                    rAH,rCH,rDH,rBH
                  };
    assert(reg.id<sizeof(regmap)/sizeof(eReg));
    assert(regmap[reg.id]!=rUNDEF);
    return regmap[reg.id];
}
LLOperand convertOperand(const x86_op_t &from)
{
    switch(from.type)
    {
        case op_unused:
            break;
        case op_register:
            return LLOperand::CreateReg2(convertRegister(from.data.reg));
        case op_immediate:
            return LLOperand::CreateImm2(from.data.sdword);
        default:
            fprintf(stderr,"convertOperand does not know how to convert %d\n",from.type);
    }
    return LLOperand::CreateImm2(0);
}
/*****************************************************************************
 Scans one machine instruction at offset ip in prog.Image and returns error.
 At the same time, fill in low-level icode details for the scanned inst.
 ****************************************************************************/

eErrorId scan(uint32_t ip, ICODE &p)
{
    PROG &prog(Project::get()->prog);
    int  op;
    p = ICODE();
    p.type = LOW_LEVEL;
    p.ll()->label = ip;			/* ip is absolute offset into image*/
    if (ip >= (uint32_t)prog.cbImage)
    {
        return (IP_OUT_OF_RANGE);
    }
    int cnt=disassembleOneLibDisasm(ip,p.insn);
    if(cnt)
    {
        convertUsedFlags(p.insn,p);
        convertPrefix(p.insn.prefix,p);

    }

    SegPrefix = RepPrefix = 0;
    pInst    = prog.image() + ip;
    pIcode   = &p;

    do
    {
        op = *pInst++;						/* First state - trivial   */
        /* Convert to Icode.opcode */
        p.ll()->set(stateTable[op].opcode,stateTable[op].flg & ICODEMASK);
        (*stateTable[op].state1)(op);		/* Second state */
        (*stateTable[op].state2)(op);		/* Third state  */

    } while (stateTable[op].state1 == prefix);	/* Loop if prefix */
    if(p.insn.group == x86_insn_t::insn_controlflow)
    {
        if(p.insn.x86_get_branch_target())
            decodeBranchTgt(p.insn);
    }
    x86_op_t *dst_op = p.insn.get_dest();
    static int only_first=1;
    if(dst_op && only_first)
    {
        only_first = 0;
        LLOperand conv = convertOperand(*dst_op);
        p.ll()->dst=conv;
        //assert(conv==p.ll()->dst);
    }
    if (p.ll()->getOpcode())
    {
        /* Save bytes of image used */
        p.ll()->numBytes = (uint8_t)((pInst - prog.image()) - ip);
        if(p.insn.is_valid())
            assert(p.ll()->numBytes == p.insn.size);
        p.ll()->numBytes = p.insn.size;
        return ((SegPrefix)? FUNNY_SEGOVR:  /* Seg. Override invalid */
                             (RepPrefix ? FUNNY_REP: NO_ERR));/* REP prefix invalid */
    }
    /* Else opcode error */
    return ((stateTable[op].flg & OP386)? INVALID_386OP: INVALID_OPCODE);
}

/***************************************************************************
 relocItem - returns true if uint16_t pointed at is in relocation table
 **************************************************************************/
static bool relocItem(const uint8_t *p)
{
    PROG &prog(Project::get()->prog);
    int		i;
    uint32_t	off = p - prog.image();

    for (i = 0; i < prog.cReloc; i++)
        if (prog.relocTable[i] == off)
            return true;
    return false;
}


/***************************************************************************
 getWord - returns next uint16_t from image
 **************************************************************************/
static uint16_t getWord(void)
{
    uint16_t w = LH(pInst);
    pInst += 2;
    return w;
}


/****************************************************************************
 signex - returns uint8_t sign extended to int
 ***************************************************************************/
static int signex(uint8_t b)
{
    long s = b;
    return ((b & 0x80)? (int)(0xFFFFFF00 | s): (int)s);
}

/****************************************************************************
 * setAddress - Updates the source or destination field for the current
 *	icode, based on fdst and the TO_REG flag.
 * 	Note: fdst == true is for the r/m part of the field (dest, unless TO_REG)
 *	      fdst == false is for reg part of the field
 ***************************************************************************/
static void setAddress(int i, bool fdst, uint16_t seg, int16_t reg, uint16_t off)
{
    LLOperand *pm;

    /* If not to register (i.e. to r/m), and talking about r/m, then this is dest */
    pm = (!(stateTable[i].flg & TO_REG) == fdst) ?
                &pIcode->ll()->dst : &pIcode->ll()->src();

    /* Set segment.  A later procedure (lookupAddr in proclist.c) will
         * provide the value of this segment in the field segValue.  */
    if (seg)  		/* segment override */
    {
        pm->seg = pm->segOver = (eReg)seg;
    }
    else
    {	/* no override, check indexed register */
        if ((reg >= INDEX_BX_SI) && (reg == INDEX_BP_SI || reg == INDEX_BP_DI || reg == INDEX_BP))
        {
            pm->seg = rSS;		/* indexed on bp */
        }
        else
        {
            pm->seg = rDS;		/* any other indexed reg */
        }
    }

    pm->regi = (eReg)reg;
    pm->off = (int16_t)off;
    if (reg && reg < INDEX_BX_SI && (stateTable[i].flg & B))
    {
        pm->regi = Machine_X86::subRegL(pm->regi);
    }

    if (seg)	/* So we can catch invalid use of segment overrides */
    {
        SegPrefix = 0;
    }
}


/****************************************************************************
 rm - Decodes r/m part of modrm uint8_t for dst (unless TO_REG) part of icode
 ***************************************************************************/
static void rm(int i)
{
    uint8_t mod = *pInst >> 6;
    uint8_t rm  = *pInst++ & 7;

    switch (mod) {
        case 0:		/* No disp unless rm == 6 */
            if (rm == 6) {
                setAddress(i, true, SegPrefix, 0, getWord());
                pIcode->ll()->setFlags(WORD_OFF);
            }
            else
                setAddress(i, true, SegPrefix, rm + INDEX_BX_SI, 0);
            break;

        case 1:		/* 1 uint8_t disp */
            setAddress(i, true, SegPrefix, rm+INDEX_BX_SI, (uint16_t)signex(*pInst++));
            break;

        case 2:		/* 2 uint8_t disp */
            setAddress(i, true, SegPrefix, rm + INDEX_BX_SI, getWord());
            pIcode->ll()->setFlags(WORD_OFF);
            break;

        case 3:		/* reg */
            setAddress(i, true, 0, rm + rAX, 0);
            break;
    }
    //pIcode->insn.get_dest()->
    if ((stateTable[i].flg & NSP) && (pIcode->ll()->src().getReg2()==rSP ||
                                      pIcode->ll()->dst.getReg2()==rSP))
        pIcode->ll()->setFlags(NOT_HLL);
}


/****************************************************************************
 modrm - Sets up src and dst from modrm uint8_t
 ***************************************************************************/
static void modrm(int i)
{
    setAddress(i, false, 0, REG(*pInst) + rAX, 0);
    rm(i);
}


/****************************************************************************
 segrm - seg encoded as reg of modrm
 ****************************************************************************/
static void segrm(int i)
{
    int	reg = REG(*pInst) + rES;

    if (reg > rDS || (reg == rCS && (stateTable[i].flg & TO_REG)))
        pIcode->ll()->setOpcode((llIcode)0); // setCBW because it has that index
    else {
        setAddress(i, false, 0, (int16_t)reg, 0);
        rm(i);
    }
}


/****************************************************************************
 regop - src/dst reg encoded as low 3 bits of opcode
 ***************************************************************************/
static void regop(int i)
{
    setAddress(i, false, 0, ((int16_t)i & 7) + rAX, 0);
    pIcode->ll()->replaceDst(LLOperand::CreateReg2(pIcode->ll()->src().getReg2()));
    //    pIcode->ll()->dst.regi = pIcode->ll()->src.regi;
}


/*****************************************************************************
 segop - seg encoded in middle of opcode
 *****************************************************************************/
static void segop(int i)
{
    setAddress(i, true, 0, (((int16_t)i & 0x18) >> 3) + rES, 0);
}


/****************************************************************************
 axImp - Plugs an implied AX dst
 ***************************************************************************/
static void axImp(int i)
{
    setAddress(i, true, 0, rAX, 0);
}

/* Implied AX source */
static void axSrcIm (int )
{
    pIcode->ll()->replaceSrc(rAX);//src.regi = rAX;
}

/* Implied AL source */
static void alImp (int )
{
    pIcode->ll()->replaceSrc(rAL);//src.regi = rAL;
}


/*****************************************************************************
 memImp - Plugs implied src memory operand with any segment override
 ****************************************************************************/
static void memImp(int i)
{
    setAddress(i, false, SegPrefix, 0, 0);
}


/****************************************************************************
 memOnly - Instruction is not valid if modrm refers to register (i.e. mod == 3)
 ***************************************************************************/
static void memOnly(int )
{
    if ((*pInst & 0xC0) == 0xC0)
        pIcode->ll()->setOpcode((llIcode)0);
}


/****************************************************************************
 memReg0 - modrm for 'memOnly' and Reg field must also be 0
 ****************************************************************************/
static void memReg0(int i)
{
    if (REG(*pInst) || (*pInst & 0xC0) == 0xC0)
        pIcode->ll()->setOpcode((llIcode)0);
    else
        rm(i);
}


/***************************************************************************
 immed - Sets up dst and opcode from modrm uint8_t
 **************************************************************************/
static void immed(int i)
{
    static llIcode immedTable[8] = {iADD, iOR, iADC, iSBB, iAND, iSUB, iXOR, iCMP};

    pIcode->ll()->setOpcode(immedTable[REG(*pInst)]) ;
    rm(i);

    if (pIcode->ll()->getOpcode() == iADD || pIcode->ll()->getOpcode() == iSUB)
        pIcode->ll()->clrFlags(NOT_HLL);	/* Allow ADD/SUB SP, immed */
}


/****************************************************************************
 shift  - Sets up dst and opcode from modrm uint8_t
 ***************************************************************************/
static void shift(int i)
{
    static llIcode shiftTable[8] =
    {
        (llIcode)iROL, (llIcode)iROR, (llIcode)iRCL, (llIcode)iRCR,
        (llIcode)iSHL, (llIcode)iSHR, (llIcode)0,	 (llIcode)iSAR};

    pIcode->ll()->setOpcode(shiftTable[REG(*pInst)]);
    rm(i);
    pIcode->ll()->replaceSrc(rCL); //src.regi =
}


/****************************************************************************
 trans - Sets up dst and opcode from modrm uint8_t
 ***************************************************************************/
static void trans(int i)
{
    static llIcode transTable[8] =
    {
        (llIcode)iINC, (llIcode)iDEC, (llIcode)iCALL, (llIcode)iCALLF,
        (llIcode)iJMP, (llIcode)iJMPF,(llIcode)iPUSH, (llIcode)0
    };
    LLInst *ll = pIcode->ll();
    if ((uint8_t)REG(*pInst) < 2 || !(stateTable[i].flg & B)) { /* INC & DEC */
        ll->setOpcode(transTable[REG(*pInst)]);   /* valid on bytes */
        rm(i);
        ll->replaceSrc( pIcode->ll()->dst );
        if (ll->match(iJMP) || ll->match(iCALL) || ll->match(iCALLF))
            ll->setFlags(NO_OPS);
        else if (ll->match(iINC) || ll->match(iPUSH) || ll->match(iDEC))
            ll->setFlags(NO_SRC);
    }
}


/****************************************************************************
 arith - Sets up dst and opcode from modrm uint8_t
 ****************************************************************************/
static void arith(int i)
{
    uint8_t opcode;
    static llIcode arithTable[8] =
    {
        iTEST,  (llIcode)0, iNOT, iNEG,
        iMUL ,       iIMUL, iDIV, iIDIV
    };
    opcode = arithTable[REG(*pInst)];
    pIcode->ll()->setOpcode((llIcode)opcode);
    rm(i);
    if (opcode == iTEST)
    {
        if (stateTable[i].flg & B)
            data1(i);
        else
            data2(i);
    }
    else if (!(opcode == iNOT || opcode == iNEG))
    {
        pIcode->ll()->replaceSrc( pIcode->ll()->dst );
        setAddress(i, true, 0, rAX, 0);			/* dst = AX  */
    }
    else if (opcode == iNEG || opcode == iNOT)
        pIcode->ll()->setFlags(NO_SRC);

    if ((opcode == iDIV) || (opcode == iIDIV))
    {
        if ( not pIcode->ll()->testFlags(B) )
            pIcode->ll()->setFlags(IM_TMP_DST);
    }
}


/*****************************************************************************
 data1 - Sets up immed from 1 uint8_t data
 *****************************************************************************/
static void data1(int i)
{
    pIcode->ll()->replaceSrc(LLOperand::CreateImm2((stateTable[i].flg & S_EXT)? signex(*pInst++): *pInst++));
    pIcode->ll()->setFlags(I);
}


/*****************************************************************************
 data2 - Sets up immed from 2 uint8_t data
 ****************************************************************************/
static void data2(int )
{
    if (relocItem(pInst))
        pIcode->ll()->setFlags(SEG_IMMED);

    /* ENTER is a special case, it does not take a destination operand,
         * but this field is being used as the number of bytes to allocate
         * on the stack.  The procedure level is stored in the immediate
         * field.  There is no source operand; therefore, the flag flg is
         * set to NO_OPS.	*/
    if (pIcode->ll()->getOpcode() == iENTER)
    {
        pIcode->ll()->dst.off = getWord();
        pIcode->ll()->setFlags(NO_OPS);
    }
    else
        pIcode->ll()->replaceSrc(getWord());
    pIcode->ll()->setFlags(I);
}


/****************************************************************************
 dispM - 2 uint8_t offset without modrm (== mod 0, rm 6) (Note:TO_REG bits are
         reversed)
 ****************************************************************************/
static void dispM(int i)
{
    setAddress(i, false, SegPrefix, 0, getWord());
}
/****************************************************************************
 dispN - 2 uint8_t disp as immed relative to ip
 ****************************************************************************/
static void dispN(int )
{
    //PROG &prog(Project::get()->prog);
    /*long off = (short)*/getWord();	/* Signed displacement */

    /* Note: the result of the subtraction could be between 32k and 64k, and
        still be positive; it is an offset from prog.Image. So this must be
        treated as unsigned */
    //    decodeBranchTgt();
}


/***************************************************************************
 dispS - 1 byte disp as immed relative to ip
 ***************************************************************************/
static void dispS(int )
{
    /*long off =*/ signex(*pInst++); 	/* Signed displacement */

    //    decodeBranchTgt();
}


/****************************************************************************
 dispF - 4 byte disp as immed 20-bit target address
 ***************************************************************************/
static void dispF(int )
{
    /*off = */(unsigned)getWord();
    /*seg = */(unsigned)getWord();
    //    decodeBranchTgt();
}


/****************************************************************************
 prefix - picks up prefix uint8_t for following instruction (LOCK is ignored
          on purpose)
 ****************************************************************************/
static void prefix(int )
{
    if (pIcode->ll()->getOpcode() == iREPE || pIcode->ll()->getOpcode() == iREPNE)
        RepPrefix = pIcode->ll()->getOpcode();
    else
        SegPrefix = pIcode->ll()->getOpcode();
}

inline void BumpOpcode(LLInst &ll)
{
    llIcode ic((llIcode)ll.getOpcode());
    ic = (llIcode)(((int)ic)+1);		// Bump this icode via the int type
    ll.setOpcode(ic);
}

/*****************************************************************************
 strop - checks RepPrefix and converts string instructions accordingly
 *****************************************************************************/
static void strop(int )
{
    if (RepPrefix)
    {
        if ( pIcode->ll()->match(iCMPS) || pIcode->ll()->match(iSCAS) )
        {
            if(pIcode->insn.prefix &  insn_rep_zero)
            {
                BumpOpcode(*pIcode->ll()); // iCMPS -> iREPE_CMPS
                BumpOpcode(*pIcode->ll());
            }
            else if(pIcode->insn.prefix &  insn_rep_notzero)
                BumpOpcode(*pIcode->ll()); // iX -> iREPNE_X
        }
        else
            if(pIcode->insn.prefix &  insn_rep_zero)
                BumpOpcode(*pIcode->ll()); // iX -> iREPE_X
        if (pIcode->ll()->match(iREP_LODS) )
            pIcode->ll()->setFlags(NOT_HLL);
        RepPrefix = 0;
    }
}


/***************************************************************************
 escop - esc operands
 ***************************************************************************/
static void escop(int i)
{
    pIcode->ll()->replaceSrc(REG(*pInst) + (uint32_t)((i & 7) << 3));
    pIcode->ll()->setFlags(I);
    rm(i);
}


/****************************************************************************
 const1
 ****************************************************************************/
static void const1(int )
{
    pIcode->ll()->replaceSrc(1);
    pIcode->ll()->setFlags(I);
}


/*****************************************************************************
 const3
 ****************************************************************************/
static void const3(int )
{
    pIcode->ll()->replaceSrc(3);
    pIcode->ll()->setFlags(I);
}


/****************************************************************************
 none1
 ****************************************************************************/
static void none1(int )
{
}


/****************************************************************************
 none2 - Sets the NO_OPS flag if the operand is immediate
 ****************************************************************************/
static void none2(int )
{
    if ( pIcode->ll()->testFlags(I) )
        pIcode->ll()->setFlags(NO_OPS);
}

/****************************************************************************
 Checks for int 34 to int 3B - if so, converts to ESC nn instruction
 ****************************************************************************/
static void checkInt(int )
{
    uint16_t wOp = (uint16_t) pIcode->ll()->src().getImm2();
    if ((wOp >= 0x34) && (wOp <= 0x3B))
    {
        /* This is a Borland/Microsoft floating point emulation instruction.
            Treat as if it is an ESC opcode */
        pIcode->ll()->replaceSrc(wOp - 0x34);
        pIcode->ll()->set(iESC,FLOAT_OP);

        escop(wOp - 0x34 + 0xD8);

    }
}
