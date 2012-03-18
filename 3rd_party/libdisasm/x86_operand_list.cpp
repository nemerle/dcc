#include <stdlib.h>
#include <cassert>
#include "libdis.h"


void x86_insn_t::x86_oplist_append( x86_oplist_t *op ) {
    x86_oplist_t *list;
    assert(this);

    list = operands;
    if (! list ) {
        operand_count = 1;
        /* Note that we have no way of knowing if this is an
                 * exlicit operand or not, since the caller fills
                 * the x86_op_t after we return. We increase the
                 * explicit count automatically, and ia32_insn_implicit_ops
                 * decrements it */
        explicit_count = 1;
        operands = op;
        return;
    }

    /* get to end of list */
    for ( ; list->next; list = list->next )
        ;

    operand_count = operand_count + 1;
    explicit_count = explicit_count + 1;
    list->next = op;

    return;
}

bool x86_insn_t::containsFlag(x86_eflags tofind, x86_flag_status in)
{
    switch(tofind)
    {
        case insn_eflag_carry:
            return (in & (insn_carry_set | insn_carry_or_zero_set | insn_carry_clear))!=0;
        case insn_eflag_zero:
            return (in & (insn_zero_set | insn_carry_or_zero_set |
                          insn_zero_set_or_sign_ne_oflow | insn_zero_clear))!=0;
        case insn_eflag_overflow:
            return (in & (insn_oflow_set | insn_zero_set_or_sign_ne_oflow |
                          insn_oflow_clear | insn_sign_eq_oflow |
                          insn_sign_ne_oflow))!=0;
        case insn_eflag_direction:
            return (in & (insn_dir_set | insn_dir_clear))!=0;
        case insn_eflag_sign:
            return (in & (insn_sign_set | insn_sign_clear | insn_zero_set_or_sign_ne_oflow |
                          insn_sign_eq_oflow | insn_sign_ne_oflow))!=0;
        case insn_eflag_parity:
            return (in & (insn_parity_set | insn_parity_clear))!=0;
    }
    return false;
}

x86_op_t * x86_insn_t::x86_operand_new( ) {
    x86_oplist_t *op;
    assert(this);
    op = (x86_oplist_t *)calloc( sizeof(x86_oplist_t), 1 );
    op->op.insn = this;
    x86_oplist_append( op );
    return( &(op->op) );
}

/** free the operand list associated with an instruction -- useful for
 * preventing memory leaks when free()ing an x86_insn_t */
void x86_insn_t::x86_oplist_free( )
{
    x86_oplist_t *op, *list;
    assert(this);
    for ( list = operands; list; ) {
        op = list;
        list = list->next;
        free(op);
    }

    operands = NULL;
    operand_count = 0;
    explicit_count = 0;

    return;
}

/* ================================================== LIBDISASM API */
/* these could probably just be #defines, but that means exposing the
   enum... yet one more confusing thing in the API */
int x86_insn_t::x86_operand_foreach( x86_operand_fn func, void *arg, enum x86_op_foreach_type type )
{
    x86_oplist_t *list;
    char _explicit = 1, implicit = 1;
    assert(this);
    if ( ! func ) {
        return 0;
    }

    /* note: explicit and implicit can be ORed together to
         * allow an "all" limited by access type, even though the
         * user is stupid to do this since it is default behavior :) */
    if ( (type & op_explicit) && ! (type & op_implicit) ) {
        implicit = 0;
    }
    if ( (type & op_implicit) && ! (type & op_explicit) ) {
        _explicit = 0;
    }

    type = (x86_op_foreach_type)((int)type & 0x0F); /* mask out explicit/implicit operands */

    for ( list = operands; list; list = list->next ) {
        if (! implicit && (list->op.flags.op_implied) ) {
            /* operand is implicit */
            continue;
        }

        if (! _explicit && ! (list->op.flags.op_implied) ) {
            /* operand is not implicit */
            continue;
        }

        switch ( type ) {
            case op_any:
                break;
            case op_dest:
                if (! (list->op.access & op_write) ) {
                    continue;
                }
                break;
            case op_src:
                if (! (list->op.access & op_read) ) {
                    continue;
                }
                break;
            case op_ro:
                if (! (list->op.access & op_read) ||
                        (list->op.access & op_write ) ) {
                    continue;
                }
                break;
            case op_wo:
                if (! (list->op.access & op_write) ||
                        (list->op.access & op_read ) ) {
                    continue;
                }
                break;
            case op_xo:
                if (! (list->op.access & op_execute) ) {
                    continue;
                }
                break;
            case op_rw:
                if (! (list->op.access & op_write) ||
                        ! (list->op.access & op_read ) ) {
                    continue;
                }
                break;
            case op_implicit: case op_explicit: /* make gcc happy */
                break;
        }
        /* any non-continue ends up here: invoke the callback */
        (*func)( &list->op, this, arg );
    }

    return 1;
}

static void count_operand( x86_op_t *op, x86_insn_t *insn, void *arg ) {
    size_t * count = (size_t *) arg;
    *count = *count + 1;
}

/** convenience routine: returns count of operands matching 'type' */
size_t x86_insn_t::x86_operand_count( enum x86_op_foreach_type type ) {
    size_t count = 0;

    /* save us a list traversal for common counts... */
    if ( type == op_any ) {
        return operand_count;
    } else if ( type == op_explicit ) {
        return explicit_count;
    }

    x86_operand_foreach( count_operand, &count, type );
    return count;
}

/* accessor functions */
x86_op_t * x86_insn_t::x86_operand_1st() {
    if (! explicit_count ) {
        return NULL;
    }

    return &(operands->op);
}

x86_op_t * x86_insn_t::x86_operand_2nd( ) {
    if ( explicit_count < 2 ) {
        return NULL;
    }

    return &(operands->next->op);
}

x86_op_t * x86_insn_t::x86_operand_3rd( ) {
    if ( explicit_count < 3 ) {
        return NULL;
    }
    return &(operands->next->next->op);
}
