#pragma once
/* Scanner functions
 * (C) Cristina Cifuentes, Jeff Ledermann
 */
#include <stdint.h>
#include "error.h"

#define REG(x)  ((uint8_t)(x & 0x38) >> 3)
//#define LH(p)  ((int)((uint8_t *)(p))[0] + ((int)((uint8_t *)(p))[1] << 8))
struct ICODE;
/* Extracts reg bits from middle of mod-reg-rm uint8_t */
extern eErrorId scan(uint32_t ip, ICODE &p);
