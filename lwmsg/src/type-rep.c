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
 *        type-rep.c
 *
 * Abstract:
 *
 *        Serializable type representation
 *
 * Authors: Brian Koropoff (bkoropoff@likewisesoftware.com)
 *
 */

#include "config.h"
#include "type-private.h"
#include "util-private.h"
#include "data-private.h"
#include "buffer-private.h"

/* Forward declarations of static functions */
static
LWMsgStatus
lwmsg_type_spec_from_rep_into(
    LWMsgTypeSpecState* state,
    LWMsgTypeRep* rep,
    LWMsgTypeSpecBuffer* buffer
    );

static
LWMsgStatus
lwmsg_type_rep_is_assignable_internal(
    LWMsgTypeAssignMap* map,
    LWMsgTypeRep* left,
    LWMsgTypeRep* right
    );

static LWMsgTypeSpec bool_enum_spec[] =
{
    LWMSG_ENUM_BEGIN(LWMsgBool, 1, LWMSG_UNSIGNED),
    LWMSG_ENUM_VALUE(LWMSG_FALSE),
    LWMSG_ENUM_VALUE(LWMSG_TRUE),
    LWMSG_ENUM_END,
    LWMSG_TYPE_END
};

static LWMsgTypeSpec sign_enum_spec[] =
{
    LWMSG_ENUM_BEGIN(LWMsgSignage, 1, LWMSG_UNSIGNED),
    LWMSG_ENUM_VALUE(LWMSG_SIGNED),
    LWMSG_ENUM_VALUE(LWMSG_UNSIGNED),
    LWMSG_ENUM_END,
    LWMSG_TYPE_END
};

static LWMsgTypeSpec kind_enum_spec[] =
{
    LWMSG_ENUM_BEGIN(LWMsgKind, 1, LWMSG_UNSIGNED),
    LWMSG_ENUM_VALUE(LWMSG_KIND_INTEGER),
    LWMSG_ENUM_VALUE(LWMSG_KIND_STRUCT),
    LWMSG_ENUM_VALUE(LWMSG_KIND_UNION),
    LWMSG_ENUM_VALUE(LWMSG_KIND_ARRAY),
    LWMSG_ENUM_VALUE(LWMSG_KIND_POINTER),
    LWMSG_ENUM_VALUE(LWMSG_KIND_VOID),
    LWMSG_ENUM_VALUE(LWMSG_KIND_ENUM),
    LWMSG_ENUM_END,
    LWMSG_TYPE_END
};

static LWMsgTypeSpec flags_enum_spec[] =
{
    LWMSG_ENUM_BEGIN(LWMsgTypeFlags, 4, LWMSG_UNSIGNED),
    LWMSG_ENUM_MASK(LWMSG_TYPE_FLAG_NOT_NULL),
    LWMSG_ENUM_MASK(LWMSG_TYPE_FLAG_SENSITIVE),
    LWMSG_ENUM_MASK(LWMSG_TYPE_FLAG_RANGE),
    LWMSG_ENUM_MASK(LWMSG_TYPE_FLAG_ALIASABLE),
    LWMSG_ENUM_END,
    LWMSG_TYPE_END
};

