/*
 * File: locIdent.h
 * Purpose: High-level local identifier definitions
 * Date: October 1993
 * (C) Cristina Cifuentes
 */

#pragma once
#include "msvc_fixes.h"
#include "types.h"
#include "Enums.h"
#include "machine_x86.h"

#include <QtCore/QString>
#include <stdint.h>
#include <vector>
#include <list>
#include <set>
#include <algorithm>

/* Type definition */
// this array has to stay in-order of addition i.e. not std::set<iICODE,std::less<iICODE> >
// TODO: why ?
struct Expr;
struct AstIdent;
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

enum frameType
{
    STK_FRAME,			/* For stack vars			*/
    REG_FRAME,			/* For register variables 	*/
    GLB_FRAME			/* For globals				*/
};

struct BWGLB_TYPE
{
    int16_t	seg;			/*   segment value							 */
    int16_t	off;			/*   offset									 */
    eReg 	regi;			/*   optional indexed register				 */
} ;


/* For TYPE_LONG_(UN)SIGN on the stack	     */
struct LONG_STKID_TYPE
{
    int		offH;	/*   high offset from BP					 */
    int		offL;	/*   low offset from BP						 */
    LONG_STKID_TYPE(int h,int l) : offH(h),offL(l) {}
};
/* For TYPE_LONG_(UN)SIGN registers			 */
struct LONGID_TYPE
{
protected:
    eReg	m_h;		/*   high register							 */
    eReg	m_l;		/*   low register							 */
public:
    void set(eReg highpart,eReg lowpart)
    {
        m_h = highpart;
        m_l = lowpart;
    }
    eReg l() const { return m_l; }
    eReg h() const { return m_h; }
    bool srcDstRegMatch(iICODE a,iICODE b) const;
    LONGID_TYPE() {} // uninitializing constructor to help valgrind catch uninit accesses
    LONGID_TYPE(eReg h,eReg l) : m_h(h),m_l(l) {}
};

struct LONGGLB_TYPE /* For TYPE_LONG_(UN)SIGN globals			 */
{
    int16_t	seg;	/*   segment value                                              */
    int16_t	offH;	/*   offset high                                                */
    int16_t	offL;	/*   offset low                                                 */
    uint8_t	regi;	/*   optional indexed register                                  */
    LONGGLB_TYPE(int16_t _seg,int16_t _H,int16_t _L,int8_t _reg=0)
    {
        seg=_seg;
        offH=_H;
        offL=_L;
        regi=_reg;
    }
};
/* ID, LOCAL_ID */
struct ID
{
protected:
    LONGID_TYPE     m_longId; /* For TYPE_LONG_(UN)SIGN registers			 */
public:
    hlType              type;       /* Probable type                            */
    IDX_ARRAY           idx;        /* Index into icode array (REG_FRAME only)  */
    frameType           loc;        /* Frame location                           */
    bool                illegal;    /* Boolean: not a valid field any more      */
    bool                hasMacro;   /* Identifier requires a macro              */
    char                macro[10];  /* Macro for this identifier                */
    QString             name;       /* Identifier's name                        */
    union ID_UNION {                         /* Different types of identifiers           */
        friend struct ID;
    protected:
        LONG_STKID_TYPE	longStkId;  /* For TYPE_LONG_(UN)SIGN on the stack */
    public:
        eReg		regi;       /* For TYPE_BYTE(WORD)_(UN)SIGN registers   */
        struct {                /* For TYPE_BYTE(WORD)_(UN)SIGN on the stack */
            uint8_t	regOff;     /*    register offset (if any)              */
            int		off;        /*    offset from BP            		*/
        } bwId;
        BWGLB_TYPE	bwGlb;	/* For TYPE_BYTE(uint16_t)_(UN)SIGN globals		 */
        LONGGLB_TYPE    longGlb;
        struct {			/* For TYPE_LONG_(UN)SIGN constants                     */
            uint32_t	h;		/*	 high uint16_t								 */
            uint32_t 	l;		/*	 low uint16_t								 */
        } longKte;
        ID_UNION() { /*new (&longStkId) LONG_STKID_TYPE();*/}
    } id;

    LONGID_TYPE &           longId() {assert(isLong() and loc==REG_FRAME); return m_longId;}
    const LONGID_TYPE &     longId() const {assert(isLong() and loc==REG_FRAME); return m_longId;}
    LONG_STKID_TYPE &       longStkId() {assert(isLong() and loc==STK_FRAME); return id.longStkId;}
    const LONG_STKID_TYPE & longStkId() const {assert(isLong() and loc==STK_FRAME); return id.longStkId;}
                            ID();
                            ID(hlType t, frameType f);
                            ID(hlType t, const LONGID_TYPE &s);
                            ID(hlType t, const LONG_STKID_TYPE &s);
                            ID(hlType t, const LONGGLB_TYPE &s);
    bool                    isSigned() const { return (type==TYPE_BYTE_SIGN) or (type==TYPE_WORD_SIGN) or (type==TYPE_LONG_SIGN);}
    uint16_t                typeBitsize() const
                            {
                                return TypeContainer::typeSize(type)*8;
                            }
    bool                    isLong() const { return (type==TYPE_LONG_UNSIGN) or (type==TYPE_LONG_SIGN); }
    void                    setLocalName(int i)
                            {
                                char buf[32];
                                sprintf (buf, "loc%d", i);
                                name=buf;
                            }
    bool                    isLongRegisterPair() const { return (loc == REG_FRAME) and isLong();}
    eReg                    getPairedRegister(eReg first) const;
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
    int newIntIdx(int16_t seg, int16_t off, eReg regi, hlType t);
    int newLongReg(hlType t, const LONGID_TYPE &longT, iICODE ix_);
    int newLong(opLoc sd, iICODE pIcode, hlFirst f, iICODE ix, operDu du, int off);
    int newLong(opLoc sd, iICODE pIcode, hlFirst f, iICODE ix, operDu du, LLInst &atOffset);
    void newIdent(hlType t, frameType f);
    void flagByteWordId(int off);
    void propLongId(uint8_t regL, uint8_t regH, const QString & name);
    size_t csym() const {return id_arr.size();}
    void newRegArg(ICODE & picode, ICODE & ticode) const;
    void processTargetIcode(ICODE & picode, int &numHlIcodes, ICODE & ticode, bool isLong) const;
    void forwardSubs(Expr *lhs, Expr *rhs, ICODE & picode, ICODE & ticode, int &numHlIcodes) const;
    AstIdent *createId(const ID *retVal, iICODE ix_);
    eReg getPairedRegisterAt(int idx,eReg first) const;
};


