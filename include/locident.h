/*
 * File: locIdent.h
 * Purpose: High-level local identifier definitions
 * Date: October 1993
 * (C) Cristina Cifuentes
 */


/* Type definition */
typedef struct {
	Int		csym;		/* # symbols used 			*/
	Int		alloc;		/* # symbols allocated 		*/
	Int		*idx;		/* Array of integer indexes */
} IDX_ARRAY;

/* Type definitions used in the decompiled program  */
typedef enum {
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
	TYPE_DOUBLE,		/* double precision float	*/
} hlType;

static char *hlTypes[13] = {"", "char", "unsigned char", "int", "unsigned int", 
			 		 "long", "unsigned long", "record", "int *", "char *",
			 		 "", "float", "double"};

typedef enum
{
	STK_FRAME,			/* For stack vars			*/
	REG_FRAME,			/* For register variables 	*/
	GLB_FRAME,			/* For globals				*/
} frameType;


/* Enumeration to determine whether pIcode points to the high or low part
 * of a long number */
typedef enum {
	HIGH_FIRST,			/* High value is first		*/
	LOW_FIRST,			/* Low value is first		*/
} hlFirst;


typedef struct
{
	int16	seg;			/*   segment value							 */
	int16	off;			/*   offset									 */
	byte 	regi;			/*   optional indexed register				 */
} BWGLB_TYPE;


typedef struct
{ /* For TYPE_LONG_(UN)SIGN on the stack	     */
	Int		offH;	/*   high offset from BP					 */
	Int		offL;	/*   low offset from BP						 */
}	LONG_STKID_TYPE;
typedef struct
{		/* For TYPE_LONG_(UN)SIGN registers			 */
	byte	h;		/*   high register							 */
	byte	l;		/*   low register							 */
} LONGID_TYPE;


/* ID, LOCAL_ID */
typedef struct {
	hlType			type;	/* Probable type							 */
	boolT			illegal;/* Boolean: not a valid field any more		 */
	IDX_ARRAY		idx;	/* Index into icode array (REG_FRAME only)	 */
	frameType		loc;	/* Frame location							 */
	boolT			hasMacro;/* Identifier requires a macro				 */
	char			macro[10];/* Macro for this identifier				 */
	char			name[20];/* Identifier's name						 */
	union {					/* Different types of identifiers 			 */
		byte		regi;	/* For TYPE_BYTE(WORD)_(UN)SIGN registers    */ 
		struct {			/* For TYPE_BYTE(WORD)_(UN)SIGN on the stack */
			byte	regOff; /*    register offset (if any)				 */
			Int		off;	/*    offset from BP						 */
		}			bwId;
		BWGLB_TYPE	bwGlb;	/* For TYPE_BYTE(WORD)_(UN)SIGN globals		 */
		LONGID_TYPE longId; /* For TYPE_LONG_(UN)SIGN registers			 */
		LONG_STKID_TYPE	longStkId;/* For TYPE_LONG_(UN)SIGN on the stack */
		struct {			/* For TYPE_LONG_(UN)SIGN globals			 */
			int16	seg;	/*   segment value							 */
			int16	offH;	/*   offset high							 */
			int16	offL;	/*   offset low								 */
			byte	regi;	/*   optional indexed register				 */
		}			longGlb;
		struct {			/* For TYPE_LONG_(UN)SIGN constants			 */
			dword	h;		/*	 high word								 */
			dword 	l;		/*	 low word								 */
		}			longKte;
	}				id;
} ID;

typedef struct {
	Int			csym;		/* No. of symbols in the table	*/
	Int			alloc;		/* No. of symbols allocated		*/
	ID			*id;		/* Identifier					*/
} LOCAL_ID;


