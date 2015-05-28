/****************************************************************************
 *          dcc project header
 * (C) Cristina Cifuentes, Mike van Emmerik
 ****************************************************************************/

/* STATE TABLE */
typedef struct
{
    dword       IP;             /* Offset into Image                    */
    int16       r[INDEXBASE];   /* Value of segs and AX                 */
    byte        f[INDEXBASE];   /* True if r[.] has a value             */
    struct
	{							/* For case stmt indexed reg            */
        byte    regi;           /*   Last conditional jump              */
        int16   immed;          /*   Contents of the previous register  */
    }           JCond;
} STATE;
typedef STATE *PSTATE;

