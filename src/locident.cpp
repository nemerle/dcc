/*
 * File: locIdent.c
 * Purpose: support routines for high-level local identifier definitions.
 * Date: October 1993
 * (C) Cristina Cifuentes
 */
#include "locident.h"

#include "dcc.h"
#include "msvc_fixes.h"

#include <cstring>

bool LONGID_TYPE::srcDstRegMatch(iICODE a, iICODE b) const
{
    return (a->ll()->src().getReg2()==m_l) and (b->ll()->m_dst.getReg2()==m_h);
}

ID::ID() : type(TYPE_UNKNOWN),illegal(false),loc(STK_FRAME),hasMacro(false)
{
    macro[0]=0;
    memset(&id,0,sizeof(id));
}
ID::ID(hlType t, frameType f) : type(t),illegal(false),hasMacro(false)
{
    macro[0]=0;
    memset(&id,0,sizeof(id));
    loc=f;
    assert(not ((t==TYPE_LONG_SIGN) or (t==TYPE_LONG_UNSIGN)));
}
ID::ID(hlType t,const LONGID_TYPE &s) : type(t),illegal(false),hasMacro(false)
{
    macro[0]=0;
    memset(&id,0,sizeof(id));
    loc=REG_FRAME;
    m_longId = s;
    assert((t==TYPE_LONG_SIGN) or (t==TYPE_LONG_UNSIGN));
}
ID::ID(hlType t,const LONG_STKID_TYPE &s) : type(t),illegal(false),hasMacro(false)
{
    macro[0]=0;
    memset(&id,0,sizeof(id));
    loc=STK_FRAME;
    id.longStkId = s;
    assert((t==TYPE_LONG_SIGN) or (t==TYPE_LONG_UNSIGN));
}

ID::ID(hlType t, const LONGGLB_TYPE &s) : type(t),illegal(false)
{
    macro[0]=0;
    memset(&id,0,sizeof(id));
    loc=GLB_FRAME;
    id.longGlb = s;
    assert((t==TYPE_LONG_SIGN) or (t==TYPE_LONG_UNSIGN));
}


#define LOCAL_ID_DELTA  25
#define IDX_ARRAY_DELTA  5

/* Creates a new identifier node of type t and returns it.
 * Arguments: locSym : local long symbol table
 *            t : type of LONG identifier
 *            f : frame where this variable is located
 *            ix : index into icode array where this var is used */
void LOCAL_ID::newIdent(hlType t, frameType f)
{
    ID newid(t,f);
    id_arr.push_back(newid);
}


/* Creates a new register identifier node of TYPE_BYTE_(UN)SIGN or
 * TYPE_WORD_(UN)SIGN type.  Returns the index to this new entry.       */
int LOCAL_ID::newByteWordReg(hlType t, eReg regi)
{
    int idx;

    /* Check for entry in the table */
    auto found=std::find_if(id_arr.begin(),id_arr.end(),[t,regi](ID &el)->bool {
            return ((el.type == t) and (el.id.regi == regi));
        });
    if(found!=id_arr.end())
        return found-id_arr.begin();
    /* Not in table, create new identifier */
    newIdent (t, REG_FRAME);
    idx = id_arr.size() - 1;
    id_arr[idx].id.regi = regi;
    return (idx);
}


/* Flags the entry associated with the offset off to illegal, as this
 * offset is part of a long stack variable.
 * Note: it is easier enough to remove this entry by moving the rest of
 *       the array 1 position.  The problem is that indexes into this
 *       array have already been saved in several positions; therefore,
 *       flagging this entry as illegal is all that can be done.    */
void LOCAL_ID::flagByteWordId (int off)
{
    auto found=std::find_if(id_arr.begin(),id_arr.end(),[off](ID &en)->bool {
     //if (((en.type == TYPE_WORD_SIGN) or (en.type == TYPE_BYTE_SIGN)) and
     if ((en.typeBitsize()<=16) and
         (en.id.bwId.off == off) and (en.id.bwId.regOff == 0))
        return true;
     return false;
    });
    if(found==id_arr.end())
    {
        printf("No entry to flag as invalid in LOCAL_ID::flagByteWordId \n");
        return;
    }
    found->illegal = true;
}

/* Creates a new stack identifier node of TYPE_BYTE_(UN)SIGN or
 * TYPE_WORD_(UN)SIGN type.  Returns the index to this new entry.       */
int LOCAL_ID::newByteWordStk(hlType t, int off, uint8_t regOff)
{
    int idx;

    /* Check for entry in the table */
    auto found=std::find_if(id_arr.begin(),id_arr.end(),[off,regOff](ID &el)->bool {
            if ((el.id.bwId.off == off) and (el.id.bwId.regOff == regOff))
                return true;
            return false;
        });
    if(found!=id_arr.end())
        return found-id_arr.begin(); //return Index to found element

    /* Not in table, create new identifier */
    newIdent (t, STK_FRAME);
    idx = id_arr.size() - 1;
    id_arr[idx].id.bwId.regOff = regOff;
    id_arr[idx].id.bwId.off = off;
    return (idx);
}


