/*
 * File: ast.c
 * Purpose: Support module for abstract syntax trees.
 * Date: September 1993
 * (C) Cristina Cifuentes
 */

#if __BORLAND__
#include <alloc.h>
#else
#include <malloc.h>		/* For free() */
#endif
#include <string.h>
#include "dcc.h"

/* Index registers **** temp solution */
static char *idxReg[8] = {"bx+si", "bx+di", "bp+si", "bp+di", 
                          "si", "di", "bp", "bx" };
/* Conditional operator symbols in C.  Index by condOp enumeration type */
static char *condOpSym[] = { " <= ", " < ", " == ", " != ", " > ", " >= ",
                             " & ", " | ", " ^ ", " ~ ",
                             " + ", " - ", " * ", " / ", 
                             " >> ", " << ", " % ", " && ", " || " };

#define EXP_SIZE 200		/* Size of the expression buffer */

/* Local expression stack */
typedef struct _EXP_STK {
    COND_EXPR       *exp;
    struct _EXP_STK *next;
} EXP_STK;

static EXP_STK *expStk = NULL;      /* local expression stack */


static char *hexStr (Int i)
/* Returns the integer i in C hexadecimal format */
{ static char buf[10];

    i &= 0xFFFF;
    sprintf (buf, "%s%lX", (i > 9) ? "0x" : "", i);
    return (buf);
}


void setRegDU (PICODE pIcode, byte regi, operDu du)
/* Sets the du record for registers according to the du flag    */
{
    switch (du) {
      case DEF:     pIcode->du.def |= duReg[regi];
                    pIcode->du1.numRegsDef++;
                    break;
      case USE:     pIcode->du.use |= duReg[regi];
                    break;
      case USE_DEF: pIcode->du.def |= duReg[regi];
                    pIcode->du1.numRegsDef++;
                    pIcode->du.use |= duReg[regi];
                    break;
      case NONE:    /* do nothing */
                    break;
    }
}


void copyDU (PICODE pIcode, PICODE duIcode, operDu du, operDu duDu)
/* Copies the def, use, or def and use fields of duIcode into pIcode */
{
    switch (du) {
      case DEF:     
            if (duDu == DEF)
                memcpy (&pIcode->du.def, &duIcode->du.def, sizeof(dword));
            else
                memcpy (&pIcode->du.def, &duIcode->du.use, sizeof(dword));
            break;
      case USE: 
            if (duDu == DEF)
                memcpy (&pIcode->du.use, &duIcode->du.def, sizeof(dword));
            else
                memcpy (&pIcode->du.use, &duIcode->du.use, sizeof(dword));
            break;
      case USE_DEF: 
            memcpy (&pIcode->du, &duIcode->du, sizeof(DU_ICODE));
            break;
    }
}


static COND_EXPR *newCondExp (condNodeType t)
/* Creates a newExp conditional expression node of type t and returns it */
{ COND_EXPR *newExp;
    
    newExp = allocStruc(COND_EXPR); 
    memset(newExp, 0, sizeof(COND_EXPR));
    newExp->type = t;
    return (newExp);
}


COND_EXPR *boolCondExp (COND_EXPR *lhs, COND_EXPR *rhs, condOp op)
/* Creates a conditional boolean expression and returns it */
{ COND_EXPR *newExp;

    newExp = newCondExp (BOOLEAN_OP);
    newExp->expr.boolExpr.op = op;
    newExp->expr.boolExpr.lhs = lhs;
    newExp->expr.boolExpr.rhs = rhs;
    return (newExp);
}


COND_EXPR *unaryCondExp (condNodeType t, COND_EXPR *exp)
/* Returns a unary conditional expression node.  This procedure should 
 * only be used with the following conditional node types: NEGATION,
 * ADDRESSOF, DEREFERENCE, POST_INC, POST_DEC, PRE_INC, PRE_DEC	*/
{ COND_EXPR *newExp;

	newExp = newCondExp (t);
	newExp->expr.unaryExp = exp;
	return (newExp);
}


COND_EXPR *idCondExpGlob (int16 segValue, int16 off)
/* Returns an identifier conditional expression node of type GLOB_VAR */
{ COND_EXPR *newExp;
  dword adr;
  Int i;

    newExp = newCondExp (IDENTIFIER);
    newExp->expr.ident.idType = GLOB_VAR;
    adr = opAdr(segValue, off);
    for (i = 0; i < symtab.csym; i++)
        if (symtab.sym[i].label == adr)
            break;
if (i == symtab.csym)
printf ("Error, glob var not found in symtab\n");
    newExp->expr.ident.idNode.globIdx = i;
    return (newExp);
}


