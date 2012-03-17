SET(disasm_LIB_SRCS
   ia32_implicit.cpp
   ia32_implicit.h
   ia32_insn.cpp
   ia32_insn.h
   ia32_invariant.cpp
   ia32_invariant.h
   ia32_modrm.cpp
   ia32_modrm.h
   ia32_opcode_tables.cpp
   ia32_opcode_tables.h
   ia32_operand.cpp
   ia32_operand.h
   ia32_reg.cpp
   ia32_reg.h
   ia32_settings.cpp
   ia32_settings.h
   libdis.h
   qword.h
   x86_disasm.cpp
   x86_format.cpp
   x86_imm.cpp
   x86_imm.h
   x86_insn.cpp
   x86_misc.cpp
   x86_operand_list.cpp
   x86_operand_list.h
)

add_library(disasm SHARED ${disasm_LIB_SRCS})
add_library(disasm_s STATIC ${disasm_LIB_SRCS})
install(TARGETS disasm DESTINATION lib)
install(FILES  libdis.h DESTINATION /include)
