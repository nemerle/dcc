#pragma once
#include "Enums.h"
/*
 *$Log:	parsehdr.h,v $
 */
/* Header file for parsehdr.c */

typedef unsigned long dword;    /* 32 bits	*/
typedef unsigned char byte;	    /* 8 bits 	*/
typedef unsigned short word;    /* 16 bits	*/
typedef unsigned char boolT;    /* 8 bits 	*/

#define BUFF_SIZE 8192          /* Holds a declaration */
#define FBUF_SIZE 32700        /* Holds part of a header file */

#define	NARGS		15
#define	NAMES_L		160
#define	TYPES_L		160
#define	FUNC_L		160

#define	ERRF	stdout

void	phError(const char * errmsg);
void	phWarning(const char * errmsg);

#define	ERR(msg)		phError(msg)
#ifdef DEBUG
#define	DBG(str) printf(str);
#else
#define DBG(str) ;
#endif
#define	WARN(msg)		phWarning(msg)
#define OUT(str)		fprintf(outfile, str)

#define	PH_PARAMS	32
#define PH_NAMESZ	15

#define  SYMLEN     16                  /* Including the null */
#define  Int        long                /* For locident.h */
//#include "locident.h"                   /* For the hlType enum */
//#define  bool       unsigned char       /* For internal use */

typedef
struct ph_func_tag
{
    char    name[SYMLEN];               /* Name of function or arg */
    hlType  typ;                        /* Return type */
    int     numArg;                     /* Number of args */
    int     firstArg;                   /* Index of first arg in chain */
    int     next;                       /* Index of next function in chain */
    bool    bVararg;                    /* True if variable num args */
} PH_FUNC_STRUCT;

typedef
struct ph_arg_tag
{
    char    name[SYMLEN];               /* Name of function or arg */
    hlType  typ;                        /* Parameter type */
} PH_ARG_STRUCT;

#define DELTA_FUNC 32                   /* Number to alloc at once */


#define	PH_JUNK			0		/* LPSTR		buffer, nothing happened */
#define	PH_PROTO		1		/* LPPH_FUNC 	ret val, func name, args */
#define	PH_FUNCTION		2		/* LPPH_FUNC	ret val, func name, args */
#define	PH_TYPEDEF		3		/* LPPH_DEF  	definer and definee      */
#define	PH_DEFINE		4		/* LPPH_DEF  	definer and definee      */
#define	PH_ERROR		5		/* LPSTR		error string             */
#define	PH_WARNING		6		/* LPSTR 		warning string           */
#define	PH_MPROTO		7		/* ????? multi proto????                 */
#define	PH_VAR			8		/* ????? var decl                        */

/* PROTOS */

boolT	phData(char *buff, int ndata);
boolT	phPost(void);
boolT	phFree(void);
void    checkHeap(char *msg);   /* For debugging only */

void	phBuffToFunc(char *buff);

void	phBuffToDef(char *buff);


#define TOK_TYPE    256         /* A type name (e.g. "int") */
#define TOK_NAME    257         /* A function or parameter name */
#define TOK_DOTS    258         /* "..." */
#define TOK_EOL     259         /* End of line */

typedef enum
{
    BT_INT, BT_CHAR, BT_FLOAT, BT_DOUBLE, BT_STRUCT, BT_VOID, BT_UNKWN
} baseType;