COND_EXPR *idCondExpReg (byte regi, flags32 icodeFlg, LOCAL_ID *locsym)
/* Returns an identifier conditional expression node of type REGISTER */
{ COND_EXPR *newExp;

    newExp = newCondExp (IDENTIFIER);
    newExp->expr.ident.idType = REGISTER;
    if ((icodeFlg & B) || (icodeFlg & SRC_B))
    {
        newExp->expr.ident.idNode.regiIdx = newByteWordRegId (locsym, 
                                                        TYPE_BYTE_SIGN, regi);
        newExp->expr.ident.regiType = BYTE_REG;
    }
    else    /* word */
    {
        newExp->expr.ident.idNode.regiIdx = newByteWordRegId (locsym, 
                                                        TYPE_WORD_SIGN, regi);
        newExp->expr.ident.regiType = WORD_REG;
    }
    return (newExp);
}


COND_EXPR *idCondExpRegIdx (Int idx, regType type)
/* Returns an identifier conditional expression node of type REGISTER */
{ COND_EXPR *newExp;

	newExp = newCondExp (IDENTIFIER);
	newExp->expr.ident.idType = REGISTER;
	newExp->expr.ident.regiType = type;
	newExp->expr.ident.idNode.regiIdx = idx;
	return (newExp);
}


COND_EXPR *idCondExpLoc (Int off, LOCAL_ID *localId)
/* Returns an identifier conditional expression node of type LOCAL_VAR */ 
{ COND_EXPR *newExp;
  Int i;

    newExp = newCondExp (IDENTIFIER);
    newExp->expr.ident.idType = LOCAL_VAR;
    for (i = 0; i < localId->csym; i++)
        if ((localId->id[i].id.bwId.off == off) &&
            (localId->id[i].id.bwId.regOff == 0))
            break;
if (i == localId->csym) printf ("Error, cannot find local var\n");
    newExp->expr.ident.idNode.localIdx = i;
    sprintf (localId->id[i].name, "loc%ld", i);
    return (newExp);
}


COND_EXPR *idCondExpParam (Int off, PSTKFRAME argSymtab)
/* Returns an identifier conditional expression node of type PARAM */ 
{ COND_EXPR *newExp;
  Int i;

    newExp = newCondExp (IDENTIFIER);
    newExp->expr.ident.idType = PARAM;
    for (i = 0; i < argSymtab->csym; i++)
        if (argSymtab->sym[i].off == off)
            break;
if (i == argSymtab->csym) printf ("Error, cannot find argument var\n");
    newExp->expr.ident.idNode.localIdx = i;
    return (newExp);
}


COND_EXPR *idCondExpIdxGlob (int16 segValue, int16 off, byte regi, 
                              LOCAL_ID *locSym)
/* Returns an identifier conditional expression node of type GLOB_VAR_IDX.
 * This global variable is indexed by regi.     */
{ COND_EXPR *newExp;
  Int i;

    newExp = newCondExp (IDENTIFIER);
    newExp->expr.ident.idType = GLOB_VAR_IDX;
    for (i = 0; i < locSym->csym; i++)
        if ((locSym->id[i].id.bwGlb.seg == segValue) &&
            (locSym->id[i].id.bwGlb.off == off) &&
            (locSym->id[i].id.bwGlb.regi == regi))
            break;
if (i == locSym->csym) 
printf ("Error, indexed-glob var not found in local id table\n");
    newExp->expr.ident.idNode.idxGlbIdx = i;
    return (newExp);
}


COND_EXPR *idCondExpKte (dword kte, byte size)
/* Returns an identifier conditional expression node of type CONSTANT */
{ COND_EXPR *newExp;

    newExp = newCondExp (IDENTIFIER);
    newExp->expr.ident.idType = CONSTANT;
    newExp->expr.ident.idNode.kte.kte = kte;
	newExp->expr.ident.idNode.kte.size = size;
    return (newExp);
}


COND_EXPR *idCondExpLongIdx (Int idx)
/* Returns an identifier conditional expression node of type LONG_VAR,
 * that points to the given index idx.  */
{ COND_EXPR *newExp;
    
    newExp = newCondExp (IDENTIFIER);
    newExp->expr.ident.idType = LONG_VAR;
    newExp->expr.ident.idNode.longIdx = idx;
    return (newExp);
}


COND_EXPR *idCondExpLong (LOCAL_ID *localId, opLoc sd, PICODE pIcode, 
                          hlFirst f, Int ix, operDu du, Int off)
