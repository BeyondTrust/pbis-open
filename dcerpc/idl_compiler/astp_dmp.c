/*
 * 
 * (c) Copyright 1989 OPEN SOFTWARE FOUNDATION, INC.
 * (c) Copyright 1989 HEWLETT-PACKARD COMPANY
 * (c) Copyright 1989 DIGITAL EQUIPMENT CORPORATION
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
 */
/*
**
**  NAME
**      ASTP_DMP.C
**
**
**  FACILITY:
**
**      Remote Procedure Call (RPC)
**
**  ABSTRACT:
**
**      Contains routines to dump the Abstract Syntax Tree.
**
**  VERSION: DCE 1.0
**
*/


#include <nidl.h>
#include <astp.h>
#include <astp_dmp.h>

#ifdef DUMPERS

#define DUMP_INDENT_STRING "    "

/* Node address dumping macros */
#define DUMP_NODE_ADDRESS_OPTION "DUMP_NODE_ADDRESS"
#define NODE_ADDR_FMT            "@ %08lX H"

static char *debug_dump = NULL;


/*
 *  Forward declarations of dump routines
 */

static void AST_dump_export_list  (
    AST_export_n_t *export_list_ptr
);

static void AST_dump_exported_item  (
    AST_export_n_t *export_ptr
);

static void AST_dump_import_list (
    AST_import_n_t *import_list_ptr
);

static void AST_dump_include_list (
    AST_include_n_t *include_list_ptr
);

static void AST_dump_ports_list  (
    AST_interface_n_t *interface_node_ptr
);

static void AST_dump_simple_types_list
    (
    AST_type_p_n_t *type_p_list,
    int indentation
);

static void AST_dump_array        (
    AST_array_n_t *array_node_ptr,
    int indentation
);

static void AST_dump_indices      (
    AST_array_index_n_t *index_node_ptr,
    unsigned short array_size,
    int indentation
);

static void AST_dump_structure  (
    AST_structure_n_t *structure_node_ptr,
    int indentation
);

static void AST_dump_disc_union    (
    AST_disc_union_n_t *disc_union_node_ptr,
    int indentation
);

static void AST_dump_arm (
    AST_arm_n_t *arm_node_ptr,
    int indentation
);

static void AST_dump_enumerators  (
    AST_enumeration_n_t *enum_node_ptr,
    int indentation
);


static void AST_dump_field_attrs (
    ASTP_node_t  *parent_node,
    AST_type_n_t *type_node,
    AST_field_attr_n_t *field_attr_node,
    int indentation
);

static void AST_dump_field_ref (
    AST_field_ref_n_t *field_ref_vector,
    fe_node_k_t parent_node_kind,
    unsigned short dimension,
    int indentation
);

static void print_type_name (
    char *format,
    AST_type_k_t type,
    int indentation
);




/********************************************************************/
/*                                                                  */
/*                    AST dumping routines                          */
/*                                                                  */
/********************************************************************/

static void indent
(
    int     scope
)
{
    int     i;

    for (i = 0; i < scope; i++)
        printf (DUMP_INDENT_STRING);
}

static void dump_node_address
(
    char   *node_name,
    char   *node_address,
    int    indentation
)
{
    if (debug_dump)
    {
        indent(indentation);
        if (node_name != NULL) printf (node_name);
        printf (NODE_ADDR_FMT, (unsigned long)node_address);
        if (node_name != NULL)
        {
            printf("\n");
        }
        else
        {
            printf(", ");
        }
    }
}

#define dump_nametable_id AST_dump_nametable_id

void AST_dump_nametable_id
(
    char   *format_string,
    NAMETABLE_id_t id
)
{
    const char   *name_ptr;

    if (id == NAMETABLE_NIL_ID)
        printf (format_string, "NAMETABLE_NIL_ID");
    else {
        NAMETABLE_id_to_string (id, &name_ptr);
        printf (format_string, name_ptr);
    }
}


#if 0 /* Currently, Unused */
static void print_boolean
(
    char *format,
    boolean value,
    int scope
)
{
    indent(scope);
    if (value)
        printf(format, "true");
    else
        printf(format, "false") ;
}
#endif


