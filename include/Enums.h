#pragma once
/* Machine registers */
enum eReg
{
    rAX =        1,  /* These are numbered relative to real 8086 */
    rCX =        2,
    rDX =        3,
    rBX =        4,
    rSP =        5,
    rBP =        6,
    rSI =        7,
    rDI =        8,

    rES =        9,
    rCS =       10,
    rSS =       11,
    rDS =       12,

    rAL =       13,
    rCL =       14,
    rDL =       15,
    rBL =       16,
    rAH =       17,
    rCH =       18,
    rDH =       19,
    rBH =       20,

    rTMP=       21,		/* temp register for DIV/IDIV/MOD	*/
    INDEXBASE = 22          /* Indexed modes go from INDEXBASE to INDEXBASE+7  */
};

/* Register types */
enum regType
{
    BYTE_REG,
    WORD_REG
};
enum condId
{
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
    LESS_EQUAL = 0, /* <=   */
    LESS,           /* <    */
    EQUAL,          /* ==   */
    NOT_EQUAL,      /* !=   */
    GREATER,        /* >    */
    GREATER_EQUAL,  /* >=   */
    /* For general expressions */
    AND,            /* &    */
    OR,				/* |	*/
    XOR,			/* ^	*/
    NOT,			/* ~	*/  /* 1's complement */
    ADD,			/* +	*/
    SUB,			/* -	*/
    MUL,			/* *	*/
    DIV,			/* /	*/
    SHR,			/* >>	*/
    SHL,			/* <<	*/
    MOD,			/* %	*/
    DBL_AND,		/* &&	*/
    DBL_OR,			/* ||	*/
    DUMMY			/*      */
};
/* LOW_LEVEL operand location: source or destination */
enum opLoc
{
    SRC,						/* Source operand 		*/
    DST,						/* Destination operand	*/
    LHS_OP						/* Left-hand side operand (for HIGH_LEVEL) */
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
    TYPE_RECORD,		/* record structure			*/
    TYPE_PTR,        	/* pointer (32 bit ptr) 	*/
    TYPE_STR,        	/* string               	*/
    TYPE_CONST,			/* constant (any type)		*/
    TYPE_FLOAT,			/* floating point			*/
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