/* Returns an identifier conditional expression node of type LONG_VAR */
{ COND_EXPR *newExp;
  Int idx;

    newExp = newCondExp (IDENTIFIER);

    /* Check for long constant and save it as a constant expression */
    if ((sd == SRC) && ((pIcode->ic.ll.flg & I) == I))  /* constant */
    {
        newExp->expr.ident.idType = CONSTANT;
        if (f == HIGH_FIRST)
            newExp->expr.ident.idNode.kte.kte = (pIcode->ic.ll.immed.op << 16) +
                                         (pIcode+off)->ic.ll.immed.op;
        else        /* LOW_FIRST */
            newExp->expr.ident.idNode.kte.kte = 
				((pIcode+off)->ic.ll.immed.op << 16)+ pIcode->ic.ll.immed.op;
		newExp->expr.ident.idNode.kte.size = 4;
    }
    /* Save it as a long expression (reg, stack or glob) */
    else
    {
        idx = newLongId (localId, sd, pIcode, f, ix, du, off);
        newExp->expr.ident.idType = LONG_VAR;
        newExp->expr.ident.idNode.longIdx = idx;
    }
    return (newExp);
}


COND_EXPR *idCondExpFunc (PPROC pproc, PSTKFRAME args)
/* Returns an identifier conditional expression node of type FUNCTION */
{ COND_EXPR *newExp;

    newExp = newCondExp (IDENTIFIER);
    newExp->expr.ident.idType = FUNCTION;
    newExp->expr.ident.idNode.call.proc = pproc;
	newExp->expr.ident.idNode.call.args = args;
    return (newExp);
}


COND_EXPR *idCondExpOther (byte seg, byte regi, int16 off)
/* Returns an identifier conditional expression node of type OTHER. 
 * Temporary solution, should really be encoded as an indexed type (eg.
 * arrays). */
{ COND_EXPR *newExp;

    newExp = newCondExp (IDENTIFIER);
    newExp->expr.ident.idType = OTHER;
    newExp->expr.ident.idNode.other.seg = seg;
    newExp->expr.ident.idNode.other.regi = regi;
    newExp->expr.ident.idNode.other.off = off;
    return (newExp);
}


COND_EXPR *idCondExpID (ID *retVal, LOCAL_ID *locsym, Int ix)
/* Returns an identifier conditional expression node of type TYPE_LONG or
 * TYPE_WORD_SIGN	*/
{ COND_EXPR *newExp;
  Int idx;

	newExp = newCondExp (IDENTIFIER);
	if (retVal->type == TYPE_LONG_SIGN)
	{
		idx = newLongRegId (locsym, TYPE_LONG_SIGN, retVal->id.longId.h,
							retVal->id.longId.l, ix);
		newExp->expr.ident.idType = LONG_VAR;
		newExp->expr.ident.idNode.longIdx = idx;
	}
	else if (retVal->type == TYPE_WORD_SIGN)
	{
		newExp->expr.ident.idType = REGISTER;
        newExp->expr.ident.idNode.regiIdx = newByteWordRegId (locsym, 
                                         TYPE_WORD_SIGN, retVal->id.regi);
        newExp->expr.ident.regiType = WORD_REG;
	}
	return (newExp);
}


COND_EXPR *idCondExp (PICODE pIcode, opLoc sd, PPROC pProc, Int i, 
                      PICODE duIcode, operDu du)
