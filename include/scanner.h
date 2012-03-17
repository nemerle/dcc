#pragma once
/* Scanner functions
 * (C) Cristina Cifuentes, Jeff Ledermann
 */
#include <stdint.h>
#include "error.h"
/* Extracts reg bits from middle of mod-reg-rm uint8_t */
#define REG(x)  ((uint8_t)(x & 0x38) >> 3)

struct ICODE;

extern eErrorId scan(uint32_t ip, ICODE &p);
