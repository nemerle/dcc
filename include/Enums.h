#pragma once

/* Register types */
enum regType
{
    BYTE_REG,
    WORD_REG
};
enum condId
{
    UNDEF=0,
    GLOB_VAR,       /* global variable  */
    REGISTER,       /* register         */
    LOCAL_VAR,      /* negative disp    */
    PARAM,          /* positive disp    */
    GLOB_VAR_IDX,   /* indexed global variable *//*** should merge w/glob-var*/
    CONSTANT,       /* constant         */
    STRING,         /* string			*/
    LONG_VAR,       /* long variable	*/
    FUNCTION,       /* function			*/
    OTHER           /* other **** tmp solution */
};

enum condOp
{
    /* For conditional expressions */
    LESS_EQUAL, /* <=   */
    LESS,           /* <    */
    EQUAL,          /* ==   */
    NOT_EQUAL,      /* !=   */
    GREATER,        /* >    */
    GREATER_EQUAL,  /* >=   */
    /* For general expressions */
    AND,            /* &    */
    OR,             /* |	*/
    XOR,            /* ^	*/
    NOT,            /* ~	*/  /* 1's complement */
    ADD,            /* +	*/
    SUB,            /* -	*/
    MUL,            /* *	*/
    DIV,            /* /	*/
    SHR,            /* >>	*/
    SHL,            /* <<	*/
    MOD,            /* %	*/
    DBL_AND,        /* &&	*/
    DBL_OR,         /* ||	*/
    DUMMY           /*      */
};
/* LOW_LEVEL operand location: source or destination */
enum opLoc
{
    SRC,						/* Source operand 		*/
    DST,						/* Destination operand	*/
    LHS_OP						/* Left-hand side operand (for HIGH_LEVEL) */
};
/* LOW_LEVEL icode flags */
#define NO_SRC_B    0xF7FFFF    /* Masks off SRC_B */
enum eLLFlags
{

    B           = 0x0000001,    /* uint8_t operands (value implicitly used) */
    I           = 0x0000002,    /* Immed. source */
    NOT_HLL     = 0x0000004,    /* Not HLL inst. */
    FLOAT_OP    = 0x0000008,    /* ESC or WAIT   */
    SEG_IMMED   = 0x0000010,    /* Number is relocated segment value */
    IMPURE      = 0x0000020,    /* Instruction modifies code */
    WORD_OFF    = 0x0000040,    /* Inst has uint16_t offset ie.could be address */
    TERMINATES  = 0x0000080,    /* Instruction terminates program */
    CASE        = 0x0000100,    /* Label as case part of switch */
    SWITCH      = 0x0000200,    /* Treat indirect JMP as switch stmt */
    TARGET      = 0x0000400,    /* Jump target */
    SYNTHETIC   = 0x0000800,    /* Synthetic jump instruction */
    NO_LABEL    = 0x0001000,    /* Immed. jump cannot be linked to a label */
    NO_CODE     = 0x0002000,    /* Hole in Icode array */
    SYM_USE     = 0x0004000,    /* Instruction uses a symbol */
    SYM_DEF     = 0x0008000,    /* Instruction defines a symbol */
    NO_SRC      = 0x0010000,    /* Opcode takes no source */
    NO_OPS      = 0x0020000,    /* Opcode takes no operands */
    IM_OPS      = 0x0040000,    /* Opcode takes implicit operands */
    SRC_B       = 0x0080000,    /* Source operand is uint8_t (dest is uint16_t) */
    HLL_LABEL   = 0x0100000, /* Icode has a high level language label */
    IM_DST      = 0x0200000, /* Implicit DST for opcode (SIGNEX) */
    IM_SRC      = 0x0400000, /* Implicit SRC for opcode (dx:ax)	*/
    IM_TMP_DST  = 0x0800000, /* Implicit rTMP DST for opcode (DIV/IDIV) */
    JMP_ICODE   = 0x1000000, /* Jmp dest immed.op converted to icode index */
    JX_LOOP     = 0x2000000, /* Cond jump is part of loop conditional exp */
    REST_STK    = 0x4000000  /* Stack needs to be restored after CALL */
#define ICODEMASK 0x0FF00FF    /* Masks off parser flags */
};
/* Types of icodes */
enum icodeType
{
    NOT_SCANNED = 0,    // not even scanned yet
    LOW_LEVEL,          // low-level icode
    HIGH_LEVEL          // high-level icode
};