/* Returns an identifier conditional expression node, according to the given
 * type.
 * Arguments: i : index into the icode array, used for newLongRegId only. 
 *            duIcode: icode instruction that needs the du set.
 *            du: operand is defined or used in current instruction.    */
{ COND_EXPR *newExp;
  PMEM pm;
  Int idx;          /* idx into pIcode->localId table */

    pm = (sd == SRC) ? &pIcode->ic.ll.src : &pIcode->ic.ll.dst;

    if (((sd == DST) && (pIcode->ic.ll.flg & IM_DST) == IM_DST) ||
        ((sd == SRC) && (pIcode->ic.ll.flg & IM_SRC)) ||
        (sd == LHS_OP))             /* for MUL lhs */
    {                                                   /* implicit dx:ax */
        idx = newLongRegId (&pProc->localId, TYPE_LONG_SIGN, rDX, rAX, i);
        newExp = idCondExpLongIdx (idx);
        setRegDU (duIcode, rDX, du); 
        setRegDU (duIcode, rAX, du);
    }

    else if ((sd == DST) && (pIcode->ic.ll.flg & IM_TMP_DST) == IM_TMP_DST)
    {                                                   /* implicit tmp */
        newExp = idCondExpReg (rTMP, 0, &pProc->localId);
        setRegDU (duIcode, rTMP, (operDu)USE);
    }

    else if ((sd == SRC) && ((pIcode->ic.ll.flg & I) == I)) /* constant */
        newExp = idCondExpKte (pIcode->ic.ll.immed.op, 2);

    else if (pm->regi == 0)                             /* global variable */
        newExp = idCondExpGlob (pm->segValue, pm->off);

    else if (pm->regi < INDEXBASE)                      /* register */
    {
        newExp = idCondExpReg (pm->regi, (sd == SRC) ? pIcode->ic.ll.flg :
                            pIcode->ic.ll.flg & NO_SRC_B, &pProc->localId);
        setRegDU (duIcode, pm->regi, du);
    }

    else if (pm->off)                                   /* offset */
    {
        if ((pm->seg == rSS) && (pm->regi == INDEXBASE + 6)) /* idx on bp */
        {
            if (pm->off >= 0)                           /* argument */
                newExp = idCondExpParam (pm->off, &pProc->args);
            else                                        /* local variable */
                newExp = idCondExpLoc (pm->off, &pProc->localId); 
        }
        else if ((pm->seg == rDS) && (pm->regi == INDEXBASE + 7)) /* bx */
        {
            if (pm->off > 0)        /* global variable */
                newExp = idCondExpIdxGlob (pm->segValue, pm->off, rBX,
                                        &pProc->localId);
            else
                newExp = idCondExpOther (pm->seg, pm->regi, pm->off);
            setRegDU (duIcode, rBX, (operDu)USE);
        }
        else                                            /* idx <> bp, bx */
            newExp = idCondExpOther (pm->seg, pm->regi, pm->off);
            /**** check long ops, indexed global var *****/
    }

    else  /* (pm->regi >= INDEXBASE && pm->off = 0) => indexed && no off */ 
    {
        if ((pm->seg == rDS) && (pm->regi > INDEXBASE + 3)) /* dereference */
        {
            switch (pm->regi) {
              case INDEXBASE + 4:   newExp = idCondExpReg(rSI, 0, &pProc->localId);
                                    setRegDU (duIcode, rSI, du);
                                    break;
              case INDEXBASE + 5:   newExp = idCondExpReg(rDI, 0, &pProc->localId);
                                    setRegDU (duIcode, rDI, du);
                                    break;
              case INDEXBASE + 6:   newExp = idCondExpReg(rBP, 0, &pProc->localId);
                                    break;
              case INDEXBASE + 7:   newExp = idCondExpReg(rBX, 0, &pProc->localId);
                                    setRegDU (duIcode, rBX, du);
                                    break;
            }
            newExp = unaryCondExp (DEREFERENCE, newExp);
        }
        else
            newExp = idCondExpOther (pm->seg, pm->regi, 0);
    }

    return (newExp);
}


condId idType (PICODE pIcode, opLoc sd)
/* Returns the identifier type */
{ PMEM pm;

    pm = (sd == SRC) ? &pIcode->ic.ll.src : &pIcode->ic.ll.dst;

    if ((sd == SRC) && ((pIcode->ic.ll.flg & I) == I))
        return (CONSTANT);
    else if (pm->regi == 0)
        return (GLOB_VAR);
    else if (pm->regi < INDEXBASE)
        return (REGISTER);
    else if ((pm->seg == rSS) && (pm->regi == INDEXBASE))
    {
        if (pm->off >= 0)
            return (PARAM);
        else
            return (LOCAL_VAR);
    }
    else
        return (OTHER);
}


/* Size of hl types */
Int hlSize[] = {2, 1, 1, 2, 2, 4, 4, 4, 2, 2, 1, 4, 4};


Int hlTypeSize (COND_EXPR *exp, PPROC pproc)
/* Returns the type of the expression */
{ Int first, second;

	if (exp == NULL)
		return (2);		/* for TYPE_UNKNOWN */

	switch (exp->type) {
	  case BOOLEAN_OP: 	first = hlTypeSize (exp->expr.boolExpr.lhs, pproc);
						second = hlTypeSize (exp->expr.boolExpr.rhs, pproc);
						if (first > second)
							return (first);
						else
							return (second);
					
	  case NEGATION:	case ADDRESSOF:	
	  case POST_INC:	case POST_DEC:
	  case PRE_INC:		case PRE_DEC:
	  case DEREFERENCE:	return (hlTypeSize (exp->expr.unaryExp, pproc));
						
	  case IDENTIFIER:
			switch (exp->expr.ident.idType) {
			  case GLOB_VAR:
					return (symtab.sym[exp->expr.ident.idNode.globIdx].size);
			  case REGISTER:
					if (exp->expr.ident.regiType == BYTE_REG)
						return (1);
					else
						return (2);
			  case LOCAL_VAR:
					return (hlSize[pproc->localId.id[exp->expr.ident.idNode.localIdx].type]);
			  case PARAM:
					return (hlSize[pproc->args.sym[exp->expr.ident.idNode.paramIdx].type]);
			  case GLOB_VAR_IDX:
					return (hlSize[pproc->localId.id[exp->expr.ident.idNode.idxGlbIdx].type]);
			  case CONSTANT:
					return (exp->expr.ident.idNode.kte.size);
			  case STRING:
					return (2);
			  case LONG_VAR:
					return (4);
			  case FUNCTION:
					return (hlSize[exp->expr.ident.idNode.call.proc->retVal.type]);
			  case OTHER:
					return (2);
			} /* eos */
			break;
	}
	return 2;			// CC: is this correct?
}