/* Checks if the entry exists in the locSym, if so, returns the idx to this
 * entry; otherwise creates a new global identifier node of type
 * TYPE_WORD_(UN)SIGN and returns the index to this new entry.
 * Arguments: locSym : ptr to the local symbol table
 *            seg: segment value for global variable
 *            off: offset from segment
 *            regi: indexed register into global variable
 *            ix: index into icode array
 *            t: HIGH_LEVEL type            */
int LOCAL_ID::newIntIdx(int16_t seg, int16_t off, eReg regi, hlType t)
{
    /* Check for entry in the table */
    for (size_t idx = 0; idx < id_arr.size(); idx++)
    {
        if (/*(locSym->id[idx].type == t) and   Not checking type */
            (id_arr[idx].id.bwGlb.seg == seg) and
            (id_arr[idx].id.bwGlb.off == off) and
            (id_arr[idx].id.bwGlb.regi == regi))
            return (idx);
    }

    /* Not in the table, create new identifier */
    newIdent (t, GLB_FRAME);
    id_arr.back().id.bwGlb.seg = seg;
    id_arr.back().id.bwGlb.off = off;
    id_arr.back().id.bwGlb.regi = regi;
    return id_arr.size() - 1;
}


/* Checks if the entry exists in the locSym, if so, returns the idx to this
 * entry; otherwise creates a new register identifier node of type
 * TYPE_LONG_(UN)SIGN and returns the index to this new entry.  */
int LOCAL_ID::newLongReg(hlType t, const LONGID_TYPE &longT, iICODE ix_)
{
    eReg regH,regL;
    regL = longT.l();
    regH = longT.h();
    size_t idx;
    //iICODE ix_;
    /* Check for entry in the table */
    for (idx = 0; idx < id_arr.size(); idx++)
    {
        ID &entry(id_arr[idx]);
        if(!entry.isLong() or (entry.loc != REG_FRAME))
            continue;
        if (/*(locSym->id[idx].type == t) and   Not checking type */
                (entry.longId().h() == regH) and
                (entry.longId().l() == regL))
        {
            /* Check for occurrence in the list */
            if (entry.idx.inList(ix_))
                return (idx);
            else
            {
                /* Insert icode index in list */
                entry.idx.push_back(ix_);
                return (idx);
            }
        }
    }

    /* Not in the table, create new identifier */
    id_arr.push_back(ID(t, LONGID_TYPE(regH,regL)));
    id_arr.back().idx.push_back(ix_);
    return (id_arr.size() - 1);
}
/* Returns an identifier conditional expression node of type TYPE_LONG or
 * TYPE_WORD_SIGN	*/
AstIdent * LOCAL_ID::createId(const ID *retVal, iICODE ix_)
{
    return AstIdent::idID(retVal,this,ix_);
}

/* Checks if the entry exists in the locSym, if so, returns the idx to this
 * entry; otherwise creates a new global identifier node of type
 * TYPE_LONG_(UN)SIGN and returns the index to this new entry.  */
int LOCAL_ID::newLongGlb(int16_t seg, int16_t offH, int16_t offL,hlType t)
{
    size_t idx;

    /* Check for entry in the table */
    for (idx = 0; idx < id_arr.size(); idx++)
    {
        if (/*(locSym->id[idx].type == t) and   Not checking type */
            (id_arr[idx].id.longGlb.seg == seg) and
            (id_arr[idx].id.longGlb.offH == offH) and
            (id_arr[idx].id.longGlb.offL == offL))
            return (idx);
    }
    printf("%d",t);
    /* Not in the table, create new identifier */
    id_arr.push_back(ID(t, LONGGLB_TYPE(seg,offH,offL)));
    return (id_arr.size() - 1);

}


/* Checks if the entry exists in the locSym, if so, returns the idx to this
 * entry; otherwise creates a new global identifier node of type
 * TYPE_LONG_(UN)SIGN and returns the index to this new entry.  */