void AST_dump_interface
(
    AST_interface_n_t *if_n_p
)
{
    if (if_n_p == NULL)
        return;

    debug_dump = getenv(DUMP_NODE_ADDRESS_OPTION);

    printf("\n\nDumping interface ");
    dump_node_address((char *)NULL, (char *)if_n_p, 0);
    dump_nametable_id ("%s \n", if_n_p->name);

    if (AST_LOCAL_SET(if_n_p)) {
        printf("Interface is local.\n");
    } else {
        printf("UUID\n");
        indent(1);
        printf("time_high = %02lx.%02lx\n", (unsigned long)if_n_p->uuid.time_hi_and_version,
                                            (unsigned long)if_n_p->uuid.time_mid);
        indent(1);
        printf("time_low  = %04lx\n", if_n_p->uuid.time_low);
        indent(1);
        printf("clock_seq_hi = %02x\n", if_n_p->uuid.clock_seq_hi_and_reserved);
        indent(1);
        printf("clock_seq_low = %02x\n", if_n_p->uuid.clock_seq_low);
        indent(1);
        printf("host      = %02x.%02x.%02x.%02x.%02x.%02x\n",
            if_n_p->uuid.node[0],
            if_n_p->uuid.node[1],
            if_n_p->uuid.node[2],
            if_n_p->uuid.node[3],
            if_n_p->uuid.node[4],
            if_n_p->uuid.node[5]);
    };

    AST_dump_ports_list (if_n_p);

    if (if_n_p->exceptions != NULL)
    {
        AST_exception_n_t *excep_p;
        printf("Exceptions:\n");
        for (excep_p = if_n_p->exceptions; excep_p; excep_p = excep_p->next)
        {
            dump_nametable_id("    %s", excep_p->name);
            if (AST_EXTERN_SET(excep_p))
                printf(" [extern]");
            printf("\n");
        }
    }

    if (if_n_p->cs_tag_rtns != NULL)
    {
        AST_name_n_t *name_p;
        printf("I-char tag routines:\n");
        for (name_p = if_n_p->cs_tag_rtns; name_p; name_p = name_p->next)
            dump_nametable_id("    %s\n", name_p->name);
    }

    if (AST_IN_LINE_SET(if_n_p)     ||
        AST_OUT_OF_LINE_SET(if_n_p) ||
        if_n_p->binding_callout_name != NAMETABLE_NIL_ID ||
        AST_CODE_SET(if_n_p)        ||
        AST_NO_CODE_SET(if_n_p)     ||
        AST_DECODE_SET(if_n_p)      ||
        AST_ENCODE_SET(if_n_p)      ||
        AST_AUTO_HANDLE_SET(if_n_p) ||
        AST_EXPLICIT_HANDLE_SET(if_n_p) ||
        AST_NO_CANCEL_SET(if_n_p)) {
            printf("Interface attributes:\n");

            if (AST_IN_LINE_SET(if_n_p)) {
                indent(1);
                printf("[in_line]\n");
            };
            if (AST_OUT_OF_LINE_SET(if_n_p)) {
                indent(1);
                printf("[out_of_line]\n");
            };
            if (if_n_p->binding_callout_name != NAMETABLE_NIL_ID) {
                indent(1);
                printf("[binding_callout(");
                dump_nametable_id ("%s)] \n", if_n_p->binding_callout_name);
            }
            if (AST_CODE_SET(if_n_p)) {
                indent(1);
                printf("[code]\n");
            };
            if (AST_NO_CODE_SET(if_n_p)) {
                indent(1);
                printf("[nocode]\n");
            };
            if (AST_DECODE_SET(if_n_p)) {
                indent(1);
                printf("[decode]\n");
            };
            if (AST_ENCODE_SET(if_n_p)) {
                indent(1);
                printf("[encode]\n");
            };
            if (AST_AUTO_HANDLE_SET(if_n_p)) {
                indent(1);
                printf("[auto_handle]\n");
            };
            if (AST_EXPLICIT_HANDLE_SET(if_n_p)) {
                indent(1);
                printf("[explicit_handle]\n");
            };
            if (AST_NO_CANCEL_SET(if_n_p)) {
                indent(1);
                printf("[nocancel]\n");
            };
    };

    printf("Version = %ld\n", if_n_p->version);

    if (if_n_p->implicit_handle_name != NAMETABLE_NIL_ID) {
        dump_nametable_id("Implicit handle name = %s \n",
                            if_n_p->implicit_handle_name);
        if (if_n_p->implicit_handle_type != NULL)
        {
            AST_dump_type(if_n_p->implicit_handle_type,"Handle type = %s\n", 0);
        }
        else
        {
            dump_nametable_id("Implicit handle type name = %s \n",
                            if_n_p->implicit_handle_type_name);
        }

    }

    printf("Operation count: %d\n", if_n_p->op_count);

    if (if_n_p->includes != NULL)
        AST_dump_include_list (if_n_p->includes);

    if (if_n_p->imports != NULL)
        AST_dump_import_list (if_n_p->imports);

    if (if_n_p->exports != NULL)
        AST_dump_export_list (if_n_p->exports);

    if (if_n_p->pipe_types != NULL) {
        printf("\nPipe types list:\n");
        AST_dump_simple_types_list (if_n_p->pipe_types, 1);
    };

    if (if_n_p->ool_types != NULL) {
        printf("\nOut-of-line types list:\n");
        AST_dump_simple_types_list (if_n_p->ool_types, 1);
    };

    if (if_n_p->ra_types != NULL) {
        printf("\nRepresent-as types list:\n");
        AST_dump_simple_types_list (if_n_p->ra_types, 1);
    };

    if (if_n_p->cs_types != NULL) {
        printf("\nI-char types list:\n");
        AST_dump_simple_types_list (if_n_p->cs_types, 1);
    };

    if (if_n_p->sp_types != NULL) {
        printf("\nSelf-pointing types list:\n");
        AST_dump_simple_types_list (if_n_p->sp_types, 1);
    };


    if (if_n_p->pa_types != NULL) {
        printf("\nPointed-at types list:\n");
        AST_dump_simple_types_list (if_n_p->pa_types, 1);
    };

    if (if_n_p->up_types != NULL) {
        printf("\nUnions containing pointers list:\n");
        AST_dump_simple_types_list (if_n_p->up_types, 1);
    };

    fflush(stdout);
}

static void AST_dump_include_list
(
    AST_include_n_t *include_list_ptr
)
{
    AST_include_n_t *ip;
    const char *include_file;

    for (ip = include_list_ptr; ip; ip = ip->next)
    {
        STRTAB_str_to_string (ip->file_name, &include_file);
        printf("Include File: %s\n", include_file);

    }

}

static void AST_dump_import_list
(
    AST_import_n_t *import_list_ptr
)
{
    AST_import_n_t *ip;
    const char *import_file;

    for (ip = import_list_ptr; ip; ip = ip->next)
    {
        STRTAB_str_to_string (ip->file_name, &import_file);
        printf("Imported File: %s\n", import_file);

    }

}

static void AST_dump_export_list
(
    AST_export_n_t *export_list_ptr
)
{
    AST_export_n_t *ep;

    for (ep = export_list_ptr; ep; ep = ep->next)
        AST_dump_exported_item (ep);
}


