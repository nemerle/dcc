/* Scanner functions
 * (C) Cristina Cifuentes, Jeff Ledermann
 */

#define LH(p)  ((int)((byte *)(p))[0] + ((int)((byte *)(p))[1] << 8))
/* Extracts reg bits from middle of mod-reg-rm byte */
#define REG(x)  ((byte)(x & 0x38) >> 3)
