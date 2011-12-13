/****************************************************************************
 *          dcc project header
 * (C) Cristina Cifuentes, Mike van Emmerik
 ****************************************************************************/

/* STATE TABLE */
#include <cstring>
struct STATE
{
    dword       IP;             /* Offset into Image                    */
    int16       r[INDEXBASE];   /* Value of segs and AX                 */
    byte        f[INDEXBASE];   /* True if r[.] has a value             */
    struct
    {                           /* For case stmt indexed reg            */
        byte    regi;           /*   Last conditional jump              */
        int16   immed;          /*   Contents of the previous register  */
    }           JCond;
    void setState(word reg, int16 value);
public:
    void checkStartup();
    STATE() : IP(0)
    {
        JCond.immed=0;
        memset(r,0,sizeof(int16)*INDEXBASE);
        memset(f,0,sizeof(byte)*INDEXBASE);
    }
};