static void AST_dump_exported_item
(
    AST_export_n_t *export_ptr
)
{

    printf ("\nExported Item: ");
    dump_node_address((char *)NULL, (char *)export_ptr, 0);

    switch (export_ptr->kind) {
    case AST_constant_k:
        dump_nametable_id("%s ", (export_ptr->thing_p.exported_constant)->name);
        printf ("Type = AST_constant_k\n");
        indent(1);
        AST_dump_constant (export_ptr->thing_p.exported_constant, 1);
        break;

    case AST_type_k:
        if (AST_DEF_AS_TAG_SET(export_ptr->thing_p.exported_type))
            dump_nametable_id("tag %s ", (export_ptr->thing_p.exported_type)->name);
        else
            dump_nametable_id("%s ", (export_ptr->thing_p.exported_type)->name);
        printf ("Type = AST_type_k\n");
        AST_dump_type (export_ptr->thing_p.exported_type, "Type is %s\n", 1);

        /* Dump the DEF_AS_TAG type associated with this type, if any */
        if (debug_dump &&
            (export_ptr->thing_p.exported_type->fe_info->tag_ptr != NULL))
        {
            printf ("\nImplicit Export Tagged Item: ");
            dump_node_address((char *)NULL, (char *)export_ptr, 0);
            dump_nametable_id("%s ", (export_ptr->thing_p.exported_type)->fe_info->tag_ptr->name);
            printf ("Type = AST_type_k\n");
            AST_dump_type(export_ptr->thing_p.exported_type->fe_info->tag_ptr, "Type is %s\n", 1);
            printf ("\n");
        }
        break;

    case AST_operation_k:
        printf ("Type = AST_operation_k\n");
        AST_dump_operation (export_ptr->thing_p.exported_operation, 1);
        break;

    }
}

static void AST_dump_ports_list
(
    AST_interface_n_t *interface_node_ptr
)
{

    int i;

    /*
     * Dump new string formated port specs
     */
    for (i = 0; i < interface_node_ptr->number_of_ports; i++)
    {
        const char *protocol, *endpoint;
        STRTAB_str_to_string((interface_node_ptr->protocol)[i],&protocol);
        STRTAB_str_to_string((interface_node_ptr->endpoints)[i],&endpoint);
        printf("Port Spec: %s:[%s]\n",protocol,endpoint);
    }
}


#if 0 /* Currently unused */

void AST_dump_types_list
(
    AST_type_p_n_t *type_p_list,
    int indentation
)
{
    AST_type_p_n_t *type_ptr;


    for (type_ptr = type_p_list; type_ptr; type_ptr = type_ptr->next)
    {
        indent(indentation+1);
        dump_nametable_id ("Type name = %s\n", type_ptr->type->name);
        AST_dump_type (type_ptr->type, "Type is %s\n", indentation+1);
        printf("\n");
    }
}

#endif



static void AST_dump_simple_types_list
(
    AST_type_p_n_t *type_p_list,
    int indentation
)
{
    AST_type_p_n_t *type_ptr;

    for (type_ptr = type_p_list; type_ptr; type_ptr = type_ptr->next)
    {
        NAMETABLE_id_t name;
        indent (indentation);
        name = type_ptr->type->name;
        /* Output the special pa by whom attributes if debug dump */
        if (debug_dump)
        {
            if (AST_IN_PA_STUB_SET(type_ptr->type)) printf("[in_pa_stub] ");
            if (AST_OUT_PA_STUB_SET(type_ptr->type)) printf("[out_pa_stub] ");
        }

        if (name != NAMETABLE_NIL_ID)
        {
            if (AST_DEF_AS_TAG_SET(type_ptr->type)) printf("tag ");
            dump_nametable_id("%s ", type_ptr->type->name);
        }
        else if ((name == NAMETABLE_NIL_ID) && (type_ptr->type->kind == AST_structure_k))
            dump_nametable_id("Struct Tag %s ", type_ptr->type->type_structure.structure->tag_name);
        else if ((name == NAMETABLE_NIL_ID) && (type_ptr->type->kind == AST_disc_union_k))
            dump_nametable_id("Union Tag %s ", type_ptr->type->type_structure.disc_union->tag_name);
        else
            printf("NAMETABLE_NIL_ID ");
        print_type_name ("Type = %s\n", type_ptr->type->kind, indentation);
        dump_node_address("Type ", (char *)type_ptr->type, indentation);
        printf("\n");
    }
}

void AST_dump_constant
(
    AST_constant_n_t *constant_node_ptr,
    int indentation
)
{
    const char   *string_val_ptr;

    switch (constant_node_ptr->kind)
    {

    case AST_boolean_const_k:
        printf ("boolean constant: ");
        if (constant_node_ptr->value.boolean_val)
        {
            printf("TRUE");
        }
        else
        {
            printf("FALSE");
        }

        break;

    case AST_int_const_k:
        printf ("integer constant: %ld ", constant_node_ptr->value.int_val);
        if ((constant_node_ptr->value.int_val > 9) ||
            (constant_node_ptr->value.int_val < 0))
        {
            printf ("(%lX H) ", constant_node_ptr->value.int_val);
        }
        break;

    case AST_hyper_int_const_k:
        printf ("hyper constant: %ld %ld ",
                    constant_node_ptr->value.hyper_int_val.high,
                    constant_node_ptr->value.hyper_int_val.low);
        printf ("(%lX H, %lX H) ", constant_node_ptr->value.hyper_int_val.high,
                    constant_node_ptr->value.hyper_int_val.low);
        break;

    case AST_string_const_k:
        STRTAB_str_to_string(constant_node_ptr->value.string_val,
                            &string_val_ptr);
        printf ("string constant: %s\n", string_val_ptr);
        break;

    case AST_char_const_k:
        printf ("char constant: %c\n", constant_node_ptr->value.char_val);
        break;

    case AST_nil_const_k:
        printf ("NULL constant: %ld\n", constant_node_ptr->value.int_val);
        break;

    default:
        break;
    }

    printf ("\n");

    if (constant_node_ptr->defined_as != NULL)
    {
        indent(indentation);
        dump_nametable_id ("Defined as Named constant %s\n",
                            constant_node_ptr->defined_as->name);
        dump_node_address("Defined as Constant node ",
                            (char *)constant_node_ptr->defined_as, indentation);
    }

}


