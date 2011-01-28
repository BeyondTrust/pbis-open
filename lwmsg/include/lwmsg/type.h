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
 *        type.h
 *
 * Abstract:
 *
 *        Type specification API (public header)
 *
 * Authors: Brian Koropoff (bkoropoff@likewisesoftware.com)
 *
 */
#ifndef __LWMSG_TYPE_H__
#define __LWMSG_TYPE_H__

#include <lwmsg/status.h>
#include <lwmsg/buffer.h>
#include <lwmsg/common.h>

#include <stddef.h>
#include <stdlib.h>
#include <inttypes.h>

/**
 * @defgroup types Types
 * @ingroup public
 * @brief Describe the structure of marshallable types
 *
 * This module provides the means to describe the layout of C types
 * such that they can be processed by the <tt>LWMsg</tt> marshaller. A type
 * specification consists of a global, statically-initialized array
 * of the #LWMsgTypeSpec type.  The initializer should consist of
 * a set of braces enclosing a comma-separated list of type specification
 * macros.
 * 
 * In order to allow for automated marshalling, a type specification
 * must include additional information which is not explicit in the
 * C type definition, such as how to determine the dynamic length
 * of arrays and pointer referents. Consider the following example
 * of a simple C structure and its <tt>LWMsg</tt> specification.
 *
 * @code
 * typedef struct _Foo
 * {
 *     char* name;
 *     size_t length;
 *     long* numbers;
 * } Foo;
 *
 * LWMsgTypeSpec foo_spec[] =
 * {
 *     // Begin layout of "Foo" structure
 *     LWMSG_STRUCT_BEGIN(Foo),
 *     // Foo has a member "name" which is a pointer to a null-terminated string
 *     LWMSG_MEMBER_PSTR(Foo, name),
 *     // Foo has a member "length" which should be marshalled as an unsigned 32-bit integer
 *     LWMSG_MEMBER_UINT32(Foo, length),
 *     // Foo has a member "numbers" which is a pointer
 *     LWMSG_MEMBER_POINTER_BEGIN(Foo, numbers),
 *     // The pointer points to one or more longs which are marshalled as signed 64-bit integers
 *     LWMSG_INT64(long),
 *     // End the member "numbers" pointer
 *     LWMSG_POINTER_END,
 *     // Indicate that the dynamic length of "numbers" is the value of "length"
 *     LWMSG_ATTR_LENGTH_MEMBER(Foo, length),
 *     // End the "Foo" structure
 *     LWMSG_STRUCT_END,
 *     // End the type specification
 *     LWMSG_TYPE_END
 * };
 * @endcode
 *
 * Type specifications have the following restrictions compared to C type definitions:
 * - The length of arrays and pointer referents must be specified:
 *     - As a static length with #LWMSG_ATTR_LENGTH_STATIC()
 *     - As the value of a <i>previous</i> integer or enum structure member with
 *       #LWMSG_ATTR_LENGTH_MEMBER()
 *     - As being implicit through zero-termination with #LWMSG_ATTR_ZERO_TERMINATED
 * - The active arm of unions must be specified through a discriminator:
 *     - Each union arm must be marked with an integer tag with #LWMSG_ATTR_TAG()
 *     - Each instance of a union type must be correlated with a <i>previous</i> integer
 *       structure member with #LWMSG_ATTR_DISCRIM()
 *
 * Type specifications have the following leniencies compared to C type definitions:
 * - The order in which structure members are specified may differ from their
 *   order in the C definition, with the expection of flexible array members.
 *   Members will be marshalled in the order of the type specification.
 * - The specified width of an integer type may differ from the C type.  The
 *   integer will be marshalled to the specified size, with widening or truncating
 *   conversions applied automatically.  <b>Caution</b>: Truncating conversions which
 *   cause overflow will generate runtime errors.
 */

/* @{ */

#ifndef DOXYGEN
struct LWMsgDataContext;
struct LWMsgContext;

typedef enum LWMsgKind
{
    LWMSG_KIND_NONE = 0,
    LWMSG_KIND_VOID = 1,
    LWMSG_KIND_INTEGER = 2,
    LWMSG_KIND_ENUM = 3,
    LWMSG_KIND_STRUCT = 4,
    LWMSG_KIND_UNION = 5,
    LWMSG_KIND_ARRAY = 6,
    LWMSG_KIND_POINTER = 7,
    LWMSG_KIND_CUSTOM = 128
} LWMsgKind;

typedef enum LWMsgArrayTermination
{
    /* Static length */
    LWMSG_TERM_STATIC,
    /* Zero-terminated */
    LWMSG_TERM_ZERO,
    /* Specified in member of dominating node */
    LWMSG_TERM_MEMBER
} LWMsgArrayTermination;

typedef enum LWMsgByteOrder
{
    LWMSG_LITTLE_ENDIAN,
    LWMSG_BIG_ENDIAN
} LWMsgByteOrder;
#endif

/**
 * @brief Indicate signed or unsigned status
 *
 * Indicates whether an integer type is signed or unsigned.
 */
typedef enum LWMsgSignage
{
    /** Signed */
    LWMSG_SIGNED,
    /** Unsigned */
    LWMSG_UNSIGNED
} LWMsgSignage;

/**
 * @brief Type flag bitmask
 *
 * Describes set/unset attributes of a type
 */
typedef enum LWMsgTypeFlags
{
    /**
     * Type may not be NULL 
     * @hideinitializer
     */
    LWMSG_TYPE_FLAG_NOT_NULL  = 0x01,
    /**
     * Type contains sensitive data
     * @hideinitializer
     */
    LWMSG_TYPE_FLAG_SENSITIVE = 0x02,
    /**
     * Type was implicitly promoted to a pointer
     * @hideinitializer
     */
    LWMSG_TYPE_FLAG_PROMOTED  = 0x04,
    /**
     * Type has a range constraint
     * @hideinitializer
     */
    LWMSG_TYPE_FLAG_RANGE     = 0x08,
    /**
     * Pointer type is aliasable
     * @hideinitializer
     */
    LWMSG_TYPE_FLAG_ALIASABLE = 0x08
} LWMsgTypeFlags;

/**
 * @brief Type
 *
 * An opqaue handle representing a type.
 */
typedef struct LWMsgType LWMsgType;

