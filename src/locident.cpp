/*
 * File: locIdent.c
 * Purpose: support routines for high-level local identifier definitions.
 * Date: October 1993
 * (C) Cristina Cifuentes
 */

#include "dcc.h"
#include <string.h>


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
Int LOCAL_ID::newByteWordReg(hlType t, byte regi)
{
    Int idx;

    /* Check for entry in the table */
    auto found=std::find_if(id_arr.begin(),id_arr.end(),[t,regi](ID &el)->bool {
            return ((el.type == t) && (el.id.regi == regi));
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
void LOCAL_ID::flagByteWordId (Int off)
{
    Int idx;
    auto found=std::find_if(id_arr.begin(),id_arr.end(),[off](ID &en)->bool {
     //if (((en.type == TYPE_WORD_SIGN) || (en.type == TYPE_BYTE_SIGN)) &&
     if ((en.isSigned()) &&
         (en.id.bwId.off == off) && (en.id.bwId.regOff == 0))
        return true;
     return false;
    });
    if(found==id_arr.end())
    {
        printf("Entry not found in LOCAL_ID::flagByteWordId \n");
        return;
    }
    found->illegal = TRUE;
}

/* Creates a new stack identifier node of TYPE_BYTE_(UN)SIGN or
 * TYPE_WORD_(UN)SIGN type.  Returns the index to this new entry.       */
Int LOCAL_ID::newByteWordStk(hlType t, Int off, byte regOff)
{
    Int idx;

    /* Check for entry in the table */
    auto found=std::find_if(id_arr.begin(),id_arr.end(),[off,regOff](ID &el)->bool {
            if ((el.id.bwId.off == off) && (el.id.bwId.regOff == regOff))
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
Int LOCAL_ID::newIntIdx(int16 seg, int16 off, byte regi,Int ix, hlType t)
{
    Int idx;

    /* Check for entry in the table */
    for (idx = 0; idx < id_arr.size(); idx++)
    {
        if (/*(locSym->id[idx].type == t) &&   Not checking type */
            (id_arr[idx].id.bwGlb.seg == seg) &&
            (id_arr[idx].id.bwGlb.off == off) &&
            (id_arr[idx].id.bwGlb.regi == regi))
            return (idx);
    }

    /* Not in the table, create new identifier */
    newIdent (t, GLB_FRAME);
    idx = id_arr.size() - 1;
    id_arr[idx].id.bwGlb.seg = seg;
    id_arr[idx].id.bwGlb.off = off;
    id_arr[idx].id.bwGlb.regi = regi;
    return (idx);
}


/* Checks if the entry exists in the locSym, if so, returns the idx to this
 * entry; otherwise creates a new register identifier node of type
 * TYPE_LONG_(UN)SIGN and returns the index to this new entry.  */
Int LOCAL_ID::newLongReg(hlType t, byte regH, byte regL, iICODE ix_)
{
    Int idx;
    //iICODE ix_;
    /* Check for entry in the table */
    for (idx = 0; idx < id_arr.size(); idx++)
    {
        ID &entry(id_arr[idx]);
        if (/*(locSym->id[idx].type == t) &&   Not checking type */
            (entry.id.longId.h == regH) &&
            (entry.id.longId.l == regL))
        {
            /* Check for occurrence in the list */
            if (entry.idx.inList(ix_)) //count(
                return (idx);
            else
            {
                /* Insert icode index in list */
                entry.idx.push_back(ix_);
                //entry.idx.insert(ix_);
                return (idx);
            }
        }
    }

    /* Not in the table, create new identifier */
    newIdent (t, REG_FRAME);
    id_arr[id_arr.size()-1].idx.push_back(ix_);//insert(ix_);
    idx = id_arr.size() - 1;
    id_arr[idx].id.longId.h = regH;
    id_arr[idx].id.longId.l = regL;
    return (idx);
}


/* Checks if the entry exists in the locSym, if so, returns the idx to this
 * entry; otherwise creates a new global identifier node of type
 * TYPE_LONG_(UN)SIGN and returns the index to this new entry.  */
Int LOCAL_ID::newLongGlb(int16 seg, int16 offH, int16 offL,hlType t)
{
    Int idx;

    /* Check for entry in the table */
    for (idx = 0; idx < id_arr.size(); idx++)
    {
        if (/*(locSym->id[idx].type == t) &&   Not checking type */
            (id_arr[idx].id.longGlb.seg == seg) &&
            (id_arr[idx].id.longGlb.offH == offH) &&
            (id_arr[idx].id.longGlb.offL == offL))
            return (idx);
    }

    /* Not in the table, create new identifier */
    newIdent (t, GLB_FRAME);
    idx = id_arr.size() - 1;
    id_arr[idx].id.longGlb.seg = seg;
    id_arr[idx].id.longGlb.offH = offH;
    id_arr[idx].id.longGlb.offL = offL;
    return (idx);
}


/* Checks if the entry exists in the locSym, if so, returns the idx to this
 * entry; otherwise creates a new global identifier node of type
 * TYPE_LONG_(UN)SIGN and returns the index to this new entry.  */
Int LOCAL_ID::newLongIdx( int16 seg, int16 offH, int16 offL,byte regi, hlType t)
{ Int idx;

    /* Check for entry in the table */
    for (idx = 0; idx < id_arr.size(); idx++)
    {
        if (/*(locSym->id[idx].type == t) &&   Not checking type */
            (id_arr[idx].id.longGlb.seg == seg) &&
            (id_arr[idx].id.longGlb.offH == offH) &&
            (id_arr[idx].id.longGlb.offL == offL) &&
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
Int LOCAL_ID::newLongStk(hlType t, Int offH, Int offL)
{
    Int idx;

    /* Check for entry in the table */
    for (idx = 0; idx < id_arr.size(); idx++)
    {
        if ((id_arr[idx].type == t) &&
            (id_arr[idx].id.longStkId.offH == offH) &&
            (id_arr[idx].id.longStkId.offL == offL))
            return (idx);
    }

    /* Not in the table; flag as invalid offH and offL */
    flagByteWordId (offH);
    flagByteWordId (offL);

    /* Create new identifier */
    newIdent (t, STK_FRAME);
    idx = id_arr.size() - 1;
    id_arr[idx].id.longStkId.offH = offH;
    id_arr[idx].id.longStkId.offL = offL;
    return (idx);
}


/* Returns the index to an appropriate long identifier.
 * Note: long constants should be checked first and stored as a long integer
 *       number in an expression record.    */
Int LOCAL_ID::newLong(opLoc sd, ICODE *pIcode, hlFirst f, iICODE ix,operDu du, Int off)
{
    Int idx;
  LLOperand *pmH, *pmL;

    if (f == LOW_FIRST)
    {
        pmL = (sd == SRC) ? &pIcode->ic.ll.src : &pIcode->ic.ll.dst;
        pmH = (sd == SRC) ? &(pIcode+off)->ic.ll.src : &(pIcode+off)->ic.ll.dst;
    }
    else        /* HIGH_FIRST */
    {
        pmH = (sd == SRC) ? &pIcode->ic.ll.src : &pIcode->ic.ll.dst;
        pmL = (sd == SRC) ? &(pIcode+off)->ic.ll.src : &(pIcode+off)->ic.ll.dst;
    }

    if (pmL->regi == 0)                                 /* global variable */
        idx = newLongGlb(pmH->segValue, pmH->off, pmL->off, TYPE_LONG_SIGN);

    else if (pmL->regi < INDEXBASE)                     /* register */
    {
        idx = newLongReg(TYPE_LONG_SIGN, pmH->regi, pmL->regi, ix);
        if (f == HIGH_FIRST)
            pIcode->setRegDU( pmL->regi, du);   /* low part */
        else
            pIcode->setRegDU( pmH->regi, du);   /* high part */
    }

    else if (pmL->off) {                                /* offset */
        if ((pmL->seg == rSS) && (pmL->regi == INDEXBASE + 6)) /* idx on bp */
            idx = newLongStk(TYPE_LONG_SIGN, pmH->off, pmL->off);
        else if ((pmL->seg == rDS) && (pmL->regi == INDEXBASE + 7))   /* bx */
        {                                       /* glb var indexed on bx */
            printf("Bx indexed global, BX is an unused parameter to newLongIdx\n");
            idx = newLongIdx(pmH->segValue, pmH->off, pmL->off,rBX,TYPE_LONG_SIGN);
            pIcode->setRegDU( rBX, eUSE);
        }
        else                                            /* idx <> bp, bx */
            printf ("long not supported, idx <> bp\n");
    }

    else  /* (pm->regi >= INDEXBASE && pm->off = 0) => indexed && no off */
        printf ("long not supported, idx && no off\n");

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
boolT checkLongEq (LONG_STKID_TYPE longId, iICODE pIcode, Int i,
                  Function * pProc, Assignment &asgn, Int off)
{
    LLOperand *pmHdst, *pmLdst, *pmHsrc, *pmLsrc;  /* pointers to LOW_LEVEL icodes */

    pmHdst = &pIcode->ic.ll.dst;
    pmLdst = &(pIcode+off)->ic.ll.dst;
    pmHsrc = &pIcode->ic.ll.src;
    pmLsrc = &(pIcode+off)->ic.ll.src;

    if ((longId.offH == pmHdst->off) && (longId.offL == pmLdst->off))
    {
        asgn.lhs = COND_EXPR::idLongIdx (i);

        if ((pIcode->ic.ll.flg & NO_SRC) != NO_SRC)
        {
            asgn.rhs = COND_EXPR::idLong (&pProc->localId, SRC, pIcode, HIGH_FIRST, pIcode, eUSE, off);
        }
        return true;
    }
    else if ((longId.offH == pmHsrc->off) && (longId.offL == pmLsrc->off))
    {
        asgn.lhs = COND_EXPR::idLong (&pProc->localId, DST, pIcode, HIGH_FIRST, pIcode,eDEF, off);
        asgn.rhs = COND_EXPR::idLongIdx (i);
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
boolT checkLongRegEq (LONGID_TYPE longId, iICODE pIcode, Int i,
                      Function * pProc, COND_EXPR *&rhs, COND_EXPR *&lhs, Int off)
{
    LLOperand *pmHdst, *pmLdst, *pmHsrc, *pmLsrc;  /* pointers to LOW_LEVEL icodes */

    pmHdst = &pIcode->ic.ll.dst;
    pmLdst = &(pIcode+off)->ic.ll.dst;
    pmHsrc = &pIcode->ic.ll.src;
    pmLsrc = &(pIcode+off)->ic.ll.src;

    if ((longId.h == pmHdst->regi) && (longId.l == pmLdst->regi))
    {
        lhs = COND_EXPR::idLongIdx (i);
        if ((pIcode->ic.ll.flg & NO_SRC) != NO_SRC)
        {
            //rhs = COND_EXPR::idLong (&pProc->localId, SRC, pIcode, HIGH_FIRST,  idx, eUSE, off);
            rhs = COND_EXPR::idLong (&pProc->localId, SRC, pIcode, HIGH_FIRST,  pIcode, eUSE, off);
        }
        return true;
    }
    else if ((longId.h == pmHsrc->regi) && (longId.l == pmLsrc->regi))
    {
        lhs = COND_EXPR::idLong (&pProc->localId, DST, pIcode, HIGH_FIRST, pIcode, eDEF, off);
        //lhs = COND_EXPR::idLong (&pProc->localId, DST, pIcode, HIGH_FIRST, idx, eDEF, off);
        rhs = COND_EXPR::idLongIdx (i);
        return true;
    }
    return false;
}



/* Given an index into the local identifier table for a long register
 * variable, determines whether regi is the high or low part, and returns
 * the other part   */
byte otherLongRegi (byte regi, Int idx, LOCAL_ID *locTbl)
{
    ID *id;

    id = &locTbl->id_arr[idx];
    if ((id->loc == REG_FRAME) && ((id->type == TYPE_LONG_SIGN) ||
        (id->type == TYPE_LONG_UNSIGN)))
    {
        if (id->id.longId.h == regi)
            return (id->id.longId.l);
        else if (id->id.longId.l == regi)
            return (id->id.longId.h);
    }
    return 0;			// Cristina: please check this!
}


/* Checks if the registers regL and regH have been used independently in
 * the local identifier table.  If so, macros for these registers are
 * placed in the local identifier table, as these registers belong to a
 * long register identifier.	*/
void LOCAL_ID::propLongId (byte regL, byte regH, const char *name)
{
    Int i;
    ID *_id;

    for (i = 0; i < id_arr.size(); i++)
    {
        _id = &id_arr[i];
        if (_id->typeBitsize()==16)
        {
            if (_id->id.regi == regL)
            {
                strcpy (_id->name, name);
                strcpy (_id->macro, "LO");
                _id->hasMacro = TRUE;
                _id->illegal = TRUE;
            }
            else if (_id->id.regi == regH)
            {
                strcpy (_id->name, name);
                strcpy (_id->macro, "HI");
                _id->hasMacro = TRUE;
                _id->illegal = TRUE;
            }
        }
    }
}
