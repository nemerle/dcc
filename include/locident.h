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
#include "types.h"
#include "Enums.h"
#include "machine_x86.h"

/* Type definition */
// this array has to stay in-order of addition i.e. not std::set<iICODE,std::less<iICODE> >
// TODO: why ?
struct COND_EXPR;
struct ICODE;
struct LLInst;
typedef std::list<ICODE>::iterator iICODE;
struct IDX_ARRAY : public std::vector<iICODE>
{
    bool inList(iICODE idx)
    {
        return std::find(begin(),end(),idx)!=end();
    }
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
    eReg 	regi;			/*   optional indexed register				 */
} BWGLB_TYPE;


typedef struct
{ /* For TYPE_LONG_(UN)SIGN on the stack	     */
    int		offH;	/*   high offset from BP					 */
    int		offL;	/*   low offset from BP						 */
}	LONG_STKID_TYPE;
struct LONGID_TYPE
{		/* For TYPE_LONG_(UN)SIGN registers			 */
    eReg	h;		/*   high register							 */
    eReg	l;		/*   low register							 */
    bool srcDstRegMatch(iICODE a,iICODE b) const;
};


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
    std::string         name;       /* Identifier's name                        */
    union {                         /* Different types of identifiers           */
        eReg		regi;       /* For TYPE_BYTE(uint16_t)_(UN)SIGN registers   */
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
        return TypeContainer::typeSize(type)*8;
    }
    void setLocalName(int i)
    {
        char buf[32];
        sprintf (buf, "loc%ld", i);
        name=buf;
    }
};

struct LOCAL_ID
{
    std::vector<ID> id_arr;
protected:
    int newLongIdx(int16_t seg, int16_t offH, int16_t offL, uint8_t regi, hlType t);
    int newLongGlb(int16_t seg, int16_t offH, int16_t offL, hlType t);
    int newLongStk(hlType t, int offH, int offL);
public:
    LOCAL_ID()
    {
        id_arr.reserve(256);
    }
    // interface to allow range based iteration
    std::vector<ID>::iterator begin() {return id_arr.begin();}
    std::vector<ID>::iterator end() {return id_arr.end();}
    int newByteWordReg(hlType t, eReg regi);
    int newByteWordStk(hlType t, int off, uint8_t regOff);
    int newIntIdx(int16_t seg, int16_t off, eReg regi, int ix, hlType t);
    int newLongReg(hlType t, eReg regH, eReg regL, iICODE ix_);
    int newLong(opLoc sd, iICODE pIcode, hlFirst f, iICODE ix, operDu du, int off);
    int newLong(opLoc sd, iICODE pIcode, hlFirst f, iICODE ix, operDu du, LLInst &atOffset);
    void newIdent(hlType t, frameType f);
    void flagByteWordId(int off);
    void propLongId(uint8_t regL, uint8_t regH, const char *name);
    size_t csym() const {return id_arr.size();}
    void newRegArg(iICODE picode, iICODE ticode) const;
    void processTargetIcode(iICODE picode, int &numHlIcodes, iICODE ticode, bool isLong) const;
    void forwardSubs(COND_EXPR *lhs, COND_EXPR *rhs, iICODE picode, iICODE ticode, int &numHlIcodes) const;
    COND_EXPR *createId(const ID *retVal, iICODE ix_);
};


