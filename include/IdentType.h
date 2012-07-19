#pragma once
#include "ast.h"
#include "types.h"
#include "machine_x86.h"
struct GlobalVariable;
struct AstIdent;
struct IDENTTYPE
{
    friend struct GlobalVariable;
    friend struct Constant;
    friend struct AstIdent;
protected:
    condId           idType;
public:
    condId           type() {return idType;}
    void             type(condId t) {idType=t;}
    union _idNode {
        int          localIdx;  /* idx into localId,  LOCAL_VAR		*/
        int          paramIdx;  /* idx into args symtab, PARAMS     */
        uint32_t        strIdx;	/* idx into image, for STRING	 	*/
        int             longIdx;	/* idx into LOCAL_ID table, LONG_VAR*/
        struct {                /* for OTHER; tmp struct            */
            eReg     seg;       /*   segment                        */
            eReg     regi;      /*   index mode                     */
            int16_t    off;       /*   offset                         */
        }            other;
    }                idNode;
    IDENTTYPE() : idType(UNDEF)
    {}
};