/**
 * @brief Custom marshal function
 *
 * A callback function which converts a presented type to its transmitted form.
 * The transmitted_object parameter will point to a block of memory large enough
 * for the transmitted type.  Any resources allocated to construct the transmitted
 * object can be freed by the corresponding #LWMsgTypeDestroyTransmittedFunction.
 *
 * @param[in]  context the data context
 * @param[in]  type the type to marshal
 * @param[in]  presented_object the presented object
 * @param[out] transmitted_object the object which will be transmitted
 * @param[in]  data the user data pointer specified to LWMSG_CUSTOM() or LWMSG_MEMBER_CUSTOM()
 * in the type specification
 * @lwmsg_status
 * @lwmsg_success
 * @lwmsg_etc{implementation-specific error}
 * @lwmsg_endstatus
 */
typedef LWMsgStatus (*LWMsgTypeMarshalFunction) (
    struct LWMsgDataContext* context,
    LWMsgType* type,
    void* object,
    void* transmit_object,
    void* data
    );

/**
 * @brief Custom unmarshal function
 *
 * A callback function type which converts a transmitted type to its presented form.
 * The presented_object parameter will point to a block of memory large enough for
 * the presented type.  Any resources allocated to construct the presented object can
 * be freed by the corresponding #LWMsgTypeDestroyPresentedFunction
 *
 * @param[in]  context the data context
 * @param[in]  type the type to marshal
 * @param[in]  transmit_object the transmitted object
 * @param[out] presented_object space for the presented object
 * @param[in]  data the user data pointer specified to LWMSG_CUSTOM() or LWMSG_MEMBER_CUSTOM()
 * in the type specification
 * @lwmsg_status
 * @lwmsg_success
 * @lwmsg_etc{implementation-specific error}
 * @lwmsg_endstatus
 */
typedef LWMsgStatus (*LWMsgTypeUnmarshalFunction) (
    struct LWMsgDataContext* context,
    LWMsgType* type,
    void* transmit_object,
    void* object,
    void* data
    );

/**
 * @brief Destroy presented object function
 *
 * A callback function type which releases any resources associated with a
 * presented object.
 *
 * @param context the data context
 * @param type the type to free
 * @param object the address of the object to free
 * @param data the user data pointer specified to LWMSG_CUSTOM() or LWMSG_MEMBER_CUSTOM()
 * in the type specification
 */
typedef void (*LWMsgTypeDestroyPresentedFunction) (
    struct LWMsgDataContext* context,
    LWMsgType* type,
    void* object,
    void* data
    );

/**
 * @brief Destroy transmitted object function
 *
 * A callback function which releases any resources associated with a
 * transmitted object.
 *
 * @param[in] context the data context
 * @param[in,out] object the transmitted object
 * @param[in] data the user data pointer specified to LWMSG_CUSTOM() or LWMSG_MEMBER_CUSTOM()
 * in the type specification
 */
typedef void (*LWMsgTypeDestroyTransmittedFunction) (
    struct LWMsgDataContext* context,
    LWMsgType* type,
    void* object,
    void* data
    );

/**
 * @brief Custom print function
 *
 * A callback function type which prints the representation of a custom type,
 * writing to the provided buffer.
 *
 * @param context the data context
 * @param object the address of the object to print
 * @param type the type to print
 * @param data the user data pointer specified to LWMSG_CUSTOM() or LWMSG_MEMBER_CUSTOM()
 * in the type specification
 * @param buffer the buffer into which to write
 */
typedef LWMsgStatus (*LWMsgTypePrintFunction) (
    struct LWMsgDataContext* context,
    LWMsgType* attr,
    void* object,
    void* data,
    LWMsgBuffer* buffer
    );

/**
 * @brief Marshaller type specification
 *
 * The fundamental type used to represent a type specification.
 * This is considered an implementation detail.
 * @hideinitializer
 */
typedef size_t const LWMsgTypeSpec;

/**
 * @brief Custom type class
 *
 * Describes a custom type which may be used with
 * LWMSG_CUSTOM() or LWMSG_MEMBER_CUSTOM() in a type specification.
 *
 * A custom type actually comprises two types:
 * - The presented type, which is the type seen by the application.
 * - The transmitted type, which is the type represented in the
 *   data stream.
 *
 * The presented type's size must be known to the marshaller so that
 * it can allocate adequate memory for it when reconstructing
 * the object graph from the data stream.  All other details
 * of the type are up to the application, which must provide
 * functions to convert to and from the transmitted type,
 * functions to destroy the presented and transmitted types,
 * and a function to print the presented type in human-readable form.
 */
typedef struct LWMsgTypeClass
{
    /** 
     * @brief Whether the type is a pointer
     *
     * If a custom type is a pointer, it will not be subject to automatic
     * pointer promotion when passed to #lwmsg_data_marshal() et al.
     */
    LWMsgBool is_pointer;
    /** @brief The type of the transmitted form */
    LWMsgTypeSpec* transmit_type;
    /** @brief Marshal function */
    LWMsgTypeMarshalFunction marshal;
    /** @brief Unmarshal function */
    LWMsgTypeUnmarshalFunction unmarshal;
    /** @brief Destroy presented function */
    LWMsgTypeDestroyPresentedFunction destroy_presented;
    /** @brief Destroy transmitted function */
    LWMsgTypeDestroyTransmittedFunction destroy_transmitted;
    /** @brief Print callback function */
    LWMsgTypePrintFunction print;
} LWMsgTypeClass;

/**
 * @brief Custom data verification function
 *
 * A callback function which performs verification of
 * in-memory data immediately before marshalling or immediately
 * after unmarshalling.
 *
 * @param context the data context
 * @param unmarshalling true when the operation being performed is unmarshalling
 * @param object the object to verify
 * @param data the user data pointer given to LWMSG_ATTR_VERIFY()
 * @lwmsg_status
 * @lwmsg_success
 * @lwmsg_code{MALFORMED, the object did not pass verification}
 * @lwmsg_endstatus
 */
typedef LWMsgStatus (*LWMsgVerifyFunction) (
    struct LWMsgDataContext* context,
    LWMsgBool unmarshalling,
    void* object,
    void *data
    );

