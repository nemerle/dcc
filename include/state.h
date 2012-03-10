/****************************************************************************
 *          dcc project header
 * (C) Cristina Cifuentes, Mike van Emmerik
 ****************************************************************************/
#pragma once
#include <stdint.h>
#include <cstring>
#include "machine_x86.h"

/* STATE TABLE */
struct STATE
{
    uint32_t       IP;             /* Offset into Image                    */
    int16_t       r[INDEX_BX_SI];   /* Value of segs and AX                 */
    uint8_t        f[INDEX_BX_SI];   /* True if r[.] has a value             */
    struct
    {                           /* For case stmt indexed reg            */
        uint8_t    regi;           /*   Last conditional jump              */
        int16_t   immed;          /*   Contents of the previous register  */
    }           JCond;
    void setState(uint16_t reg, int16_t value);
    void checkStartup();
    STATE() : IP(0)
    {
        JCond.regi=0;
        JCond.immed=0;

        memset(r,0,sizeof(int16_t)*INDEX_BX_SI); //TODO: move this to machine_x86
        memset(f,0,sizeof(uint8_t)*INDEX_BX_SI);
    }
};


