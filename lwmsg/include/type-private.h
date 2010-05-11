/*
 * Copyright (c) Likewise Software.  All rights Reserved.
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the license, or (at
 * your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser
 * General Public License for more details.  You should have received a copy
 * of the GNU Lesser General Public License along with this program.  If
 * not, see <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * LESSER GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
 */

/*
 * Module Name:
 *
 *        type-private.h
 *
 * Abstract:
 *
 *        Type specification API (private header)
 *
 * Authors: Brian Koropoff (bkoropoff@likewisesoftware.com)
 *
 */
#ifndef __TYPE_PRIVATE_H__
#define __TYPE_PRIVATE_H__

#include <lwmsg/type.h>
#include <lwmsg/protocol.h>
#include <lwmsg/status.h>
#include <lwmsg/common.h>

#include "util-private.h"
#include "status-private.h"

typedef struct LWMsgType
{
    /** Set flags */
    LWMsgTypeFlags flags;
    /** Custom flags from #LWMSG_ATTR_CUSTOM() */
    size_t custom;
    /** Minimum value of range constraint */
    size_t range_low;
    /** Maximum value of range constraint */
    size_t range_high;
} LWMsgTypeAttrs;

/* Iteration */
typedef struct LWMsgTypeIter
{
    /* Source type spec */
    LWMsgTypeSpec* spec;
    /* Kind of type */
    LWMsgKind kind;
    /* Offset of member when in structure, or 0 */
    size_t offset;
    /* Size of type */
    size_t size;
    /* Tag of arm when in union
       Value of enum variant when in enum */
    uint64_t tag;

    /* User verification function */
    LWMsgVerifyFunction verify;
    /* User verification data */
    void* verify_data;
    /* Generic (public) type attributes */
    LWMsgTypeAttrs attrs;

    /* Union of type-specific informatin */
    union
    {
        struct
        {
            LWMsgBool is_mask;
        } kind_variant;
        /* Integer types */
        struct
        {
            size_t width;
            LWMsgSignage sign;
        } kind_integer;
        /* Compound (struct, union) types */
        struct
        {
            /* Discriminator offset and size for union types */
            struct
            {
                size_t offset;
                size_t size;
            } discrim;
        } kind_compound;
        /* Indirect (pointer, array) types */
        struct
        {
            /* Termination strategy */
            LWMsgArrayTermination term;
            /* Union of termination info */
            union
            {
                /* Offset and size of correlated length member */
                struct
                {
                    size_t offset;
                    size_t size;
                } member;
                /* Static length of array */
                size_t static_length;
            } term_info;
            /* Character encoding if array represents a text string */
            const char* encoding;
        } kind_indirect;
        /* Custom types */
        struct
        {
            /* Custom type class structure */
            LWMsgTypeClass* typeclass;
            /* Per-instance data pointer (passed to methods in type class) */
            void* typedata;
        } kind_custom;
    } info;

    /* Start of inner type definition (e.g. contents of struct type) */
    LWMsgTypeSpec* inner;
    /* Start of next type definition (e.g. next member within a struct) */
    LWMsgTypeSpec* next;
    /* Most recent dominating object in memory.  This is used to track
       the structure where correlated length members/discriminators should
       be looked up for the current type */
    unsigned char* dom_object;
    
    /* Optional metadata - name of C type and struct/union member */
    struct
    {
        const char* type_name;
        const char* member_name;
    } meta;
    
    /* Optional debug info - file and line number where type appears */
    struct
    {
        const char* file;
        unsigned int line;
    } debug;
} LWMsgTypeIter;

typedef struct LWMsgMemberRep
{
    struct LWMsgTypeRep* type;
    char* name;
} LWMsgFieldRep;

typedef struct LWMsgArmRep
{
    struct LWMsgTypeRep* type;
    uint32_t tag;
    char* name;
} LWMsgArmRep;

typedef struct LWMsgVariantRep
{
    uint8_t is_mask;
    uint64_t value;
    char* name;
} LWMsgVariantRep;