#ifndef DOXYGEN
typedef enum LWMsgTypeDirective
{
        LWMSG_CMD_END,
        LWMSG_CMD_STRUCT,
        LWMSG_CMD_UNION,
        LWMSG_CMD_INTEGER,
        LWMSG_CMD_POINTER,
        LWMSG_CMD_ARRAY,
        LWMSG_CMD_TYPESPEC,
        LWMSG_CMD_TERMINATION,
        LWMSG_CMD_TAG,
        LWMSG_CMD_DISCRIM,
        LWMSG_CMD_CUSTOM,
        LWMSG_CMD_VERIFY,
        LWMSG_CMD_RANGE,
        LWMSG_CMD_NOT_NULL,
        LWMSG_CMD_CUSTOM_ATTR,
        LWMSG_CMD_VOID,
        LWMSG_CMD_ENCODING,
        LWMSG_CMD_SENSITIVE,
        LWMSG_CMD_ALIASABLE,
        LWMSG_CMD_ENUM,
        LWMSG_CMD_ENUM_VALUE,
        LWMSG_CMD_ENUM_MASK,
        LWMSG_CMD_MAX_ALLOC,
        LWMSG_FLAG_MEMBER = 0x10000,
        LWMSG_FLAG_META = 0x20000,
        LWMSG_FLAG_DEBUG = 0x40000
} LWMsgTypeDirective;

#define LWMSG_CMD_MASK 0xFFFF
#define _TYPEARG(_v_) ((size_t) (_v_))
#define _TYPECMD(_c_) ((size_t) ((_c_) | _TYPEFLAG_DEBUG)) _TYPEDEBUG
#define _TYPECMD_TYPE(_c_, _t_) ((size_t) ((_c_) | _TYPEFLAG_META | _TYPEFLAG_DEBUG)) _TYPEMETA(_t_) _TYPEDEBUG
#define _TYPECMD_MEMBER(_c_, _t_, _m_) ((size_t) ((_c_) | LWMSG_FLAG_MEMBER | _TYPEFLAG_META | _TYPEFLAG_DEBUG)) _TYPEMETA_MEMBER(_t_, _m_) _TYPEDEBUG
#define _TYPECMD_NAMED(_c_, _n_) ((size_t) ((_c_) | _TYPEFLAG_META | _TYPEFLAG_DEBUG)) _TYPENAME(_n_) _TYPEDEBUG

#ifndef LWMSG_SPEC_META
#define LWMSG_SPEC_META
#endif

#ifdef LWMSG_OMIT_TYPESPEC_METADATA
#  undef LWMSG_SPEC_META
#endif

#ifdef LWMSG_SPEC_META
#define _TYPEFLAG_META (LWMSG_FLAG_META)
#define _TYPEMETA(type) ,_TYPEARG(#type)
#define _TYPENAME(name) ,_TYPEARG(name)
#define _TYPEMETA_MEMBER(type, field) ,_TYPEARG(#field)
#else
#define _TYPEFLAG_META (0)
#define _TYPEMETA(type)
#define _TYPENAME(name)
#define _TYPEMETA_MEMBER(type, field)
#endif

#ifdef LWMSG_SPEC_DEBUG
#define _TYPEFLAG_DEBUG (LWMSG_FLAG_DEBUG)
#define _TYPEDEBUG ,_TYPEARG(__FILE__), _TYPEARG(__LINE__)
#else
#define _TYPEFLAG_DEBUG (0)
#define _TYPEDEBUG
#endif

#endif /* DOXYGEN */

/**
 * @brief Specify an empty type
 *
 * Specifies an empty (zero-length) type.
 * @hideinitializer
 */
#define LWMSG_VOID \
    _TYPECMD(LWMSG_CMD_VOID)

/**
 * @brief Specify an empty member
 *
 * Specifies an empty (zero-length) member.  This is primarily useful
 * for indicating empty arms of a union.  The field name can be arbitrary.
 *
 * @param type the name of the containing type
 * @param field the name of the member
 * @hideinitializer
 */
#define LWMSG_MEMBER_VOID(type, field) \
    _TYPECMD_MEMBER(LWMSG_CMD_VOID, type, field)

/**
 * @brief Reference another type specification
 *
 * References another type specification, treating it as though it
 * were inserted in place into the type specification currently
 * being defined.  This mechanism may be used to avoid repeating
 * definitions for common types.
 *
 * @param spec the #LWMsgTypeSpec[] to reference
 * @hideinitializer
 */
#define LWMSG_TYPESPEC(spec)                    \
    _TYPECMD(LWMSG_CMD_TYPESPEC),               \
        _TYPEARG(spec)

/**
 * @brief Reference another type specification as a member
 *
 * Defines a member of a structure or union by referencing a
 * separate type specification.  This mechanism may be used to avoid
 * repeating definitions for common types.
 *
 * @param type the name of the containing type
 * @param field the name of the member
 * @param spec the #LWMsgTypeSpec[] which specifies the member type
 * @hideinitializer
 */
#define LWMSG_MEMBER_TYPESPEC(type, field, spec)                      \
    _TYPECMD_MEMBER(LWMSG_CMD_TYPESPEC, type, field),                 \
        _TYPEARG(sizeof(((type*)0)->field)),                          \
        _TYPEARG(offsetof(type, field)),                              \
        _TYPEARG(spec)

/**
 * @brief End a type specification
 *
 * Marks the end of a type specification.  All type specifications
 * must be terminated with this macro.
 *
 * @hideinitializer
 */
#define LWMSG_TYPE_END \
    _TYPECMD(LWMSG_CMD_END)

/**
 * @brief End a structure specification
 *
 * Marks the end of a structure specification started with 
 * #LWMSG_STRUCT_BEGIN() or #LWMSG_MEMBER_STRUCT_BEGIN().
 *
 * @hideinitializer
 */
#define LWMSG_STRUCT_END \
    _TYPECMD(LWMSG_CMD_END)

/**
 * @brief End a union specification
 *
 * Marks the end of a union specification started with 
 * #LWMSG_UNION_BEGIN() or #LWMSG_MEMBER_UNION_BEGIN().
 *
 * @hideinitializer
 */
#define LWMSG_UNION_END \
    _TYPECMD(LWMSG_CMD_END)

/**
 * @brief End a pointer specification
 *
 * Marks the end of a pointer specification started with 
 * #LWMSG_POINTER_BEGIN() or #LWMSG_MEMBER_POINTER_BEGIN().
 *
 * @hideinitializer
 */
