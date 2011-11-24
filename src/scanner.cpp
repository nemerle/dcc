/*****************************************************************************
 *			dcc project scanner module
 * Implements a simple state driven scanner to convert 8086 machine code into
 * I-code
 * (C) Cristina Cifuentes, Jeff Ledermann
 ****************************************************************************/

#include "dcc.h"
#include "scanner.h"
#include <string.h>

#define iZERO (llIcode)0			// For neatness
#define IC	  llIcode

static struct {
    void (*state1)(Int);
    void (*state2)(Int);
    flags32 flg;
    llIcode opcode;
    byte df;
    byte uf;
} stateTable[] = {
    {  modrm,   none2, B					, iADD	, Sf | Zf | Cf,		},	/* 00 */
    {  modrm,   none2, 0 					, iADD	, Sf | Zf | Cf,		},	/* 01 */
    {  modrm,   none2, TO_REG | B			, iADD	, Sf | Zf | Cf,		},	/* 02 */
    {  modrm,   none2, TO_REG 			, iADD	, Sf | Zf | Cf,		},	/* 03 */
    {  data1,   axImp, B					, iADD	, Sf | Zf | Cf,		},	/* 04 */
    {  data2,   axImp, 0					, iADD	, Sf | Zf | Cf,		},	/* 05 */
    {  segop,   none2, NO_SRC				, iPUSH	, 0	,				},	/* 06 */
    {  segop,   none2, NO_SRC 			, iPOP	, 0	,				},	/* 07 */
    {  modrm,   none2, B					, iOR	, Sf | Zf | Cf,		},	/* 08 */
    {  modrm,   none2, NSP				, iOR	, Sf | Zf | Cf,		},	/* 09 */
    {  modrm,   none2, TO_REG | B			, iOR	, Sf | Zf | Cf,		},	/* 0A */
    {  modrm,   none2, TO_REG | NSP		, iOR	, Sf | Zf | Cf,		},	/* 0B */
    {  data1,   axImp, B					, iOR	, Sf | Zf | Cf,		},	/* 0C */
    {  data2,   axImp, 0					, iOR	, Sf | Zf | Cf,		},	/* 0D */
    {  segop,   none2, NO_SRC				, iPUSH	, 0			  ,		},	/* 0E */
    {  none1,	  none2, OP386 				, iZERO , 0			  ,		},	/* 0F */
    {  modrm,   none2, B					, iADC	, Sf | Zf | Cf, Cf 	},	/* 10 */
    {  modrm,   none2, NSP				, iADC	, Sf | Zf | Cf, Cf 	},	/* 11 */
    {  modrm,   none2, TO_REG | B			, iADC	, Sf | Zf | Cf, Cf	},	/* 12 */
    {  modrm,   none2, TO_REG | NSP		, iADC	, Sf | Zf | Cf, Cf	},	/* 13 */
    {  data1,   axImp, B					, iADC	, Sf | Zf | Cf, Cf	},	/* 14 */
    {  data2,   axImp, 0					, iADC	, Sf | Zf | Cf, Cf	},	/* 15 */
    {  segop,   none2, NOT_HLL | NO_SRC	, iPUSH	, 0	,				},	/* 16 */
    {  segop,   none2, NOT_HLL | NO_SRC	, iPOP	, 0	,				},	/* 17 */
    {  modrm,   none2, B					, iSBB	, Sf | Zf | Cf, Cf	},	/* 18 */
    {  modrm,   none2, NSP				, iSBB	, Sf | Zf | Cf, Cf	},	/* 19 */
    {  modrm,   none2, TO_REG | B			, iSBB	, Sf | Zf | Cf, Cf	},	/* 1A */
    {  modrm,   none2, TO_REG | NSP		, iSBB	, Sf | Zf | Cf, Cf	},	/* 1B */
    {  data1,   axImp, B					, iSBB	, Sf | Zf | Cf, Cf 	},	/* 1C */
    {  data2,   axImp, 0					, iSBB	, Sf | Zf | Cf, Cf	},	/* 1D */
    {  segop,   none2, NO_SRC				, iPUSH	, 0	,				},	/* 1E */
    {  segop,   none2, NO_SRC				, iPOP	, 0	,				},	/* 1F */
    {  modrm,   none2, B					, iAND	, Sf | Zf | Cf,		},	/* 20 */
    {  modrm,   none2, NSP				, iAND	, Sf | Zf | Cf,		},	/* 21 */
    {  modrm,   none2, TO_REG | B			, iAND	, Sf | Zf | Cf,		},	/* 22 */
    {  modrm,   none2, TO_REG | NSP		, iAND	, Sf | Zf | Cf,		},	/* 23 */
    {  data1,   axImp, B					, iAND	, Sf | Zf | Cf,		},	/* 24 */
    {  data2,   axImp, 0					, iAND	, Sf | Zf | Cf,		},	/* 25 */
    { prefix,   none2, 0					, (IC)rES,0			  ,		},	/* 26 */
    {  none1,   axImp, NOT_HLL | B|NO_SRC	, iDAA	, Sf | Zf | Cf,		},	/* 27 */
    {  modrm,   none2, B					, iSUB	, Sf | Zf | Cf,		},	/* 28 */
    {  modrm,   none2, 0					, iSUB	, Sf | Zf | Cf,		},	/* 29 */
    {  modrm,   none2, TO_REG | B			, iSUB	, Sf | Zf | Cf,		},	/* 2A */
    {  modrm,   none2, TO_REG				, iSUB	, Sf | Zf | Cf,		},	/* 2B */
    {  data1,   axImp, B					, iSUB	, Sf | Zf | Cf,		},	/* 2C */
    {  data2,   axImp, 0					, iSUB	, Sf | Zf | Cf,		},	/* 2D */
    { prefix,   none2, 0					, (IC)rCS,0			  ,		},	/* 2E */
    {  none1,   axImp, NOT_HLL | B|NO_SRC	, iDAS	, Sf | Zf | Cf,		},	/* 2F */
    {  modrm,   none2, B					, iXOR	, Sf | Zf | Cf,		},	/* 30 */
    {  modrm,   none2, NSP				, iXOR	, Sf | Zf | Cf,		},	/* 31 */
    {  modrm,   none2, TO_REG | B			, iXOR	, Sf | Zf | Cf,		},	/* 32 */
    {  modrm,   none2, TO_REG | NSP		, iXOR	, Sf | Zf | Cf,		},	/* 33 */
    {  data1,   axImp, B					, iXOR	, Sf | Zf | Cf,		},	/* 34 */
    {  data2,   axImp, 0					, iXOR	, Sf | Zf | Cf,		},	/* 35 */
    { prefix,   none2, 0					, (IC)rSS,0			  ,		},	/* 36 */
    {  none1,   axImp, NOT_HLL | NO_SRC	, iAAA	, Sf | Zf | Cf,		},	/* 37 */
    {  modrm,   none2, B					, iCMP	, Sf | Zf | Cf,		},	/* 38 */
    {  modrm,   none2, NSP				, iCMP	, Sf | Zf | Cf,		},	/* 39 */
    {  modrm,   none2, TO_REG | B			, iCMP	, Sf | Zf | Cf,		},	/* 3A */
    {  modrm,   none2, TO_REG | NSP		, iCMP	, Sf | Zf | Cf,		},	/* 3B */
    {  data1,   axImp, B					, iCMP	, Sf | Zf | Cf,		},	/* 3C */
    {  data2,   axImp, 0					, iCMP	, Sf | Zf | Cf,		},	/* 3D */
    { prefix,   none2, 0					, (IC)rDS,0			  ,		},	/* 3E */
    {  none1,   axImp, NOT_HLL | NO_SRC	, iAAS	, Sf | Zf | Cf,		},	/* 3F */
    {  regop,   none2, 0					, iINC	, Sf | Zf,			},	/* 40 */
    {  regop,   none2, 0					, iINC	, Sf | Zf,			},	/* 41 */
    {  regop,   none2, 0					, iINC	, Sf | Zf,			},	/* 42 */
    {  regop,   none2, 0					, iINC	, Sf | Zf,			},	/* 43 */
    {  regop,   none2, NOT_HLL			, iINC	, Sf | Zf,			},	/* 44 */
    {  regop,   none2, 0					, iINC	, Sf | Zf,			},	/* 45 */
    {  regop,   none2, 0					, iINC	, Sf | Zf,			},	/* 46 */
    {  regop,   none2, 0					, iINC	, Sf | Zf,			},	/* 47 */
    {  regop,   none2, 0					, iDEC	, Sf | Zf,			},	/* 48 */
    {  regop,   none2, 0					, iDEC	, Sf | Zf,			},	/* 49 */
    {  regop,   none2, 0					, iDEC	, Sf | Zf,			},	/* 4A */
    {  regop,   none2, 0					, iDEC	, Sf | Zf,			},	/* 4B */
    {  regop,   none2, NOT_HLL			, iDEC	, Sf | Zf,			},	/* 4C */
    {  regop,   none2, 0					, iDEC	, Sf | Zf,			},	/* 4D */
    {  regop,   none2, 0					, iDEC	, Sf | Zf,			},	/* 4E */
    {  regop,   none2, 0					, iDEC	, Sf | Zf,			},	/* 4F */
    {  regop,   none2, NO_SRC				, iPUSH	, 0	,				},	/* 50 */
    {  regop,   none2, NO_SRC				, iPUSH	, 0	,				},	/* 51 */
    {  regop,   none2, NO_SRC				, iPUSH	, 0	,				},	/* 52 */
    {  regop,   none2, NO_SRC				, iPUSH	, 0	,				},	/* 53 */
    {  regop,   none2, NOT_HLL | NO_SRC	, iPUSH	, 0	,				},	/* 54 */
    {  regop,   none2, NO_SRC				, iPUSH	, 0	,				},	/* 55 */
    {  regop,   none2, NO_SRC				, iPUSH	, 0	,				},	/* 56 */
    {  regop,   none2, NO_SRC				, iPUSH	, 0	,				},	/* 57 */
    {  regop,   none2, NO_SRC				, iPOP	, 0	,				},	/* 58 */
    {  regop,   none2, NO_SRC				, iPOP	, 0	,				},	/* 59 */
    {  regop,   none2, NO_SRC 			, iPOP	, 0	,				},	/* 5A */
    {  regop,   none2, NO_SRC				, iPOP	, 0	,				},	/* 5B */
    {  regop,   none2, NOT_HLL | NO_SRC	, iPOP	, 0	,				},	/* 5C */
    {  regop,   none2, NO_SRC				, iPOP	, 0	,				},	/* 5D */
    {  regop,   none2, NO_SRC				, iPOP	, 0	,				},	/* 5E */
    {  regop,   none2, NO_SRC				, iPOP	, 0	,				},	/* 5F */
    {  none1,   none2, NOT_HLL | NO_OPS	, iPUSHA, 0	,				},	/* 60 */
    {  none1,   none2, NOT_HLL | NO_OPS	, iPOPA	, 0	,				},	/* 61 */
    { memOnly,  modrm, TO_REG | NSP		, iBOUND, 0	,				},	/* 62 */
    {  none1,   none2, OP386 				, iZERO	, 0	,				},	/* 63 */
    {  none1,   none2, OP386 				, iZERO	, 0	,				},	/* 64 */
    {  none1,   none2, OP386 				, iZERO	, 0	,				},	/* 65 */
    {  none1,   none2, OP386 				, iZERO	, 0	,				},	/* 66 */
    {  none1,   none2, OP386 				, iZERO	, 0	,				},	/* 67 */
    {  data2,   none2, NO_SRC				, iPUSH	, 0		,			},	/* 68 */
    {  modrm,   data2, TO_REG | NSP		, iIMUL	, Sf | Zf | Cf,		},	/* 69 */
    {  data1,   none2, S | NO_SRC			, iPUSH	, 0	,				},	/* 6A */
    {  modrm,   data1, TO_REG | NSP | S	, iIMUL	, Sf | Zf | Cf,		},	/* 6B */
    {  strop,  memImp, NOT_HLL | B|IM_OPS , iINS	, 0	, Df			},	/* 6C */
    {  strop,  memImp, NOT_HLL | IM_OPS	, iINS	, 0	, Df			},	/* 6D */
    {  strop,  memImp, NOT_HLL | B|IM_OPS , iOUTS	, 0	, Df			},	/* 6E */
    {  strop,  memImp, NOT_HLL | IM_OPS	, iOUTS	, 0	, Df			},	/* 6F */
    {  dispS,   none2, NOT_HLL			, iJO	, 0	,				},	/* 70 */
    {  dispS,   none2, NOT_HLL			, iJNO	, 0	,				},	/* 71 */
    {  dispS,   none2, 0					, iJB	, 0	, Cf			},	/* 72 */
    {  dispS,   none2, 0					, iJAE	, 0	, Cf			},	/* 73 */
    {  dispS,   none2, 0					, iJE	, 0	, Zf			},	/* 74 */
    {  dispS,   none2, 0					, iJNE	, 0	, Zf			},	/* 75 */
    {  dispS,   none2, 0					, iJBE	, 0	, Zf | Cf 		},	/* 76 */
    {  dispS,   none2, 0					, iJA	, 0	, Zf | Cf 		},	/* 77 */
    {  dispS,   none2, 0					, iJS	, 0	, Sf			},	/* 78 */
    {  dispS,   none2, 0					, iJNS	, 0	, Sf			},	/* 79 */
    {  dispS,   none2, NOT_HLL			, iJP	, 0	,				},	/* 7A */
    {  dispS,   none2, NOT_HLL			, iJNP	, 0	,				},	/* 7B */
    {  dispS,   none2, 0					, iJL	, 0	, Sf			},	/* 7C */
    {  dispS,   none2, 0					, iJGE	, 0	, Sf			},	/* 7D */
    {  dispS,   none2, 0					, iJLE	, 0	, Sf | Zf 		},	/* 7E */
    {  dispS,   none2, 0					, iJG	, 0	, Sf | Zf	 	},	/* 7F */
    {  immed,   data1, B					, iZERO	, 0	,				},	/* 80 */
    {  immed,   data2, NSP				, iZERO	, 0	,				},	/* 81 */
    {  immed,   data1, B					, iZERO	, 0	,				},	/* 82 */ /* ?? */
    {  immed,   data1, NSP | S			, iZERO	, 0	,				},	/* 83 */
    {  modrm,   none2, TO_REG | B			, iTEST	, Sf | Zf | Cf, 	},	/* 84 */
    {  modrm,   none2, TO_REG | NSP		, iTEST	, Sf | Zf | Cf, 	},	/* 85 */
    {  modrm,   none2, TO_REG | B			, iXCHG	, 0	,				},	/* 86 */
    {  modrm,   none2, TO_REG | NSP		, iXCHG	, 0	,				},	/* 87 */
    {  modrm,   none2, B					, iMOV	, 0	,				},	/* 88 */
    {  modrm,   none2, 0					, iMOV	, 0	,				},	/* 89 */
    {  modrm,   none2, TO_REG | B			, iMOV	, 0	,				},	/* 8A */
    {  modrm,   none2, TO_REG 			, iMOV	, 0	,				},	/* 8B */
    {  segrm,   none2, NSP				, iMOV	, 0	,				},	/* 8C */
    { memOnly,  modrm, TO_REG | NSP		, iLEA	, 0	,				},	/* 8D */
    {  segrm,   none2, TO_REG | NSP		, iMOV	, 0	,				},	/* 8E */
    { memReg0,  none2, NO_SRC				, iPOP	, 0	,				},	/* 8F */
    {   none1,  none2, NO_OPS				, iNOP	, 0	,				},	/* 90 */
    {  regop,   axImp, 0					, iXCHG	, 0	,				},	/* 91 */
    {  regop,   axImp, 0					, iXCHG	, 0	,				},	/* 92 */
    {  regop,   axImp, 0					, iXCHG	, 0	,				},	/* 93 */
    {  regop,   axImp, NOT_HLL			, iXCHG	, 0	,				},	/* 94 */
    {  regop,   axImp, 0					, iXCHG	, 0	,				},	/* 95 */
    {  regop,   axImp, 0					, iXCHG	, 0	,				},	/* 96 */
    {  regop,   axImp, 0					, iXCHG	, 0	,				},	/* 97 */
    {  alImp,   axImp, SRC_B | S			, iSIGNEX,0	,				},	/* 98 */
    {axSrcIm,   axImp, IM_DST | S			, iSIGNEX,0	,				},	/* 99 */
    {  dispF,   none2, 0					, iCALLF ,0	,				},	/* 9A */
    {  none1,   none2, FLOAT_OP| NO_OPS	, iWAIT	, 0	,				},	/* 9B */
    {  none1,   none2, NOT_HLL | NO_OPS	, iPUSHF, 0	,				},	/* 9C */
    {  none1,   none2, NOT_HLL | NO_OPS	, iPOPF	, Sf | Zf | Cf | Df,},	/* 9D */
    {  none1,   none2, NOT_HLL | NO_OPS	, iSAHF	, Sf | Zf | Cf,		},	/* 9E */
    {  none1,   none2, NOT_HLL | NO_OPS	, iLAHF	, 0 , Sf | Zf | Cf 	},	/* 9F */
    {  dispM,   axImp, B					, iMOV	, 0	,				},	/* A0 */
    {  dispM,   axImp, 0					, iMOV	, 0	,				},	/* A1 */
    {  dispM,   axImp, TO_REG | B			, iMOV	, 0	,				},	/* A2 */
    {  dispM,   axImp, TO_REG 			, iMOV	, 0	,				},	/* A3 */
    {  strop,  memImp, B | IM_OPS			, iMOVS	, 0	, Df			},	/* A4 */
    {  strop,  memImp, IM_OPS				, iMOVS	, 0	, Df			},	/* A5 */
    {  strop,  memImp, B | IM_OPS			, iCMPS	, Sf | Zf | Cf, Df	},	/* A6 */
    {  strop,  memImp, IM_OPS				, iCMPS	, Sf | Zf | Cf, Df	},	/* A7 */
    {  data1,   axImp, B					, iTEST	, Sf | Zf | Cf,		},	/* A8 */
    {  data2,   axImp, 0					, iTEST	, Sf | Zf | Cf,		},	/* A9 */
    {  strop,  memImp, B | IM_OPS			, iSTOS	, 0	, Df			},	/* AA */
    {  strop,  memImp, IM_OPS				, iSTOS	, 0	, Df			},	/* AB */
    {  strop,  memImp, B | IM_OPS			, iLODS	, 0	, Df			},	/* AC */
    {  strop,  memImp, IM_OPS				, iLODS	, 0	, Df			},	/* AD */
    {  strop,  memImp, B | IM_OPS			, iSCAS	, Sf | Zf | Cf, Df  },	/* AE */
    {  strop,  memImp, IM_OPS				, iSCAS	, Sf | Zf | Cf, Df	},	/* AF */
    {  regop,   data1, B					, iMOV	, 0	,				},	/* B0 */
    {  regop,   data1, B					, iMOV	, 0	,				},	/* B1 */
    {  regop,   data1, B					, iMOV	, 0	,				},	/* B2 */
    {  regop,   data1, B					, iMOV	, 0	,				},	/* B3 */
    {  regop,   data1, B					, iMOV	, 0	,				},	/* B4 */
    {  regop,   data1, B					, iMOV	, 0	,				},	/* B5 */
    {  regop,   data1, B					, iMOV	, 0	,				},	/* B6 */
    {  regop,   data1, B					, iMOV	, 0	,				},	/* B7 */
    {  regop,   data2, 0					, iMOV	, 0	,				},	/* B8 */
    {  regop,   data2, 0					, iMOV	, 0	,				},	/* B9 */
    {  regop,   data2, 0					, iMOV	, 0	,				},	/* BA */
    {  regop,   data2, 0					, iMOV	, 0	,				},	/* BB */
    {  regop,   data2, NOT_HLL			, iMOV	, 0	,				},	/* BC */
    {  regop,   data2, 0					, iMOV	, 0	,				},	/* BD */
    {  regop,   data2, 0					, iMOV	, 0	,				},	/* BE */
    {  regop,   data2, 0					, iMOV	, 0	,				},	/* BF */
    {  shift,   data1, B					, iZERO	, 0	,				},	/* C0 */
    {  shift,   data1, NSP | SRC_B		, iZERO	, 0	,				},	/* C1 */
    {  data2,   none2, 0					, iRET	, 0	,				},	/* C2 */
    {  none1,   none2, NO_OPS				, iRET	, 0	,				},	/* C3 */
    { memOnly,  modrm, TO_REG | NSP		, iLES	, 0	,				},	/* C4 */
    { memOnly,  modrm, TO_REG | NSP		, iLDS	, 0	,				},	/* C5 */
    { memReg0,  data1, B					, iMOV	, 0	,				},	/* C6 */
    { memReg0,  data2, 0					, iMOV	, 0	,				},	/* C7 */
    {  data2,   data1, 0					, iENTER, 0	,				},	/* C8 */
    {  none1,   none2, NO_OPS				, iLEAVE, 0	,				},	/* C9 */
    {  data2,   none2, 0					, iRETF	, 0	,				},	/* CA */
    {  none1,   none2, NO_OPS				, iRETF	, 0	,				},	/* CB */
    { const3,   none2, NOT_HLL			, iINT	, 0	,				},	/* CC */
    {  data1,checkInt, NOT_HLL			, iINT	, 0	,				},	/* CD */
    {  none1,   none2, NOT_HLL | NO_OPS	, iINTO	, 0	,				},	/* CE */
    {  none1,   none2, NOT_HLL | NO_OPS	, iIRET	, 0	,				},	/* Cf */
    {  shift,  const1, B					, iZERO	, 0	,				},	/* D0 */
    {  shift,  const1, SRC_B				, iZERO	, 0	,				},	/* D1 */
    {  shift,   none1, B					, iZERO	, 0	,				},	/* D2 */
    {  shift,   none1, SRC_B				, iZERO	, 0	,				},	/* D3 */
    {  data1,   axImp, NOT_HLL			, iAAM	, Sf | Zf | Cf,		},	/* D4 */
    {  data1,   axImp, NOT_HLL			, iAAD	, Sf | Zf | Cf,		},	/* D5 */
    {  none1,   none2, 0					, iZERO	, 0	,				},	/* D6 */
    { memImp,   axImp, NOT_HLL | B| IM_OPS, iXLAT	, 0	,				},	/* D7 */
    {  escop,   none2, FLOAT_OP			, iESC	, 0	,				},	/* D8 */
    {  escop,   none2, FLOAT_OP			, iESC	, 0	,				},	/* D9 */
    {  escop,   none2, FLOAT_OP			, iESC	, 0	,				},	/* DA */
    {  escop,   none2, FLOAT_OP			, iESC	, 0	,				},	/* DB */
    {  escop,   none2, FLOAT_OP			, iESC	, 0	,				},	/* DC */
    {  escop,   none2, FLOAT_OP			, iESC	, 0	,				},	/* DD */
    {  escop,   none2, FLOAT_OP			, iESC	, 0	,				},	/* DE */
    {  escop,   none2, FLOAT_OP			, iESC	, 0	,				},	/* Df */
    {  dispS,   none2, 0					, iLOOPNE,0	, Zf			},	/* E0 */
    {  dispS,   none2, 0					, iLOOPE, 0	, Zf			},	/* E1 */
    {  dispS,   none2, 0					, iLOOP	, 0	,				},	/* E2 */
    {  dispS,   none2, 0					, iJCXZ	, 0	,				},	/* E3 */
    {  data1,   axImp, NOT_HLL | B|NO_SRC , iIN	, 0	,				},	/* E4 */
    {  data1,   axImp, NOT_HLL | NO_SRC	, iIN	, 0	,				},	/* E5 */
    {  data1,   axImp, NOT_HLL | B|NO_SRC , iOUT	, 0	,				},	/* E6 */
    {  data1,   axImp, NOT_HLL | NO_SRC	, iOUT	, 0	,				},	/* E7 */
    {  dispN,   none2, 0					, iCALL	, 0	,				},	/* E8 */
    {  dispN,   none2, 0					, iJMP	, 0	,				},	/* E9 */
    {  dispF,   none2, 0					, iJMPF	, 0	,				},	/* EA */
    {  dispS,   none2, 0					, iJMP	, 0	,				},	/* EB */
    {  none1,   axImp, NOT_HLL | B|NO_SRC , iIN	, 0	,				},	/* EC */
    {  none1,   axImp, NOT_HLL | NO_SRC	, iIN	, 0	,				},	/* ED */
    {  none1,   axImp, NOT_HLL | B|NO_SRC , iOUT	, 0	,				},	/* EE */
    {  none1,   axImp, NOT_HLL | NO_SRC	, iOUT	, 0	,				},	/* EF */
    {  none1,   none2, NOT_HLL | NO_OPS	, iLOCK	, 0	,				},	/* F0 */
    {  none1,   none2, 0					, iZERO	, 0	,				},	/* F1 */
    { prefix,   none2, 0					, iREPNE, 0	,				},	/* F2 */
    { prefix,   none2, 0					, iREPE	, 0	,				},	/* F3 */
    {  none1,   none2, NOT_HLL | NO_OPS	, iHLT	, 0	,				},	/* F4 */
    {  none1,   none2, NO_OPS				, iCMC	, Cf, Cf			},	/* F5 */
    {  arith,   none1, B					, iZERO	, 0	,				},	/* F6 */
    {  arith,   none1, NSP				, iZERO	, 0	,				},	/* F7 */
    {  none1,   none2, NO_OPS				, iCLC	, Cf,				},	/* F8 */
    {  none1,   none2, NO_OPS				, iSTC	, Cf,				},	/* F9 */
    {  none1,   none2, NOT_HLL | NO_OPS	, iCLI	, 0	,				},	/* FA */
    {  none1,   none2, NOT_HLL | NO_OPS	, iSTI	, 0	,				},	/* FB */
    {  none1,   none2, NO_OPS				, iCLD	, Df,				},	/* FC */
    {  none1,   none2, NO_OPS				, iSTD	, Df,				},	/* FD */
    {  trans,   none1, B					, iZERO	, 0	,				},	/* FE */
    {  trans,   none1, NSP				, iZERO	, 0	,				}	/* FF */
} ;

