/*
 *
 * (c) Copyright 1993 OPEN SOFTWARE FOUNDATION, INC.
 * (c) Copyright 1993 HEWLETT-PACKARD COMPANY
 * (c) Copyright 1993 DIGITAL EQUIPMENT CORPORATION
 * To anyone who acknowledges that this file is provided "AS IS"
 * without any express or implied warranty:
 *                 permission to use, copy, modify, and distribute this
 * file for any purpose is hereby granted without fee, provided that
 * the above copyright notices and this notice appears in all source
 * code copies, and that none of the names of Open Software
 * Foundation, Inc., Hewlett-Packard Company, or Digital Equipment
 * Corporation be used in advertising or publicity pertaining to
 * distribution of the software without specific, written prior
 * permission.  Neither Open Software Foundation, Inc., Hewlett-
 * Packard Company, nor Digital Equipment Corporation makes any
 * representations about the suitability of this software for any
 * purpose.
 *
 */
/*
**  NAME:
**
**      mtsbacke.c
**
**  FACILITY:
**
**      Interface Definition Language (IDL) Compiler
**
**  ABSTRACT:
**
**  Backend control for MTS compiler
**
*/

#include <nidl.h>
#include <ast.h>
#include <irep.h>       /* Intermediate Representation defs */
#include <command.h>
#include <ddbe.h>       /* Data Driven Backend defs */
#include <mtsbacke.h>
#include <hdgen.h>
#include <cstubmts.h>
#include <sstubmts.h>

/*
 * Global type instances and boolean
 */
boolean *BE_cmd_opt;
void    **BE_cmd_val;
AST_type_n_t *BE_ulong_int_p, *BE_ushort_int_p, *BE_pointer_p, *BE_function_p;
AST_type_n_t *BE_short_null_p, *BE_long_null_p, *BE_hyper_null_p;
boolean BE_space_opt, BE_bug_array_align, BE_bug_array_align2,
        BE_bug_boolean_def
                                                                ;

char* cstub_pref;
char* sstub_pref;

#ifdef DUMPERS
boolean BE_dump_debug, BE_dump_flat, BE_dump_mnode, BE_dump_mool,
        BE_dump_recs, BE_dump_sends, BE_dump_unode, BE_dump_uool,
        BE_zero_mem;
#endif

/*
 *  be_init
 *
 * Initialize the various backend globals
 */
static void be_init
(
    boolean *cmd_opt,
    void **cmd_val
)
{
    boolean *bugs;

    /*
     * initialize various options-related globals
     */
    BE_cmd_opt = cmd_opt;
    BE_cmd_val = cmd_val;
    BE_space_opt = cmd_opt[opt_space_opt];
#ifdef DUMPERS
    BE_dump_debug = cmd_opt[opt_dump_debug];
    BE_dump_flat  = cmd_opt[opt_dump_flat];
    BE_dump_mnode = cmd_opt[opt_dump_mnode];
    BE_dump_mool  = cmd_opt[opt_dump_mool];
    BE_dump_recs  = cmd_opt[opt_dump_recs];
    BE_dump_sends = cmd_opt[opt_dump_sends];
    BE_dump_unode = cmd_opt[opt_dump_unode];
    BE_dump_uool  = cmd_opt[opt_dump_uool];
    BE_zero_mem   = (getenv("IDL_zero_mem") != NULL);
#endif

    /*
     *  Backwards compatibility marshalling
     */
    bugs = (boolean *)cmd_val[opt_do_bug];
    BE_bug_array_align = bugs[bug_array_align];
    BE_bug_array_align2 = bugs[bug_array_align2];
    BE_bug_boolean_def = bugs[bug_boolean_def];


    /*
     * initialize global type instances
     */
    BE_ulong_int_p = BE_get_type_node (AST_long_unsigned_k);
    BE_ulong_int_p->alignment_size = 4;
    BE_ulong_int_p->ndr_size = 4;

    BE_ushort_int_p = BE_get_type_node (AST_short_unsigned_k);
    BE_ushort_int_p->alignment_size = 2;
    BE_ushort_int_p->ndr_size = 2;

    BE_pointer_p = BE_get_type_node (AST_pointer_k);
    BE_pointer_p->alignment_size = 4;
    BE_pointer_p->ndr_size = 4;

    BE_function_p = BE_get_type_node (AST_function_k);
    BE_function_p->alignment_size = 0;
    BE_function_p->ndr_size = 0;

    BE_short_null_p = BE_get_type_node (AST_null_k);
    BE_short_null_p->alignment_size = 2;
    BE_short_null_p->ndr_size = 0;

    BE_long_null_p = BE_get_type_node (AST_null_k);
    BE_long_null_p->alignment_size = 4;
    BE_long_null_p->ndr_size = 0;

    BE_hyper_null_p = BE_get_type_node (AST_null_k);
    BE_hyper_null_p->alignment_size = 8;
    BE_hyper_null_p->ndr_size = 0;

    cstub_pref = cmd_val[opt_cstub_pref];
    sstub_pref = cmd_val[opt_sstub_pref];
}