#define LWMSG_POINTER_END \
    _TYPECMD(LWMSG_CMD_END)

/**
 * @brief End an array specification
 *
 * Marks the end of an array specification started with 
 * #LWMSG_MEMBER_ARRAY_BEGIN().
 *
 * @hideinitializer
 */
#define LWMSG_ARRAY_END \
    _TYPECMD(LWMSG_CMD_END)

/**
 * @brief Begin a structure definition
 *
 * Begins the definition of a structure within a type specification.
 * The end of the definition must be marked with LWMSG_STRUCT_END
 *
 * @param type the name of the structure type
 * @hideinitializer
 */
#define LWMSG_STRUCT_BEGIN(type)               \
    _TYPECMD_TYPE(LWMSG_CMD_STRUCT, type),     \
        _TYPEARG(sizeof(type))

/**
 * @brief Begin a structure definition as a member
 *
 * Begins the definition of a structure which is an inline member
 * of the structure or union currently being defined. The end of the
 * definition must be marked with LWMSG_STRUCT_END.
 *
 * @param type the name of the containing structure or union type
 * @param field the name of the member
 * @hideinitializer
 */
#define LWMSG_MEMBER_STRUCT_BEGIN(type, field)        \
    _TYPECMD_MEMBER(LWMSG_CMD_STRUCT, type, field),   \
        _TYPEARG(sizeof(((type*)0)->field)),          \
        _TYPEARG(offsetof(type, field))
        

/**
 * @brief Begin a union definition
 *
 * Begins the definition of a union within a type specification.
 * The end of the definition must be marked with LWMSG_UNION_END
 *
 * @param type the name of the union type
 * @hideinitializer
 */
#define LWMSG_UNION_BEGIN(type)                \
    _TYPECMD_TYPE(LWMSG_CMD_UNION, type),      \
        _TYPEARG(sizeof(type))

/**
 * @brief Begin a union definition as a member
 *
 * Begins the definition of a union which is an inline member
 * of the structure or union currently being defined. The end of the
 * definition must be marked with LWMSG_UNION_END.
 *
 * @param type the name of the containing structure or union type
 * @param field the name of the member
 * @hideinitializer
 */
#define LWMSG_MEMBER_UNION_BEGIN(type, field)      \
    _TYPECMD_MEMBER(LWMSG_CMD_UNION, type, field), \
        _TYPEARG(sizeof(((type*)0)->field)),       \
        _TYPEARG(offsetof(type, field))

/**
 * @brief Define an integer member
 *
 * Defines an integer member of a struct or union.
 *
 * @param type the name of the containing structure or union type
 * @param field the name of the member
 * @param width the marshalled size of the integer in bytes
 * @param sign the signedness of the integer as a #LWMsgSignage
 * @hideinitializer
 */
#define LWMSG_MEMBER_INTEGER(type, field, width, sign)                 \
    _TYPECMD_MEMBER(LWMSG_CMD_INTEGER, type, field),                   \
        _TYPEARG(sizeof(((type*)0)->field)),                           \
        _TYPEARG(offsetof(type, field)),                               \
        _TYPEARG(width),                                               \
        _TYPEARG(sign)

/**
 * @brief Define an integer
 *
 * Defines an integer
 *
 * @param type the unmarshalled, C type of the integer
 * @param width the marshalled size of the integer in bytes
 * @param sign the signedness of the integer as a #LWMsgSignage
 * @hideinitializer
 */
#define LWMSG_INTEGER(type, width, sign)       \
    _TYPECMD_TYPE(LWMSG_CMD_INTEGER, type),    \
        _TYPEARG(sizeof(type)),                \
        _TYPEARG(width),                       \
        _TYPEARG(sign)

/**
 * @brief Indicate static length
 *
 * Indicates the static length of the immediately previous
 * array or pointer type in a type specification
 *
 * @param count the number of elements in the previous type
 * @hideinitializer
 */
#define LWMSG_ATTR_LENGTH_STATIC(count)  \
    _TYPECMD(LWMSG_CMD_TERMINATION),     \
        _TYPEARG(LWMSG_TERM_STATIC),     \
        _TYPEARG(count)

/**
 * @brief Indicate correlated length member
 *
 * Indicates that that length of the immediately previous
 * array or pointer member is equal to the value of the
 * specified integer member, which must be defined before
 * the pointer or array member.
 *
 * @param type the type of the containing structure
 * @param field the name of the field which contains the length
 * @hideinitializer
 */
#define LWMSG_ATTR_LENGTH_MEMBER(type, field)   \
    _TYPECMD(LWMSG_CMD_TERMINATION),            \
        _TYPEARG(LWMSG_TERM_MEMBER),            \
        _TYPEARG(offsetof(type, field)),        \
        _TYPEARG(sizeof(((type*)0)->field))

/**
 * @brief Indicate null termination
 *
 * Indicates that the immediately previous array or pointer
 * member has a length determined by null- or zero- termination.
 * @hideinitializer
 */
#define LWMSG_ATTR_ZERO_TERMINATED              \
    _TYPECMD(LWMSG_CMD_TERMINATION),            \
        _TYPEARG(LWMSG_TERM_ZERO)

/**
 * @brief Indicate data encoding
 *
 * Indicates that the immediately previous array or pointer
 * represents data in the specified encoding.  This is used
 * as a hint by functions such as #lwmsg_data_print_graph_alloc()
 * to show the data in a human-readable form, but has no effect
 * on how the data is marshalled.  The encoding is specified as
 * an all-uppercase string which should be a valid encoding name
 * recognized by the system iconv_open() function.  Alternatively,
 * it can be one of the following special lowercase strings
 * recognized by <tt>lwmsg</tt>:
 *
 * - <tt>"ucs-2"</tt>: specifies UCS-2 encoding with native endianness.
 *   The iconv-recognized "UCS-2" encoding name is inconsistent across different
 *   implementations -- on some platforms it indicates native endianness,
 *   while on others it is always big or little regardless of CPU architecture.
 *   Use this alias instead for consistent cross-platform behavior.
 * - <tt>"hex+ascii"</tt>: indicates that the data should be displayed in
 *   side-by-side hexadecimal and ASCII form, like the <tt>hexdump -C</tt>
 *   command available on many UNIX systems. Each 16-byte block is shown on its
 *   own line with its 32-bit offset in hex, the hex value of each byte, and the
 *   ASCII interpretation of each byte if it is a valid ASCII codepoint and not
 *   a control character, or '<tt>.</tt>' otherwise.
 *
 * @param enc a string specifying the encoding 
 * @hideinitializer
 */
