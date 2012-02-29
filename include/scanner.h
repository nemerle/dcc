/* Scanner functions
 * (C) Cristina Cifuentes, Jeff Ledermann
 */

//#define LH(p)  ((int)((uint8_t *)(p))[0] + ((int)((uint8_t *)(p))[1] << 8))
/* Extracts reg bits from middle of mod-reg-rm uint8_t */
#define REG(x)  ((uint8_t)(x & 0x38) >> 3)