typedef struct LWMsgTypeRep
{
    LWMsgKind kind;
    LWMsgTypeFlags flags;
    char* name;
    union LWMsgTypeRepInfo
    {
        struct LWMsgIntegerRep
        {
            uint8_t width;
            LWMsgSignage sign;
            uint64_t lower_bound;
            uint64_t upper_bound;
        } integer_rep;
        struct LWMsgEnumRep
        {
            uint8_t width;
            LWMsgSignage sign;
            uint16_t variant_count;
            LWMsgVariantRep* variants;
        } enum_rep;
        struct LWMsgStructRep
        {
            uint16_t field_count;
            LWMsgFieldRep* fields;
        } struct_rep;
        struct LWMsgUnionRep
        {
            uint16_t arm_count;
            LWMsgArmRep* arms;
            uint16_t discrim_member_index;
        } union_rep;
        struct LWMsgPointerRep
        {
            struct LWMsgTypeRep* pointee_type;
            LWMsgBool zero_terminated;
            uint32_t static_length;
            int16_t length_member_index;
            char* encoding;
        } pointer_rep;
        struct LWMsgArrayRep
        {
            struct LWMsgTypeRep* element_type;
            LWMsgBool zero_terminated;
            uint32_t static_length;
            int16_t length_member_index;
            char* encoding;
        } array_rep;
        struct LWMsgCustomRep
        {
            struct LWMsgTypeRep* transmitted_type;
        } custom_rep;
    } info;
} LWMsgTypeRep;

typedef union LWMsgTypeRepInfo LWMsgTypeRepInfo;
typedef struct LWMsgIntegerRep LWMsgIntegerRep;
typedef struct LWMsgStructRep LWMsgStructRep;
typedef struct LWMsgUnionRep LWMsgUnionRep;
typedef struct LWMsgPointerRep LWMsgPointerRep;
typedef struct LWMsgArrayRep LWMsgArrayRep;
typedef struct LWMsgCustomRep LWMsgCustomRep;
typedef struct LWMsgEnumRep LWMsgEnumRep;

typedef struct LWMsgTypeRepMapEntry
{
    LWMsgTypeSpec* spec;
    LWMsgTypeRep* rep;
    LWMsgRing ring1;
    LWMsgRing ring2;
} LWMsgTypeRepMapEntry;

typedef struct LWMsgTypeRepMap
{
    LWMsgHashTable hash_by_spec;
    LWMsgHashTable hash_by_rep;
    LWMsgTypeIter* dominating_iter;
    LWMsgTypeIter* dominating_rep;
} LWMsgTypeRepMap;

void
lwmsg_type_iterate(
    LWMsgTypeSpec* spec,
    LWMsgTypeIter* iter
    );

static inline
LWMsgBool
lwmsg_type_valid(
    LWMsgTypeIter* iter
    )
{
    return iter->kind != LWMSG_KIND_NONE;
}

static inline
void
lwmsg_type_next(
    LWMsgTypeIter* iter
    )
{
    if (iter->next)
    {
        lwmsg_type_iterate(iter->next, iter);
    }
    else
    {
        iter->kind = LWMSG_KIND_NONE;
    }
}

static inline
void
lwmsg_type_enter(
    LWMsgTypeIter* iter,
    LWMsgTypeIter* new_iter
    )
{
    if (iter->inner)
    {
        lwmsg_type_iterate(iter->inner, new_iter);

        new_iter->dom_object = iter->dom_object;
    }
    else
    {
        new_iter->kind = LWMSG_KIND_NONE;
    }
}

void
lwmsg_type_iterate_promoted(
    LWMsgTypeSpec* spec,
    LWMsgTypeIter* iter
    );

LWMsgStatus
lwmsg_type_rep_from_spec(
    LWMsgTypeSpec* spec,
    LWMsgTypeRep** rep
    );

extern LWMsgTypeSpec* lwmsg_type_rep_spec;

#endif