/******************************************************************************/
/* BE Temporary-Memory Management                                             */
/******************************************************************************/
/* ABSTRACT:                                                                  */
/*   Special BE memory management routines.  The following three routines     */
/*   provides memory management in contexts (heap zones).  When entering a    */
/*   context the BE_push_malloc_ctx routine is called, upon entering a        */
/*   context all calls to BE_ctx_malloc to allocate memory will be associated */
/*   with the context.  When the context is exited by calling                 */
/*   BE_pop_malloc_ctx, all memory allocated with BE_ctx_malloc, is freed.    */
/*                                                                            */
/* NOTE:                                                                      */
/*   Memory allocated via BE_ctx_malloc, cannot be freed other than by        */
/*   exiting the current malloc context as it adds a header to the memory     */
/*   in order to keep a list of active allocations.  Calls to free() with     */
/*   allocations returned from BE_ctx_malloc, will cause heap corruption.     */
/*                                                                            */
/* ROUTINES:                                                                  */
/*   BE_cxt_malloc  -- Same interface as MALLOC, if not within a context      */
/*                     returns memory directly from malloc()                  */
/*   BE_push_malloc_ctx -- Start new temporary context                        */
/*                         is used directly and the memory never freed        */
/*   BE_push_perm_malloc_ctx -- Start new temporary context in which MALLOC   */
/*   BE_pop_malloc_ctx -- Free all memory allocated since start of context    */
/*                                                                            */
/******************************************************************************/

/*
 * Type used to add our context header to allocations returned from MALLOC
 */
typedef struct malloc_t {
      struct malloc_t *next;    /* Pointer to next allocation on this chain */
      void *data;               /* Start of memory returned to caller */
      } malloc_t;

/*
 * Type used to maintain a list of allocation contexts.
 */
typedef struct malloc_ctx_t {
      struct malloc_ctx_t *next;    /* Pointer to next context */
      malloc_t *list;               /* Head of allocation chain for this context. */
      boolean permanent;            /* If true, this is a permanent context */
      } malloc_ctx_t;

/*
 * Current malloc context, initially NULL so normal MALLOC is used
 */
static malloc_ctx_t *malloc_ctx = NULL;

/*
** BE_ctx_malloc: allocate memory in the current context.
*/
heap_mem *BE_ctx_malloc
(
    size_t size
)
{
      malloc_t *new;

      /* If no malloc context, just return memory */
      if (malloc_ctx == NULL)
	  return MALLOC (size);

      /* If current malloc context is permanent, then just return memory */
      if (malloc_ctx->permanent == true)
	  return MALLOC (size);

      /* Allocate memory with our context header */
      new = MALLOC (size + sizeof(malloc_t));

      /* Link the new allocation on the current context list */
      new->next = malloc_ctx->list;
      malloc_ctx->list = new;

#ifdef DUMPERS
      /* If BE_zero_mem set, initialize allocated memory to help find bugs */
      if (BE_zero_mem)
	  memset(&new->data, 0xFF, size);
#endif

      /* Return the value after our header for use by the caller */
      return &new->data;
}

/*
** BE_push_malloc_ctx: Push a new context in which memory is allocated
*/
void BE_push_malloc_ctx
(
      void
)
{
      /*
       * Allocate a malloc context block to hang allocations made in this
       * context off of.
       */
      malloc_ctx_t *new = NEW (malloc_ctx_t);

      /* Link new context on the top of the context stack */
      new->next = malloc_ctx;
      new->list = NULL;
      new->permanent = false;
      malloc_ctx = new;
}

