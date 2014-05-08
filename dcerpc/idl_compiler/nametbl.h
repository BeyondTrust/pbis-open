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
**
**  NAME
**
**      NAMETBL.H
**
**  FACILITY:
**
**      Interface Definition Language (IDL) Compiler
**
**  ABSTRACT:
**
**      Header file for Name Table module, NAMETBL.C
**
**  VERSION: DCE 1.0
**
*/


#ifndef  nametable_incl
#define nametable_incl

/*
** IDL.H needs the definition of STRTAB_str_t, so put it first.
*/

/*
 * it is opaque enough but gives the compiler
 * the oportunity to check the type when neeed
 */
typedef struct NAMETABLE_n_t * NAMETABLE_id_t;
#define NAMETABLE_NIL_ID NULL

#ifdef MSDOS
typedef int  STRTAB_str_t ;
#define STRTAB_NULL_STR  ((STRTAB_str_t) 0)
#else
typedef NAMETABLE_id_t  STRTAB_str_t ;
#define STRTAB_NULL_STR  NULL
#endif


#include <nidl.h>

#define NAMETABLE_id_too_long         1
#define NAMETABLE_no_space            2
#define NAMETABLE_different_casing    3
#define NAMETABLE_string_to_long      4
#define NAMETABLE_bad_id_len          5
#define NAMETABLE_bad_string_len      6

/*
 * This constant needs to be arbitrarily large since derived names added to
 * the nametable can get arbitrarily large, e.g. with nested structures.
 */
#define max_string_len                4096

NAMETABLE_id_t NAMETABLE_add_id(
    const char *id
);

NAMETABLE_id_t NAMETABLE_lookup_id(
    const char *id
);

void NAMETABLE_id_to_string(
    NAMETABLE_id_t NAMETABLE_id,
    const char **str_ptr
);

boolean NAMETABLE_add_binding(
    NAMETABLE_id_t id,
    const void * binding
);

const void* NAMETABLE_lookup_binding(
    NAMETABLE_id_t identifier
);

boolean NAMETABLE_add_tag_binding(
    NAMETABLE_id_t id,
    const void * binding
);

const void* NAMETABLE_lookup_tag_binding(
    NAMETABLE_id_t identifier
);

const void* NAMETABLE_lookup_local(
    NAMETABLE_id_t identifier
);

void  NAMETABLE_push_level(
    void
);

void  NAMETABLE_pop_level(
    void
);

void  NAMETABLE_set_temp_name_mode (
    void
);

void  NAMETABLE_set_perm_name_mode (
    void
);

void  NAMETABLE_clear_temp_name_mode (
    void
);

STRTAB_str_t   STRTAB_add_string(
    const char *string
);

void  STRTAB_str_to_string(
    STRTAB_str_t str,
    const char **strp
);

void  NAMETABLE_init(
    void
);

#ifdef DUMPERS
void  NAMETABLE_dump_tab(
    void
);

#endif
void  STRTAB_init(
    void
);

NAMETABLE_id_t NAMETABLE_add_derived_name(
    NAMETABLE_id_t id,
    const char *matrix
);

NAMETABLE_id_t NAMETABLE_add_derived_name2(
    NAMETABLE_id_t id1,
    NAMETABLE_id_t id2,
    char *matrix
);



#endif