hlType expType (COND_EXPR *exp, PPROC pproc)
/* Returns the type of the expression */
{ hlType first, second;

	if (exp == NULL)
		return (TYPE_UNKNOWN);

	switch (exp->type) {
	  case BOOLEAN_OP: 	first = expType (exp->expr.boolExpr.lhs, pproc);
						second = expType (exp->expr.boolExpr.rhs, pproc);
						if (first != second)
						{
							if (hlTypeSize (exp->expr.boolExpr.lhs, pproc) > 
								hlTypeSize (exp->expr.boolExpr.rhs, pproc))
								return (first);
							else
								return (second);
						}
						else
							return (first);

	  case POST_INC: case POST_DEC:
	  case PRE_INC:  case PRE_DEC:
	  case NEGATION:	return (expType (exp->expr.unaryExp, pproc));

	  case ADDRESSOF:	return (TYPE_PTR);		/***????****/
	  case DEREFERENCE:	return (TYPE_PTR);
	  case IDENTIFIER:
			switch (exp->expr.ident.idType) {
			  case GLOB_VAR:
					return (symtab.sym[exp->expr.ident.idNode.globIdx].type);
			  case REGISTER:
					if (exp->expr.ident.regiType == BYTE_REG)
						return (TYPE_BYTE_SIGN);
					else
						return (TYPE_WORD_SIGN);
			  case LOCAL_VAR:
					return (pproc->localId.id[exp->expr.ident.idNode.localIdx].type);
			  case PARAM:
					return (pproc->args.sym[exp->expr.ident.idNode.paramIdx].type);
			  case GLOB_VAR_IDX:
					return (pproc->localId.id[exp->expr.ident.idNode.idxGlbIdx].type);
			  case CONSTANT:
					return (TYPE_CONST);
			  case STRING:
					return (TYPE_STR);
			  case LONG_VAR:
					return (pproc->localId.id[exp->expr.ident.idNode.longIdx].type);
			  case FUNCTION:
					return (exp->expr.ident.idNode.call.proc->retVal.type);
			  case OTHER:
					return (TYPE_UNKNOWN);
			} /* eos */
	}
	return TYPE_UNKNOWN;		// CC: Correct?
}


void removeRegFromLong (byte regi, LOCAL_ID *locId, COND_EXPR *tree)
/* Removes the register from the tree.  If the register was part of a long
 * register (eg. dx:ax), the node gets transformed into an integer register
 * node.        */
{
	IDENTTYPE* ident;     	/* ptr to an identifier */
	byte otherRegi;         /* high or low part of long register */

    switch (tree->type) {
      case BOOLEAN_OP:
                        break;
	  case POST_INC: case POST_DEC:
	  case PRE_INC: case PRE_DEC:
      case NEGATION: case ADDRESSOF:
      case DEREFERENCE:
                        break;
      case IDENTIFIER:
            ident = &tree->expr.ident;
            if (ident->idType == LONG_VAR)
            {
                otherRegi = otherLongRegi (regi, ident->idNode.longIdx, locId);
                ident->idType = REGISTER;
                ident->regiType = WORD_REG;
                ident->idNode.regiIdx = newByteWordRegId (locId, TYPE_WORD_SIGN,
                                                      otherRegi); 
            }
            break;
    }
}


static char *getString (Int offset)
/* Returns the string located in image, formatted in C format. */
{ char *o;
  Int strLen, i;

	strLen = strSize (&prog.Image[offset], '\0');
	o = (char *) allocMem((strLen*2+1) * sizeof(char));
	o[0] = '"';
	o[1] = '\0';
	for (i = 0; i < strLen; i++)
		strcat (o, cChar(prog.Image[offset+i]));
	strcat (o, "\"\0");
	return (o);
}