static word    SegPrefix, RepPrefix;
static byte  *pInst;		/* Ptr. to current byte of instruction */
static ICODE * pIcode;		/* Ptr to Icode record filled in by scan() */


/*****************************************************************************
 Scans one machine instruction at offset ip in prog.Image and returns error.
 At the same time, fill in low-level icode details for the scanned inst.
 ****************************************************************************/
Int scan(dword ip, ICODE *p)
{
    Int  op;

    memset(p, 0, sizeof(ICODE));
    p->type = LOW_LEVEL;
    p->ic.ll.label = ip;			/* ip is absolute offset into image*/
    if (ip >= (dword)prog.cbImage)
    {
        return (IP_OUT_OF_RANGE);
    }

    SegPrefix = RepPrefix = 0;
    pInst    = prog.Image + ip;
    pIcode   = p;

    do
    {
        op = *pInst++;						/* First state - trivial   */
        p->ic.ll.opcode = stateTable[op].opcode;  /* Convert to Icode.opcode */
        p->ic.ll.flg    = stateTable[op].flg & ICODEMASK;
        p->ic.ll.flagDU.d = stateTable[op].df;
        p->ic.ll.flagDU.u = stateTable[op].uf;

        (*stateTable[op].state1)(op);		/* Second state */
        (*stateTable[op].state2)(op);		/* Third state  */

    } while (stateTable[op].state1 == prefix);	/* Loop if prefix */

    if (p->ic.ll.opcode)
    {
        /* Save bytes of image used */
        p->ic.ll.numBytes = (byte)((pInst - prog.Image) - ip);
        return ((SegPrefix)? FUNNY_SEGOVR:  /* Seg. Override invalid */
                             (RepPrefix ? FUNNY_REP: 0));/* REP prefix invalid */
    }
    /* Else opcode error */
    return ((stateTable[op].flg & OP386)? INVALID_386OP: INVALID_OPCODE);
}


