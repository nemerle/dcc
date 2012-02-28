/*
 * File: locIdent.h
 * Purpose: High-level local identifier definitions
 * Date: October 1993
 * (C) Cristina Cifuentes
 */

#pragma once
#include <stdint.h>
#include <vector>
#include <list>
#include <set>
#include <algorithm>

#include "Enums.h"

/* Type definition */
// this array has to stay in-order of addition i.e. not std::set<iICODE,std::less<iICODE> >
// TODO: why ?
struct ICODE;
typedef std::list<ICODE>::iterator iICODE;
struct IDX_ARRAY : public std::vector<iICODE>
{
    bool inList(iICODE idx)
    {
        return std::find(begin(),end(),idx)!=end();
    }
};

static constexpr const char *hlTypes[13] = {
    "", "char", "unsigned char", "int", "unsigned int",
    "long", "unsigned long", "record", "int *", "char *",
    "", "float", "double"
};

typedef enum
{
    STK_FRAME,			/* For stack vars			*/
    REG_FRAME,			/* For register variables 	*/
    GLB_FRAME			/* For globals				*/
} frameType;

typedef struct
{
    int16_t	seg;			/*   segment value							 */
    int16_t	off;			/*   offset									 */
    uint8_t 	regi;			/*   optional indexed register				 */
} BWGLB_TYPE;


typedef struct
{ /* For TYPE_LONG_(UN)SIGN on the stack	     */
    int		offH;	/*   high offset from BP					 */
    int		offL;	/*   low offset from BP						 */
}	LONG_STKID_TYPE;
typedef struct
{		/* For TYPE_LONG_(UN)SIGN registers			 */
    uint8_t	h;		/*   high register							 */
    uint8_t	l;		/*   low register							 */
} LONGID_TYPE;


/* ID, LOCAL_ID */
struct ID
{
    hlType              type;       /* Probable type                            */
    bool                illegal;    /* Boolean: not a valid field any more      */
    //std::vector<iICODE> idx;
    IDX_ARRAY           idx;        /* Index into icode array (REG_FRAME only)  */
    frameType           loc;        /* Frame location                           */
    bool                hasMacro;   /* Identifier requires a macro              */
    char                macro[10];  /* Macro for this identifier                */
    char                name[20];   /* Identifier's name                        */
    union {                         /* Different types of identifiers           */
        uint8_t		regi;       /* For TYPE_BYTE(uint16_t)_(UN)SIGN registers   */
        struct {                    /* For TYPE_BYTE(uint16_t)_(UN)SIGN on the stack */
            uint8_t	regOff;     /*    register offset (if any)              */
            int		off;        /*    offset from BP            		*/
        }               bwId;
        BWGLB_TYPE	bwGlb;	/* For TYPE_BYTE(uint16_t)_(UN)SIGN globals		 */
        LONGID_TYPE     longId; /* For TYPE_LONG_(UN)SIGN registers			 */
        LONG_STKID_TYPE	longStkId;  /* For TYPE_LONG_(UN)SIGN on the stack */
        struct {			/* For TYPE_LONG_(UN)SIGN globals			 */
            int16_t	seg;	/*   segment value                                              */
            int16_t	offH;	/*   offset high                                                */
            int16_t	offL;	/*   offset low                                                 */
            uint8_t	regi;	/*   optional indexed register                                  */
        }			longGlb;
        struct {			/* For TYPE_LONG_(UN)SIGN constants                     */
            uint32_t	h;		/*	 high uint16_t								 */
            uint32_t 	l;		/*	 low uint16_t								 */
        } longKte;
    } id;
    ID();
    ID(hlType t, frameType f);
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
    int newByteWordReg(hlType t, uint8_t regi);
    int newByteWordStk(hlType t, int off, uint8_t regOff);
    int newIntIdx(int16_t seg, int16_t off, uint8_t regi, int ix, hlType t);
    int newLongReg(hlType t, uint8_t regH, uint8_t regL, iICODE ix_);
    int newLong(opLoc sd, iICODE pIcode, hlFirst f, iICODE ix, operDu du, int off);
    void newIdent(hlType t, frameType f);
    void flagByteWordId(int off);
    void propLongId(uint8_t regL, uint8_t regH, const char *name);
    size_t csym() const {return id_arr.size();}
protected:
    int newLongIdx(int16_t seg, int16_t offH, int16_t offL, uint8_t regi, hlType t);
    int newLongGlb(int16_t seg, int16_t offH, int16_t offL, hlType t);
    int newLongStk(hlType t, int offH, int offL);
};