char *walkCondExpr (COND_EXPR* exp, PPROC pProc, Int* numLoc)
/* Walks the conditional expression tree and returns the result on a string */
{ int16 off;                /* temporal - for OTHER */
  ID* id;                   /* Pointer to local identifier table */
  char* o;                  /* Operand string pointer */
  boolT needBracket;        /* Determine whether parenthesis is needed */
  BWGLB_TYPE* bwGlb;     	/* Ptr to BWGLB_TYPE (global indexed var) */
  PSTKSYM psym;				/* Pointer to argument in the stack */
  char* condExp, *e;		/* Return and intermediate expressions */

	condExp = (char*) allocMem (EXP_SIZE * sizeof(char));
	condExp[0] = '\0';

    if (exp == NULL)
        return (condExp);

    needBracket = TRUE;
    switch (exp->type)
    {
      case BOOLEAN_OP:	strcat (condExp, "(");
                        e = walkCondExpr(exp->expr.boolExpr.lhs, pProc, numLoc);
						strcat (condExp, e);
                        strcat (condExp, condOpSym[exp->expr.boolExpr.op]);
                        e = walkCondExpr(exp->expr.boolExpr.rhs, pProc, numLoc);
						strcat (condExp, e);
                        strcat (condExp, ")");
                        break;

      case NEGATION:    if (exp->expr.unaryExp->type == IDENTIFIER)
                        {
                            needBracket = FALSE;
                            strcat (condExp, "!");
                        }
                        else
                            strcat (condExp, "! (");
                        e = walkCondExpr (exp->expr.unaryExp, pProc, numLoc);
						strcat (condExp, e);
                        if (needBracket == TRUE)
                            strcat (condExp, ")");
                        break;

      case ADDRESSOF:   if (exp->expr.unaryExp->type == IDENTIFIER)
                        {
                            needBracket = FALSE;
                            strcat (condExp, "&");
                        }
                        else
                            strcat (condExp, "&(");
                        e = walkCondExpr (exp->expr.unaryExp, pProc, numLoc);
						strcat (condExp, e);
                        if (needBracket == TRUE)
                            strcat (condExp, ")");
                        break;

      case DEREFERENCE: if (exp->expr.unaryExp->type == IDENTIFIER)
                        {
                            needBracket = FALSE;
                            strcat (condExp, "*");
                        }
                        else
                            strcat (condExp, "*(");
                        e = walkCondExpr (exp->expr.unaryExp, pProc, numLoc);
						strcat (condExp, e);
                        if (needBracket == TRUE)
                            strcat (condExp, ")");
                        break;

	  case POST_INC:	e = walkCondExpr (exp->expr.unaryExp, pProc, numLoc);
						strcat (condExp, e);
						strcat (condExp, "++");
						break;

	  case POST_DEC:	e = walkCondExpr (exp->expr.unaryExp, pProc, numLoc);
						strcat (condExp, e);
						strcat (condExp, "--");
						break;

      case PRE_INC:     strcat (condExp, "++");
                        e = walkCondExpr (exp->expr.unaryExp, pProc, numLoc);
						strcat (condExp, e);
                        break;

      case PRE_DEC:     strcat (condExp, "--");
                        e = walkCondExpr (exp->expr.unaryExp, pProc, numLoc);
						strcat (condExp, e);
                        break;

      case IDENTIFIER:
      					o = (char*) allocMem (operandSize);
                        switch (exp->expr.ident.idType) {
                          case GLOB_VAR:    sprintf (o, "%s",
                            symtab.sym[exp->expr.ident.idNode.globIdx].name);
                                            break;
                          case REGISTER:    
							id = &pProc->localId.id[exp->expr.ident.idNode.regiIdx];
							if (id->name[0] == '\0')	/* no name */
							{
								sprintf (id->name, "loc%ld", ++(*numLoc));
								if (id->id.regi < rAL)
									appendStrTab (&cCode.decl, 
											  "%s %s; /* %s */\n",
											  hlTypes[id->type], id->name,
                                        	  wordReg[id->id.regi - rAX]);
								else
									appendStrTab (&cCode.decl, 
											  "%s %s; /* %s */\n",
											  hlTypes[id->type], id->name,
                                        	  byteReg[id->id.regi - rAL]);
							}
                            if (id->hasMacro)
								sprintf (o, "%s(%s)", id->macro, id->name);
							else
								sprintf (o, "%s", id->name);
                            break;

                          case LOCAL_VAR:   
                            sprintf (o, "%s",
                                     pProc->localId.id[exp->expr.ident.idNode.localIdx].name);
                            break;

                          case PARAM:
                            psym = &pProc->args.sym[exp->expr.ident.idNode.paramIdx];
							if (psym->hasMacro)
								sprintf (o, "%s(%s)", psym->macro, psym->name);
							else
								sprintf (o, "%s", psym->name);
                            break;

                          case GLOB_VAR_IDX:
                            bwGlb = &pProc->localId.id[exp->expr.ident.idNode.idxGlbIdx].id.bwGlb;
                          
                            sprintf (o, "%d[%s]", (bwGlb->seg << 4) +
                                     bwGlb->off, wordReg[bwGlb->regi - rAX]);
                            break;

                          case CONSTANT:       
                            if (exp->expr.ident.idNode.kte.kte < 1000)
                               sprintf (o,"%d",exp->expr.ident.idNode.kte.kte);
                            else
                               sprintf(o,"0x%X",exp->expr.ident.idNode.kte.kte);
                            break;

						  case STRING:
							o = getString (exp->expr.ident.idNode.strIdx);
							break;

                          case LONG_VAR:    
                            id = &pProc->localId.id[exp->expr.ident.idNode.longIdx];
							if (id->name[0] != '\0') /* STK_FRAME & REG w/name*/
                                sprintf (o, "%s", id->name);
                            else if (id->loc == REG_FRAME)
							{
								sprintf (id->name, "loc%ld", ++(*numLoc));
								appendStrTab (&cCode.decl, 
											  "%s %s; /* %s:%s */\n",
											  hlTypes[id->type], id->name,
                                        	  wordReg[id->id.longId.h - rAX],
											  wordReg[id->id.longId.l - rAX]);
								sprintf (o, "%s", id->name);
								propLongId (&pProc->localId, id->id.longId.l,
											id->id.longId.h, id->name); 
							}
                            else    /* GLB_FRAME */
                            {
                                if (id->id.longGlb.regi == 0)  /* not indexed */
                                    sprintf (o, "[%ld]", (id->id.longGlb.seg<<4)
                                             + id->id.longGlb.offH);
                                else if (id->id.longGlb.regi == rBX)
                                    sprintf (o, "[%ld][bx]", 
                                             (id->id.longGlb.seg<<4) + 
                                             id->id.longGlb.offH);
                            }
                            break;

                          case FUNCTION:
                            o = writeCall (exp->expr.ident.idNode.call.proc,
										   exp->expr.ident.idNode.call.args, 
										   pProc, numLoc);
                            break;

                          case OTHER:       
                            off = exp->expr.ident.idNode.other.off; 
                            if (off == 0)   
                                sprintf (o, "%s[%s]", 
                                wordReg[exp->expr.ident.idNode.other.seg - rAX],
                                idxReg[exp->expr.ident.idNode.other.regi - INDEXBASE]);
                            else if (off < 0)
                                sprintf (o, "%s[%s-%s]",
                                wordReg[exp->expr.ident.idNode.other.seg - rAX],
                                idxReg[exp->expr.ident.idNode.other.regi - INDEXBASE],
                                hexStr (-off));
                            else
                                sprintf (o, "%s[%s+%s]",
                                wordReg[exp->expr.ident.idNode.other.seg - rAX],
                                idxReg[exp->expr.ident.idNode.other.regi - INDEXBASE],
                                hexStr (off));
                        } /* eos */
                        strcat (condExp, o);
                        break;
    }

	return (condExp);
}