/***************************************************************************
 relocItem - returns TRUE if word pointed at is in relocation table
 **************************************************************************/
static boolT relocItem(byte *p)
{
    Int		i;
    dword	off = p - prog.Image;

    for (i = 0; i < prog.cReloc; i++)
        if (prog.relocTable[i] == off)
            return TRUE;
    return FALSE;
}


/***************************************************************************
 getWord - returns next word from image
 **************************************************************************/
static word getWord(void)
{
    word w = LH(pInst);
    pInst += 2;
    return w;
}


/****************************************************************************
 signex - returns byte sign extended to Int
 ***************************************************************************/
static Int signex(byte b)
{
    long s = b;
    return ((b & 0x80)? (Int)(0xFFFFFF00 | s): (Int)s);
}


/****************************************************************************
 * setAddress - Updates the source or destination field for the current
 *	icode, based on fdst and the TO_REG flag.
 * 	Note: fdst == TRUE is for the r/m part of the field (dest, unless TO_REG)
 *	      fdst == FALSE is for reg part of the field
 ***************************************************************************/
static void setAddress(Int i, boolT fdst, word seg, int16 reg, word off)
{
    ICODEMEM *pm;

    /* If not to register (i.e. to r/m), and talking about r/m,
                then this is dest */
    pm = (!(stateTable[i].flg & TO_REG) == fdst) ?
                &pIcode->ic.ll.dst : &pIcode->ic.ll.src;

    /* Set segment.  A later procedure (lookupAddr in proclist.c) will
         * provide the value of this segment in the field segValue.  */
    if (seg)  		/* segment override */
    {
        pm->seg = pm->segOver = (byte)seg;
    }
    else
    {	/* no override, check indexed register */
        if ((reg >= INDEXBASE) && (reg == INDEXBASE + 2 ||
                                   reg == INDEXBASE + 3 || reg == INDEXBASE + 6))
        {
            pm->seg = rSS;		/* indexed on bp */
        }
        else
        {
            pm->seg = rDS;		/* any other indexed reg */
        }
    }
    pm->regi = (byte)reg;
    pm->off = (int16)off;
    if (reg && reg < INDEXBASE && (stateTable[i].flg & B))
    {
        pm->regi += rAL - rAX;
    }

    if (seg)	/* So we can catch invalid use of segment overrides */
    {
        SegPrefix = 0;
    }
}