static void print_type_name
(
    char   *format,
    AST_type_k_t type,
    int    indentation
)
{
    indent (indentation);

    switch (type) {
    case AST_small_integer_k:
        printf (format, "AST_small_integer_k");
        break;
    case AST_short_integer_k:
        printf (format, "AST_short_integer_k");
        break;
    case AST_long_integer_k:
        printf (format, "AST_long_integer_k");
        break;
    case AST_hyper_integer_k:
        printf (format, "AST_hyper_integer_k");
        break;
    case AST_small_unsigned_k:
        printf (format, "AST_small_unsigned_k");
        break;
    case AST_short_unsigned_k:
        printf (format, "AST_short unsigned_k");
        break;
    case AST_long_unsigned_k:
        printf (format, "AST_long_unsigned_k");
        break;
    case AST_hyper_unsigned_k:
        printf (format, "AST_hyper_unsigned_k");
        break;
    case AST_enum_k:
        printf (format, "AST_enum_k");
        break;
    case AST_short_float_k:
        printf (format, "AST_short_float_k");
        break;
    case AST_long_float_k:
        printf (format, "AST_long_float_k");
        break;
    case AST_array_k:
        printf (format, "AST_array_k");
        break;
    case AST_structure_k:
        printf (format, "AST_structure_k");
        break;
    case AST_pointer_k:
        printf (format, "AST_pointer_k");
        break;
    case AST_void_k:
        printf (format, "AST_void_k");
        break;
    case AST_character_k:
        printf (format, "AST_character_k");
        break;
    case AST_boolean_k:
        printf (format, "AST_boolean_k");
        break;
    case AST_function_k:
        printf (format, "AST_function_k");
        break;
    case AST_handle_k:
        printf (format, "AST_handle_k") ;
        break;
    case AST_disc_union_k:
        printf (format, "AST_disc_union_k") ;
        break;
    case AST_pipe_k:
        printf (format, "AST_pipe_k") ;
        break;
    case AST_byte_k:
        printf (format, "AST_byte_k") ;
        break;

    default:
        /* Not sure why we're here - look into later if time */
        /* For now, just print the new line to finish the statement */
        printf ("\n");
        break;
    }
}

static void AST_dump_array
(
    AST_array_n_t *array_node_ptr,
    int     indentation
)
{
    dump_node_address("Array ", (char *)array_node_ptr, indentation);
    AST_dump_type (array_node_ptr->element_type,
                    "Array element type = %s\n", indentation);
    AST_dump_indices (array_node_ptr->index_vec, array_node_ptr->index_count,
                        indentation);
}


static void AST_dump_indices
(
    AST_array_index_n_t *index_node_ptr,
    unsigned short array_size,
    int indentation
)
{

    unsigned short dimension;

    for (dimension=0; dimension<array_size ; index_node_ptr++, dimension++)
    {
        indent(indentation);
        printf("Index %d: \n", dimension);
        if (AST_FIXED_LOWER_SET(index_node_ptr))
        {
            indent(indentation+1);
            printf("[Fixed Lower] ");
            AST_dump_constant (index_node_ptr->lower_bound, indentation+1);
        }

        if (AST_FIXED_UPPER_SET(index_node_ptr))
        {
            indent(indentation+1);
            printf("[Fixed Upper] ");
            AST_dump_constant (index_node_ptr->upper_bound, indentation+1);
        }

    }
}


