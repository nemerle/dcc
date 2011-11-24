#pragma once
#include "ast.h"
#include "types.h"
struct IDENTTYPE
{
    condId           idType;
    regType          regiType;  /* for REGISTER only                */
    union _idNode {
        Int          regiIdx;   /* index into localId, REGISTER		*/
        Int          globIdx;   /* index into symtab for GLOB_VAR   */
        Int          localIdx;  /* idx into localId,  LOCAL_VAR		*/
        Int          paramIdx;  /* idx into args symtab, PARAMS     */
        Int			 idxGlbIdx;	/* idx into localId, GLOB_VAR_IDX   */
        struct _kte
        {			/* for CONSTANT only					*/
            dword   kte;   	/*   value of the constant			*/
            byte    size;       /*   #bytes size constant	 		*/
        } kte;
        dword		 strIdx;	/* idx into image, for STRING	 	*/
        Int			 longIdx;	/* idx into LOCAL_ID table, LONG_VAR*/
        struct _call {			/* for FUNCTION only				*/
            Function     *proc;
            STKFRAME *args;
        }			 call;
        struct {                /* for OTHER; tmp struct            */
            byte     seg;       /*   segment                        */
            byte     regi;      /*   index mode                     */
            int16    off;       /*   offset                         */
        }            other;
    }                idNode;
};