/****************************************************************************
 rm - Decodes r/m part of modrm byte for dst (unless TO_REG) part of icode
 ***************************************************************************/
static void rm(Int i)
{
    byte mod = *pInst >> 6;
    byte rm  = *pInst++ & 7;

    switch (mod) {
        case 0:		/* No disp unless rm == 6 */
            if (rm == 6) {
                setAddress(i, TRUE, SegPrefix, 0, getWord());
                pIcode->ic.ll.flg |= WORD_OFF;
            }
            else	setAddress(i, TRUE, SegPrefix, rm + INDEXBASE, 0);
            break;

        case 1:		/* 1 byte disp */
            setAddress(i, TRUE, SegPrefix, rm+INDEXBASE, (word)signex(*pInst++));
            break;

        case 2:		/* 2 byte disp */
            setAddress(i, TRUE, SegPrefix, rm + INDEXBASE, getWord());
            pIcode->ic.ll.flg |= WORD_OFF;
            break;

        case 3:		/* reg */
            setAddress(i, TRUE, 0, rm + rAX, 0);
            break;
    }

    if ((stateTable[i].flg & NSP) && (pIcode->ic.ll.src.regi==rSP ||
                                      pIcode->ic.ll.dst.regi==rSP))
        pIcode->ic.ll.flg |= NOT_HLL;
}