void AST_dump_type
(
    AST_type_n_t * type_n_p,
    char   * format,
    int      indentation
)
{
    static int visit_count = 0;
#define MAX_DEPTH 10

    if (visit_count > MAX_DEPTH) {
        printf("...\n") ;
        return ;
    } ;

    if (type_n_p == NULL)
    {
        indent (indentation);
        printf ("No type node supplied.\n");
        return;
    }

    ++visit_count ;

    print_type_name (format, type_n_p->kind, indentation);

    dump_node_address("Type ", (char *)type_n_p, indentation);

    indent (indentation);
    printf("NDR size = %d, Alignment size = %d\n", type_n_p->ndr_size, type_n_p->alignment_size);

    /* dump out addresses of all clones of the type */
    if (type_n_p->fe_info->fe_type_id == fe_clone_info)
    {
       AST_type_p_n_t        *clone_p;
       clone_p = type_n_p->fe_info->type_specific.clone;
       while (clone_p != NULL)
       {
          dump_node_address("Clone ", (char *)clone_p->type, indentation);
          dump_node_address("Original ", (char *)clone_p->type->fe_info->original, indentation);
          clone_p = clone_p->next;
       }
    }


    if (type_n_p->xmit_as_type != NULL) {
        indent (indentation);
        dump_nametable_id ("Transmit type = %s\n",
                                type_n_p->xmit_as_type->name);
    }

    if (type_n_p->rep_as_type != NULL) {
        indent (indentation);
        dump_nametable_id ("Represent as = %s\n",
                                type_n_p->rep_as_type->type_name);
    }

    if (type_n_p->cs_char_type != NULL) {
        indent (indentation);
        dump_nametable_id ("I-char type = %s\n",
                                type_n_p->cs_char_type->type_name);
    }


    if (AST_UNALIGN_SET(type_n_p)) {
        indent (indentation);
        printf ("[unalign]\n");
    }

    if (AST_HEAP_SET(type_n_p)) {
        indent (indentation);
        printf ("[heap]\n");
    }

    if (AST_IN_LINE_SET(type_n_p)) {
        indent (indentation);
        printf ("[in_line]\n");
    }

    if (AST_DEF_AS_TAG_SET(type_n_p)) {
        indent (indentation);
        printf ("[def_as_tag]\n");
    }

    if (AST_OUT_OF_LINE_SET(type_n_p)) {
        indent (indentation);
        printf ("[out_of_line]\n");
    }

    if (AST_STRING_SET(type_n_p)) {
        indent (indentation);
        printf ("[string]\n");
    }

    if ((debug_dump != NULL) && AST_VARYING_SET(type_n_p)) {
        indent (indentation);
        printf ("[varying]\n");
    }

    if (AST_STRING0_SET(type_n_p)) {
        indent (indentation);
        printf ("[string0]\n");
    }

    if (AST_UNIQUE_SET(type_n_p)) {
        indent (indentation);
        printf ("[unique]\n");
    }

    if (AST_REF_SET(type_n_p)) {
        indent (indentation);
        printf ("[ref]\n");
    }

    if (AST_PTR_SET(type_n_p)) {
        indent (indentation);
        printf ("[ptr]\n");
    }

    if (AST_IGNORE_SET(type_n_p)) {
        indent (indentation);
        printf ("[ignore]\n");
    }

    if (AST_SMALL_SET(type_n_p)) {
        indent (indentation);
        printf ("[small]\n");
    }

    if (AST_HANDLE_SET(type_n_p)) {
        indent (indentation);
        printf("[handle]\n");
    }

    if (AST_CONTEXT_RD_SET(type_n_p)) {
        indent (indentation);
        printf ("[context (w/ rundown)]\n");
    }

    if (AST_IN_SET(type_n_p)) {
        indent (indentation);
        printf ("[used for IN param");
        if (AST_IN_VARYING_SET(type_n_p)) {
            printf (", varying");
        }
        if (AST_IN_FIXED_SET(type_n_p)) {
            printf (", fixed");
        }
        printf("]\n");
    }

    if (AST_OUT_SET(type_n_p)) {
        indent (indentation);
        printf ("[used for OUT param");
        if (AST_OUT_VARYING_SET(type_n_p))
            printf (", varying");
        if (AST_OUT_FIXED_SET(type_n_p))
            printf (", fixed");
        if (AST_OUT_PA_REF_SET(type_n_p))
            printf (", pa by ref");
        printf("]\n");
    }

    if (AST_SELF_POINTER_SET(type_n_p)) {
        indent (indentation);
        printf ("[self_pointing]\n");
    }

    if (AST_CONFORMANT_SET(type_n_p)) {
        indent (indentation);
        printf ("[conformant]\n");
    }

    if (AST_V1_ENUM_SET(type_n_p)) {
        indent (indentation);
        printf ("[v1_enum]\n");
    }


    if (type_n_p->defined_as != NULL) {
        indent (indentation);
        dump_nametable_id ("Defined as Named type %s\n", type_n_p->defined_as->name);
        dump_node_address("Defined as Type node ",
                            (char *)type_n_p->defined_as, indentation);
    }


    switch (type_n_p->kind) {

    case AST_function_k:
        AST_dump_operation(type_n_p->type_structure.function,
                            indentation + 1);
        break;

    case AST_array_k:
        AST_dump_array(type_n_p->type_structure.array, indentation+1);
        break;

    case AST_structure_k:
        AST_dump_structure (type_n_p->type_structure.structure, indentation+1);
        break;

    case AST_disc_union_k:
        AST_dump_disc_union (type_n_p->type_structure.disc_union, indentation+1);
        break;

    case AST_pointer_k:
        /* if pointer type is recursively defined, just output the type name */
        if (AST_SELF_POINTER_SET(type_n_p->type_structure.pointer->pointee_type)) {
            AST_type_n_t *ptype = type_n_p->type_structure.pointer->pointee_type;
            NAMETABLE_id_t name = ptype->name;

            /* If a named type, then use that name */
            indent (indentation + 1);
            printf("Pointer to ");
            if (AST_DEF_AS_TAG_SET(ptype)) printf ("tag ");
            dump_nametable_id ("%s\n", name);

            /* print out the type node address to simplify look up */
            indent (indentation + 1);
            printf("Type node address = %08lX H \n", (unsigned long)ptype);
        }
        else
            AST_dump_type (type_n_p->type_structure.pointer->pointee_type,
                            "Pointer to %s\n", indentation + 1);

        /* Dump the array_rep_type if used as an array */
        if (type_n_p->type_structure.pointer->pointee_type->array_rep_type != NULL)
            AST_dump_type (type_n_p->type_structure.pointer->pointee_type->array_rep_type,
                            "Representation of Pointer = %s\n", indentation + 1);

        break;

    case AST_enum_k:
        AST_dump_enumerators (type_n_p->type_structure.enumeration, indentation + 1);
        break;

    case AST_pipe_k:
        indent(indentation);
        dump_nametable_id ("Pipe name = %s\n", type_n_p->name);
        AST_dump_type (type_n_p->type_structure.pipe->base_type,
                        "Base type of %s\n", indentation + 1);
        break;

    default:
        break;

    }
    --visit_count ;
}




static void AST_dump_enumerators
(
    AST_enumeration_n_t *enum_node_ptr,
    int indentation
)
{
    AST_constant_n_t    *cp;

    dump_node_address("Enumerator ", (char *)enum_node_ptr, indentation);
    for (cp = enum_node_ptr->enum_constants; cp; cp = cp->next) {
        indent (indentation );
        dump_nametable_id ("Enumerator name: %s ", cp->name);
        AST_dump_constant (cp, indentation);
    }
}


