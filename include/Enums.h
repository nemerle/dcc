#pragma once
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
/* Operand is defined, used or both flag */
enum operDu
{
    eDEF=0x10,			/* Operand is defined						*/
    eUSE=0x100,			/* Operand is used							*/
    USE_DEF,		/* Operand is used and defined				*/
    NONE			/* No operation is required on this operand	*/
};