/****************************************************************************
 modrm - Sets up src and dst from modrm byte
 ***************************************************************************/
static void modrm(Int i)
{
    setAddress(i, FALSE, 0, REG(*pInst) + rAX, 0);
    rm(i);
}


/****************************************************************************
 segrm - seg encoded as reg of modrm
 ****************************************************************************/
static void segrm(Int i)
{
    Int	reg = REG(*pInst) + rES;

    if (reg > rDS || (reg == rCS && (stateTable[i].flg & TO_REG)))
        pIcode->ic.ll.opcode = (llIcode)0;
    else {
        setAddress(i, FALSE, 0, (int16)reg, 0);
        rm(i);
    }
}


/****************************************************************************
 regop - src/dst reg encoded as low 3 bits of opcode
 ***************************************************************************/
static void regop(Int i)
{
    setAddress(i, FALSE, 0, ((int16)i & 7) + rAX, 0);
    pIcode->ic.ll.dst.regi = pIcode->ic.ll.src.regi;
}


/*****************************************************************************
 segop - seg encoded in middle of opcode
 *****************************************************************************/
static void segop(Int i)
{
    setAddress(i, TRUE, 0, (((int16)i & 0x18) >> 3) + rES, 0);
}


/****************************************************************************
 axImp - Plugs an implied AX dst
 ***************************************************************************/