/*
** BE_pop_malloc_ctx: Pop the current context, freeing all memory allocated
** within this context (unless it was a permanent context).
*/
void BE_pop_malloc_ctx
(
    void
)
{
      malloc_t *list,*curr;
      malloc_ctx_t *ctx;

      /* If we are called with an empty stack, then abort */
      if (malloc_ctx == NULL)
          error(NIDL_INTERNAL_ERROR,__FILE__,__LINE__);

      /* Loop through the context freeing all memory */
      list = malloc_ctx->list;
      while (list != NULL)
      {
          curr = list;
          list = list->next;
          free(curr);
      }

      /* Remove context from the stack, and free the context header */
      ctx = malloc_ctx;
      malloc_ctx = malloc_ctx->next;
      free(ctx);
}


/*
 *  BE_main
 */
boolean BE_main              /* returns true on successful completion */
(
    boolean             *cmd_opt,   /* [in] array of cmd option flags */
    void                **cmd_val,  /* [in] array of cmd option values */
    FILE                *h_fid,     /* [in] header file handle, or NULL */
    FILE                *caux_fid ATTRIBUTE_UNUSED,  /* [in] client aux file handle, or NULL */
    FILE                *saux_fid ATTRIBUTE_UNUSED,  /* [in] server aux file handle, or NULL */
    FILE                *cstub_fid, /* [in] cstub file handle, or NULL */
    FILE                *sstub_fid, /* [in] sstub file handle, or NULL */
    AST_interface_n_t   *int_p      /* [in] ptr to interface node */
)
{
    DDBE_vectors_t      *dd_vip;    /* Data driven BE vector information ptr */

    be_init (cmd_opt, cmd_val);

    /* Generate the intermediate representation if stubs are required. */
    if (cstub_fid || sstub_fid)	{
        IR_gen_irep(cmd_opt, cmd_val, int_p);
	 }

    /* Print accumulated errors and warnings generated by irep, if any. */
    if (!cmd_opt[opt_confirm])
        print_errors();

    /* Call the Data Driven Backend if stubs are required. */
    if (cstub_fid || sstub_fid)
        DDBE_main(cmd_opt, cmd_val, int_p, &dd_vip);

    if (h_fid)
    {
        BE_gen_c_header(h_fid, int_p,
            (boolean *)cmd_val[opt_do_bug], cmd_opt);
    }

    /*
     * emit client stub file if requested
     */
    if (cstub_fid && error_count == 0)
        DDBE_gen_cstub(cstub_fid, int_p, lang_c_k,
            (char *)cmd_val[opt_header], cmd_opt, cmd_val, dd_vip);

    /*
     * emit server stub file if requested
     */
    if (sstub_fid && error_count == 0)
        BE_gen_sstub (sstub_fid, int_p, lang_c_k, (char *)cmd_val[opt_header],
                         cmd_opt, cmd_val, dd_vip);

    return (error_count == 0);
}

/*
 * Output #includes needed at the start of MTS stubs
 */
void CSPELL_mts_includes
(
    FILE *fid,
    char header_name[]
)
{

    fprintf (fid, USER_INCLUDE_TEMPLATE, header_name);

    fprintf (fid, INCLUDE_TEMPLATE, "idlddefs.h");
}

/*
 * BE_get_name
 *
 * Returns a character string given a NAMETABLE_id_t
 */
char const *BE_get_name
(
    NAMETABLE_id_t id
)
{
    char const *retval;

    NAMETABLE_id_to_string(id, &retval);
    return retval;
}

/*
 * BE_get_type_node
 *
 * Allocates and returns a type node
 */
AST_type_n_t *BE_get_type_node
(
    AST_type_k_t kind
)
{
    AST_type_n_t *new_type = (AST_type_n_t *)BE_ctx_malloc(sizeof(AST_type_n_t));

    new_type->fe_info = NULL;
    new_type->be_info.other = NULL;
    new_type->name = NAMETABLE_NIL_ID;
    new_type->defined_as = NULL;
    new_type->kind = kind;
    new_type->flags = 0;
    new_type->xmit_as_type = NULL;
    new_type->rep_as_type = NULL;
    new_type->cs_char_type = NULL;

    return new_type;
}

/******************************************************************************/
/*                                                                            */
/*    Dummy - Control of generation of pipe routine declarations              */
/*                                                                            */
/******************************************************************************/
void BE_gen_pipe_routine_decls
(
    FILE *fid ATTRIBUTE_UNUSED,
    AST_interface_n_t *p_interface ATTRIBUTE_UNUSED
)
{
}


