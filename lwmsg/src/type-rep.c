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

#define LWMSG_SPEC_META

#include "type-private.h"
#include "util-private.h"
#include "data-private.h"

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
    LWMSG_ENUM_VALUE(LWMSG_KIND_CUSTOM),
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
    LWMSG_ENUM_MASK(LWMSG_TYPE_FLAG_PROMOTED),
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

static LWMsgTypeSpec custom_rep_spec[] =
{
    LWMSG_STRUCT_BEGIN(LWMsgCustomRep),
    LWMSG_MEMBER_TYPESPEC(LWMsgCustomRep, transmitted_type, lwmsg_type_rep_spec),
    LWMSG_ATTR_NOT_NULL,
    LWMSG_ATTR_ALIASABLE,
    LWMSG_MEMBER_TYPESPEC(LWMsgCustomRep, is_pointer, bool_enum_spec),
    LWMSG_MEMBER_PSTR(LWMsgCustomRep, name),
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
    LWMSG_MEMBER_TYPESPEC(LWMsgTypeRepInfo, custom_rep, custom_rep_spec),
    LWMSG_ATTR_TAG(LWMSG_KIND_CUSTOM),
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
void*
lwmsg_type_rep_map_get_key_rep(
    const void* entry
    )
{
    return ((LWMsgTypeRepMapEntry*) entry)->rep;
}

static
size_t
lwmsg_type_rep_map_digest_rep(
    const void* key
    )
{
    return (size_t) key;
}

static
LWMsgBool
lwmsg_type_rep_map_equal_rep(
    const void* key1,
    const void* key2
    )
{
    return key1 == key2;
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
                          offsetof(LWMsgTypeRepMapEntry, ring1)));

        BAIL_ON_ERROR(status = lwmsg_hash_init(
                          &map->hash_by_rep,
                          11,
                          lwmsg_type_rep_map_get_key_rep,
                          lwmsg_type_rep_map_digest_rep,
                          lwmsg_type_rep_map_equal_rep,
                          offsetof(LWMsgTypeRepMapEntry, ring2)));
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

    BAIL_ON_ERROR(status = lwmsg_type_rep_map_init(map));

    LWMsgTypeRepMapEntry* entry = lwmsg_hash_find_key(&map->hash_by_spec, &key);

    if (!entry)
    {
        BAIL_ON_ERROR(status = LWMSG_STATUS_NOT_FOUND);
    }

    *rep = entry->rep;

error:

    return status;
}

#if 0
static
LWMsgStatus
lwmsg_type_rep_map_find_rep(
    LWMsgTypeRepMap* map,
    LWMsgTypeRep* rep,
    LWMsgTypeSpec** spec
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;

    BAIL_ON_ERROR(status = lwmsg_type_rep_map_init(map));

    LWMsgTypeRepMapEntry* entry = lwmsg_hash_find_key(&map->hash_by_rep, rep);

    if (!entry)
    {
        BAIL_ON_ERROR(status = LWMSG_STATUS_NOT_FOUND);
    }

    *spec = entry->spec;

error:

    return status;
}
#endif

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

    lwmsg_ring_init(&entry->ring1);
    lwmsg_ring_init(&entry->ring2);
    entry->spec.kind = kind;
    entry->spec.spec = spec;
    entry->rep = rep;

    if (spec)
    {
        lwmsg_hash_insert_entry(&map->hash_by_spec, entry);
    }

    if (rep)
    {
        lwmsg_hash_insert_entry(&map->hash_by_rep, entry);
    }

done:

    return status;

error:

    if (entry)
    {
        free(entry);
    }

    goto done;
}

static
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

    if (map->hash_by_rep.buckets)
    {
        lwmsg_hash_destroy(&map->hash_by_rep);
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

static
LWMsgStatus
lwmsg_type_rep_from_custom(
    LWMsgTypeRepMap* map,
    LWMsgTypeIter* iter,
    LWMsgTypeRep* rep
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    LWMsgTypeIter inner;

    rep->info.custom_rep.is_pointer = iter->info.kind_custom.typeclass->is_pointer;

    lwmsg_type_iterate(iter->info.kind_custom.typeclass->transmit_type, &inner);

    BAIL_ON_ERROR(status = lwmsg_strdup(
                      map->context,
                      lwmsg_type_name_suffix(iter->meta.type_name),
                      &rep->info.custom_rep.name));

    BAIL_ON_ERROR(status = lwmsg_type_rep_from_spec_internal(
                      map,
                      &inner,
                      &rep->info.custom_rep.transmitted_type));

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
        (*rep)->flags = iter->attrs.flags;

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
        case LWMSG_KIND_CUSTOM:
            BAIL_ON_ERROR(status = lwmsg_type_rep_from_custom(
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