#define LWMSG_ATTR_ENCODING(enc)                \
    _TYPECMD(LWMSG_CMD_ENCODING),               \
        _TYPEARG((enc))

/**
 * @brief Indicate sensitive information
 *
 * Indicates that the immediately previous type or member
 * contains sensitive information (e.g. a password) and should
 * not be displayed when printed.
 * @hideinitializer
 */
#define LWMSG_ATTR_SENSITIVE                    \
    _TYPECMD(LWMSG_CMD_SENSITIVE)

/**
 * @brief Indicate aliasable pointer
 *
 * Indicates that the immediately previous type or member,
 * which must be a pointer or pointer-like type,
 * is aliasable -- that is, multiple instances of the same
 * pointer may appear in the data object graph.
 * @hideinitializer
 */
#define LWMSG_ATTR_ALIASABLE                    \
    _TYPECMD(LWMSG_CMD_ALIASABLE)

/**
 * @brief Indicate C string
 *
 * Indicates that the immediately previous array or pointer
 * represents a plain C string.  That is, it is nul-terminated
 * and encoded in the program's current locale.
 * @hideinitializer
 */
#define LWMSG_ATTR_STRING \
    LWMSG_ATTR_ZERO_TERMINATED,                 \
    LWMSG_ATTR_ENCODING("")

/**
 * @brief Limit memory allocation for pointer referent
 *
 * Indicates that the marshaller should refuse to allocate more
 * than the given number of bytes when unmarshalling the referent
 * of the immediately previous pointer type.  An attempt to exceed
 * this value will result in an #LWMSG_STATUS_OVERFLOW error.
 * @param max the maximum number of bytes to allocate
 * @hideinitializer
 */
#define LWMSG_ATTR_MAX_ALLOC(max) \
    _TYPECMD(LWMSG_CMD_MAX_ALLOC), \
        _TYPEARG((max))

/**
 * @brief Indicate union tag
 *
 * Indicates that the immediately previous member of a union
 * is associated with a particular integer tag.  All members
 * of a union must be marked with this attribute.
 * @param value the integer value associated with the member
 * @hideinitializer
 */
#define LWMSG_ATTR_TAG(value)                  \
    _TYPECMD(LWMSG_CMD_TAG),                   \
        _TYPEARG(value)

/**
 * @brief Indicate correlated union discriminator
 *
 * Indicates that the immediately previous member, which
 * must be a union type, has an active arm which is determined
 * by the tag stored in the specified discriminator member.
 * All uses of unions must be marked with this attribute.
 *
 * @param type the name of the containing structure
 * @param field the name of the member which holds the tag value
 * @hideinitializer
 */
#define LWMSG_ATTR_DISCRIM(type, field)         \
    _TYPECMD(LWMSG_CMD_DISCRIM),                \
        _TYPEARG(offsetof(type, field)),        \
        _TYPEARG(sizeof(((type*)0)->field))

/**
 * @brief Apply custom data verifier
 *
 * Applies a custom data verification function to the previous type
 * or member.  The function will be called with the in-memory form
 * of the data immediately before it is marshalled or immediately
 * after it is unmarshalled.
 *
 * Only one custom data verifier may be applied to a given
 * type or member.
 *
 * @param func the verifier function
 * @param data a constant user data pointer to pass to the function
 * @hideinitializer
 */
#define LWMSG_ATTR_VERIFY(func, data)           \
    _TYPECMD(LWMSG_CMD_VERIFY),                 \
        _TYPEARG(func),                         \
        _TYPEARG(data)


/**
 * @brief Constrain range of integer type
 *
 * Constrains the range of an integer type to the specified bounds.
 * Attempts to marshal or unmarshal data where the type exceeds these
 * bounds will result in an immediate error.
 *
 * @param low the lower bound of the range (inclusive)
 * @param high the upper bound of the range (inclusive)
 * @hideinitializer
 */
#define LWMSG_ATTR_RANGE(low, high)           \
    _TYPECMD(LWMSG_CMD_RANGE),                \
        _TYPEARG(low),                        \
        _TYPEARG(high)


/**
 * @brief Restrict pointer nullability
 *
 * Specifies that the previous type or member, which must be a pointer,
 * must not be NULL.  Attempts to marshal or unmarshal data where the
 * affected type or member is NULL will result in an immediate
 * error.
 *
 * @hideinitializer
 */
#define LWMSG_ATTR_NOT_NULL                     \
    _TYPECMD(LWMSG_CMD_NOT_NULL)

/**
 * @brief Define a pointer as a member
 *
 * Defines a pointer which is a member of the current structure
 * or union.  This must be followed by the definition of the
 * pointer's contents.  The end of the pointer definition
 * must be marked by LWMSG_POINTER_END.
 *
 * @param type the name of the containing structure
 * @param field the name of the pointer member
 * @hideinitializer
 */
#define LWMSG_MEMBER_POINTER_BEGIN(type, field)       \
    _TYPECMD_MEMBER(LWMSG_CMD_POINTER, type, field),  \
        _TYPEARG(offsetof(type, field))

/**
 * @brief Define a pointer
 *
 * Defines a pointer. This must be followed by the definition of
 * the pointer's contents.  The end of the pointer definition
 * must be marked by #LWMSG_POINTER_END.
 *
 * @hideinitializer
 */
#define LWMSG_POINTER_BEGIN                     \
    _TYPECMD(LWMSG_CMD_POINTER)

/**
 * @brief Define an array as a member
 *
 * Defines an array which is a member of the current structure
 * or union.  This must be followed by the definition of the
 * array's contents.  The end of the array definition
 * must be marked by LWMSG_ARRAY_END.
 *
 * An array, as opposed to a pointer, has contents which are
 * stored inline in the the containing structure or union.
 * That is, each element of the array is stored as if it were
 * itself a member.  This difference corresponds naturally
 * to the difference between * and [] types in C structures.
 *
 * @param type the name of the containing structure
 * @param field the name of the array member
 * @hideinitializer
 */