int LOCAL_ID::newLongIdx( int16_t seg, int16_t offH, int16_t offL,uint8_t regi, hlType t)
{
    size_t idx;

    /* Check for entry in the table */
    for (idx = 0; idx < id_arr.size(); idx++)
    {
        if (/*(locSym->id[idx].type == t) and   Not checking type */
            (id_arr[idx].id.longGlb.seg == seg) and
            (id_arr[idx].id.longGlb.offH == offH) and
            (id_arr[idx].id.longGlb.offL == offL) and
            (id_arr[idx].id.longGlb.regi == regi))
            return (idx);
    }

    /* Not in the table, create new identifier */
    newIdent (t, GLB_FRAME);
    idx = id_arr.size() - 1;
    id_arr[idx].id.longGlb.seg = seg;
    id_arr[idx].id.longGlb.offH = offH;
    id_arr[idx].id.longGlb.offL = offL;
    id_arr[idx].id.longGlb.regi = regi;
    return (idx);
}


/* Creates a new stack identifier node of type TYPE_LONG_(UN)SIGN.
 * Returns the index to this entry. */
int LOCAL_ID::newLongStk(hlType t, int offH, int offL)
{
    size_t idx;

    /* Check for entry in the table */
    for (idx = 0; idx < id_arr.size(); idx++)
    {
        if(id_arr[idx].loc!=STK_FRAME)
            continue;
        if ((id_arr[idx].type == t) and
            (id_arr[idx].longStkId().offH == offH) and
            (id_arr[idx].longStkId().offL == offL))
            return (idx);
    }

    /* Not in the table; flag as invalid offH and offL */
    flagByteWordId (offH);
    flagByteWordId (offL);

    /* Create new identifier */
    id_arr.push_back(ID(t,LONG_STKID_TYPE(offH,offL)));
    return (id_arr.size() - 1);
}


/* Returns the index to an appropriate long identifier.
 * Note: long constants should be checked first and stored as a long integer
 *       number in an expression record.    */
int LOCAL_ID::newLong(opLoc sd, iICODE pIcode, hlFirst f, iICODE ix,operDu du, LLInst &atOffset)
{
    size_t idx = ~0; //WARNING: clients of this method might propagate this bogus value!
    const LLOperand *pmH, *pmL;
        LLInst &p_ll(*pIcode->ll());
    if (f == LOW_FIRST)
    {
        pmL = p_ll.get(sd);
        pmH = atOffset.get(sd);
    }
    else        /* HIGH_FIRST */
    {
        pmL = atOffset.get(sd);
        pmH = p_ll.get(sd);
    }

    if (pmL->regi == 0)                                 /* global variable */
        idx = newLongGlb(pmH->segValue, pmH->off, pmL->off, TYPE_LONG_SIGN);

    else if (pmL->regi < INDEX_BX_SI)                     /* register */
    {
        idx = newLongReg(TYPE_LONG_SIGN, LONGID_TYPE(pmH->regi, pmL->regi), ix);
        if (f == HIGH_FIRST)
            pIcode->setRegDU( pmL->regi, du);   /* low part */
        else
            pIcode->setRegDU( pmH->regi, du);   /* high part */
    }

    else if (pmL->off) {                                /* offset */
        if ((pmL->seg == rSS) and (pmL->regi == INDEX_BP)) /* idx on bp */
            idx = newLongStk(TYPE_LONG_SIGN, pmH->off, pmL->off);
        else if ((pmL->seg == rDS) and (pmL->regi == INDEX_BX))   /* bx */
        {                                       /* glb var indexed on bx */
            printf("Bx indexed global, BX is an unused parameter to newLongIdx\n");
            idx = newLongIdx(pmH->segValue, pmH->off, pmL->off,rBX,TYPE_LONG_SIGN);
            pIcode->setRegDU( rBX, eUSE);
        }
        else                                            /* idx <> bp, bx */
            printf ("long not supported, idx <> bp\n");
    }

    else  /* (pm->regi >= INDEXBASE and pm->off = 0) => indexed and no off */
        printf ("long not supported, idx and no off\n");

    return (idx);
}


/* Checks whether the long stack identifier is equivalent to the source or
 * destination operands of pIcode and pIcode+1 (ie. these are LOW_LEVEL
 * icodes at present).  If so, returns the rhs and lhs of this instruction.
 * Arguments: longId    : long stack identifier
 *            pIcode    : ptr to first LOW_LEVEL icode instruction
 *            i         : idx into local identifier table for longId
 *            idx       : idx into icode array
 *            pProc     : ptr to current procedure record
 *            rhs, lhs  : return expressions if successful. */