COND_EXPR *copyCondExp (COND_EXPR *exp)
/* Makes a copy of the given expression.  Allocates newExp storage for each
 * node.  Returns the copy. */
{
	COND_EXPR* newExp;        /* Expression node copy */

    switch (exp->type) {
      case BOOLEAN_OP:     
			newExp = allocStruc(COND_EXPR);
			memcpy(newExp, exp, sizeof(COND_EXPR));
            newExp->expr.boolExpr.lhs = copyCondExp (exp->expr.boolExpr.lhs);
            newExp->expr.boolExpr.rhs = copyCondExp (exp->expr.boolExpr.rhs);
            break;

      case NEGATION:    
      case ADDRESSOF:
      case DEREFERENCE:
			newExp = allocStruc(COND_EXPR);
			memcpy(newExp, exp, sizeof(COND_EXPR));
            newExp->expr.unaryExp = copyCondExp (exp->expr.unaryExp);
            break;

      case IDENTIFIER:  
			newExp = allocStruc(COND_EXPR);
			memcpy(newExp, exp, sizeof(COND_EXPR));
    }
    return (newExp);
}


void changeBoolCondExpOp (COND_EXPR *exp, condOp newOp)
/* Changes the boolean conditional operator at the root of this expression */
{
    exp->expr.boolExpr.op = newOp;
}