#define LWMSG_MEMBER_ARRAY_BEGIN(type, field)         \
    _TYPECMD_MEMBER(LWMSG_CMD_ARRAY, type, field),    \
        _TYPEARG(offsetof(type, field))

/**
 * @brief Define a custom type
 *
 * Defines a custom type with user-specified marshaller logic.
 *
 * @param type the C type
 * @param tclass a constant pointer to the LWMsgCustomTypeClass structure
 * containing marshalling methods
 * @param tdata a constant pointer to arbitrary data which will be passed
 * to the marshalling methods
 * @hideinitializer
 */
#define LWMSG_CUSTOM(type, tclass, tdata)        \
    _TYPECMD_TYPE(LWMSG_CMD_CUSTOM, type),       \
        _TYPEARG(sizeof(type)),                  \
        _TYPEARG(tclass),                        \
        _TYPEARG(tdata)

/**
 * @brief Define a custom type as a member
 *
 * Defines a custom type with user-specified marshaller logic as
 * a member of a structure or union.
 *
 * @param type the name of the containing type
 * @param field the name of the member
 * @param tclass a constant pointer to the LWMsgCustomTypeClass structure
 * containing marshalling methods
 * @param tdata a constant pointer to arbitrary data which will be passed
 * to the marshalling methods
 * @hideinitializer
 */
#define LWMSG_MEMBER_CUSTOM(type, field, tclass, tdata)        \
    _TYPECMD_MEMBER(LWMSG_CMD_CUSTOM, type, field),            \
        _TYPEARG(sizeof(((type*)0)->field)),                   \
        _TYPEARG(offsetof(type, field)),                       \
        _TYPEARG(tclass),                                      \
        _TYPEARG(tdata)

/**
 * @brief Apply custom type attribute
 *
 * Applies a custom attribute to the previous type or member,
 * which must be a custom type.  The bitwise or of all custom
 * attribute values will be made available to the marshal and
 * unmarshal functions for the custom type.
 *
 * @param value the value of the attribute to apply
 * @hideinitializer
 */
#define LWMSG_ATTR_CUSTOM(value)                \
    _TYPECMD(LWMSG_CMD_CUSTOM_ATTR),            \
        _TYPEARG(value)

/**
 * @brief Begin enum definition
 *
 * Defines an enumerated integer type with the given
 * underlying C type, width, and sign.  An enum value
 * can consist of one of a set of scalar values (specified
 * with #LWMSG_ENUM_VALUE()) and/or a bitwise-or'd
 * set of mask values (specified with #LWMSG_ENUM_MASK())
 *
 * @param type the name of the C enum type
 * @param width the width of the enum when marshalled, in bytes
 * @param sign the sign of the enum when marshalled, an #LWMsgSignage value
 * @hideinitializer
 */
#define LWMSG_ENUM_BEGIN(type, width, sign)    \
    _TYPECMD_TYPE(LWMSG_CMD_ENUM, type),       \
        _TYPEARG(sizeof(type)),                \
        _TYPEARG(width),                       \
        _TYPEARG(sign)

/**
 * @brief End enum definition
 *
 * Ends an enum definition started by #LWMSG_ENUM_BEGIN()
 * @hideinitializer
 */
#define LWMSG_ENUM_END \
    _TYPECMD(LWMSG_CMD_END)

/**
 * @brief Define enum scalar value
 *
 * Defines a possible scalar value of an enum type
 *
 * @param value the value
 * @hideinitializer
 */
#define LWMSG_ENUM_VALUE(value)                   \
    _TYPECMD_TYPE(LWMSG_CMD_ENUM_VALUE, value),   \
        _TYPEARG((value))

/**
 * @brief Define enum scalar value with explicit name
 *
 * Defines a possible scalar value of an enum type with
 * an explicit name.
 *
 * @param name the name of the constant
 * @param value the value
 * @hideinitializer
 */
#define LWMSG_ENUM_NAMED_VALUE(name, value)       \
    _TYPECMD_NAMED(LWMSG_CMD_ENUM_VALUE, name),   \
        _TYPEARG((value))

/**
 * @brief Define enum mask value
 *
 * Defines a potential bitmask value of an enum type
 *
 * @param value the value
 * @hideinitializer
 */
#define LWMSG_ENUM_MASK(value)                      \
    _TYPECMD_TYPE(LWMSG_CMD_ENUM_MASK, value),      \
        _TYPEARG((value))

/**
 * @brief Define enum mask value with explicit name
 *
 * Defines a potential bitmask value of an enum type with
 * an explicit name.
 *
 * @param name the name of the constant
 * @param value the value
 * @hideinitializer
 */
#define LWMSG_ENUM_NAMED_MASK(name, value)          \
    _TYPECMD_NAMED(LWMSG_CMD_ENUM_MASK, name),      \
        _TYPEARG((value))

/* Handy aliases for more complicated commands */

/**
 * @brief Define a signed 8-bit integer member
 *
 * Defines a signed 8-bit integer member of a struct or union.
 * This is a convenient shortcut for a full LWMSG_MEMBER_INTEGER()
 * invocation.
 *
 * @param type the name of the containing structure or union
 * @param field the name of the member
 * @hideinitializer
 */
#define LWMSG_MEMBER_INT8(type, field) LWMSG_MEMBER_INTEGER(type, field, 1, LWMSG_SIGNED)

/**
 * @brief Define a signed 16-bit integer member
 *
 * Defines a signed 16-bit integer member of a struct or union.
 * This is a convenient shortcut for a full LWMSG_MEMBER_INTEGER()
 * invocation.
 *
 * @param type the name of the containing structure or union
 * @param field the name of the member
 * @hideinitializer
 */
#define LWMSG_MEMBER_INT16(type, field) LWMSG_MEMBER_INTEGER(type, field, 2, LWMSG_SIGNED)

/**
 * @brief Define a signed 32-bit integer member
 *
 * Defines a signed 32-bit integer member of a struct or union.
 * This is a convenient shortcut for a full LWMSG_MEMBER_INTEGER()
 * invocation.
 *
 * @param type the name of the containing structure or union
 * @param field the name of the member
 * @hideinitializer
 */
#define LWMSG_MEMBER_INT32(type, field) LWMSG_MEMBER_INTEGER(type, field, 4, LWMSG_SIGNED)