static void axImp(Int i)
{
    setAddress(i, TRUE, 0, rAX, 0);
}


static void axSrcIm (Int i)
/* Implied AX source */
{
    pIcode->ic.ll.src.regi = rAX;
}


static void alImp (Int i)
/* Implied AL source */
{
    pIcode->ic.ll.src.regi = rAL;
}


/*****************************************************************************
 memImp - Plugs implied src memory operand with any segment override
 ****************************************************************************/
static void memImp(Int i)
{
    setAddress(i, FALSE, SegPrefix, 0, 0);
}


/****************************************************************************
 memOnly - Instruction is not valid if modrm refers to register (i.e. mod == 3)
 ***************************************************************************/
static void memOnly(Int i)
{
    if ((*pInst & 0xC0) == 0xC0)
        pIcode->ic.ll.opcode = (llIcode)0;
}


/****************************************************************************
 memReg0 - modrm for 'memOnly' and Reg field must also be 0
 ****************************************************************************/
static void memReg0(Int i)
{
    if (REG(*pInst) || (*pInst & 0xC0) == 0xC0)
        pIcode->ic.ll.opcode = (llIcode)0;
    else
        rm(i);
}


/***************************************************************************
 immed - Sets up dst and opcode from modrm byte
 **************************************************************************/
static void immed(Int i)
{
    static llIcode immedTable[8] = {iADD, iOR, iADC, iSBB, iAND, iSUB, iXOR, iCMP};
    static byte uf[8] = { 0,  0,  Cf,  Cf,  0,   0,   0,   0  };

    pIcode->ic.ll.opcode = immedTable[REG(*pInst)];
    pIcode->ic.ll.flagDU.u = uf[REG(*pInst)];
    pIcode->ic.ll.flagDU.d = (Sf | Zf | Cf);
    rm(i);

    if (pIcode->ic.ll.opcode == iADD || pIcode->ic.ll.opcode == iSUB)
        pIcode->ic.ll.flg &= ~NOT_HLL;	/* Allow ADD/SUB SP, immed */
}