static void AST_dump_structure
(
    AST_structure_n_t * structure_node_ptr,
    int     indentation
)
{
    AST_field_n_t * fp;

    dump_node_address("Structure ", (char *)structure_node_ptr, indentation);

    if (structure_node_ptr->tag_name != NAMETABLE_NIL_ID)
    {
        indent(indentation);
        dump_nametable_id("Struct tag = %s\n", structure_node_ptr->tag_name);
    }

    for (fp = structure_node_ptr->fields; fp; fp = fp->next) {
        printf("\n");
        indent (indentation);
        dump_nametable_id ("Field_name = %s\n", fp->name);

        if AST_IN_LINE_SET(fp) {
            indent (indentation);
            printf ("[in_line]\n");
        }

        if AST_OUT_OF_LINE_SET(fp) {
            indent (indentation);
            printf ("[out_of_line]\n");
        }

        if AST_STRING_SET(fp) {
            indent (indentation);
            printf ("[string]\n");
        }

        if AST_STRING0_SET(fp) {
            indent (indentation);
            printf ("[string0]\n");
        }

        if AST_IGNORE_SET(fp) {
            indent (indentation);
            printf ("[ignore]\n");
        }

        if AST_SMALL_SET(fp) {
            indent (indentation);
            printf ("[small]\n");
        }

        if AST_CONTEXT_SET(fp) {
            indent (indentation);
            printf ("[context]\n");
        }

        if AST_VARYING_SET(fp) {
            indent (indentation);
            printf ("[varying]\n");
        }

        if AST_PTR_SET(fp) {
            indent (indentation);
            printf ("[ptr]\n");
        }

        if AST_REF_SET(fp) {
            indent (indentation);
            printf ("[ref]\n");
        }

        if AST_UNIQUE_SET(fp) {
            indent (indentation);
            printf ("[unique]\n");
        }

        AST_dump_field_attrs((ASTP_node_t *)fp, fp->type,
                                fp->field_attrs, indentation);
        AST_dump_type (fp->type, "Field type = %s\n", indentation+1);
    }

}



static void AST_dump_disc_union
(
    AST_disc_union_n_t * disc_union_node_ptr,
    int     indentation
)
{
    AST_arm_n_t * ap;

    indent (indentation);
    dump_nametable_id ("Union tag = %s\n", disc_union_node_ptr->tag_name);
    indent (indentation);
    dump_nametable_id ("Union name = %s\n", disc_union_node_ptr->union_name);
    indent (indentation);
    dump_node_address ("Union ", (char *)disc_union_node_ptr, indentation);
    dump_nametable_id ("Discriminant name = %s\n", disc_union_node_ptr->discrim_name);
    if (disc_union_node_ptr->discrim_type != NULL)
        AST_dump_type (disc_union_node_ptr->discrim_type,
                        "Discriminant type = %s\n", indentation);

    for (ap = disc_union_node_ptr->arms; ap; ap = ap->next)
        AST_dump_arm (ap, indentation + 1);

}


static void AST_dump_arm
(
    AST_arm_n_t  * arm_node_ptr,
    int     indentation
)
{
    AST_case_label_n_t * lp;



    /*
     * Output the labels
     */
    for (lp = arm_node_ptr->labels; lp; lp = lp->next) {
        indent (indentation);
        printf ("Case Label = ");
        if (lp->default_label)
            printf ("DEFAULT\n");
        else
            AST_dump_constant (lp->value, indentation);
    }


    /*
     * Output the name and type of this arm
     */
    indent (indentation+1);
    dump_nametable_id ("Arm name = %s\n", arm_node_ptr->name);
    dump_node_address("Arm ", (char *)arm_node_ptr, indentation);

    /* Dump arm attributes */
    if AST_STRING_SET(arm_node_ptr) {
        indent (indentation+1);
        printf ("[string]\n");
    }

    if AST_STRING0_SET(arm_node_ptr) {
        indent (indentation+1);
        printf ("[string0]\n");
    }

    if AST_SMALL_SET(arm_node_ptr) {
        indent (indentation+1);
        printf ("[small]\n");
    }

    if AST_REF_SET(arm_node_ptr) {
        indent (indentation+1);
        printf ("[ref]\n");
    }

    if AST_PTR_SET(arm_node_ptr) {
        indent (indentation+1);
        printf ("[ptr]\n");
    }

    if AST_UNIQUE_SET(arm_node_ptr) {
        indent (indentation+1);
        printf ("[unique]\n");
    }

    if (arm_node_ptr->type != NULL)
        AST_dump_type (arm_node_ptr->type, "Arm type = %s\n", indentation+1);
    else
    {
        indent(indentation+1);
        printf("Arm type = NULL\n");
    }
}