/**
 * @brief Define a signed 64-bit integer member
 *
 * Defines a signed 64-bit integer member of a struct or union.
 * This is a convenient shortcut for a full LWMSG_MEMBER_INTEGER()
 * invocation.
 *
 * @param type the name of the containing structure or union
 * @param field the name of the member
 * @hideinitializer
 */
#define LWMSG_MEMBER_INT64(type, field) LWMSG_MEMBER_INTEGER(type, field, 8, LWMSG_SIGNED)

/**
 * @brief Define an unsigned 8-bit integer member
 *
 * Defines an unsigned 8-bit integer member of a struct or union.
 * This is a convenient shortcut for a full LWMSG_MEMBER_INTEGER()
 * invocation.
 *
 * @param type the name of the containing structure or union
 * @param field the name of the member
 * @hideinitializer
 */
#define LWMSG_MEMBER_UINT8(type, field) LWMSG_MEMBER_INTEGER(type, field, 1, LWMSG_UNSIGNED)

/**
 * @brief Define an unsigned 16-bit integer member
 *
 * Defines an unsigned 16-bit integer member of a struct or union.
 * This is a convenient shortcut for a full LWMSG_MEMBER_INTEGER()
 * invocation.
 *
 * @param type the name of the containing structure or union
 * @param field the name of the member
 * @hideinitializer
 */
#define LWMSG_MEMBER_UINT16(type, field) LWMSG_MEMBER_INTEGER(type, field, 2, LWMSG_UNSIGNED)

/**
 * @brief Define an unsigned 32-bit integer member
 *
 * Defines an unsigned 32-bit integer member of a struct or union.
 * This is a convenient shortcut for a full LWMSG_MEMBER_INTEGER()
 * invocation.
 *
 * @param type the name of the containing structure or union
 * @param field the name of the member
 * @hideinitializer
 */
#define LWMSG_MEMBER_UINT32(type, field) LWMSG_MEMBER_INTEGER(type, field, 4, LWMSG_UNSIGNED)

/**
 * @brief Define an unsigned 64-bit integer member
 *
 * Defines an unsigned 64-bit integer member of a struct or union.
 * This is a convenient shortcut for a full LWMSG_MEMBER_INTEGER()
 * invocation.
 *
 * @param type the name of the containing structure or union
 * @param field the name of the member
 * @hideinitializer
 */
#define LWMSG_MEMBER_UINT64(type, field) LWMSG_MEMBER_INTEGER(type, field, 8, LWMSG_UNSIGNED)

/**
 * @brief Define a signed 8-bit integer
 *
 * Defines a signed 8-bit integer type.  This is a convenient
 * shortcut for a full LWMSG_INTEGER() invocation.
 *
 * @param type the unmarshalled type
 * @hideinitializer
 */
#define LWMSG_INT8(type) LWMSG_INTEGER(type, 1, LWMSG_SIGNED)

/**
 * @brief Define a signed 16-bit integer
 *
 * Defines a signed 16-bit integer type.  This is a convenient
 * shortcut for a full LWMSG_INTEGER() invocation.
 *
 * @param type the unmarshalled type
 * @hideinitializer
 */
#define LWMSG_INT16(type) LWMSG_INTEGER(type, 2, LWMSG_SIGNED)

/**
 * @brief Define a signed 32-bit integer
 *
 * Defines a signed 32-bit integer type.  This is a convenient
 * shortcut for a full LWMSG_INTEGER() invocation.
 *
 * @param type the unmarshalled type
 * @hideinitializer
 */
#define LWMSG_INT32(type) LWMSG_INTEGER(type, 4, LWMSG_SIGNED)

/**
 * @brief Define a signed 64-bit integer
 *
 * Defines a signed 64-bit integer type.  This is a convenient
 * shortcut for a full LWMSG_INTEGER() invocation.
 *
 * @param type the unmarshalled type
 * @hideinitializer
 */
#define LWMSG_INT64(type) LWMSG_INTEGER(type, 8, LWMSG_SIGNED)

/**
 * @brief Define an unsigned 8-bit integer
 *
 * Defines an unsigned 8-bit integer type.  This is a convenient
 * shortcut for a full LWMSG_INTEGER() invocation.
 *
 * @param type the unmarshalled type
 * @hideinitializer
 */
#define LWMSG_UINT8(type) LWMSG_INTEGER(type, 1, LWMSG_UNSIGNED)

/**
 * @brief Define an unsigned 16-bit integer
 *
 * Defines an unsigned 16-bit integer type.  This is a convenient
 * shortcut for a full LWMSG_INTEGER() invocation.
 *
 * @param type the unmarshalled type
 * @hideinitializer
 */
#define LWMSG_UINT16(type) LWMSG_INTEGER(type, 2, LWMSG_UNSIGNED)

/**
 * @brief Define an unsigned 32-bit integer
 *
 * Defines an unsigned 32-bit integer type.  This is a convenient
 * shortcut for a full LWMSG_INTEGER() invocation.
 *
 * @param type the unmarshalled type
 * @hideinitializer
 */
#define LWMSG_UINT32(type) LWMSG_INTEGER(type, 4, LWMSG_UNSIGNED)

/**
 * @brief Define an unsigned 64-bit integer
 *
 * Defines an unsigned 64-bit integer type.  This is a convenient
 * shortcut for a full LWMSG_INTEGER() invocation.
 *
 * @param type the unmarshalled type
 * @hideinitializer
 */
#define LWMSG_UINT64(type) LWMSG_INTEGER(type, 8, LWMSG_UNSIGNED)

/**
 * @brief Define a member pointer to a string
 *
 * Defines a pointer to an 8-bit, null-terminated character string
 * as a member of a struct or union.  This is a convenient shortcut
 * for marshalling plain C strings, and is equivalent to the following:
 *
 * @code
 * LWMSG_MEMBER_POINTER_BEGIN(type, field),
 * LWMSG_UINT8(char),
 * LWMSG_POINTER_END,
 * LWMSG_ATTR_STRING
 * @endcode
 *
 * @param type the containing struct or union type
 * @param field the member of the struct or union
 * @hideinitializer
 */
#define LWMSG_MEMBER_PSTR(type, field)          \
    LWMSG_MEMBER_POINTER_BEGIN(type, field),    \
    LWMSG_UINT8(char),                          \
    LWMSG_POINTER_END,                          \
    LWMSG_ATTR_STRING