static LWMsgTypeSpec integer_def_rep_spec[] =
{
    LWMSG_STRUCT_BEGIN(LWMsgIntegerDefRep),
    LWMSG_MEMBER_UINT8(LWMsgIntegerDefRep, width),
    LWMSG_ATTR_RANGE(1, 8),
    LWMSG_MEMBER_TYPESPEC(LWMsgIntegerDefRep, sign, sign_enum_spec),
    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

static LWMsgTypeSpec field_rep_spec[] =
{
    LWMSG_STRUCT_BEGIN(LWMsgFieldRep),
    LWMSG_MEMBER_TYPESPEC(LWMsgFieldRep, type, lwmsg_type_rep_spec),
    LWMSG_ATTR_NOT_NULL,
    LWMSG_MEMBER_PSTR(LWMsgFieldRep, name),
    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

static LWMsgTypeSpec struct_def_rep_spec[] =
{
    LWMSG_STRUCT_BEGIN(LWMsgStructDefRep),
    LWMSG_MEMBER_UINT16(LWMsgStructDefRep, field_count),
    LWMSG_MEMBER_POINTER(LWMsgStructDefRep, fields, LWMSG_TYPESPEC(field_rep_spec)),
    LWMSG_ATTR_NOT_NULL,
    LWMSG_ATTR_LENGTH_MEMBER(LWMsgStructDefRep, field_count),
    LWMSG_MEMBER_PSTR(LWMsgStructDefRep, name),
    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

static LWMsgTypeSpec arm_rep_spec[] =
{
    LWMSG_STRUCT_BEGIN(LWMsgArmRep),
    LWMSG_MEMBER_TYPESPEC(LWMsgArmRep, type, lwmsg_type_rep_spec),
    LWMSG_ATTR_NOT_NULL,
    LWMSG_MEMBER_UINT32(LWMsgArmRep, tag),
    LWMSG_MEMBER_PSTR(LWMsgArmRep, name),
    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

static LWMsgTypeSpec union_def_rep_spec[] =
{
    LWMSG_STRUCT_BEGIN(LWMsgUnionDefRep),
    LWMSG_MEMBER_UINT16(LWMsgUnionDefRep, arm_count),
    LWMSG_MEMBER_POINTER(LWMsgUnionDefRep, arms, LWMSG_TYPESPEC(arm_rep_spec)),
    LWMSG_ATTR_NOT_NULL,
    LWMSG_ATTR_LENGTH_MEMBER(LWMsgUnionDefRep, arm_count),
    LWMSG_MEMBER_PSTR(LWMsgUnionDefRep, name),
    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

static LWMsgTypeSpec variant_rep_spec[] =
{
    LWMSG_STRUCT_BEGIN(LWMsgVariantRep),
    LWMSG_MEMBER_TYPESPEC(LWMsgVariantRep, is_mask, bool_enum_spec),
    LWMSG_MEMBER_UINT64(LWMsgVariantRep, value),
    LWMSG_MEMBER_PSTR(LWMsgVariantRep, name),
    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

static LWMsgTypeSpec enum_def_rep_spec[] =
{
    LWMSG_STRUCT_BEGIN(LWMsgEnumDefRep),
    LWMSG_MEMBER_UINT8(LWMsgEnumDefRep, width),
    LWMSG_ATTR_RANGE(1, 8),
    LWMSG_MEMBER_TYPESPEC(LWMsgEnumDefRep, sign, sign_enum_spec),
    LWMSG_MEMBER_UINT16(LWMsgEnumDefRep, variant_count),
    LWMSG_MEMBER_POINTER(LWMsgEnumDefRep, variants, LWMSG_TYPESPEC(variant_rep_spec)),
    LWMSG_ATTR_NOT_NULL,
    LWMSG_ATTR_LENGTH_MEMBER(LWMsgEnumDefRep, variant_count),
    LWMSG_MEMBER_PSTR(LWMsgEnumDefRep, name),
    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

static LWMsgTypeSpec integer_rep_spec[] =
{
    LWMSG_STRUCT_BEGIN(LWMsgIntegerRep),
    LWMSG_MEMBER_POINTER(LWMsgIntegerRep, definition, LWMSG_TYPESPEC(integer_def_rep_spec)),
    LWMSG_ATTR_ALIASABLE,
    LWMSG_ATTR_NOT_NULL,
    LWMSG_MEMBER_UINT64(LWMsgIntegerRep, lower_bound),
    LWMSG_MEMBER_UINT64(LWMsgIntegerRep, upper_bound),
    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

static LWMsgTypeSpec struct_rep_spec[] =
{
    LWMSG_STRUCT_BEGIN(LWMsgStructRep),
    LWMSG_MEMBER_POINTER(LWMsgStructRep, definition, LWMSG_TYPESPEC(struct_def_rep_spec)),
    LWMSG_ATTR_ALIASABLE,
    LWMSG_ATTR_NOT_NULL,
    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

static LWMsgTypeSpec enum_rep_spec[] =
{
    LWMSG_STRUCT_BEGIN(LWMsgEnumRep),
    LWMSG_MEMBER_POINTER(LWMsgEnumRep, definition, LWMSG_TYPESPEC(enum_def_rep_spec)),
    LWMSG_ATTR_ALIASABLE,
    LWMSG_ATTR_NOT_NULL,
    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

static LWMsgTypeSpec union_rep_spec[] =
{
    LWMSG_STRUCT_BEGIN(LWMsgUnionRep),
    LWMSG_MEMBER_POINTER(LWMsgUnionRep, definition, LWMSG_TYPESPEC(union_def_rep_spec)),
    LWMSG_ATTR_ALIASABLE,
    LWMSG_ATTR_NOT_NULL,
    LWMSG_MEMBER_INT16(LWMsgUnionRep, discrim_member_index),
    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

static LWMsgTypeSpec pointer_rep_spec[] =
{
    LWMSG_STRUCT_BEGIN(LWMsgPointerRep),
    LWMSG_MEMBER_TYPESPEC(LWMsgPointerRep, pointee_type, lwmsg_type_rep_spec),
    LWMSG_ATTR_NOT_NULL,
    LWMSG_ATTR_ALIASABLE,
    LWMSG_MEMBER_TYPESPEC(LWMsgPointerRep, zero_terminated, bool_enum_spec),
    LWMSG_MEMBER_UINT32(LWMsgPointerRep, static_length),
    LWMSG_MEMBER_INT16(LWMsgPointerRep, length_member_index),
    LWMSG_MEMBER_PSTR(LWMsgPointerRep, encoding),
    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

static LWMsgTypeSpec array_rep_spec[] =
{
    LWMSG_STRUCT_BEGIN(LWMsgArrayRep),
    LWMSG_MEMBER_TYPESPEC(LWMsgArrayRep, element_type, lwmsg_type_rep_spec),
    LWMSG_ATTR_NOT_NULL,
    LWMSG_ATTR_ALIASABLE,
    LWMSG_MEMBER_TYPESPEC(LWMsgArrayRep, zero_terminated, bool_enum_spec),
    LWMSG_MEMBER_UINT8(LWMsgArrayRep, zero_terminated),
    LWMSG_MEMBER_UINT32(LWMsgArrayRep, static_length),
    LWMSG_MEMBER_INT16(LWMsgArrayRep, length_member_index),
    LWMSG_MEMBER_PSTR(LWMsgArrayRep, encoding),
    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

static LWMsgTypeSpec type_info_spec[] =
{
    LWMSG_UNION_BEGIN(LWMsgTypeRepInfo),
    LWMSG_MEMBER_TYPESPEC(LWMsgTypeRepInfo, integer_rep, integer_rep_spec),
    LWMSG_ATTR_TAG(LWMSG_KIND_INTEGER),
    LWMSG_MEMBER_TYPESPEC(LWMsgTypeRepInfo, enum_rep, enum_rep_spec),
    LWMSG_ATTR_TAG(LWMSG_KIND_ENUM),
    LWMSG_MEMBER_TYPESPEC(LWMsgTypeRepInfo, struct_rep, struct_rep_spec),
    LWMSG_ATTR_TAG(LWMSG_KIND_STRUCT),
    LWMSG_MEMBER_TYPESPEC(LWMsgTypeRepInfo, union_rep, union_rep_spec),
    LWMSG_ATTR_TAG(LWMSG_KIND_UNION),
    LWMSG_MEMBER_TYPESPEC(LWMsgTypeRepInfo, pointer_rep, pointer_rep_spec),
    LWMSG_ATTR_TAG(LWMSG_KIND_POINTER),
    LWMSG_MEMBER_TYPESPEC(LWMsgTypeRepInfo, array_rep, array_rep_spec),
    LWMSG_ATTR_TAG(LWMSG_KIND_ARRAY),
    LWMSG_MEMBER_VOID(LWMsgTypeRepInfo, void_rep),
    LWMSG_ATTR_TAG(LWMSG_KIND_VOID),
    LWMSG_UNION_END,
    LWMSG_TYPE_END
};

static LWMsgTypeSpec type_rep_spec[] =
{
    LWMSG_STRUCT_BEGIN(LWMsgTypeRep),
    LWMSG_MEMBER_TYPESPEC(LWMsgTypeRep, kind, kind_enum_spec),
    LWMSG_MEMBER_TYPESPEC(LWMsgTypeRep, flags, flags_enum_spec),
    LWMSG_MEMBER_TYPESPEC(LWMsgTypeRep, info, type_info_spec),
    LWMSG_ATTR_DISCRIM(LWMsgTypeRep, kind),
    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

LWMsgTypeSpec lwmsg_type_rep_spec[] =
{
    LWMSG_POINTER(LWMSG_TYPESPEC(type_rep_spec)),
    LWMSG_ATTR_ALIASABLE,
    LWMSG_TYPE_END
};

static
void*
lwmsg_type_rep_map_get_key_spec(
    const void* entry
    )
{
    return &((LWMsgTypeRepMapEntry*) entry)->spec;
}

static
size_t
lwmsg_type_rep_map_digest_spec(
    const void* key
    )
{
    size_t hash = 0;
    const struct SpecKey* spec = key;
    int i;

    hash = (size_t) spec->spec;

    for (i = 0; i < spec->kind; i++)
    {
        hash = hash * 31;
    }

    return hash;
}

static
LWMsgBool
lwmsg_type_rep_map_equal_spec(
    const void* key1,
    const void* key2
    )
{
    const struct SpecKey* spec1 = key1;
    const struct SpecKey* spec2 = key2;

    return spec1->kind == spec2->kind && spec1->spec == spec2->spec;
}

static
LWMsgStatus
lwmsg_type_rep_map_init(
    LWMsgTypeRepMap* map
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;

    if (!map->hash_by_spec.buckets)
    {
        BAIL_ON_ERROR(status = lwmsg_hash_init(
                          &map->hash_by_spec,
                          11,
                          lwmsg_type_rep_map_get_key_spec,
                          lwmsg_type_rep_map_digest_spec,
                          lwmsg_type_rep_map_equal_spec,
                          offsetof(LWMsgTypeRepMapEntry, ring)));
    }

error:

    return status;
}

static
LWMsgStatus
lwmsg_type_rep_map_find_spec(
    LWMsgTypeRepMap* map,
    enum SpecKind kind,
    LWMsgTypeSpec* spec,
    void** rep
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    struct SpecKey key = {kind, spec};
    LWMsgTypeRepMapEntry* entry = NULL;

    BAIL_ON_ERROR(status = lwmsg_type_rep_map_init(map));

    entry = lwmsg_hash_find_key(&map->hash_by_spec, &key);

    if (!entry)
    {
        BAIL_ON_ERROR(status = LWMSG_STATUS_NOT_FOUND);
    }

    *rep = entry->rep;

error:

    return status;
}

static
LWMsgStatus
lwmsg_type_rep_map_insert(
    LWMsgTypeRepMap* map,
    enum SpecKind kind,
    LWMsgTypeSpec* spec,
    void* rep
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    LWMsgTypeRepMapEntry* entry = NULL;

    BAIL_ON_ERROR(status = LWMSG_ALLOC(&entry));

    lwmsg_ring_init(&entry->ring);
    entry->spec.kind = kind;
    entry->spec.spec = spec;
    entry->rep = rep;

    lwmsg_hash_insert_entry(&map->hash_by_spec, entry);

done:

    return status;

error:

    if (entry)
    {
        free(entry);
    }

    goto done;
}

void
lwmsg_type_rep_map_destroy(
    LWMsgTypeRepMap* map
    )
{
    LWMsgHashIter iter = {0};
    LWMsgTypeRepMapEntry* entry = NULL;

    if (map->hash_by_spec.buckets)
    {
        lwmsg_hash_iter_begin(&map->hash_by_spec, &iter);
        while ((entry = lwmsg_hash_iter_next(&map->hash_by_spec, &iter)))
        {
            lwmsg_hash_remove_entry(&map->hash_by_spec, entry);
            free(entry);
        }
        lwmsg_hash_iter_end(&map->hash_by_spec, &iter);

        lwmsg_hash_destroy(&map->hash_by_spec);
    }
}

static
size_t
lwmsg_type_member_count(
    LWMsgTypeIter* iter
    )
{
    LWMsgTypeIter member;
    size_t count = 0;

    for (lwmsg_type_enter(iter, &member);
         lwmsg_type_valid(&member);
         lwmsg_type_next(&member))
    {
        count++;
    }

    return count;
}

static
const char*
lwmsg_type_name_suffix(
    const char* name
    )
{
    static const char* prefix_struct = "struct ";
    static const char* prefix_union = "union ";
    static const char* prefix_enum = "enum ";

    if (name == NULL)
    {
        return NULL;
    }
    else if (!strncmp(name, prefix_struct, strlen(prefix_struct)))
    {
        return name + strlen(prefix_struct);
    }
    else if (!strncmp(name, prefix_union, strlen(prefix_union)))
    {
        return name + strlen(prefix_union);
    }
    else if (!strncmp(name, prefix_enum, strlen(prefix_enum)))
    {
        return name + strlen(prefix_enum);
    }
    else
    {
        return name;
    }
}

static
LWMsgStatus
lwmsg_type_rep_from_integer(
    LWMsgTypeRepMap* map,
    LWMsgTypeIter* iter,
    LWMsgTypeRep* rep
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;

    BAIL_ON_ERROR(status = lwmsg_context_alloc(
                      map->context,
                      sizeof(*rep->info.integer_rep.definition),
                      (void**) (void*) &rep->info.integer_rep.definition));

    rep->info.integer_rep.definition->width = (uint8_t) iter->info.kind_integer.width;
    rep->info.integer_rep.definition->sign = (uint8_t) iter->info.kind_integer.sign;

    if (iter->attrs.flags & LWMSG_TYPE_FLAG_RANGE)
    {
        rep->info.integer_rep.lower_bound = iter->attrs.range_low;
        rep->info.integer_rep.upper_bound = iter->attrs.range_high;
    }

error:

    return status;
}

static
LWMsgStatus
lwmsg_type_rep_from_enum(
    LWMsgTypeRepMap* map,
    LWMsgTypeIter* iter,
    LWMsgTypeRep* rep
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    LWMsgTypeIter member;
    size_t count = lwmsg_type_member_count(iter);
    size_t i = 0;

    lwmsg_type_enter(iter, &member);

    status = lwmsg_type_rep_map_find_spec(
        map,
        SPEC_DEF,
        member.spec,
        (void**) (void*) &rep->info.enum_rep.definition);

    if (status == LWMSG_STATUS_NOT_FOUND)
    {
        BAIL_ON_ERROR(status = lwmsg_context_alloc(
                          map->context,
                          sizeof(*rep->info.enum_rep.definition),
                          (void**) (void*) &rep->info.enum_rep.definition));

        BAIL_ON_ERROR(status = lwmsg_type_rep_map_insert(
                          map,
                          SPEC_DEF,
                          member.spec,
                          rep->info.enum_rep.definition));

        BAIL_ON_ERROR(status = lwmsg_strdup(
                          map->context,
                          lwmsg_type_name_suffix(iter->meta.type_name),
                          &rep->info.enum_rep.definition->name));

        rep->info.enum_rep.definition->width = (uint8_t) iter->info.kind_integer.width;
        rep->info.enum_rep.definition->sign = (uint8_t) iter->info.kind_integer.sign;
        rep->info.enum_rep.definition->variant_count = (uint16_t) count;

        BAIL_ON_ERROR(status = lwmsg_context_alloc(
                          map->context,
                          count * sizeof(LWMsgVariantRep),
                          (void**) (void*) &rep->info.enum_rep.definition->variants));

        for (lwmsg_type_enter(iter, &member), i = 0;
             lwmsg_type_valid(&member);
             lwmsg_type_next(&member), i++)
        {
            rep->info.enum_rep.definition->variants[i].is_mask = member.info.kind_variant.is_mask;
            rep->info.enum_rep.definition->variants[i].value = member.tag;
            BAIL_ON_ERROR(status = lwmsg_strdup(map->context,
                                                member.meta.type_name,
                                                &rep->info.enum_rep.definition->variants[i].name));
        }
    }
    else
    {
        BAIL_ON_ERROR(status);
    }

error:

    return status;
}

static
LWMsgStatus
lwmsg_type_rep_from_struct(
    LWMsgTypeRepMap* map,
    LWMsgTypeIter* iter,
    LWMsgTypeRep* rep
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    size_t count = lwmsg_type_member_count(iter);
    size_t i = 0;
    LWMsgTypeIter member;
    LWMsgTypeIter* old_struct = NULL;

    lwmsg_type_enter(iter, &member);

    status = lwmsg_type_rep_map_find_spec(
        map,
        SPEC_DEF,
        member.spec,
        (void**) (void*) &rep->info.struct_rep.definition);

    if (status == LWMSG_STATUS_NOT_FOUND)
    {
        BAIL_ON_ERROR(status = lwmsg_context_alloc(
                          map->context,
                          sizeof(*rep->info.struct_rep.definition),
                          (void**) (void*) &rep->info.struct_rep.definition));

        BAIL_ON_ERROR(status = lwmsg_type_rep_map_insert(
                          map,
                          SPEC_DEF,
                          member.spec,
                          rep->info.struct_rep.definition));

        BAIL_ON_ERROR(status = lwmsg_strdup(
                          map->context,
                          lwmsg_type_name_suffix(iter->meta.type_name),
                          &rep->info.struct_rep.definition->name));

        rep->info.struct_rep.definition->field_count = (uint16_t) count;

        BAIL_ON_ERROR(status = lwmsg_context_alloc(
                          map->context,
                          count * sizeof(LWMsgFieldRep),
                          (void**) (void*) &rep->info.struct_rep.definition->fields));

        old_struct = map->dominating_iter;
        map->dominating_iter = iter;

        for (lwmsg_type_enter(iter, &member), i = 0;
             lwmsg_type_valid(&member);
             lwmsg_type_next(&member), i++)
        {
            BAIL_ON_ERROR(status = lwmsg_strdup(
                              map->context,
                              member.meta.member_name,
                              &rep->info.struct_rep.definition->fields[i].name));

            BAIL_ON_ERROR(status = lwmsg_type_rep_from_spec_internal(
                              map,
                              &member,
                              &rep->info.struct_rep.definition->fields[i].type));
        }

        map->dominating_iter = old_struct;
    }

error:

    return status;
}

static
LWMsgStatus
lwmsg_type_rep_from_union(
    LWMsgTypeRepMap* map,
    LWMsgTypeIter* iter,
    LWMsgTypeRep* rep
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    size_t count = lwmsg_type_member_count(iter);
    size_t i = 0;
    LWMsgTypeIter member;

    lwmsg_type_enter(iter, &member);

    status = lwmsg_type_rep_map_find_spec(
        map,
        SPEC_DEF,
        member.spec,
        (void**) (void*) &rep->info.struct_rep.definition);

    if (status == LWMSG_STATUS_NOT_FOUND)
    {
        BAIL_ON_ERROR(status = lwmsg_context_alloc(
                          map->context,
                          sizeof(*rep->info.union_rep.definition),
                          (void**) (void*) &rep->info.union_rep.definition));

        BAIL_ON_ERROR(status = lwmsg_type_rep_map_insert(
                          map,
                          SPEC_DEF,
                          member.spec,
                          rep->info.union_rep.definition));

        BAIL_ON_ERROR(status = lwmsg_strdup(
                          map->context,
                          lwmsg_type_name_suffix(iter->meta.type_name),
                          &rep->info.union_rep.definition->name));

        rep->info.union_rep.definition->arm_count = (uint16_t) count;
        rep->info.union_rep.discrim_member_index = -1;

        for (lwmsg_type_enter(map->dominating_iter, &member), i = 0;
             lwmsg_type_valid(&member);
             lwmsg_type_next(&member), i++)
        {
            if (member.offset == iter->info.kind_compound.discrim.offset)
            {
                rep->info.union_rep.discrim_member_index = i;
                break;
            }
        }

        BAIL_ON_ERROR(status = lwmsg_context_alloc(
                          map->context,
                          count * sizeof(LWMsgArmRep),
                          (void**) (void*) &rep->info.union_rep.definition->arms));

        for (lwmsg_type_enter(iter, &member), i = 0;
             lwmsg_type_valid(&member);
             lwmsg_type_next(&member), i++)
        {
            BAIL_ON_ERROR(status = lwmsg_strdup(
                              map->context,
                              member.meta.member_name,
                              &rep->info.union_rep.definition->arms[i].name));
            rep->info.union_rep.definition->arms[i].tag = member.tag;

            BAIL_ON_ERROR(status = lwmsg_type_rep_from_spec_internal(
                              map,
                              &member,
                              &rep->info.union_rep.definition->arms[i].type));
        }
    }

error:

    return status;
}

static
LWMsgStatus
lwmsg_type_rep_from_pointer(
    LWMsgTypeRepMap* map,
    LWMsgTypeIter* iter,
    LWMsgTypeRep* rep
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    LWMsgTypeIter inner;
    size_t i = 0;

    rep->info.pointer_rep.length_member_index = -1;

    BAIL_ON_ERROR(status = lwmsg_strdup(map->context, iter->info.kind_indirect.encoding, &rep->info.pointer_rep.encoding));

    switch (iter->info.kind_indirect.term)
    {
    case LWMSG_TERM_STATIC:
        rep->info.pointer_rep.static_length = iter->info.kind_indirect.term_info.static_length;
        break;
    case LWMSG_TERM_MEMBER:
        for (lwmsg_type_enter(map->dominating_iter, &inner);
             lwmsg_type_valid(&inner);
             lwmsg_type_next(&inner), i++)
        {
            if (inner.offset == iter->info.kind_indirect.term_info.member.offset)
            {
                rep->info.pointer_rep.length_member_index = i;
                break;
            }
        }
        break;
    case LWMSG_TERM_ZERO:
        rep->info.pointer_rep.zero_terminated = LWMSG_TRUE;
        break;
    }

    lwmsg_type_enter(iter, &inner);

    BAIL_ON_ERROR(status = lwmsg_type_rep_from_spec_internal(
                      map,
                      &inner,
                      &rep->info.pointer_rep.pointee_type));

error:

    return status;
}

static
LWMsgStatus
lwmsg_type_rep_from_array(
    LWMsgTypeRepMap* map,
    LWMsgTypeIter* iter,
    LWMsgTypeRep* rep
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    LWMsgTypeIter inner;
    size_t i = 0;

    rep->info.array_rep.length_member_index = -1;

    switch (iter->info.kind_indirect.term)
    {
    case LWMSG_TERM_STATIC:
        rep->info.array_rep.static_length = iter->info.kind_indirect.term_info.static_length;
        break;
    case LWMSG_TERM_MEMBER:
        for (lwmsg_type_enter(map->dominating_iter, &inner);
             lwmsg_type_valid(&inner);
             lwmsg_type_next(&inner), i++)
        {
            if (inner.offset == iter->info.kind_indirect.term_info.member.offset)
            {
                rep->info.array_rep.length_member_index = i;
                break;
            }
        }
        break;
    case LWMSG_TERM_ZERO:
        rep->info.array_rep.zero_terminated = LWMSG_TRUE;
        break;
    }

    lwmsg_type_enter(iter, &inner);

    BAIL_ON_ERROR(status = lwmsg_type_rep_from_spec_internal(
                      map,
                      &inner,
                      &rep->info.array_rep.element_type));

error:

    return status;
}

LWMsgStatus
lwmsg_type_rep_from_spec_internal(
    LWMsgTypeRepMap* map,
    LWMsgTypeIter* iter,
    LWMsgTypeRep** rep
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;

    /* Treat custom types as a special case and just return the rep of the transmitted type.
       This is because the type representation is only concerned with what can legally appear
       in the marshalled data stream. */
    if (iter->kind == LWMSG_KIND_CUSTOM)
    {
        LWMsgTypeIter transmit_iter;
        
        lwmsg_type_iterate(iter->info.kind_custom.typeclass->transmit_type, &transmit_iter);
        
        return lwmsg_type_rep_from_spec_internal(map, &transmit_iter, rep);
    }

    status = lwmsg_type_rep_map_find_spec(map, SPEC_TYPE, iter->spec, (void**) (void*) rep);

    if (status == LWMSG_STATUS_NOT_FOUND)
    {
        status = LWMSG_STATUS_SUCCESS;

        BAIL_ON_ERROR(status = lwmsg_context_alloc(
                          map->context,
                          sizeof(**rep),
                          (void**) (void*) rep));

        BAIL_ON_ERROR(status = lwmsg_type_rep_map_insert(map, SPEC_TYPE, iter->spec, *rep));

        (*rep)->kind = iter->kind;
        (*rep)->flags = iter->attrs.flags & ~(LWMSG_TYPE_FLAG_PROMOTED | LWMSG_TYPE_FLAG_SENSITIVE);

        switch (iter->kind)
        {
        case LWMSG_KIND_INTEGER:
            BAIL_ON_ERROR(status = lwmsg_type_rep_from_integer(
                              map,
                              iter,
                              *rep));
            break;
        case LWMSG_KIND_ENUM:
            BAIL_ON_ERROR(status = lwmsg_type_rep_from_enum(
                              map,
                              iter,
                              *rep));
            break;
        case LWMSG_KIND_STRUCT:
            BAIL_ON_ERROR(status = lwmsg_type_rep_from_struct(
                              map,
                              iter,
                              *rep));
            break;
        case LWMSG_KIND_UNION:
            BAIL_ON_ERROR(status = lwmsg_type_rep_from_union(
                              map,
                              iter,
                              *rep));
            break;
        case LWMSG_KIND_POINTER:
            BAIL_ON_ERROR(status = lwmsg_type_rep_from_pointer(
                              map,
                              iter,
                              *rep));
            break;
        case LWMSG_KIND_ARRAY:
            BAIL_ON_ERROR(status = lwmsg_type_rep_from_array(
                              map,
                              iter,
                              *rep));
            break;
        case LWMSG_KIND_VOID:
            break;
        default:
            BAIL_ON_ERROR(status = LWMSG_STATUS_MALFORMED);
            break;
        }
    }
    else
    {
        BAIL_ON_ERROR(status);
    }

error:

    return status;
}

LWMsgStatus
lwmsg_type_rep_from_spec(
    const LWMsgContext* context,
    LWMsgTypeSpec* spec,
    LWMsgTypeRep** rep
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    LWMsgTypeIter iter;
    LWMsgTypeRepMap map;

    memset(&map, 0, sizeof(map));

    map.context = context;

    lwmsg_type_iterate(spec, &iter);

    BAIL_ON_ERROR(status = lwmsg_type_rep_from_spec_internal(
                      &map,
                      &iter,
                      rep));

done:

    lwmsg_type_rep_map_destroy(&map);

    return status;

error:

    goto done;
}

static
void*
lwmsg_type_spec_map_get_key(
    const void* entry
    )
{
    return ((LWMsgTypeSpecBuffer*) entry)->rep;
}

static
size_t
lwmsg_type_spec_map_digest(
    const void* key
    )
{
    return (size_t) key;
}

static
LWMsgBool
lwmsg_type_spec_map_equal(
    const void* key1,
    const void* key2
    )
{
    return key1 == key2;
}

static
LWMsgStatus
lwmsg_type_spec_map_init(
    LWMsgTypeSpecMap* map
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;

    if (!map->hash_by_rep.buckets)
    {
        BAIL_ON_ERROR(status = lwmsg_hash_init(
                          &map->hash_by_rep,
                          11,
                          lwmsg_type_spec_map_get_key,
                          lwmsg_type_spec_map_digest,
                          lwmsg_type_spec_map_equal,
                          offsetof(LWMsgTypeSpecBuffer, ring)));
    }

error:

    return status;
}

static
LWMsgStatus
lwmsg_type_spec_map_find(
    LWMsgTypeSpecMap* map,
    void* rep,
    LWMsgTypeSpecBuffer** buffer
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    LWMsgTypeSpecBuffer* entry = NULL;

    BAIL_ON_ERROR(status = lwmsg_type_spec_map_init(map));
    
    entry = lwmsg_hash_find_key(&map->hash_by_rep, rep);

    if (!entry)
    {
        BAIL_ON_ERROR(status = LWMSG_STATUS_NOT_FOUND);
    }
    
    *buffer = entry;

error:

    return status;
}

static
void
lwmsg_type_spec_map_insert(
    LWMsgTypeSpecMap* map,
    LWMsgTypeSpecBuffer* buffer
    )
{
    lwmsg_hash_insert_entry(&map->hash_by_rep, buffer);
}

void
lwmsg_type_spec_map_destroy(
    LWMsgTypeSpecMap* map
    )
{
    LWMsgHashIter iter = {0};
    LWMsgTypeSpecBuffer* entry = NULL;
    LWMsgRing* ring = NULL;
    LWMsgRing* next = NULL;
    LWMsgTypeSpecLink* link = NULL;

    if (map->hash_by_rep.buckets)
    {
        lwmsg_hash_iter_begin(&map->hash_by_rep, &iter);
        while ((entry = lwmsg_hash_iter_next(&map->hash_by_rep, &iter)))
        {
            lwmsg_hash_remove_entry(&map->hash_by_rep, entry);
            
            for (ring = entry->backlinks.next;
                 ring != &entry->backlinks;
                 ring = next)
            {
                link = LWMSG_OBJECT_FROM_MEMBER(ring, LWMsgTypeSpecLink, ring);
                lwmsg_ring_remove(ring);
                free(link);
            }

            if (entry->member_metrics)
            {
                free(entry->member_metrics);
            }
            free(entry);
        }
        lwmsg_hash_iter_end(&map->hash_by_rep, &iter);

        lwmsg_hash_destroy(&map->hash_by_rep);
    }
}

static
LWMsgStatus
lwmsg_type_spec_buffer_new(
    LWMsgTypeSpecMap* map,
    void* rep,
    LWMsgTypeSpecBuffer** buffer
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    LWMsgTypeSpecBuffer* my_buffer = NULL;

    BAIL_ON_ERROR(status = LWMSG_ALLOC(&my_buffer));

    lwmsg_ring_init(&my_buffer->ring);
    lwmsg_ring_init(&my_buffer->backlinks);

    my_buffer->rep = rep;
    my_buffer->buffer_capacity = 8;
    
    BAIL_ON_ERROR(status = LWMSG_CONTEXT_ALLOC_ARRAY(
                      map->context,
                      my_buffer->buffer_capacity,
                      &my_buffer->buffer));

    *buffer = my_buffer;

done:

    return status;

error:

    if (my_buffer)
    {
        lwmsg_context_free(map->context, my_buffer);
    }

    goto done;
}

static
LWMsgStatus
lwmsg_type_spec_buffer_ensure_capacity(
    LWMsgTypeSpecMap* map,
    LWMsgTypeSpecBuffer* buffer,
    size_t additional
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    size_t* new_buffer = NULL;

    if (buffer->buffer_capacity < buffer->buffer_size + additional)
    {
        BAIL_ON_ERROR(status = lwmsg_context_realloc(
                          map->context,
                          buffer->buffer,
                          buffer->buffer_capacity * sizeof(size_t),
                          (buffer->buffer_size + additional) * sizeof(size_t),
                          (void**) (void*) &new_buffer));

        buffer->buffer_capacity = buffer->buffer_size + additional;
        buffer->buffer = new_buffer;
    }

error:

    return status;
}

static
LWMsgStatus
lwmsg_type_spec_buffer_append(
    LWMsgTypeSpecMap* map,
    LWMsgTypeSpecBuffer* buffer,
    size_t* values,
    unsigned int count
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;

    BAIL_ON_ERROR(status = lwmsg_type_spec_buffer_ensure_capacity(
                      map,
                      buffer,
                      count * sizeof(size_t)));

    memcpy(buffer->buffer + buffer->buffer_size,
           values,
           count * sizeof(size_t));

    buffer->buffer_size += count;

error:

    return status;
}

static
LWMsgStatus
lwmsg_type_spec_buffer_append_value(
    LWMsgTypeSpecMap* map,
    LWMsgTypeSpecBuffer* buffer,
    size_t value
    )
{
    return lwmsg_type_spec_buffer_append(map, buffer, &value, 1);
}

static
LWMsgStatus
lwmsg_type_spec_buffer_append_string(
    LWMsgTypeSpecMap* map,
    LWMsgTypeSpecBuffer* buffer,
    const char* string
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    char* copy = NULL;

    BAIL_ON_ERROR(status = lwmsg_strdup(map->context, string, &copy));
    BAIL_ON_ERROR(status = lwmsg_type_spec_buffer_append_value(map, buffer, (size_t) copy));

done:

    return status;

error:

    if (copy)
    {
        lwmsg_context_free(map->context, copy);
    }

    goto done;
}

static
LWMsgStatus
lwmsg_type_spec_buffer_link(
    LWMsgTypeSpecMap* map,
    LWMsgTypeSpecBuffer* from,
    LWMsgTypeSpecBuffer* to
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    LWMsgTypeSpecLink* link = NULL;

    if (to->buffer_capacity)
    {
        BAIL_ON_ERROR(status = LWMSG_ALLOC(&link));
        
        lwmsg_ring_init(&link->ring);
        
        link->buffer = from;
        link->offset = from->buffer_size;
        
        lwmsg_ring_enqueue(&to->backlinks, &link->ring);

        BAIL_ON_ERROR(status = lwmsg_type_spec_buffer_append_value(
                          map,
                          from,
                          (size_t) -1));

        map->backlinks++;
    }
    else
    {
        BAIL_ON_ERROR(status = lwmsg_type_spec_buffer_append_value(
                          map,
                          from,
                          (size_t) to->buffer));
    }

error:

    return status;
}

static
void
lwmsg_type_spec_buffer_finalize(
    LWMsgTypeSpecMap* map,
    LWMsgTypeSpecBuffer* buffer
    )
{
    LWMsgRing* ring = NULL;
    LWMsgRing* next = NULL;
    LWMsgTypeSpecLink* link = NULL;

    for (ring = buffer->backlinks.next;
         ring != &buffer->backlinks;
         ring = next)
    {
        next = ring->next;
        link = LWMSG_OBJECT_FROM_MEMBER(ring, LWMsgTypeSpecLink, ring);

        LWMSG_ASSERT(link->buffer->buffer[link->offset] == (size_t) -1);
        link->buffer->buffer[link->offset] = (size_t) buffer->buffer;

        lwmsg_ring_remove(ring);
        free(link);

        map->backlinks--;
    }

    buffer->buffer_capacity = 0;
}

static
LWMsgStatus
lwmsg_type_spec_from_common_attrs(
    LWMsgTypeSpecState* state,
    LWMsgTypeRep* rep,
    LWMsgTypeSpecBuffer* buffer
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;

    if (rep->flags & LWMSG_TYPE_FLAG_NOT_NULL)
    {
        BAIL_ON_ERROR(status = lwmsg_type_spec_buffer_append_value(
                          state->map,
                          buffer,
                          LWMSG_CMD_NOT_NULL));
    }

error:

    return status;
}

static
void
align_to_width(
    ssize_t* offset,
    size_t width
    )
{
    if (*offset % width)
    {
        *offset += (width - *offset % width);
    }
}

static
LWMsgStatus
lwmsg_type_spec_from_spec(
    LWMsgTypeSpecState* state,
    LWMsgTypeSpecBuffer* spec,
    LWMsgTypeSpecBuffer* buffer
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    size_t cmd = 0;

    cmd = LWMSG_CMD_TYPESPEC;
    
    if (state->member_offset >= 0)
    {
        cmd |= LWMSG_FLAG_MEMBER;
    }

    if (state->member_name)
    {
        cmd |= LWMSG_FLAG_META;
    }

    BAIL_ON_ERROR(status = lwmsg_type_spec_buffer_append_value(
                      state->map,
                      buffer,
                      cmd));

    if (cmd & LWMSG_FLAG_META)
    {
        BAIL_ON_ERROR(status = lwmsg_type_spec_buffer_append_string(
                          state->map,
                          buffer,
                          state->member_name));
    }

    if (cmd & LWMSG_FLAG_MEMBER)
    {
        align_to_width(&state->member_offset, spec->type_size);

        BAIL_ON_ERROR(status = lwmsg_type_spec_buffer_append_value(
                          state->map,
                          buffer,
                          spec->type_size));

        BAIL_ON_ERROR(status = lwmsg_type_spec_buffer_append_value(
                          state->map,
                          buffer,
                          state->member_offset));

        state->member_size = spec->type_size;
    }

    BAIL_ON_ERROR(status = lwmsg_type_spec_buffer_link(
                      state->map,
                      buffer,
                      spec));

    buffer->type_size = spec->type_size;

error:
    
    return status;
}

static
LWMsgStatus
lwmsg_type_spec_from_integer(
    LWMsgTypeSpecState* state,
    LWMsgTypeRep* rep,
    LWMsgTypeSpecBuffer* buffer
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    size_t cmd = 0;
    size_t spec[3];

    cmd = LWMSG_CMD_INTEGER;

    if (state->member_offset >= 0)
    {
        cmd |= LWMSG_FLAG_MEMBER;
    }

    if (state->member_name)
    {
        cmd |= LWMSG_FLAG_META;
    }

    BAIL_ON_ERROR(status = lwmsg_type_spec_buffer_append_value(
                      state->map,
                      buffer,
                      cmd));

    if (cmd & LWMSG_FLAG_META)
    {
        BAIL_ON_ERROR(status = lwmsg_type_spec_buffer_append_string(
                          state->map,
                          buffer,
                          state->member_name));
    }

    if (cmd & LWMSG_FLAG_MEMBER)
    {
        align_to_width(&state->member_offset, rep->info.integer_rep.definition->width);

        BAIL_ON_ERROR(status = lwmsg_type_spec_buffer_append_value(
                          state->map,
                          buffer,
                          rep->info.integer_rep.definition->width));

        BAIL_ON_ERROR(status = lwmsg_type_spec_buffer_append_value(
                          state->map,
                          buffer,
                          state->member_offset));

        state->member_size = rep->info.integer_rep.definition->width;
    }
    else
    {
        BAIL_ON_ERROR(status = lwmsg_type_spec_buffer_append_value(
                          state->map,
                          buffer,
                          rep->info.integer_rep.definition->width)); 
    }

    spec[0] = rep->info.integer_rep.definition->width;
    spec[1] = rep->info.integer_rep.definition->sign;

    BAIL_ON_ERROR(status = lwmsg_type_spec_buffer_append(
                      state->map,
                      buffer,
                      spec,
                      2));

    if (rep->flags & LWMSG_TYPE_FLAG_RANGE)
    {
        spec[0] = LWMSG_CMD_RANGE;
        spec[1] = (size_t) rep->info.integer_rep.lower_bound;
        spec[2] = (size_t) rep->info.integer_rep.upper_bound;

        BAIL_ON_ERROR(status = lwmsg_type_spec_buffer_append(
                          state->map,
                          buffer,
                          spec,
                          3));
    }

    buffer->type_size = rep->info.integer_rep.definition->width;
  
error:
    
    return status;
}

static
LWMsgStatus
lwmsg_type_spec_from_enum(
    LWMsgTypeSpecState* state,
    LWMsgTypeRep* rep,
    LWMsgTypeSpecBuffer* buffer
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    unsigned int i = 0;
    LWMsgTypeSpecBuffer* defbuf = NULL;
    LWMsgEnumDefRep* def = rep->info.enum_rep.definition;
    size_t cmd = 0;
    size_t spec[3];

    if ((status = lwmsg_type_spec_map_find(
             state->map,
             def,
             &defbuf)) != LWMSG_STATUS_NOT_FOUND)
    {
        BAIL_ON_ERROR(status);
    }
    else
    {
        BAIL_ON_ERROR(status = lwmsg_type_spec_buffer_new(
                          state->map,
                          def,
                          &defbuf));
        
        lwmsg_type_spec_map_insert(state->map, defbuf);
        
        cmd = LWMSG_CMD_ENUM;
        
        if (def->name)
        {
            cmd |= LWMSG_FLAG_META;
        }
        
        BAIL_ON_ERROR(status = lwmsg_type_spec_buffer_append_value(
                          state->map,
                          defbuf,
                          cmd));
        
        if (cmd & LWMSG_FLAG_META)
        {
            BAIL_ON_ERROR(status = lwmsg_type_spec_buffer_append_string(
                              state->map,
                              defbuf,
                              def->name));
        }
        
        spec[0] = def->width;
        spec[1] = def->width;
        spec[2] = def->sign;

        BAIL_ON_ERROR(status = lwmsg_type_spec_buffer_append(
                          state->map,
                          defbuf,
                          spec,
                          3));

        for (i = 0; i < def->variant_count; i++)
        {
            cmd = def->variants[i].is_mask ? LWMSG_CMD_ENUM_MASK : LWMSG_CMD_ENUM_VALUE;
            
            if (def->variants[i].name)
            {
                cmd |= LWMSG_FLAG_META;
            }
            
            BAIL_ON_ERROR(status = lwmsg_type_spec_buffer_append_value(
                              state->map,
                              defbuf,
                              cmd));
            
            if (cmd & LWMSG_FLAG_META)
            {
                BAIL_ON_ERROR(status = lwmsg_type_spec_buffer_append_string(
                                  state->map,
                                  defbuf,
                                  def->variants[i].name));
            }

            BAIL_ON_ERROR(status = lwmsg_type_spec_buffer_append_value(
                              state->map,
                              defbuf,
                              (size_t) def->variants[i].value));
        }
        
        BAIL_ON_ERROR(status = lwmsg_type_spec_buffer_append_value(
                          state->map,
                          defbuf,
                          LWMSG_CMD_END));

        lwmsg_type_spec_buffer_finalize(state->map, defbuf);

        defbuf->type_size = def->width;
    }

    BAIL_ON_ERROR(status = lwmsg_type_spec_from_spec(
                      state,
                      defbuf,
                      buffer));

error:

    return status;
}

static
LWMsgStatus
lwmsg_type_spec_from_struct(
    LWMsgTypeSpecState* state,
    LWMsgTypeRep* rep,
    LWMsgTypeSpecBuffer* buffer
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    unsigned int i = 0;
    LWMsgTypeSpecBuffer* defbuf = NULL;
    LWMsgStructDefRep* def = rep->info.struct_rep.definition;
    size_t cmd = 0;

    if ((status = lwmsg_type_spec_map_find(
             state->map,
             def,
             &defbuf)) != LWMSG_STATUS_NOT_FOUND)
    {
        BAIL_ON_ERROR(status);
    }
    else
    {
        LWMsgTypeSpecState my_state = {0};
        size_t size_offset = 0;

        BAIL_ON_ERROR(status = lwmsg_type_spec_buffer_new(
                          state->map,
                          def,
                          &defbuf));
        
        lwmsg_type_spec_map_insert(state->map, defbuf);

        BAIL_ON_ERROR(status = LWMSG_ALLOC_ARRAY(def->field_count, &defbuf->member_metrics));

        my_state.map = state->map;
        my_state.member_offset = 0;
        my_state.member_name = NULL;
        my_state.dominating_struct = defbuf;
     
        cmd = LWMSG_CMD_STRUCT;
        
        if (def->name)
        {
            cmd |= LWMSG_FLAG_META;
        }
        
        BAIL_ON_ERROR(status = lwmsg_type_spec_buffer_append_value(
                          state->map,
                          defbuf,
                          cmd));
        
        if (cmd & LWMSG_FLAG_META)
        {
            BAIL_ON_ERROR(status = lwmsg_type_spec_buffer_append_string(
                              state->map,
                              defbuf,
                              def->name));
        }

        /* Remember location where size of structure should be written
           and put a placeholder 0 in there for now */
        size_offset = defbuf->buffer_size;

        BAIL_ON_ERROR(status = lwmsg_type_spec_buffer_append_value(
                          state->map,
                          defbuf,
                          0));
        
        for (i = 0; i < def->field_count; i++)
        {
            my_state.member_name = def->fields[i].name;
            
            BAIL_ON_ERROR(status = lwmsg_type_spec_from_rep_into(
                              &my_state,
                              def->fields[i].type,
                              defbuf));
            
            defbuf->member_metrics[i].offset = my_state.member_offset;
            defbuf->member_metrics[i].size = my_state.member_size;
            my_state.member_offset += my_state.member_size;
        }
        
        BAIL_ON_ERROR(status = lwmsg_type_spec_buffer_append_value(
                          state->map,
                          defbuf,
                          LWMSG_CMD_END));

        lwmsg_type_spec_buffer_finalize(state->map, defbuf);

        defbuf->type_size = my_state.member_offset;

        /* Write structure size to correct location */
        defbuf->buffer[size_offset] = defbuf->type_size;
    }

    BAIL_ON_ERROR(status = lwmsg_type_spec_from_spec(
                      state,
                      defbuf,
                      buffer));

error:

    return status;
}

static
LWMsgStatus
lwmsg_type_spec_from_union(
    LWMsgTypeSpecState* state,
    LWMsgTypeRep* rep,
    LWMsgTypeSpecBuffer* buffer
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    unsigned int i = 0;
    LWMsgTypeSpecBuffer* defbuf = NULL;
    LWMsgUnionDefRep* def = rep->info.union_rep.definition;
    size_t cmd = 0;
    size_t spec[3];

    if ((status = lwmsg_type_spec_map_find(
             state->map,
             def,
             &defbuf)) != LWMSG_STATUS_NOT_FOUND)
    {
        BAIL_ON_ERROR(status);
    }
    else
    {
        LWMsgTypeSpecState my_state = {0};
        size_t size = 0;
        size_t size_offset = 0;

        my_state.map = state->map;
        my_state.member_offset = 0;
        my_state.member_name = NULL;

        BAIL_ON_ERROR(status = lwmsg_type_spec_buffer_new(
                          state->map,
                          def,
                          &defbuf));
        
        lwmsg_type_spec_map_insert(state->map, defbuf);
        
        cmd = LWMSG_CMD_UNION;
        
        if (def->name)
        {
            cmd |= LWMSG_FLAG_META;
        }
        
        BAIL_ON_ERROR(status = lwmsg_type_spec_buffer_append_value(
                          state->map,
                          defbuf,
                          cmd));
        
        if (cmd & LWMSG_FLAG_META)
        {
            BAIL_ON_ERROR(status = lwmsg_type_spec_buffer_append_string(
                              state->map,
                              defbuf,
                              def->name));
        }

        /* Remember location where size of union should be written
           and put a placeholder 0 in there for now */
        size_offset = defbuf->buffer_size;

        BAIL_ON_ERROR(status = lwmsg_type_spec_buffer_append_value(
                          state->map,
                          defbuf,
                          0));
        
        for (i = 0; i < def->arm_count; i++)
        {
            my_state.member_name = def->arms[i].name;
            
            BAIL_ON_ERROR(status = lwmsg_type_spec_from_rep_into(
                              &my_state,
                              def->arms[i].type,
                              defbuf));

            if (size < my_state.member_size)
            {
                size = my_state.member_size;
            }

            spec[0] = LWMSG_CMD_TAG;
            spec[1] = def->arms[i].tag;
            
            BAIL_ON_ERROR(status = lwmsg_type_spec_buffer_append(
                              state->map,
                              defbuf,
                              spec,
                              2));
        }
        
        BAIL_ON_ERROR(status = lwmsg_type_spec_buffer_append_value(
                          state->map,
                          defbuf,
                          LWMSG_CMD_END));

        lwmsg_type_spec_buffer_finalize(state->map, defbuf);

        defbuf->type_size = size;

        /* Write union size to correct location */
        defbuf->buffer[size_offset] = defbuf->type_size;
    }

    BAIL_ON_ERROR(status = lwmsg_type_spec_from_spec(
                      state,
                      defbuf,
                      buffer));

    spec[0] = LWMSG_CMD_DISCRIM;
    spec[1] = state->dominating_struct->member_metrics[rep->info.union_rep.discrim_member_index].offset;
    spec[2] = state->dominating_struct->member_metrics[rep->info.union_rep.discrim_member_index].size;
    
    BAIL_ON_ERROR(status = lwmsg_type_spec_buffer_append(
                      state->map,
                      buffer,
                      spec,
                      3));

error:

    return status;
}

static
LWMsgStatus
lwmsg_type_spec_from_pointer(
    LWMsgTypeSpecState* state,
    LWMsgTypeRep* rep,
    LWMsgTypeSpecBuffer* buffer
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    LWMsgTypeSpecBuffer* pointeebuf = NULL;
    size_t cmd = 0;
    size_t spec[4];
    LWMsgTypeSpecState my_state = {0};

    my_state.map = state->map;
    my_state.member_offset = -1;

    buffer->type_size = sizeof(void*);

    cmd = LWMSG_CMD_POINTER;
    
    if (state->member_offset >= 0)
    {
        cmd |= LWMSG_FLAG_MEMBER;
    }

    if (state->member_name)
    {
        cmd |= LWMSG_FLAG_META;
    }

    BAIL_ON_ERROR(status = lwmsg_type_spec_buffer_append_value(
                      state->map,
                      buffer,
                      cmd));

    if (cmd & LWMSG_FLAG_META)
    {
        BAIL_ON_ERROR(status = lwmsg_type_spec_buffer_append_string(
                          state->map,
                          buffer,
                          state->member_name));
    }
    
    if (cmd & LWMSG_FLAG_MEMBER)
    {
        align_to_width(&state->member_offset, sizeof(void*));

        BAIL_ON_ERROR(status = lwmsg_type_spec_buffer_append_value(
                          state->map,
                          buffer,
                          state->member_offset));

        state->member_size = sizeof(void*);
    }

    BAIL_ON_ERROR(status = lwmsg_type_spec_from_rep_internal(
                      state->map,
                      rep->info.pointer_rep.pointee_type,
                      &pointeebuf));

    BAIL_ON_ERROR(status = lwmsg_type_spec_from_spec(
                      &my_state,
                      pointeebuf,
                      buffer));
    
    BAIL_ON_ERROR(status = lwmsg_type_spec_buffer_append_value(
                      state->map,
                      buffer,
                      LWMSG_CMD_END));

    if (rep->info.pointer_rep.zero_terminated)
    {
        spec[0] = LWMSG_CMD_TERMINATION;
        spec[1] = LWMSG_TERM_ZERO;

        BAIL_ON_ERROR(status = lwmsg_type_spec_buffer_append(
                          state->map,
                          buffer,
                          spec,
                          2));
    }

    if (rep->info.pointer_rep.static_length)
    {
        spec[0] = LWMSG_CMD_TERMINATION;
        spec[1] = LWMSG_TERM_STATIC;
        spec[2] = rep->info.pointer_rep.static_length;

        BAIL_ON_ERROR(status = lwmsg_type_spec_buffer_append(
                          state->map,
                          buffer,
                          spec,
                          3));
    }

    if (rep->info.pointer_rep.length_member_index >= 0)
    {
        spec[0] = LWMSG_CMD_TERMINATION;
        spec[1] = LWMSG_TERM_MEMBER;
        spec[2] = state->dominating_struct->member_metrics[rep->info.pointer_rep.length_member_index].offset;
        spec[3] = state->dominating_struct->member_metrics[rep->info.pointer_rep.length_member_index].size;
        
        BAIL_ON_ERROR(status = lwmsg_type_spec_buffer_append(
                          state->map,
                          buffer,
                          spec,
                          4));
    }

    if (rep->info.pointer_rep.encoding)
    {
        BAIL_ON_ERROR(status = lwmsg_type_spec_buffer_append_value(
                          state->map,
                          buffer,
                          LWMSG_CMD_ENCODING));

        BAIL_ON_ERROR(status = lwmsg_type_spec_buffer_append_string(
                          state->map,
                          buffer,
                          rep->info.pointer_rep.encoding));
    }

    buffer->type_size = sizeof(void*);

error:

    return status;
}

static
LWMsgStatus
lwmsg_type_spec_from_array(
    LWMsgTypeSpecState* state,
    LWMsgTypeRep* rep,
    LWMsgTypeSpecBuffer* buffer
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    LWMsgTypeSpecBuffer* elementbuf = NULL;
    size_t cmd = 0;
    size_t spec[4];

    cmd = LWMSG_CMD_ARRAY;
    
    if (state->member_offset >= 0)
    {
        cmd |= LWMSG_FLAG_MEMBER;
    }

    if (state->member_name)
    {
        cmd |= LWMSG_FLAG_META;
    }

    BAIL_ON_ERROR(status = lwmsg_type_spec_buffer_append_value(
                      state->map,
                      buffer,
                      cmd));

    if (cmd & LWMSG_FLAG_META)
    {
        BAIL_ON_ERROR(status = lwmsg_type_spec_buffer_append_string(
                          state->map,
                          buffer,
                          state->member_name));
    }
    
    if (cmd & LWMSG_FLAG_MEMBER)
    {
        align_to_width(&state->member_offset, sizeof(void*));

        BAIL_ON_ERROR(status = lwmsg_type_spec_buffer_append_value(
                          state->map,
                          buffer,
                          state->member_offset));

        state->member_size = sizeof(void*);
    }

    BAIL_ON_ERROR(status = lwmsg_type_spec_from_rep_internal(
                      state->map,
                      rep->info.array_rep.element_type,
                      &elementbuf));

    BAIL_ON_ERROR(status = lwmsg_type_spec_from_spec(
                      state,
                      elementbuf,
                      buffer));

    BAIL_ON_ERROR(status = lwmsg_type_spec_buffer_append_value(
                      state->map,
                      buffer,
                      LWMSG_CMD_END));

    if (rep->info.array_rep.zero_terminated)
    {
        spec[0] = LWMSG_CMD_TERMINATION;
        spec[1] = LWMSG_TERM_ZERO;

        BAIL_ON_ERROR(status = lwmsg_type_spec_buffer_append(
                          state->map,
                          buffer,
                          spec,
                          2));
    }

    if (rep->info.array_rep.static_length)
    {
        spec[0] = LWMSG_CMD_TERMINATION;
        spec[1] = LWMSG_TERM_STATIC;
        spec[2] = rep->info.array_rep.static_length;

        BAIL_ON_ERROR(status = lwmsg_type_spec_buffer_append(
                          state->map,
                          buffer,
                          spec,
                          3));
    }

    if (rep->info.array_rep.length_member_index >= 0)
    {
        spec[0] = LWMSG_CMD_TERMINATION;
        spec[1] = LWMSG_TERM_MEMBER;
        spec[2] = state->dominating_struct->member_metrics[rep->info.pointer_rep.length_member_index].offset;
        spec[3] = state->dominating_struct->member_metrics[rep->info.pointer_rep.length_member_index].size;
        
        BAIL_ON_ERROR(status = lwmsg_type_spec_buffer_append(
                          state->map,
                          buffer,
                          spec,
                          4));
    }

    if (rep->info.array_rep.encoding)
    {
        spec[0] = LWMSG_CMD_ENCODING;
        spec[1] = (size_t) rep->info.array_rep.encoding;

        BAIL_ON_ERROR(status = lwmsg_type_spec_buffer_append(
                          state->map,
                          buffer,
                          spec,
                          2));
    }

error:

    return status;
}

static
LWMsgStatus
lwmsg_type_spec_from_void(
    LWMsgTypeSpecState* state,
    LWMsgTypeRep* rep,
    LWMsgTypeSpecBuffer* buffer
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    size_t cmd = 0;

    cmd = LWMSG_CMD_VOID;

    if (state->member_offset >= 0)
    {
        cmd |= LWMSG_FLAG_MEMBER;
    }

    if (state->member_name)
    {
        cmd |= LWMSG_FLAG_META;
    }

    BAIL_ON_ERROR(status = lwmsg_type_spec_buffer_append_value(
                      state->map,
                      buffer,
                      cmd));

    if (cmd & LWMSG_FLAG_META)
    {
        BAIL_ON_ERROR(status = lwmsg_type_spec_buffer_append_string(
                          state->map,
                          buffer,
                          state->member_name));
    }

    buffer->type_size = 0;
  
error:
    
    return status;
}

static
LWMsgStatus
lwmsg_type_spec_from_rep_into(
    LWMsgTypeSpecState* state,
    LWMsgTypeRep* rep,
    LWMsgTypeSpecBuffer* buffer
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    
    switch (rep->kind)
    {
    case LWMSG_KIND_INTEGER:
        BAIL_ON_ERROR(status = lwmsg_type_spec_from_integer(
                          state,
                          rep,
                          buffer));
        break;
    case LWMSG_KIND_ENUM:
        BAIL_ON_ERROR(status = lwmsg_type_spec_from_enum(
                          state,
                          rep,
                          buffer));
        break;
    case LWMSG_KIND_STRUCT:
        BAIL_ON_ERROR(status = lwmsg_type_spec_from_struct(
                          state,
                          rep,
                          buffer));
        break;
    case LWMSG_KIND_UNION:
        BAIL_ON_ERROR(status = lwmsg_type_spec_from_union(
                          state,
                          rep,
                          buffer));
        break;
    case LWMSG_KIND_POINTER:
        BAIL_ON_ERROR(status = lwmsg_type_spec_from_pointer(
                          state,
                          rep,
                          buffer));
        break;
    case LWMSG_KIND_ARRAY:
        BAIL_ON_ERROR(status = lwmsg_type_spec_from_array(
                          state,
                          rep,
                          buffer));
        break;
    case LWMSG_KIND_VOID:
        BAIL_ON_ERROR(status = lwmsg_type_spec_from_void(
                          state,
                          rep,
                          buffer));
        break;
    default:
        BAIL_ON_ERROR(status = LWMSG_STATUS_MALFORMED);
        break;
    }

    BAIL_ON_ERROR(status = lwmsg_type_spec_from_common_attrs(state, rep, buffer));

error:

    return status;
}

LWMsgStatus
lwmsg_type_spec_from_rep_internal(
    LWMsgTypeSpecMap* map,
    LWMsgTypeRep* rep,
    LWMsgTypeSpecBuffer** buffer
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    LWMsgTypeSpecBuffer* entry = NULL;
    LWMsgTypeSpecState state = {0};

    state.map = map;
    state.member_offset = -1;
    state.member_name = NULL;

    if ((status = lwmsg_type_spec_map_find(map, rep, &entry)) != LWMSG_STATUS_NOT_FOUND)
    {
        BAIL_ON_ERROR(status);

        *buffer = entry;
    }
    else
    {
        BAIL_ON_ERROR(status = lwmsg_type_spec_buffer_new(
                          map,
                          rep,
                          &entry));

        lwmsg_type_spec_map_insert(map, entry);

        BAIL_ON_ERROR(status = lwmsg_type_spec_from_rep_into(
                          &state,
                          rep,
                          entry));

        BAIL_ON_ERROR(status = lwmsg_type_spec_buffer_append_value(
                          map,
                          entry,
                          LWMSG_CMD_END));

        lwmsg_type_spec_buffer_finalize(map, entry);
        
        *buffer = entry;
    }
    
error:

    return status;
}

LWMsgStatus
lwmsg_type_spec_from_rep(
    const LWMsgContext* context,
    LWMsgTypeRep* rep,
    LWMsgTypeSpec** spec
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    LWMsgTypeSpecMap map = {0};
    LWMsgTypeSpecBuffer* buffer = NULL;

    map.context = context;

    BAIL_ON_ERROR(status = lwmsg_type_spec_from_rep_internal(
                      &map,
                      rep,
                      &buffer));

    *spec = buffer->buffer;

    LWMSG_ASSERT(map.backlinks == 0);

error:

    lwmsg_type_spec_map_destroy(&map);

    return status;
}

void
lwmsg_type_free_rep(
    const LWMsgContext* context,
    LWMsgTypeRep* rep
    )
{
    if (rep)
    {
        lwmsg_data_free_graph_cleanup(context, lwmsg_type_rep_spec, rep);
    }
}


static
void*
lwmsg_type_assign_map_get_key(
    const void* entry
    )
{
    return &((LWMsgTypeAssignEntry*) entry)->pair;
}

static
size_t
lwmsg_type_assign_map_digest(
    const void* key
    )
{
    const LWMsgTypeAssignPair* pair = key;

    return (size_t) pair->left ^ (size_t) pair->right;
}

static
LWMsgBool
lwmsg_type_assign_map_equal(
    const void* key1,
    const void* key2
    )
{
    const LWMsgTypeAssignPair* pair1 = key1;
    const LWMsgTypeAssignPair* pair2 = key2;

    return pair1->left == pair2->left && pair1->right == pair2->right;
}

static
LWMsgStatus
lwmsg_type_assign_map_init(
    LWMsgTypeAssignMap* map
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;

    if (!map->hash.buckets)
    {
        BAIL_ON_ERROR(status = lwmsg_hash_init(
                          &map->hash,
                          11,
                          lwmsg_type_assign_map_get_key,
                          lwmsg_type_assign_map_digest,
                          lwmsg_type_assign_map_equal,
                          offsetof(LWMsgTypeAssignEntry, ring)));
    }

error:

    return status;
}

static
LWMsgStatus
lwmsg_type_assign_map_find(
    LWMsgTypeAssignMap* map,
    LWMsgTypeRep* left,
    LWMsgTypeRep* right
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    LWMsgTypeAssignPair pair = {left, right};
    LWMsgTypeAssignEntry* entry = NULL;

    BAIL_ON_ERROR(status = lwmsg_type_assign_map_init(map));
    
    entry = lwmsg_hash_find_key(&map->hash, &pair);

    if (!entry)
    {
        BAIL_ON_ERROR(status = LWMSG_STATUS_NOT_FOUND);
    }

error:

    return status;
}

static
LWMsgStatus
lwmsg_type_assign_map_insert(
    LWMsgTypeAssignMap* map,
    LWMsgTypeRep* left,
    LWMsgTypeRep* right
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    LWMsgTypeAssignEntry* entry = NULL;
    
    BAIL_ON_ERROR(status = LWMSG_ALLOC(&entry));

    lwmsg_ring_init(&entry->ring);
    entry->pair.left = left;
    entry->pair.right = right;

    lwmsg_hash_insert_entry(&map->hash, entry);

error:

    return status;
}

static
void
lwmsg_type_assign_map_remove(
    LWMsgTypeAssignMap* map,
    LWMsgTypeRep* left,
    LWMsgTypeRep* right
    )
{
    LWMsgTypeAssignPair pair = {left, right};
    
    lwmsg_hash_remove_key(&map->hash, &pair);
}

static
void
lwmsg_type_assign_map_destroy(
    LWMsgTypeAssignMap* map
    )
{
    LWMsgHashIter iter = {0};
    LWMsgTypeAssignEntry* entry = NULL;

    if (map->hash.buckets)
    {
        lwmsg_hash_iter_begin(&map->hash, &iter);
        while ((entry = lwmsg_hash_iter_next(&map->hash, &iter)))
        {
            lwmsg_hash_remove_entry(&map->hash, entry);           
            free(entry);
        }
        lwmsg_hash_iter_end(&map->hash, &iter);

        lwmsg_hash_destroy(&map->hash);
    }
}

static
LWMsgStatus
lwmsg_type_rep_is_assignable_integer(
    LWMsgTypeAssignMap* map,
    LWMsgTypeRep* left,
    LWMsgTypeRep* right
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    LWMsgIntegerDefRep* ldef = left->info.integer_rep.definition;
    LWMsgIntegerDefRep* rdef = right->info.integer_rep.definition;

    if (ldef->width != rdef->width || ldef->sign != rdef->sign)
    {
        /* Basic characteristics don't match */
        BAIL_ON_ERROR(status = LWMSG_STATUS_MALFORMED);
    }

    if (left->flags & LWMSG_TYPE_FLAG_RANGE)
    {
        if (!(right->flags & LWMSG_TYPE_FLAG_RANGE))
        {
            /* The left side has a range restriction that the right side lacks */
            BAIL_ON_ERROR(status = LWMSG_STATUS_MALFORMED);
        }
        else if ((ldef->sign == LWMSG_UNSIGNED &&
                  (left->info.integer_rep.lower_bound > right->info.integer_rep.lower_bound ||
                   left->info.integer_rep.upper_bound < right->info.integer_rep.upper_bound)) ||
                 (ldef->sign == LWMSG_SIGNED &&
                  ((int64_t) left->info.integer_rep.lower_bound > (int64_t) right->info.integer_rep.lower_bound ||
                   (int64_t) left->info.integer_rep.upper_bound < (int64_t) right->info.integer_rep.upper_bound)))
        {
            /* The left side has a stricter range than the right side */
            BAIL_ON_ERROR(status = LWMSG_STATUS_MALFORMED);
        }
    }

error:

    return status;
}

static
LWMsgStatus
lwmsg_type_rep_is_assignable_enum(
    LWMsgTypeAssignMap* map,
    LWMsgTypeRep* left,
    LWMsgTypeRep* right
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    LWMsgEnumDefRep* ldef = left->info.enum_rep.definition;
    LWMsgEnumDefRep* rdef = right->info.enum_rep.definition;
    int l = 0;
    int r = 0;
    LWMsgBool found = LWMSG_FALSE;

    if (ldef->width != rdef->width || ldef->sign != rdef->sign)
    {
        /* Basic characteristics don't match */
        BAIL_ON_ERROR(status = LWMSG_STATUS_MALFORMED);
    }

    /* Ensure each variant in the rhs is present in the lhs */
    for (r = 0; r < rdef->variant_count; r++)
    {
        found = LWMSG_FALSE;

        for (l = 0; l < ldef->variant_count; l++)
        {
            if (ldef->variants[l].is_mask == rdef->variants[r].is_mask &&
                ldef->variants[l].value == rdef->variants[r].value)
            {
                found = LWMSG_TRUE;
                break;
            }
        }

        if (!found)
        {
            BAIL_ON_ERROR(status = LWMSG_STATUS_MALFORMED);
        }
    }

error:

    return status;
}

static
LWMsgStatus
lwmsg_type_rep_is_assignable_struct(
    LWMsgTypeAssignMap* map,
    LWMsgTypeRep* left,
    LWMsgTypeRep* right
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    LWMsgStructDefRep* ldef = left->info.struct_rep.definition;
    LWMsgStructDefRep* rdef = right->info.struct_rep.definition;
    int i = 0;

    if (ldef->field_count != rdef->field_count)
    {
        /* Unequal number of fields */
        BAIL_ON_ERROR(status = LWMSG_STATUS_MALFORMED);
    }

    /* Ensure each field in the rhs is assignable to the field in the lhs */
    for (i = 0; i < rdef->field_count; i++)
    {
        BAIL_ON_ERROR(status = lwmsg_type_rep_is_assignable_internal(
                          map,
                          ldef->fields[i].type,
                          rdef->fields[i].type));
    }

error:

    return status;
}

static
LWMsgStatus
lwmsg_type_rep_is_assignable_union(
    LWMsgTypeAssignMap* map,
    LWMsgTypeRep* left,
    LWMsgTypeRep* right
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    LWMsgUnionDefRep* ldef = left->info.union_rep.definition;
    LWMsgUnionDefRep* rdef = right->info.union_rep.definition;
    int l = 0;
    int r = 0;
    LWMsgBool found = LWMSG_FALSE;

    if (left->info.union_rep.discrim_member_index != right->info.union_rep.discrim_member_index)
    {
        /* Discriminators don't match */
        BAIL_ON_ERROR(status = LWMSG_STATUS_MALFORMED);
    }

    /* Ensure each arm in the rhs is present and assignable in the lhs */
    for (r = 0; r < rdef->arm_count; r++)
    {
        found = LWMSG_FALSE;

        for (l = 0; l < ldef->arm_count; l++)
        {
            if (ldef->arms[l].tag == rdef->arms[r].tag)
            {
                found = LWMSG_TRUE;

                BAIL_ON_ERROR(status = lwmsg_type_rep_is_assignable_internal(
                                  map,
                                  ldef->arms[l].type,
                                  rdef->arms[r].type));
                break;
            }
        }

        if (!found)
        {
            BAIL_ON_ERROR(status = LWMSG_STATUS_MALFORMED);
        }
    }

error:

    return status;
}

static
LWMsgStatus
lwmsg_type_rep_is_assignable_pointer(
    LWMsgTypeAssignMap* map,
    LWMsgTypeRep* left,
    LWMsgTypeRep* right
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;

    if (left->flags != right->flags ||
        left->info.pointer_rep.zero_terminated != right->info.pointer_rep.zero_terminated ||
        left->info.pointer_rep.static_length != right->info.pointer_rep.static_length ||
        left->info.pointer_rep.length_member_index != right->info.pointer_rep.length_member_index)
    {
        /* Basic characteristics don't match */
        BAIL_ON_ERROR(status = LWMSG_STATUS_MALFORMED);
    }

    /* Ensure pointee types are assignable */
    BAIL_ON_ERROR(status = lwmsg_type_rep_is_assignable_internal(
                      map,
                      left->info.pointer_rep.pointee_type,
                      right->info.pointer_rep.pointee_type));
    
error:

    return status;
}

static
LWMsgStatus
lwmsg_type_rep_is_assignable_array(
    LWMsgTypeAssignMap* map,
    LWMsgTypeRep* left,
    LWMsgTypeRep* right
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;

    if (left->flags != right->flags ||
        left->info.array_rep.zero_terminated != right->info.array_rep.zero_terminated ||
        left->info.array_rep.static_length != right->info.array_rep.static_length ||
        left->info.array_rep.length_member_index != right->info.array_rep.length_member_index)
    {
        /* Basic characteristics don't match */
        BAIL_ON_ERROR(status = LWMSG_STATUS_MALFORMED);
    }

    /* Ensure element types are assignable */
    BAIL_ON_ERROR(status = lwmsg_type_rep_is_assignable_internal(
                      map,
                      left->info.array_rep.element_type,
                      right->info.array_rep.element_type));
    
error:

    return status;
}

static
LWMsgStatus
lwmsg_type_rep_is_assignable_internal(
    LWMsgTypeAssignMap* map,
    LWMsgTypeRep* left,
    LWMsgTypeRep* right
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;

    if (left->kind != right->kind)
    {
        BAIL_ON_ERROR(status = LWMSG_STATUS_MALFORMED);
    }
  
    if ((status = lwmsg_type_assign_map_find(map, left, right)) != LWMSG_STATUS_NOT_FOUND)
    {
        goto error;
    }

    /* Provisionally insert the pair into the map */
    BAIL_ON_ERROR(status = lwmsg_type_assign_map_insert(map, left, right));
    
    switch (left->kind)
    {
    case LWMSG_KIND_INTEGER:
        BAIL_ON_ERROR(status = lwmsg_type_rep_is_assignable_integer(
                          map,
                          left,
                          right));
        break;
    case LWMSG_KIND_ENUM:
        BAIL_ON_ERROR(status = lwmsg_type_rep_is_assignable_enum(
                          map,
                          left,
                          right));
        break;
    case LWMSG_KIND_STRUCT:
        BAIL_ON_ERROR(status = lwmsg_type_rep_is_assignable_struct(
                          map,
                          left,
                          right));
        break;
    case LWMSG_KIND_UNION:
        BAIL_ON_ERROR(status = lwmsg_type_rep_is_assignable_union(
                          map,
                          left,
                          right));
        break;
    case LWMSG_KIND_POINTER:
        BAIL_ON_ERROR(status = lwmsg_type_rep_is_assignable_pointer(
                          map,
                          left,
                          right));
        break;
    case LWMSG_KIND_ARRAY:
        BAIL_ON_ERROR(status = lwmsg_type_rep_is_assignable_array(
                          map,
                          left,
                          right));
        break;
    case LWMSG_KIND_VOID:
        /* Trivially assignable */
        break;
    default:
        BAIL_ON_ERROR(status = LWMSG_STATUS_MALFORMED);
        break;
    }

error:

    if (status == LWMSG_STATUS_MALFORMED)
    {
        lwmsg_type_assign_map_remove(map, left, right);
    }

    return status;
}

LWMsgStatus
lwmsg_type_rep_is_assignable(
    LWMsgTypeRep* left,
    LWMsgTypeRep* right
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    LWMsgTypeAssignMap map;

    memset(&map, 0, sizeof(map));

    BAIL_ON_ERROR(status = lwmsg_type_rep_is_assignable_internal(
                      &map,
                      left,
                      right));

error:

    lwmsg_type_assign_map_destroy(&map);

    return status;
}