/* LOW_LEVEL icode opcodes */
enum llIcode
{
    //iINVALID,
    iCBW,		/* 0 */
    iAAA,
    iAAD,
    iAAM,
    iAAS,
    iADC,
    iADD,
    iAND,
    iBOUND,
    iCALL,
    iCALLF,		/* 10 */
    iCLC,
    iCLD,
    iCLI,
    iCMC,
    iCMP,
    iCMPS,
    iREPNE_CMPS,
    iREPE_CMPS,
    iDAA,
    iDAS,		/* 20 */
    iDEC,
    iDIV,
    iENTER,
    iESC,
    iHLT,
    iIDIV,
    iIMUL,
    iIN,
    iINC,
    iINS,		/* 30 */
    iREP_INS,
    iINT,
    iIRET,
    iJB,
    iJBE,
    iJAE,
    iJA,
    iJE,
    iJNE,
    iJL,		/* 40 */
    iJGE,
    iJLE,
    iJG,
    iJS,
    iJNS,
    iJO,
    iJNO,
    iJP,
    iJNP,
    iJCXZ,		/* 50 */
    iJMP,
    iJMPF,
    iLAHF,
    iLDS,
    iLEA,
    iLEAVE,
    iLES,
    iLOCK,
    iLODS,
    iREP_LODS,	/* 60 */
    iLOOP,
    iLOOPE,
    iLOOPNE,
    iMOV,		/* 64 */
    iMOVS,
    iREP_MOVS,
    iMUL,		/* 67 */
    iNEG,
    iNOT,
    iOR,   		/* 70 */
    iOUT,
    iOUTS,
    iREP_OUTS,
    iPOP,
    iPOPA,
    iPOPF,
    iPUSH,
    iPUSHA,
    iPUSHF,
    iRCL,		/* 80 */
    iRCR,
    iROL,
    iROR,
    iRET,		/* 84 */
    iRETF,
    iSAHF,
    iSAR,
    iSHL,
    iSHR,
    iSBB,		/* 90 */
    iSCAS,
    iREPNE_SCAS,
    iREPE_SCAS,
    iSIGNEX,
    iSTC,
    iSTD,
    iSTI,
    iSTOS,
    iREP_STOS,
    iSUB,		/* 100 */
    iTEST,
    iWAIT,
    iXCHG,
    iXLAT,
    iXOR,
    iINTO,
    iNOP,
    iREPNE,
    iREPE,
    iMOD		/* 110 */
};
/* Conditional Expression enumeration nodes and operators               */
enum condNodeType
{
    UNKNOWN_OP=0,
    BOOLEAN_OP,         /* condOps  			*/

    NEGATION,           /* not (2's complement)	*/
    ADDRESSOF,          /* addressOf (&)		*/
    DEREFERENCE,        /* contents of (*) 		*/
    IDENTIFIER,         /* {register | local | param | constant | global} */
    /* The following are only available to C programs */
    POST_INC,           /* ++ (post increment)	*/
    POST_DEC,           /* -- (post decrement)	*/
    PRE_INC,            /* ++ (pre increment)	*/
    PRE_DEC             /* -- (pre decrement)	*/
} ;

/* Enumeration to determine whether pIcode points to the high or low part
 * of a long number */
enum hlFirst
{
        HIGH_FIRST,			/* High value is first		*/
        LOW_FIRST			/* Low value is first		*/
};
/* HIGH_LEVEL icodes opcodes */
enum hlIcode
{
    HLI_INVALID=0,
    HLI_ASSIGN,         /* :=               		*/
    HLI_CALL,			/* Call procedure			*/
    HLI_JCOND,          /* Conditional jump 		*/
    HLI_RET,			/* Return from procedure	*/
    /* pseudo high-level icodes */
    HLI_POP,			/* Pop expression			*/
    HLI_PUSH			/* Push expression			*/
} ;
/* Type definitions used in the decompiled program  */
enum hlType
{
    TYPE_UNKNOWN = 0,   /* unknown so far      		*/
    TYPE_BYTE_SIGN,		/* signed byte (8 bits) 	*/
    TYPE_BYTE_UNSIGN,	/* unsigned byte 			*/
    TYPE_WORD_SIGN,     /* signed word (16 bits) 	*/
    TYPE_WORD_UNSIGN,	/* unsigned word (16 bits)	*/
    TYPE_LONG_SIGN,		/* signed long (32 bits)	*/
    TYPE_LONG_UNSIGN,	/* unsigned long (32 bits)	*/
    TYPE_RECORD,        /* record structure			*/
    TYPE_PTR,        	/* pointer (32 bit ptr) 	*/
    TYPE_STR,        	/* string               	*/
    TYPE_CONST,         /* constant (any type)		*/
    TYPE_FLOAT,         /* floating point			*/
    TYPE_DOUBLE		/* double precision float	*/
};

/* Operand is defined, used or both flag */
enum operDu
{
    eDEF=0x10,			/* Operand is defined						*/
    eUSE=0x100,			/* Operand is used							*/
    USE_DEF,		/* Operand is used and defined				*/
    NONE			/* No operation is required on this operand	*/
};

/* LOW_LEVEL icode, DU flag bits */
enum eDuFlags
{
    Cf=1,
    Sf=2,
    Zf=4,
    Df=8
};