/**
 * @brief Define a pointer to a string
 *
 * Defines a pointer to an 8-bit, null-terminated character string.
 * This is a convenient shortcut for marshalling plain C strings,
 * and is equivalent to the following:
 *
 * @code
 * LWMSG_POINTER_BEGIN,
 * LWMSG_UINT8(char),
 * LWMSG_POINTER_END,
 * LWMSG_ATTR_STRING
 * @endcode
 *
 * @param type the containing struct or union type
 * @param field the member of the struct or union
 * @hideinitializer
 */
#define LWMSG_PSTR       \
    LWMSG_POINTER_BEGIN, \
    LWMSG_UINT8(char),   \
    LWMSG_POINTER_END,   \
    LWMSG_ATTR_STRING

/**
 * @brief Define a structure (compact)
 *
 * Defines a structure in a more compact fashion.  It is equivalent
 * to the following expanded form:
 *
 * @code
 * LWMSG_STRUCT_BEGIN(type),
 * ... ,
 * LWMSG_STRUCT_END
 * @endcode
 * 
 * @param type the C structure type
 * @param ... the contents of the structure specification
 * @hideinitializer
 */
#define LWMSG_STRUCT(type, ...)  \
    LWMSG_STRUCT_BEGIN(type),    \
    __VA_ARGS__,                 \
    LWMSG_STRUCT_END

/**
 * @brief Define a pointer (compact)
 *
 * Defines a pointer in a more compact fashion.  It is equivalent
 * to the following expanded form:
 *
 * @code
 * LWMSG_POINTER_BEGIN,
 * ... ,
 * LWMSG_POINTER_END
 * @endcode
 * 
 * @param ... the contents of the pointer specification
 * @hideinitializer
 */
#define LWMSG_POINTER(...) \
    LWMSG_POINTER_BEGIN,   \
    __VA_ARGS__,           \
    LWMSG_POINTER_END

/**
 * @brief Define a union (compact)
 *
 * Defines a union in a more compact fashion.  It is equivalent
 * to the following expanded form:
 *
 * @code
 * LWMSG_UNION_BEGIN(type),
 * ... ,
 * LWMSG_UNION_END
 * @endcode
 * 
 * @param type the C union type
 * @param ... the contents of the union specification
 * @hideinitializer
 */
#define LWMSG_UNION(type, ...)   \
    LWMSG_UNION_BEGIN(type),     \
    __VA_ARGS__,                 \
    LWMSG_UNION_END


/**
 * @brief Define a structure as a member (compact)
 *
 * Defines a structure as a member in a more compact fashion.
 * It is equivalent to the following expanded form:
 *
 * @code
 * LWMSG_MEMBER_STRUCT_BEGIN(type, field),
 * ... ,
 * LWMSG_STRUCT_END
 * @endcode
 * 
 * @param type the containing C type
 * @param field the member name
 * @param ... the contents of the structure specification
 * @hideinitializer
 */
#define LWMSG_MEMBER_STRUCT(type, field, ...)  \
    LWMSG_MEMBER_STRUCT_BEGIN(type, field),    \
    __VA_ARGS__,                               \
    LWMSG_STRUCT_END


/**
 * @brief Define a pointer as a member (compact)
 *
 * Defines a pointer as a member in a more compact fashion.
 * It is equivalent to the following expanded form:
 *
 * @code
 * LWMSG_MEMBER_POINTER_BEGIN(type, field),
 * ... ,
 * LWMSG_POINTER_END
 * @endcode
 * 
 * @param type the containing C type
 * @param field the member name
 * @param ... the contents of the structure specification
 * @hideinitializer
 */
#define LWMSG_MEMBER_POINTER(type, field, ...) \
    LWMSG_MEMBER_POINTER_BEGIN(type, field),   \
    __VA_ARGS__,                               \
    LWMSG_POINTER_END

/**
 * @brief Define a union as a member (compact)
 *
 * Defines a union as a member in a more compact fashion.
 * It is equivalent to the following expanded form:
 *
 * @code
 * LWMSG_MEMBER_UNION_BEGIN(type, field),
 * ... ,
 * LWMSG_UNION_END
 * @endcode
 * 
 * @param type the containing C type
 * @param field the member name
 * @param ... the contents of the union specification
 * @hideinitializer
 */
#define LWMSG_MEMBER_UNION(type, field, ...)   \
    LWMSG_MEMBER_UNION_BEGIN(type, field),     \
    __VA_ARGS__,                               \
    LWMSG_UNION_END

/**
 * @brief Define an array as a member (compact)
 *
 * Defines an array as a member in a more compact fashion.
 * It is equivalent to the following expanded form:
 *
 * @code
 * LWMSG_MEMBER_ARRAY_BEGIN(type, field),
 * ... ,
 * LWMSG_ARRAY_END
 * @endcode
 * 
 * @param type the containing C type
 * @param field the member name
 * @param ... the contents of the structure specification
 * @hideinitializer
 */
#define LWMSG_MEMBER_ARRAY(type, field, ...)   \
    LWMSG_MEMBER_ARRAY_BEGIN(type, field),     \
    __VA_ARGS__,                               \
    LWMSG_ARRAY_END

/**
 * @brief Get type flags
 *
 * Gets the flags for the specified type.
 *
 * @param[in] type the type
 * @return the flags for the type
 */
LWMsgTypeFlags
lwmsg_type_get_flags(
    const LWMsgType* type
    );

/**
 * @brief Get custom type flags
 *
 * Gets custom flags for the specified type.
 *
 * @param[in] type the type
 * @return custom flags for the type
 */
size_t
lwmsg_type_get_custom_flags(
    const LWMsgType* type
    );

/**
 * @brief Get integer range constraint
 *
 * Gets the bounds of an integer type.
 * The specified type must be an integer type
 * and must have the #LWMSG_TYPE_FLAG_RANGE flag
 * set.
 *
 * @param[in] type the type
 * @param[out] low the lowest allowed value
 * @param[out] high the highest allowed value
 * @lwmsg_status
 * @lwmsg_success
 * @lwmsg_code{INVALID_PARAMETER, the type was not an integer or did not have a range set}
 * @lwmsg_endstatus
 */
LWMsgStatus
lwmsg_type_get_integer_range(
    const LWMsgType* type,
    size_t* low,
    size_t* high
    );

/*@}*/

#endif
