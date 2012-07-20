#include <stdio.h>
#include <stdlib.h>
#include <cassert>

#include "libdis.h"

#ifdef _MSC_VER
#define snprintf        _snprintf
#define inline          __inline
#endif

int x86_insn_is_valid( x86_insn_t *insn ) {
    if ( insn && insn->type != insn_invalid && insn->size > 0 ) {
        return 1;
    }

    return 0;
}

bool x86_insn_t::is_valid( )
{
    if ( this && this->type != insn_invalid && this->size > 0 )
    {
        return 1;
    }

    return 0;
}
/* Get Address: return the value of an offset operand, or the offset of
 * a segment:offset absolute address */

uint32_t x86_insn_t::x86_get_address()
{
    x86_oplist_t *op_lst;
    assert(this);
    if (! operands )
    {
        return 0;
    }

    for (op_lst = operands; op_lst; op_lst = op_lst->next ) {
        if ( op_lst->op.type == op_offset ) {
            return op_lst->op.data.offset;
        } else if ( op_lst->op.type == op_absolute ) {
            if ( op_lst->op.datatype == op_descr16 ) {
                return (uint32_t)
                        op_lst->op.data.absolute.offset.off16;
            }
            return op_lst->op.data.absolute.offset.off32;
        }
    }

    return 0;
}

/** Get Relative Offset: return as a sign-extended int32_t the near or far
 * relative offset operand, or 0 if there is none. There can be only one
 * relaive offset operand in an instruction. */
int32_t x86_insn_t::x86_get_rel_offset( ) {
    x86_oplist_t *op_lst;
    assert(this);
    if (! operands ) {
        return 0;
    }

    for (op_lst = operands; op_lst; op_lst = op_lst->next ) {
        if ( op_lst->op.type == op_relative_near ) {
            return (int32_t) op_lst->op.data.relative_near;
        } else if ( op_lst->op.type == op_relative_far ) {
            return op_lst->op.data.relative_far;
        }
    }

    return 0;
}

/** Get Branch Target: return the x86_op_t containing the target of
 * a jump or call operand, or NULL if there is no branch target.
 * Internally, a 'branch target' is defined as any operand with
 * Execute Access set. There can be only one branch target per instruction. */
x86_op_t * x86_insn_t::x86_get_branch_target() {
    x86_oplist_t *op_lst;
    assert(this);
    if (! operands ) {
        return NULL;
    }

    for (op_lst = operands; op_lst; op_lst = op_lst->next ) {
        if ( op_lst->op.access & op_execute ) {
            return &(op_lst->op);
        }
    }

    return NULL;
}
x86_op_t * x86_insn_t::get_dest() {
    x86_oplist_t *op_lst;
    assert(this);
    if ( ! operands ) {
        return NULL;
    }
    for (op_lst = operands; op_lst; op_lst = op_lst->next ) {
        if ( op_lst->op.access & op_write)
            return &(op_lst->op);
    }

    return NULL;
}

/** \brief Get Immediate: return the x86_op_t containing the immediate operand
  for this instruction, or NULL if there is no immediate operand. There
  can be only one immediate operand per instruction */
x86_op_t * x86_insn_t::x86_get_imm() {
    x86_oplist_t *op_lst;
    assert(this);
    if ( ! operands ) {
        return NULL;
    }

    for (op_lst = operands; op_lst; op_lst = op_lst->next ) {
        if ( op_lst->op.type == op_immediate ) {
            return &(op_lst->op);
        }
    }

    return NULL;
}

#define IS_PROPER_IMM( x ) \
    x->op.type == op_immediate && ! (x->op.flags.op_hardcode)


/** \brief if there is an immediate value in the instruction, return a pointer to it
 * Get Raw Immediate Data: returns a pointer to the immediate data encoded
 * in the instruction. This is useful for large data types [>32 bits] currently
 * not supported by libdisasm, or for determining if the disassembler
 * screwed up the conversion of the immediate data. Note that 'imm' in this
 * context refers to immediate data encoded at the end of an instruction as
 * detailed in the Intel Manual Vol II Chapter 2; it does not refer to the
 * 'op_imm' operand (the third operand in instructions like 'mul' */
uint8_t *x86_insn_t::x86_get_raw_imm() {
    int size, offset;
    x86_op_t *op  = NULL;
    assert(this);
    if (! operands ) {
        return(NULL);
    }

    /* a bit inelegant, but oh well... */
    if ( IS_PROPER_IMM( operands ) ) {
        op = &operands->op;
    } else if ( operands->next ) {
        if ( IS_PROPER_IMM( operands->next ) ) {
            op = &operands->next->op;
        } else if ( operands->next->next &&
                    IS_PROPER_IMM( operands->next->next ) ) {
            op = &operands->next->next->op;
        }
    }

    if (! op ) {
        return( NULL );
    }

    /* immediate data is at the end of the insn */
    size = op->operand_size();
    offset = size - size;
    return( &bytes[offset] );
}


size_t x86_op_t::operand_size() {
    switch (datatype ) {
        case op_byte:    return 1;
        case op_word:    return 2;
        case op_dword:   return 4;
        case op_qword:   return 8;
        case op_dqword:  return 16;
        case op_sreal:   return 4;
        case op_dreal:   return 8;
        case op_extreal: return 10;
        case op_bcd:     return 10;
        case op_ssimd:   return 16;
        case op_dsimd:   return 16;
        case op_sssimd:  return 4;
        case op_sdsimd:  return 8;
        case op_descr32: return 6;
        case op_descr16: return 4;
        case op_pdescr32: return 6;
        case op_pdescr16: return 6;
        case op_bounds16: return 4;
        case op_bounds32: return 8;
        case op_fpuenv16:  return 14;
        case op_fpuenv32:  return 28;
        case op_fpustate16:  return 94;
        case op_fpustate32:  return 108;
        case op_fpregset: return 512;
        case op_fpreg: return 10;
        case op_none: return 0;
    }
    return(4);      /* default size */
}

void x86_insn_t::x86_set_insn_addr( uint32_t _addr ) {
    addr = _addr;
}

void x86_insn_t::x86_set_insn_offset( unsigned int _offset ){
    offset = _offset;
}

void x86_insn_t::x86_set_insn_function( void * func ){
    function = func;
}

void x86_insn_t::x86_set_insn_block( void * _block ){
    block = _block;
}

void x86_insn_t::x86_tag_insn(){
    tag = 1;
}

void x86_insn_t::x86_untag_insn(){
    tag = 0;
}

int x86_insn_t::x86_insn_is_tagged(){
    return tag;
}