/****************************************************************************
 shift  - Sets up dst and opcode from modrm byte
 ***************************************************************************/
static void shift(Int i)
{
    static llIcode shiftTable[8] =
    {
        (llIcode)iROL, (llIcode)iROR, (llIcode)iRCL, (llIcode)iRCR,
        (llIcode)iSHL, (llIcode)iSHR, (llIcode)0,	 (llIcode)iSAR};
    static byte uf[8]	  = {0,   0,   Cf,  Cf,  0,   0,   0, 0  };
    static byte df[8]	  = {Cf,  Cf,  Cf,  Cf, Sf | Zf | Cf,
                             Sf | Zf | Cf, 0, Sf | Zf | Cf};

    pIcode->ic.ll.opcode = shiftTable[REG(*pInst)];
    pIcode->ic.ll.flagDU.u = uf[REG(*pInst)];
    pIcode->ic.ll.flagDU.d = df[REG(*pInst)];
    rm(i);
    pIcode->ic.ll.src.regi = rCL;
}


/****************************************************************************
 trans - Sets up dst and opcode from modrm byte
 ***************************************************************************/
static void trans(Int i)
{
    static llIcode transTable[8] =
    {
        (llIcode)iINC, (llIcode)iDEC, (llIcode)iCALL, (llIcode)iCALLF,
        (llIcode)iJMP, (llIcode)iJMPF,(llIcode)iPUSH, (llIcode)0
    };
    static byte df[8]	= {Sf | Zf, Sf | Zf, 0, 0, 0, 0, 0, 0};

    if ((byte)REG(*pInst) < 2 || !(stateTable[i].flg & B)) { /* INC & DEC */
        pIcode->ic.ll.opcode = transTable[REG(*pInst)];   /* valid on bytes */
        pIcode->ic.ll.flagDU.d = df[REG(*pInst)];
        rm(i);
        memcpy(&pIcode->ic.ll.src, &pIcode->ic.ll.dst, sizeof(ICODEMEM));
        if (pIcode->ic.ll.opcode == iJMP || pIcode->ic.ll.opcode == iCALL ||
                pIcode->ic.ll.opcode == iCALLF)
            pIcode->ic.ll.flg |= NO_OPS;
        else if (pIcode->ic.ll.opcode == iINC || pIcode->ic.ll.opcode == iPUSH
                 || pIcode->ic.ll.opcode == iDEC)
            pIcode->ic.ll.flg |= NO_SRC;
    }
}


/****************************************************************************
 arith - Sets up dst and opcode from modrm byte
 ****************************************************************************/
static void arith(Int i)
{ byte opcode;
    static llIcode arithTable[8] =
    {
        (llIcode)iTEST, (llIcode)0,		(llIcode)iNOT, (llIcode)iNEG,
        (llIcode)iMUL,  (llIcode)iIMUL, (llIcode)iDIV, (llIcode)iIDIV
    };
    static byte df[8]	  = {Sf | Zf | Cf, 0, 0, Sf | Zf | Cf,
                             Sf | Zf | Cf, Sf | Zf | Cf, Sf | Zf | Cf,
                             Sf | Zf | Cf};

    opcode = pIcode->ic.ll.opcode = arithTable[REG(*pInst)];
    pIcode->ic.ll.flagDU.d = df[REG(*pInst)];
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
        memcpy(&pIcode->ic.ll.src, &pIcode->ic.ll.dst, sizeof(ICODEMEM));
        setAddress(i, TRUE, 0, rAX, 0);			/* dst = AX  */
    }
    else if (opcode == iNEG || opcode == iNOT)
        pIcode->ic.ll.flg |= NO_SRC;

    if ((opcode == iDIV) || (opcode == iIDIV))
    {
        if ((pIcode->ic.ll.flg & B) != B)
            pIcode->ic.ll.flg |= IM_TMP_DST;
    }
}


/*****************************************************************************
 data1 - Sets up immed from 1 byte data
 *****************************************************************************/
