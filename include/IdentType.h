#pragma once
#include "ast.h"
#include "types.h"
#include "machine_x86.h"
struct IDENTTYPE
{
    condId           idType;
    regType          regiType;  /* for REGISTER only                */
    union _idNode {
        int          regiIdx;   /* index into localId, REGISTER		*/
        int          globIdx;   /* index into symtab for GLOB_VAR   */
        int          localIdx;  /* idx into localId,  LOCAL_VAR		*/
        int          paramIdx;  /* idx into args symtab, PARAMS     */
        int			 idxGlbIdx;	/* idx into localId, GLOB_VAR_IDX   */
        struct _kte
        {			/* for CONSTANT only					*/
            uint32_t   kte;   	/*   value of the constant			*/
            uint8_t    size;       /*   #bytes size constant	 		*/
        } kte;
        uint32_t        strIdx;	/* idx into image, for STRING	 	*/
        int             longIdx;	/* idx into LOCAL_ID table, LONG_VAR*/
        struct _call {			/* for FUNCTION only				*/
            Function     *proc;
            STKFRAME *args;
        }			 call;
        struct {                /* for OTHER; tmp struct            */
            eReg     seg;       /*   segment                        */
            eReg     regi;      /*   index mode                     */
            int16_t    off;       /*   offset                         */
        }            other;
    }                idNode;
};
