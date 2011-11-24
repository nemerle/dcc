/* Scanner functions
 * (C) Cristina Cifuentes, Jeff Ledermann
 */

#define LH(p)  ((int)((byte *)(p))[0] + ((int)((byte *)(p))[1] << 8))

static void rm(Int i);
static void modrm(Int i);
static void segrm(Int i);
static void data1(Int i);
static void data2(Int i);
static void regop(Int i);
static void segop(Int i);
static void strop(Int i);
static void escop(Int i);
static void axImp(Int i);
static void alImp(Int i);
static void axSrcIm(Int i);
static void memImp(Int i);
static void memReg0(Int i);
static void memOnly(Int i);
static void dispM(Int i);
static void dispS(Int i);
static void dispN(Int i);
static void dispF(Int i);
static void prefix(Int i);
static void immed(Int i);
static void shift(Int i);
static void arith(Int i);
static void trans(Int i);
static void const1(Int i);
static void const3(Int i);
static void none1(Int i);
static void none2(Int i);
static void checkInt(Int i);

/* Extracts reg bits from middle of mod-reg-rm byte */
#define REG(x)  ((byte)(x & 0x38) >> 3)
