/*
 * File: locIdent.h
 * Purpose: High-level local identifier definitions
 * Date: October 1993
 * (C) Cristina Cifuentes
 */

#pragma once
#include <vector>
#include <set>
#include <algorithm>
#include "icode.h"
/* Type definition */
// this array has to stay in-order of addition i.e. not std::set<iICODE,std::less<iICODE> >
// TODO: why ?
struct IDX_ARRAY : public std::vector<iICODE>
{
    bool inList(iICODE idx)
    {
        return std::find(begin(),end(),idx)!=end();
    }
};
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
    TYPE_DOUBLE		/* double precision float	*/
} hlType;

static constexpr const char *hlTypes[13] = {"", "char", "unsigned char", "int", "unsigned int",
                                  "long", "unsigned long", "record", "int *", "char *",
                                  "", "float", "double"};

typedef enum
{
    STK_FRAME,			/* For stack vars			*/
    REG_FRAME,			/* For register variables 	*/
    GLB_FRAME			/* For globals				*/
} frameType;

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
struct ID
{
    hlType              type;       /* Probable type                            */
    boolT               illegal;    /* Boolean: not a valid field any more      */
    //std::vector<iICODE> idx;
    IDX_ARRAY           idx;        /* Index into icode array (REG_FRAME only)  */
    frameType           loc;        /* Frame location                           */
    boolT               hasMacro;   /* Identifier requires a macro              */
    char                macro[10];  /* Macro for this identifier                */
    char                name[20];   /* Identifier's name                        */
    union {                         /* Different types of identifiers           */
        byte		regi;       /* For TYPE_BYTE(WORD)_(UN)SIGN registers   */
        struct {                    /* For TYPE_BYTE(WORD)_(UN)SIGN on the stack */
            byte	regOff;     /*    register offset (if any)              */
            Int		off;        /*    offset from BP            		*/
        }               bwId;
        BWGLB_TYPE	bwGlb;	/* For TYPE_BYTE(WORD)_(UN)SIGN globals		 */
        LONGID_TYPE     longId; /* For TYPE_LONG_(UN)SIGN registers			 */
        LONG_STKID_TYPE	longStkId;  /* For TYPE_LONG_(UN)SIGN on the stack */
        struct {			/* For TYPE_LONG_(UN)SIGN globals			 */
            int16	seg;	/*   segment value                                              */
            int16	offH;	/*   offset high                                                */
            int16	offL;	/*   offset low                                                 */
            byte	regi;	/*   optional indexed register                                  */
        }			longGlb;
        struct {			/* For TYPE_LONG_(UN)SIGN constants                     */
            dword	h;		/*	 high word								 */
            dword 	l;		/*	 low word								 */
        } longKte;
    } id;
    ID() : type(TYPE_UNKNOWN),illegal(false),loc(STK_FRAME),hasMacro(false)
    {
        name[0]=0;
        macro[0]=0;
        memset(&id,0,sizeof(id));
    }
    ID(hlType t, frameType f) : type(t),illegal(false),hasMacro(false)
    {
        name[0]=0;
        macro[0]=0;
        memset(&id,0,sizeof(id));
        loc=f;
    }
    bool isSigned() const { return (type==TYPE_BYTE_SIGN)||(type==TYPE_WORD_SIGN)||(type==TYPE_LONG_SIGN);}
    uint16_t typeBitsize() const
    {
        switch(type)
        {
            case TYPE_WORD_SIGN: case TYPE_WORD_UNSIGN:
            return 16;
        }
        return ~0;
    }
};

struct LOCAL_ID
{
    std::vector<ID> id_arr;
public:
    LOCAL_ID()
    {
        id_arr.reserve(256);
    }
    Int newByteWordReg(hlType t, byte regi);
    Int newByteWordStk(hlType t, Int off, byte regOff);
    Int newIntIdx(int16 seg, int16 off, byte regi, Int ix, hlType t);
    Int newLongReg(hlType t, byte regH, byte regL, iICODE ix_);
    Int newLong(opLoc sd, iICODE pIcode, hlFirst f, iICODE ix, operDu du, Int off);
    void newIdent(hlType t, frameType f);
    void flagByteWordId(Int off);
    void propLongId(byte regL, byte regH, const char *name);
    size_t csym() const {return id_arr.size();}
protected:
    Int newLongIdx(int16 seg, int16 offH, int16 offL, byte regi, hlType t);
    Int newLongGlb(int16 seg, int16 offH, int16 offL, hlType t);
    Int newLongStk(hlType t, Int offH, Int offL);
};