bool checkLongEq (LONG_STKID_TYPE longId, iICODE pIcode, int i, Function * pProc, Assignment &asgn, LLInst &atOffset)
{
    /* pointers to LOW_LEVEL icodes */
    const LLOperand *pmHdst, *pmLdst, *pmHsrc, *pmLsrc;

    pmHdst = &pIcode->ll()->m_dst;
    pmLdst = &atOffset.m_dst;
    pmHsrc = &pIcode->ll()->src();
    pmLsrc = &atOffset.src();
//    if ((longId.offH == pmHsrc->off) and (longId.offL == pmLsrc->off))
//    {
//        asgn.lhs = AstIdent::LongIdx (i);

//        if ( not pIcode->ll()->testFlags(NO_SRC) )
//        {
//            asgn.rhs = AstIdent::Long (&pProc->localId, SRC, pIcode, HIGH_FIRST, pIcode, eUSE, atOffset);
//        }
//        return true;
//    }
//    else if ((longId.offH == pmHdst->off) and (longId.offL == pmLdst->off))
//    {
//        asgn.lhs = AstIdent::Long (&pProc->localId, DST, pIcode, HIGH_FIRST, pIcode,eDEF, atOffset);
//        asgn.rhs = AstIdent::LongIdx (i);
//        return true;
//    }

    if ((longId.offH == pmHdst->off) and (longId.offL == pmLdst->off))
    {
        asgn.lhs = AstIdent::LongIdx (i);

        if ( not pIcode->ll()->testFlags(NO_SRC) )
        {
            asgn.rhs = AstIdent::Long (&pProc->localId, SRC, pIcode, HIGH_FIRST, pIcode, eUSE, atOffset);
        }
        return true;
    }
    else if ((longId.offH == pmHsrc->off) and (longId.offL == pmLsrc->off))
    {
        asgn.lhs = AstIdent::Long (&pProc->localId, DST, pIcode, HIGH_FIRST, pIcode,eDEF, atOffset);
        asgn.rhs = AstIdent::LongIdx (i);
        return true;
    }
    return false;
}


/* Checks whether the long stack identifier is equivalent to the source or
 * destination operands of pIcode and pIcode+1 (ie. these are LOW_LEVEL
 * icodes at present).  If so, returns the rhs and lhs of this instruction.
 * Arguments: longId    : long stack identifier
 *            pIcode    : ptr to first LOW_LEVEL icode instruction
 *            i         : idx into local identifier table for longId
 *            idx       : idx into icode array
 *            pProc     : ptr to current procedure record
 *            rhs, lhs  : return expressions if successful. */
bool checkLongRegEq (LONGID_TYPE longId, iICODE pIcode, int i,
                      Function * pProc, Assignment &asgn, LLInst &atOffset)
{
    /* pointers to LOW_LEVEL icodes */
    const LLOperand *pmHdst, *pmLdst, *pmHsrc, *pmLsrc;

    pmHdst = &pIcode->ll()->m_dst;
    pmLdst = &atOffset.m_dst;
    pmHsrc = &pIcode->ll()->src();
    pmLsrc = &atOffset.src();

    if ((longId.h() == pmHdst->regi) and (longId.l() == pmLdst->regi))
    {
        asgn.lhs = AstIdent::LongIdx (i);
        if ( not pIcode->ll()->testFlags(NO_SRC) )
        {
            asgn.rhs = AstIdent::Long (&pProc->localId, SRC, pIcode, HIGH_FIRST,  pIcode, eUSE, atOffset);
        }
        return true;
    }
    else if ((longId.h() == pmHsrc->regi) and (longId.l() == pmLsrc->regi))
    {
        asgn.lhs = AstIdent::Long (&pProc->localId, DST, pIcode, HIGH_FIRST, pIcode, eDEF, atOffset);
        asgn.rhs = AstIdent::LongIdx (i);
        return true;
    }
    return false;
}



/* Given an index into the local identifier table for a long register
 * variable, determines whether regi is the high or low part, and returns
 * the other part   */
eReg otherLongRegi (eReg regi, int idx, LOCAL_ID *locTbl)
{
    ID *id;

    id = &locTbl->id_arr[idx];
    if ((id->loc == REG_FRAME) and ((id->type == TYPE_LONG_SIGN) or
        (id->type == TYPE_LONG_UNSIGN)))
    {
        if (id->longId().h() == regi)
            return (id->longId().l());
        else if (id->longId().l() == regi)
            return (id->longId().h());
    }
    return rUNDEF;	// Cristina: please check this!
}


/* Checks if the registers regL and regH have been used independently in
 * the local identifier table.  If so, macros for these registers are
 * placed in the local identifier table, as these registers belong to a
 * long register identifier.	*/
void LOCAL_ID::propLongId (uint8_t regL, uint8_t regH, const char *name)
{
    for (ID &rid : id_arr)
    {
        if (rid.typeBitsize()!=16)
            continue;
        if ( (rid.id.regi != regL) and (rid.id.regi != regH) )
            continue;
        // otherwise at least 1 is ok
        rid.name = name;
        rid.hasMacro = true;
        rid.illegal = true;
        if (rid.id.regi == regL)
        {
            strcpy (rid.macro, "LO");
            }
        else // if (rid.id.regi == regH)
            {
            strcpy (rid.macro, "HI");
        }
    }
}