void AST_dump_operation
(
    AST_operation_n_t *operation_node_ptr,
    int indentation
)
{
    AST_parameter_n_t *pp;

    printf("\n");
    indent (indentation);
    printf("Operation ");
    dump_node_address((char *)NULL, (char *)operation_node_ptr, 0);
    dump_nametable_id ("name = %s\n", operation_node_ptr->name);

    indent (indentation);
    printf("Operation number: %d\n", operation_node_ptr->op_number);

    indent (indentation);
    printf("routine attributes\n") ;

    if (operation_node_ptr->cs_tag_rtn_name != NAMETABLE_NIL_ID) {
        indent (indentation);
        dump_nametable_id ("[cs_tag_rtn(%s)] \n",
                           operation_node_ptr->cs_tag_rtn_name);
    }

    if AST_BROADCAST_SET(operation_node_ptr) {
        indent (indentation);
        printf ("[broadcast] \n");
    }

    if AST_MAYBE_SET(operation_node_ptr) {
        indent (indentation);
        printf ("[maybe] \n");
    }

    if AST_CODE_SET(operation_node_ptr) {
        indent (indentation);
        printf ("[code] \n");
    }

    if AST_NO_CODE_SET(operation_node_ptr) {
        indent (indentation);
        printf ("[nocode] \n");
    }

    if AST_DECODE_SET(operation_node_ptr) {
        indent (indentation);
        printf ("[decode] \n");
    }

    if AST_ENCODE_SET(operation_node_ptr) {
        indent (indentation);
        printf ("[encode] \n");
    }

    if AST_ENABLE_ALLOCATE_SET(operation_node_ptr) {
        indent (indentation);
        printf ("[enable_allocate] \n");
    }

    if AST_EXPLICIT_HANDLE_SET(operation_node_ptr) {
        indent (indentation);
        printf ("[explicit_handle] \n");
    }

    if AST_NO_CANCEL_SET(operation_node_ptr) {
        indent (indentation);
        printf ("[nocancel] \n");
    }

    if AST_IDEMPOTENT_SET(operation_node_ptr) {
        indent (indentation);
        printf ("[idempotent] \n");
    }

    if AST_REFLECT_DELETIONS_SET(operation_node_ptr) {
        indent (indentation);
        printf ("[reflect_deletions] \n");
    }

    if AST_HAS_INS_SET(operation_node_ptr) {
        indent (indentation);
        printf ("[has inputs");
        if AST_HAS_IN_CTX_SET(operation_node_ptr)
        {
            printf (" with context");
        }
        if AST_HAS_IN_OOLS_SET(operation_node_ptr) {
            printf (" with ools");
        }
        printf ("] \n");
    }

    if AST_HAS_IN_PIPES_SET(operation_node_ptr) {
        indent (indentation);
        printf ("[has input pipes");
        /* Special case logic if only ins in op are pipes which contain ools */
        if (!AST_HAS_INS_SET(operation_node_ptr)
            &&  AST_HAS_IN_OOLS_SET(operation_node_ptr))
            printf(" with ools");
        printf("] \n");
    }

    if AST_HAS_IN_PTRS_SET(operation_node_ptr) {
        indent (indentation);
        printf ("[has input ptrs] \n");
    }

    if (AST_HAS_FULL_PTRS_SET(operation_node_ptr) && debug_dump) {
        indent (indentation);
        printf ("[has full ptrs] \n");
    }

    if AST_HAS_OUTS_SET(operation_node_ptr) {
        indent (indentation);
        printf ("[has outputs");
        if AST_HAS_OUT_CTX_SET(operation_node_ptr)
        {
            printf (" with context");
        }
        if AST_HAS_OUT_OOLS_SET(operation_node_ptr) {
            printf (" with ools");
        }
        printf ("] \n");
    }

    if AST_HAS_OUT_PIPES_SET(operation_node_ptr) {
        indent (indentation);
        printf ("[has output pipes");
        /* Special case logic if only outs in op are pipes which contain ools */
        if (!AST_HAS_OUTS_SET(operation_node_ptr)
            &&  AST_HAS_OUT_OOLS_SET(operation_node_ptr))
            printf(" with ools");
        printf("] \n");
    }

    if AST_HAS_OUT_PTRS_SET(operation_node_ptr) {
        indent (indentation);
        printf ("[has output ptrs] \n");
    }

    if AST_HAS_XMIT_AS_SET(operation_node_ptr) {
        indent (indentation);
        printf ("[has transmit_as types] \n");
    }


    indent (indentation);
    printf ("routine parameters\n");
    for (pp = operation_node_ptr->parameters; pp != NULL; pp = pp->next) {
        AST_dump_parameter (pp, indentation+1);
        if (pp->uplink != operation_node_ptr)
            printf("\n\n***Consistancy Check Failure***: Uplink not set\n\n");
        }
    if (operation_node_ptr->result != NULL) {
        indent (indentation);
        printf ("operation result\n");
        AST_dump_parameter(operation_node_ptr->result, indentation+1);
        if (operation_node_ptr->result->uplink != operation_node_ptr)
            printf("\n\n***Consistancy Check Failure***: Uplink not set\n\n");
    }
}


void AST_dump_parameter
(
    AST_parameter_n_t *param_node_ptr,
    int     indentation
)
{

    printf("\n");
    indent (indentation);
    printf ("Parameter ");
    dump_node_address((char *)NULL, (char *)param_node_ptr, 0);
    dump_nametable_id ("name = %s\n", param_node_ptr->name);

    if AST_HEAP_SET(param_node_ptr) {
        indent (indentation);
        printf ("[heap]\n");
    }

    if AST_IN_LINE_SET(param_node_ptr) {
        indent (indentation);
        printf ("[in_line]\n");
    }

    if AST_OUT_OF_LINE_SET(param_node_ptr) {
        indent (indentation);
        printf ("[out_of_line]\n");
    }

    if AST_CS_STAG_SET(param_node_ptr) {
        indent (indentation);
        printf ("[cs_stag]\n");
    }

    if AST_CS_DRTAG_SET(param_node_ptr) {
        indent (indentation);
        printf ("[cs_drtag]\n");
    }

    if AST_CS_RTAG_SET(param_node_ptr) {
        indent (indentation);
        printf ("[cs_rtag]\n");
    }

    if AST_HIDDEN_SET(param_node_ptr) {
        indent (indentation);
        printf ("[hidden]\n");
    }

    if AST_STRING_SET(param_node_ptr) {
        indent (indentation);
        printf ("[string]\n");
    }

    if AST_STRING0_SET(param_node_ptr) {
        indent (indentation);
        printf ("[string0]\n");
    }

    if AST_IGNORE_SET(param_node_ptr) {
        indent (indentation);
        printf ("[ignore]\n");
    }

    if AST_SMALL_SET(param_node_ptr) {
        indent (indentation);
        printf ("[small]\n");
    }

    if AST_CONTEXT_SET(param_node_ptr) {
        indent (indentation);
        printf ("[context]\n");
    }

    if AST_VARYING_SET(param_node_ptr) {
        indent (indentation);
        printf ("[varying]\n");
    }

    if AST_IN_SET(param_node_ptr) {
        indent (indentation);
        printf ("[in]\n");
    }

    if AST_IN_SHAPE_SET(param_node_ptr) {
        indent (indentation);
        printf ("[in(shape)]\n");
    }

    if AST_OUT_SET(param_node_ptr) {
        indent (indentation);
        printf ("[out]\n");
    }

    if AST_OUT_SHAPE_SET(param_node_ptr) {
        indent (indentation);
        printf ("[out(shape)]\n");
    }

    if AST_COMM_STATUS_SET(param_node_ptr) {
        indent (indentation);
        printf ("[comm_status]\n");
    }

    if AST_ADD_COMM_STATUS_SET(param_node_ptr) {
        indent (indentation);
        printf ("[add_comm_status]\n");
    }

    if AST_FAULT_STATUS_SET(param_node_ptr) {
        indent (indentation);
        printf ("[fault_status]\n");
    }

    if AST_ADD_FAULT_STATUS_SET(param_node_ptr) {
        indent (indentation);
        printf ("[add_fault_status]\n");
    }

    if AST_REF_SET(param_node_ptr) {
        indent (indentation);
        printf ("[ref]\n");
    }

    if AST_PTR_SET(param_node_ptr) {
        indent (indentation);
        printf ("[ptr]\n");
    }

    if AST_UNIQUE_SET(param_node_ptr) {
        indent (indentation);
        printf ("[unique]\n");
    }

    AST_dump_field_attrs((ASTP_node_t *)param_node_ptr, param_node_ptr->type,
                         param_node_ptr->field_attrs, indentation);

    AST_dump_type (param_node_ptr->type, "parameter type = %s\n",
                    indentation+1);
}