boolT insertSubTreeReg (COND_EXPR *exp, COND_EXPR **tree, byte regi,
                        LOCAL_ID *locsym)
/* Inserts the expression exp into the tree at the location specified by the
 * register regi */
{ byte treeReg;

    if (*tree == NULL)
        return FALSE;

    switch ((*tree)->type) {
      case IDENTIFIER:  
        if ((*tree)->expr.ident.idType == REGISTER)
		{
            treeReg = locsym->id[(*tree)->expr.ident.idNode.regiIdx].id.regi;
            if (treeReg == regi)                        /* word reg */
            {
                *tree = exp;
                return TRUE;
            }
            else if ((regi >= rAX) && (regi <= rBX))    /* word/byte reg */
            {
                if ((treeReg == (regi + rAL-1)) || (treeReg == (regi + rAH-1)))
                {
                    *tree = exp;
                    return TRUE;
                }
            }
		}
        return FALSE;

      case BOOLEAN_OP:     
            if (insertSubTreeReg (exp, &(*tree)->expr.boolExpr.lhs, regi,
                                  locsym))
                return TRUE;
            if (insertSubTreeReg (exp, &(*tree)->expr.boolExpr.rhs, regi,
                                  locsym))
                return TRUE;
            return FALSE;

      case NEGATION:
      case ADDRESSOF:
      case DEREFERENCE: 
            if (insertSubTreeReg(exp, &(*tree)->expr.unaryExp,regi, locsym))
                return TRUE;
            return FALSE;
    }
	return FALSE;
}


boolT insertSubTreeLongReg (COND_EXPR *exp, COND_EXPR **tree, Int longIdx)
/* Inserts the expression exp into the tree at the location specified by the
 * long register index longIdx*/
{
    switch ((*tree)->type) {
      case IDENTIFIER:  if ((*tree)->expr.ident.idNode.longIdx == longIdx)
                        {
                            *tree = exp;
                            return TRUE;
                        }
                        return FALSE;
                        
      case BOOLEAN_OP:	if (insertSubTreeLongReg (exp, 
                                    &(*tree)->expr.boolExpr.lhs, longIdx))
                            return TRUE;
                        if (insertSubTreeLongReg (exp, 
                                    &(*tree)->expr.boolExpr.rhs, longIdx))
                            return TRUE;
                        return FALSE;
                        
      case NEGATION:
      case ADDRESSOF:
      case DEREFERENCE: if (insertSubTreeLongReg (exp, 
                                        &(*tree)->expr.unaryExp, longIdx))
                            return TRUE;
                        return FALSE;
    }
	return FALSE;
}
                        

void freeCondExpr (COND_EXPR *exp)
/* Recursively deallocates the abstract syntax tree rooted at *exp */
{
    switch (exp->type) {
      case BOOLEAN_OP:     freeCondExpr (exp->expr.boolExpr.lhs);
                        freeCondExpr (exp->expr.boolExpr.rhs);
                        break;
      case NEGATION:
      case ADDRESSOF:
      case DEREFERENCE: freeCondExpr (exp->expr.unaryExp);
                        break;
    }
    free (exp);
}


/***************************************************************************
 * Expression stack functions
 **************************************************************************/

void initExpStk()
/* Reinitalizes the expression stack (expStk) to NULL, by freeing all the
 * space allocated (if any).        */
{ EXP_STK *top;

    while (expStk != NULL)
    {
        top = expStk;
        expStk = expStk->next;
        free (top);
    }
}


void pushExpStk (COND_EXPR *exp)
/* Pushes the given expression onto the local stack (expStk). */
{ EXP_STK *newExp;

    newExp = allocStruc(EXP_STK);
    newExp->exp = exp;
    newExp->next = expStk;
    expStk = newExp;
}


COND_EXPR *popExpStk()
/* Returns the element on the top of the local expression stack (expStk),
 * and deallocates the space allocated by this node.
 * If there are no elements on the stack, returns NULL. */
{ EXP_STK *top;
  COND_EXPR *topExp;

    if (expStk != NULL)
    {
        topExp = expStk->exp;
        top = expStk;
        expStk = expStk->next;
        free (top);
        return (topExp);
    }
    else
        return (NULL);
}

Int numElemExpStk()
/* Returns the number of elements available in the expression stack */
{ EXP_STK *top;
  Int num;

	num = 0; 
	top = expStk;
	while (top)
	{
		top = top->next;
		num++;
	}
	return (num);
}

boolT emptyExpStk()
/* Returns whether the expression stack is empty or not */
{
	if (expStk == NULL)
		return TRUE;
	return FALSE;
}