static void data1(Int i)
{
    pIcode->ic.ll.immed.op = (stateTable[i].flg & S)? signex(*pInst++):
                                                      *pInst++;
    pIcode->ic.ll.flg |= I;
}


/*****************************************************************************
 data2 - Sets up immed from 2 byte data
 ****************************************************************************/
static void data2(Int i)
{
    if (relocItem(pInst))
        pIcode->ic.ll.flg |= SEG_IMMED;

    /* ENTER is a special case, it does not take a destination operand,
         * but this field is being used as the number of bytes to allocate
         * on the stack.  The procedure level is stored in the immediate
         * field.  There is no source operand; therefore, the flag flg is
         * set to NO_OPS.	*/
    if (pIcode->ic.ll.opcode == iENTER)
    {
        pIcode->ic.ll.dst.off = getWord();
        pIcode->ic.ll.flg |= NO_OPS;
    }
    else
        pIcode->ic.ll.immed.op = getWord();
    pIcode->ic.ll.flg |= I;
}


/****************************************************************************
 dispM - 2 byte offset without modrm (== mod 0, rm 6) (Note:TO_REG bits are
         reversed)
 ****************************************************************************/
static void dispM(Int i)
{
    setAddress(i, FALSE, SegPrefix, 0, getWord());
}


/****************************************************************************
 dispN - 2 byte disp as immed relative to ip
 ****************************************************************************/
static void dispN(Int i)
{
    long off = (short)getWord();	/* Signed displacement */
    
    /* Note: the result of the subtraction could be between 32k and 64k, and
        still be positive; it is an offset from prog.Image. So this must be
        treated as unsigned */
    pIcode->ic.ll.immed.op = (dword)(off + (unsigned)(pInst - prog.Image));
    pIcode->ic.ll.flg |= I;
}


/***************************************************************************
 dispS - 1 byte disp as immed relative to ip
 ***************************************************************************/
static void dispS(Int i)
{
    long off = signex(*pInst++); 	/* Signed displacement */

    pIcode->ic.ll.immed.op = (dword)(off + (unsigned)(pInst - prog.Image));
    pIcode->ic.ll.flg |= I;
}


/****************************************************************************
 dispF - 4 byte disp as immed 20-bit target address
 ***************************************************************************/
static void dispF(Int i)
{
    dword off = (unsigned)getWord();
    dword seg = (unsigned)getWord();

    pIcode->ic.ll.immed.op = off + ((dword)(unsigned)seg << 4);
    pIcode->ic.ll.flg |= I;
}


/****************************************************************************
 prefix - picks up prefix byte for following instruction (LOCK is ignored
          on purpose)
 ****************************************************************************/
static void prefix(Int i)
{
    if (pIcode->ic.ll.opcode == iREPE || pIcode->ic.ll.opcode == iREPNE)
        RepPrefix = pIcode->ic.ll.opcode;
    else
        SegPrefix = pIcode->ic.ll.opcode;
}

inline void BumpOpcode(llIcode& ic)
{
    ic = (llIcode)(((int)ic)+1);		// Bump this icode via the int type
}

/*****************************************************************************
 strop - checks RepPrefix and converts string instructions accordingly
 *****************************************************************************/
static void strop(Int i)
{
    if (RepPrefix)
    {
        //		pIcode->ic.ll.opcode += ((pIcode->ic.ll.opcode == iCMPS ||
        //								  pIcode->ic.ll.opcode == iSCAS)
        //								&& RepPrefix == iREPE)? 2: 1;
        if ((pIcode->ic.ll.opcode == iCMPS || pIcode->ic.ll.opcode == iSCAS)
                && RepPrefix == iREPE)
            BumpOpcode(pIcode->ic.ll.opcode);	// += 2
        BumpOpcode(pIcode->ic.ll.opcode);		// else += 1
        if (pIcode->ic.ll.opcode == iREP_LODS)
            pIcode->ic.ll.flg |= NOT_HLL;
        RepPrefix = 0;
    }
}


/***************************************************************************
 escop - esc operands
 ***************************************************************************/
static void escop(Int i)
{
    pIcode->ic.ll.immed.op = REG(*pInst) + (dword)((i & 7) << 3);
    pIcode->ic.ll.flg |= I;
    rm(i);
}


/****************************************************************************
 const1
 ****************************************************************************/
static void const1(Int i)
{
    pIcode->ic.ll.immed.op = 1;
    pIcode->ic.ll.flg |= I;
}


/*****************************************************************************
 const3
 ****************************************************************************/
static void const3(Int i)
{
    pIcode->ic.ll.immed.op = 3;
    pIcode->ic.ll.flg |= I;
}


/****************************************************************************
 none1
 ****************************************************************************/
static void none1(Int i)
{
}


/****************************************************************************
 none2 - Sets the NO_OPS flag if the operand is immediate
 ****************************************************************************/
static void none2(Int i)
{
    if (pIcode->ic.ll.flg & I)
        pIcode->ic.ll.flg |= NO_OPS;
}

/****************************************************************************
 Checks for int 34 to int 3B - if so, converts to ESC nn instruction
 ****************************************************************************/
static void checkInt(Int i)
{
    word wOp = (word) pIcode->ic.ll.immed.op;
    if ((wOp >= 0x34) && (wOp <= 0x3B))
    {
        /* This is a Borland/Microsoft floating point emulation instruction.
            Treat as if it is an ESC opcode */
        pIcode->ic.ll.immed.op = wOp - 0x34;
        pIcode->ic.ll.opcode = iESC;
        pIcode->ic.ll.flg |= FLOAT_OP;

        escop(wOp - 0x34 + 0xD8);

    }
}
