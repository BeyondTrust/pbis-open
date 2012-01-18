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
    /** Maximum memory allocation allowed */
    size_t max_alloc;
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
        const char* container_name;
    } meta;
    
    /* Optional debug info - file and line number where type appears */
    struct
    {
        const char* file;
        unsigned int line;
    } debug;
} LWMsgTypeIter;

typedef struct LWMsgIntegerDefRep
{
    uint8_t width;
    LWMsgSignage sign;
    unsigned seen:1;
} LWMsgIntegerDefRep;

typedef struct LWMsgFieldRep
{
    struct LWMsgTypeRep* type;
    char* name;
} LWMsgFieldRep;

typedef struct LWMsgStructDefRep
{
    uint16_t field_count;
    LWMsgFieldRep* fields;
    char* name;
    unsigned seen:1;
} LWMsgStructDefRep;

typedef struct LWMsgArmRep
{
    struct LWMsgTypeRep* type;
    uint32_t tag;
    char* name;
} LWMsgArmRep;

typedef struct LWMsgUnionDefRep
{
    uint16_t arm_count;
    LWMsgArmRep* arms;
    char* name;
    unsigned seen:1;
} LWMsgUnionDefRep;

typedef struct LWMsgVariantRep
{
    uint8_t is_mask;
    uint64_t value;
    char* name;
} LWMsgVariantRep;

typedef struct LWMsgEnumDefRep
{
    uint8_t width;
    LWMsgSignage sign;
    uint16_t variant_count;
    LWMsgVariantRep* variants;
    char* name;
    unsigned seen:1;
} LWMsgEnumDefRep;

typedef struct LWMsgTypeRep
{
    LWMsgKind kind;
    LWMsgTypeFlags flags;
    union LWMsgTypeRepInfo
    {
        struct LWMsgIntegerRep
        {
            LWMsgIntegerDefRep* definition;
            uint64_t lower_bound;
            uint64_t upper_bound;
        } integer_rep;
        struct LWMsgEnumRep
        {
            LWMsgEnumDefRep* definition;
        } enum_rep;
        struct LWMsgStructRep
        {
            LWMsgStructDefRep* definition;
        } struct_rep;
        struct LWMsgUnionRep
        {
            LWMsgUnionDefRep* definition;
            int16_t discrim_member_index;
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
    struct SpecKey
    {
        enum SpecKind
        {
            SPEC_TYPE,
            SPEC_DEF
        } kind;
        LWMsgTypeSpec* spec;
    } spec;
    void* rep;
    LWMsgRing ring;
} LWMsgTypeRepMapEntry;

typedef struct LWMsgTypeRepMap
{
    const LWMsgContext* context;
    LWMsgHashTable hash_by_spec;
    LWMsgTypeIter* dominating_iter;
    LWMsgTypeIter* dominating_rep;
} LWMsgTypeRepMap;

typedef struct LWMsgTypeSpecBuffer
{
    /* Associated type rep */
    void* rep;
    /* Buffer for constructed type spec */
    unsigned int buffer_capacity;
    unsigned int buffer_size;
    size_t* buffer;
    /* Backlinks to other specs that will reference this one */
    LWMsgRing backlinks;
    /* Array of offsets of C members */
    struct
    {
        size_t offset;
        size_t size;
    }* member_metrics;
    /* Size of the C type */
    size_t type_size;
    /* Link into hash table */
    LWMsgRing ring;
} LWMsgTypeSpecBuffer;

typedef struct LWMsgTypeSpecLink
{
    LWMsgRing ring;
    LWMsgTypeSpecBuffer* buffer;
    unsigned int offset;
} LWMsgTypeSpecLink;

typedef struct LWMsgTypeSpecMap
{
    const LWMsgContext* context;
    LWMsgHashTable hash_by_rep;
    size_t backlinks;
} LWMsgTypeSpecMap;

typedef struct LWMsgTypeSpecState
{
    LWMsgTypeSpecMap* map;
    LWMsgTypeSpecBuffer* dominating_struct;
    ssize_t member_offset;
    size_t member_size;
    char* member_name;
} LWMsgTypeSpecState;

typedef struct LWMsgTypeAssignPair
{
    LWMsgTypeRep* left;
    LWMsgTypeRep* right;
} LWMsgTypeAssignPair;

typedef struct LWMsgTypeAssignEntry
{
    LWMsgTypeAssignPair pair;
    LWMsgRing ring;
} LWMsgTypeAssignEntry;

typedef struct LWMsgTypeAssignMap
{
    LWMsgHashTable hash;
    size_t backlinks;
} LWMsgTypeAssignMap;

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
    const char* container = iter->meta.container_name;

    if (iter->next)
    {
        lwmsg_type_iterate(iter->next, iter);
        iter->meta.container_name = container;
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
        if (iter->kind == LWMSG_KIND_STRUCT || iter->kind == LWMSG_KIND_UNION)
        {
            new_iter->meta.container_name = iter->meta.type_name;
        }
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
lwmsg_type_rep_from_spec_internal(
    LWMsgTypeRepMap* map,
    LWMsgTypeIter* iter,
    LWMsgTypeRep** rep
    );

LWMsgStatus
lwmsg_type_rep_from_spec(
    const LWMsgContext* context,
    LWMsgTypeSpec* spec,
    LWMsgTypeRep** rep
    );

LWMsgStatus
lwmsg_type_spec_from_rep_internal(
    LWMsgTypeSpecMap* map,
    LWMsgTypeRep* rep,
    LWMsgTypeSpecBuffer** buffer
    );

LWMsgStatus
lwmsg_type_spec_from_rep(
    const LWMsgContext* context,
    LWMsgTypeRep* rep,
    LWMsgTypeSpec** spec
    );

void
lwmsg_type_spec_map_destroy(
    LWMsgTypeSpecMap* map
    );

void
lwmsg_type_free_rep(
    const LWMsgContext* context,
    LWMsgTypeRep* rep
    );

LWMsgStatus
lwmsg_type_print_rep(
    LWMsgTypeRep* rep,
    unsigned int indent,
    LWMsgBuffer* buffer
    );

LWMsgStatus
lwmsg_type_print_spec_alloc(
    const LWMsgContext* context,
    LWMsgTypeSpec* type,
    char** result
    );

LWMsgStatus
lwmsg_type_rep_is_assignable(
    LWMsgTypeRep* left,
    LWMsgTypeRep* right
    );

void
lwmsg_type_rep_map_destroy(
    LWMsgTypeRepMap* map
    );

extern LWMsgTypeSpec lwmsg_type_rep_spec[];

#define ITER_FROM_ATTRS(attr) LWMSG_OBJECT_FROM_MEMBER((attr), LWMsgTypeIter, attrs)

#endif
