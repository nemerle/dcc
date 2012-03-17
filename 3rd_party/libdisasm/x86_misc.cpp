#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "libdis.h"
#include "ia32_insn.h"
#include "ia32_reg.h"		/* for ia32_reg wrapper */
#include "ia32_settings.h"
extern ia32_settings_t ia32_settings;

#ifdef _MSC_VER
        #define snprintf        _snprintf
        #define inline          __inline
#endif


/* =========================================================== INIT/TERM */

int X86_Disasm::x86_init( enum x86_options options, DISASM_REPORTER reporter, void * arg )
{
        ia32_settings.options = options;
        __x86_reporter_func = reporter;
	__x86_reporter_arg = arg;

        return 1;
}

void X86_Disasm::x86_set_reporter( DISASM_REPORTER reporter, void * arg ) {
        __x86_reporter_func = reporter;
	__x86_reporter_arg = arg;
}

void X86_Disasm::x86_set_options( enum x86_options options ){
        ia32_settings.options = options;
}

enum x86_options X86_Disasm::x86_get_options( void ) {
        return ia32_settings.options;
}

int X86_Disasm::x86_cleanup( void )
{
        return 1;
}

/* =========================================================== ERRORS */
void X86_Disasm::x86_report_error( enum x86_report_codes code, void *data ) {
        if ( __x86_reporter_func ) {
                (*__x86_reporter_func)(code, data, __x86_reporter_arg);
        }
}


/* =========================================================== MISC */
unsigned int X86_Disasm::x86_endian(void)       { return ia32_settings.endian;  }
unsigned int X86_Disasm::x86_addr_size(void)    { return ia32_settings.sz_addr; }
unsigned int X86_Disasm::x86_op_size(void)      { return ia32_settings.sz_oper; }
unsigned int X86_Disasm::x86_word_size(void)    { return ia32_settings.sz_word; }
unsigned int X86_Disasm::x86_max_insn_size(void){ return ia32_settings.max_insn; }
unsigned int X86_Disasm::x86_sp_reg(void)       { return ia32_settings.id_sp_reg;      }
unsigned int X86_Disasm::x86_fp_reg(void)       { return ia32_settings.id_fp_reg;      }
unsigned int X86_Disasm::x86_ip_reg(void)       { return ia32_settings.id_ip_reg;      }
unsigned int X86_Disasm::x86_flag_reg(void)     { return ia32_settings.id_flag_reg;  }

/* wrapper function to hide the IA32 register fn */
void x86_reg_t::x86_reg_from_id( unsigned int _id) {
	ia32_handle_register( this, _id );
	return;
}