static void AST_dump_field_attrs
(
    ASTP_node_t  *parent_node,
    AST_type_n_t *type_node,
    AST_field_attr_n_t *field_attr_node,
    int indentation
)
{

    AST_type_n_t *tp;
    unsigned short dimension;

    if (type_node == NULL)
        return;

    if (field_attr_node != (AST_field_attr_n_t *) NULL )
    {
        dump_node_address ("Field ", (char *)field_attr_node, indentation);

      tp = ASTP_chase_ptr_to_kind(type_node, AST_disc_union_k);
      if (tp != NULL)
        dimension = 1;
      else
      {
        tp = ASTP_chase_ptr_to_kind(type_node, AST_array_k);

        if ((tp == NULL) && (type_node->kind != AST_pointer_k))
        {
            return;
        }

        if (tp != NULL)
        {
            dimension = tp->type_structure.array->index_count;
        }
        else
        {
            /* A pointer with field attributes is a single dimensioned array */
            dimension = 1;
        }
      }

        if (field_attr_node->first_is_vec != NULL)
        {
            indent (indentation);
            printf("[first_is] attributes:\n");
            AST_dump_field_ref(field_attr_node->first_is_vec,
                               parent_node->fe_info->node_kind,
                               dimension, indentation+1);
        }

        if (field_attr_node->last_is_vec != NULL)
        {
            indent (indentation);
            printf("[last_is] attributes:\n");
            AST_dump_field_ref(field_attr_node->last_is_vec,
                               parent_node->fe_info->node_kind,
                               dimension, indentation+1);
        }

        if (field_attr_node->length_is_vec != NULL)
        {
            indent (indentation);
            printf("[length_is] attributes:\n");
            AST_dump_field_ref(field_attr_node->length_is_vec,
                               parent_node->fe_info->node_kind,
                               dimension, indentation+1);
        }

        if (field_attr_node->min_is_vec != NULL)
        {
            indent (indentation);
            printf("[min_is] attributes:\n");
            AST_dump_field_ref(field_attr_node->min_is_vec,
                               parent_node->fe_info->node_kind,
                               dimension, indentation+1);
        }

        if (field_attr_node->max_is_vec != NULL)
        {
            indent (indentation);
            printf("[max_is] attributes:\n");
            AST_dump_field_ref(field_attr_node->max_is_vec,
                               parent_node->fe_info->node_kind,
                               dimension, indentation+1);
        }

        if (field_attr_node->size_is_vec != NULL)
        {
            indent (indentation);
            printf("[size_is] attributes:\n");
            AST_dump_field_ref(field_attr_node->size_is_vec,
                               parent_node->fe_info->node_kind,
                               dimension, indentation+1);
        }
        if (field_attr_node->switch_is != NULL)
        {
            indent (indentation);
            printf("[switch_is] attribute:\n");
            AST_dump_field_ref(field_attr_node->switch_is,
                               parent_node->fe_info->node_kind,
                               1, indentation+1);
        }
    }
}

static void AST_dump_field_ref
(
    AST_field_ref_n_t *field_ref_vector,
    fe_node_k_t parent_node_kind,
    unsigned short dimension,
    int indentation
)
{
    AST_field_ref_n_t *reference_ptr;
    unsigned short  i;


    for (i=0, reference_ptr = field_ref_vector;
         i < dimension;
         i++, reference_ptr++)
    {
        if (reference_ptr->valid)
        {
            indent(indentation);
            printf("Index %d: \n", i);

            if (parent_node_kind == fe_field_n_k)
            {
                indent(indentation);
                dump_nametable_id ("Reference is field  %s\n",
                                    reference_ptr->ref.f_ref->name);
                AST_dump_type (reference_ptr->ref.f_ref->type,
                                "Field type = %s\n", indentation+1);
            }
            else
            {
                if (parent_node_kind == fe_parameter_n_k)
                {
                    indent(indentation);
                    dump_nametable_id ("Reference is parameter %s\n",
                                        reference_ptr->ref.p_ref->name);
                    AST_dump_type (reference_ptr->ref.p_ref->type,
                                    "Parameter type = %s\n", indentation+1);
                }
                else
                {
                    indent(indentation);
                    printf("**** Field attributes are not valid here.\n ");
                }
            }

        }
    }

}


void AST_enable_hex_dump ()
{
    debug_dump = (char*)1;
}

#endif /* Dumpers */
