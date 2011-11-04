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
 *        types.h
 *
 * Abstract:
 *
 *        Common type and constant definitions
 *
 * Authors: Brian Koropoff (bkoropoff@likewise.com)
 *          Danilo Almeida (dalmeida@likewise.com)
 *          Rafal Szczesniak (rafal@likewise.com)
 *
 */

#ifndef __LWBASE_TYPES_H__
#define __LWBASE_TYPES_H__

//
// Define base sized integer types based on whether doing DCE IDL.
// The fundamental issue is that this file can be included in IDL
// files but it cannot include system headers in that case.
// So for IDL purposes, some type definitions are skipped.
// In particular, the following are skipped:
//
// TODO-Get rid of LW_(P)(U)INT types.
// - LW_(P)(U)INT since it is based on "int", which is not
//   considered to be of any particular size by DCE IDL.
//
// - LW_(P)HANDLE/LW_PHANDLE due to [context_handle] attribute issues.
//
// - LW_(P)(S)SIZE_T and LW_(P)(U)LONG_PTR types since they are based on
//   variable length integer types that cannot be represented in DCE IDL.
//
// Note that <S>int<N>_t types cannot be used in unprotected typedefs
// because otherwise the resulting DCE RPC stubs will have conflicts
// with system headers.  Therefore, the LW_<S>INT<N> types are
// used instead with different definitions depending on whether
// this is a DCE IDL compiler pass.
//

#ifdef _DCE_IDL_

cpp_quote("#include <lw/types.h>")
cpp_quote("#if 0")

#endif

#ifndef _DCE_IDL_
#include <stddef.h>
#include <inttypes.h>
#include <wchar16.h>
#include <sys/types.h>

#define _LW_IDL_STRING

typedef uint8_t            LW_UINT8, *LW_PUINT8;
typedef uint16_t           LW_UINT16, *LW_PUINT16;
typedef uint32_t           LW_UINT32, *LW_PUINT32;
typedef uint64_t           LW_UINT64, *LW_PUINT64;
typedef int8_t             LW_INT8, *LW_PINT8;
typedef int16_t            LW_INT16, *LW_PINT16;
typedef int32_t            LW_INT32, *LW_PINT32;
typedef int64_t            LW_INT64, *LW_PINT64;

#else

#define _LW_IDL_STRING [string]

typedef unsigned short int wchar16_t;

typedef unsigned small int LW_UINT8, *LW_PUINT8;
typedef unsigned short int LW_UINT16, *LW_PUINT16;
typedef unsigned long int  LW_UINT32, *LW_PUINT32;
typedef unsigned hyper int LW_UINT64, *LW_PUINT64;
typedef small int          LW_INT8, *LW_PINT8;
typedef short int          LW_INT16, *LW_PINT16;
typedef long int           LW_INT32, *LW_PINT32;
typedef hyper int          LW_INT64, *LW_PINT64;

#define const
#endif

typedef void               LW_VOID, *LW_PVOID;
typedef void const        *LW_PCVOID;
#ifdef __cplusplus
#define LW_VOID void
#define LW_PVOID void*
#define LW_PCVOID const void*
#endif
typedef LW_UINT8           LW_BOOLEAN, *LW_PBOOLEAN;
typedef LW_UINT8           LW_BYTE, *LW_PBYTE;
typedef char               LW_CHAR, *LW_PCHAR;
// DCE IDL does not allow a char type to be used in size_is attribute.
#ifndef _DCE_IDL_
typedef unsigned char      LW_UCHAR, *LW_PUCHAR;
#else
typedef LW_UINT8           LW_UCHAR, *LW_PUCHAR;
#endif
typedef wchar16_t          LW_WCHAR, *LW_PWCHAR;
typedef LW_INT16           LW_SHORT, *LW_PSHORT;
typedef LW_UINT16          LW_USHORT, *LW_PUSHORT;
typedef LW_INT32           LW_LONG, *LW_PLONG;
typedef LW_UINT32          LW_ULONG, *LW_PULONG;
typedef LW_INT64           LW_LONG64, *LW_PLONG64;
typedef LW_UINT64          LW_ULONG64, *LW_PULONG64;
#ifndef _DCE_IDL_
typedef int                LW_INT, *LW_PINT;
typedef unsigned int       LW_UINT, *LW_PUINT;
typedef void              *LW_HANDLE, **LW_PHANDLE;
typedef size_t             LW_SIZE_T, *LW_PSIZE_T;
typedef ssize_t            LW_SSIZE_T, *LW_PSSIZE_T;
typedef size_t             LW_ULONG_PTR, *LW_PULONG_PTR;
typedef ssize_t            LW_LONG_PTR, *LW_PLONG_PTR;
#endif

typedef _LW_IDL_STRING char              *LW_PSTR;
typedef _LW_IDL_STRING char const        *LW_PCSTR;
typedef _LW_IDL_STRING wchar16_t         *LW_PWSTR;
typedef _LW_IDL_STRING wchar16_t const   *LW_PCWSTR;

typedef LW_UINT32          LW_BOOL, *LW_PBOOL;
typedef LW_UINT16          LW_WORD, *LW_PWORD;
typedef LW_UINT32          LW_DWORD, *LW_PDWORD;

typedef LW_DWORD LW_WINERROR, *LW_PWINERROR;
typedef LW_LONG  LW_NTSTATUS, *LW_PNTSTATUS;

#ifdef UNICODE
typedef LW_WCHAR           LW_TCHAR;
#else
typedef LW_CHAR            LW_TCHAR;
#endif 

typedef struct _LW_GUID {
    LW_ULONG Data1;
    LW_USHORT Data2;
    LW_USHORT Data3;
#ifndef _DCE_IDL_
    LW_UCHAR Data4[8];
#else
    LW_BYTE Data4[8];
#endif
} LW_GUID, *LW_PGUID;


//
// Locally unique identifier (LUID).
//

typedef struct _LW_LUID {
    LW_ULONG LowPart;
    LW_LONG HighPart;
} LW_LUID, *LW_PLUID;

#define LW_TRUE  1
#define LW_FALSE 0

// ISSUE-On Windows, this is ((HANDLE)-1).  Should not define this here
// as it is an artifact of a few Win32 APIs (such as CreateFile).
#define LW_INVALID_HANDLE_VALUE 0

#define LW_MAXUCHAR ((LW_UCHAR) -1)
#define LW_MAXCHAR ((LW_CHAR) (LW_MAXUCHAR >> 1))
#define LW_MINCHAR ((LW_CHAR) ~LW_MAXCHAR)

#define LW_MAXUSHORT ((LW_USHORT) -1)
#define LW_MAXSHORT ((LW_SHORT) (LW_MAXUSHORT >> 1))
#define LW_MINSHORT ((LW_SHORT) ~LW_MAXSHORT)

#define LW_MAXULONG ((LW_ULONG) -1)
#define LW_MAXLONG ((LW_LONG) (LW_MAXULONG >> 1))
#define LW_MINLONG ((LW_LONG) ~LW_MAXLONG)

#define LW_MAXULONG64 ((LW_ULONG64) -1)
#define LW_MAXLONG64 ((LW_LONG64) (LW_MAXULONG64 >> 1))
#define LW_MINLONG64 ((LW_LONG64) ~LW_MAXLONG64)

#define LW_MAXSIZE_T ((LW_SIZE_T) -1)
#define LW_MAXSSIZE_T ((LW_SSIZE_T) (LW_MAXSIZE_T >> 1))
#define LW_MINSSIZE_T ((LW_SSIZE_T) ~LW_MAXSSIZE_T)

#define LW_MAXULONG_PTR ((LW_ULONG_PTR) -1)
#define LW_MAXLONG_PTR ((LW_LONG_PTR) (LW_MAXULONG_PTR >> 1))
#define LW_MINLONG_PTR ((LW_SSIZE_T) ~LW_MAXLONG_PTR)

#define LW_MAXBYTE ((LW_BYTE) -1)
#define LW_MAXWORD ((LW_WORD) -1)
#define LW_MAXDWORD ((LW_DWORD) -1)

#define LW_OUT_PPVOID(_expr_) ((LW_PVOID*) (LW_PVOID) (_expr_))

#define LW_MIN(a, b) (((a) < (b)) ? (a) : (b))
#define LW_MAX(a, b) (((a) > (b)) ? (a) : (b))

#define LwSetFlag(Variable, Flags)   ((Variable) |= (Flags))
#define LwClearFlag(Variable, Flags) ((Variable) &= ~(Flags))
#define LwIsSetFlag(Variable, Flags) (((Variable) & (Flags)) != 0)

#define LW_IS_BOTH_OR_NEITHER(Condition1, Condition2) \
    !(!!(Condition1) ^ !!(Condition2))

#define LW_IS_VALID_FLAGS(Flags, Mask) \
    (((Flags) & ~(Mask)) == 0)

#define LwRtlPointerToOffset(BasePointer, Pointer) \
    ((int)((char*)(Pointer) - (char*)(BasePointer)))

#define LwRtlOffsetToPointer(Pointer, Offset) \
    ((char*)(Pointer) + Offset)

// TODO-Remove these deprecated macro names
#define LW_PTR_OFFSET(BasePointer, Pointer) LwRtlPointerToOffset(BasePointer, Pointer)
#define LW_PTR_ADD(Pointer, Offset) LwRtlOffsetToPointer(Pointer, Offset)

#define LW_FIELD_OFFSET(Type, Field) offsetof(Type, Field)

#define LW_FIELD_SIZE(Type, Field) \
    (sizeof(((Type*)(0))->Field))

#define LW_STRUCT_FROM_FIELD(Pointer, Type, Field) \
    ((Type*)LwRtlOffsetToPointer(Pointer, -((ssize_t)LW_FIELD_OFFSET(Type, Field))))

#define LW_ARRAY_SIZE(StaticArray) \
    (sizeof(StaticArray)/sizeof((StaticArray)[0]))

#ifndef LW_STRICT_NAMESPACE

typedef LW_UINT8    UINT8;
typedef LW_PUINT8   PUINT8;
typedef LW_UINT16   UINT16;
typedef LW_PUINT16  PUINT16;
typedef LW_UINT32   UINT32;
typedef LW_PUINT32  PUINT32;
typedef LW_UINT64   UINT64;
typedef LW_PUINT64  PUINT64;
typedef LW_INT8     INT8;
typedef LW_PINT8    PINT8;
typedef LW_INT16    INT16;
typedef LW_PINT16   PINT16;
typedef LW_INT32    INT32;
typedef LW_PINT32   PINT32;
typedef LW_INT64    INT64;
typedef LW_PINT64   PINT64;

typedef LW_VOID     VOID;
typedef LW_PVOID    PVOID;
typedef LW_PCVOID   PCVOID;
#ifdef __cplusplus
#define VOID void
#define PVOID void*
#define PCVOID const void*
#endif
typedef LW_BOOLEAN  BOOLEAN;
typedef LW_PBOOLEAN PBOOLEAN;
typedef LW_BYTE     BYTE;
typedef LW_PBYTE    PBYTE;
typedef LW_CHAR     CHAR;
typedef LW_WCHAR    WCHAR;
typedef LW_PCHAR    PCHAR;
typedef LW_PWCHAR   PWCHAR;
typedef LW_UCHAR    UCHAR;
typedef LW_PUCHAR   PUCHAR;
typedef LW_SHORT    SHORT;
typedef LW_PSHORT   PSHORT;
typedef LW_USHORT   USHORT;
typedef LW_PUSHORT  PUSHORT;
typedef LW_LONG     LONG;
typedef LW_PLONG    PLONG;
typedef LW_ULONG    ULONG;
typedef LW_PULONG   PULONG;
typedef LW_LONG64   LONG64;
typedef LW_PLONG64  PLONG64;
typedef LW_ULONG64  ULONG64;
typedef LW_PULONG64 PULONG64;
#ifndef _DCE_IDL_
typedef LW_INT      INT;
typedef LW_PINT     PINT;
typedef LW_UINT     UINT;
typedef LW_PUINT    PUINT;
typedef LW_HANDLE   HANDLE;
typedef LW_PHANDLE  PHANDLE;
typedef LW_SIZE_T   SIZE_T;
typedef LW_PSIZE_T  PSIZE_T;
typedef LW_SSIZE_T  SSIZE_T;
typedef LW_PSSIZE_T PSSIZE_T;
typedef LW_ULONG_PTR ULONG_PTR;
typedef LW_PULONG_PTR PULONG_PTR;
typedef LW_LONG_PTR LONG_PTR;
typedef LW_PLONG_PTR PLONG_PTR;
#endif

typedef LW_PSTR     PSTR;
typedef LW_PCSTR    PCSTR;
typedef LW_PWSTR    PWSTR;
typedef LW_PCWSTR   PCWSTR;

typedef LW_BOOL     BOOL;
typedef LW_PBOOL    PBOOL;
typedef LW_WORD     WORD;
typedef LW_PWORD    PWORD;
typedef LW_DWORD    DWORD;
typedef LW_PDWORD   PDWORD;

typedef LW_WINERROR  WINERROR;
typedef LW_PWINERROR PWINERROR;
typedef LW_NTSTATUS  NTSTATUS;
typedef LW_PNTSTATUS PNTSTATUS;

typedef LW_TCHAR    TCHAR;

typedef LW_GUID     GUID;
typedef LW_PGUID    PGUID;
typedef LW_LUID     LUID;
typedef LW_PLUID    PLUID;

#ifndef TRUE
#define TRUE LW_TRUE
#endif

#ifndef FALSE
#define FALSE LW_FALSE
#endif

#define INVALID_HANDLE_VALUE LW_INVALID_HANDLE_VALUE

#define MAXUCHAR LW_MAXUCHAR
#define MAXCHAR LW_MAXCHAR
#define MINCHAR LW_MINCHAR

#define MAXUSHORT LW_MAXUSHORT
#define MAXSHORT LW_MAXSHORT
#define MINSHORT LW_MINSHORT

#define MAXULONG LW_MAXULONG
#define MAXLONG LW_MAXLONG
#define MINLONG LW_MINLONG

#define MAXULONG64 LW_MAXULONG64
#define MAXLONG64 LW_MAXLONG64
#define MINLONG64 LW_MINLONG64

#define MAXSIZE_T LW_MAXSIZE_T
#define MAXSSIZE_T LW_MAXSSIZE_T
#define MINSSIZE_T LW_MINSSIZE_T

#define MAXULONG_PTR LW_MAXULONG_PTR
#define MAXLONG_PTR LW_MAXLONG_PTR
#define MINLONG_PTR LW_MINLONG_PTR

#define MAXBYTE LW_MAXBYTE
#define MAXWORD LW_MAXWORD
#define MAXDWORD LW_MAXDWORD

#define BYTE_MAX MAXBYTE
#define WORD_MAX MAXWORD
#define DWORD_MAX MAXDWORD

#define OUT_PPVOID LW_OUT_PPVOID

#define SetFlag(Variable, Flags) LwSetFlag(Variable, Flags)
#define ClearFlag(Variable, Flags) LwClearFlag(Variable, Flags)
#define IsSetFlag(Variable, Flags) LwIsSetFlag(Variable, Flags)

#define IS_BOTH_OR_NEITHER(Condition1, Condition2) LW_IS_BOTH_OR_NEITHER(Condition1, Condition2)

#endif /* LW_STRICT_NAMESPACE */

typedef struct _LW_UNICODE_STRING {
    LW_USHORT Length;
    LW_USHORT MaximumLength;
#ifdef _DCE_IDL_
    [size_is(MaximumLength/2),length_is(Length/2)]
#endif
    LW_PWCHAR Buffer;
} LW_UNICODE_STRING, *LW_PUNICODE_STRING;

// Need to do division and multiplication to round down.
#define LW_UNICODE_STRING_MAX_CHARS (LW_MAXUSHORT / 2)
#define LW_UNICODE_STRING_MAX_BYTES (LW_UNICODE_STRING_MAX_CHARS * 2)

typedef struct _LW_ANSI_STRING {
    LW_USHORT Length;
    LW_USHORT MaximumLength;
#ifdef _DCE_IDL_
    [size_is(MaximumLength),length_is(Length)]
#endif
    LW_PCHAR Buffer;
} LW_ANSI_STRING, *LW_PANSI_STRING;

// TODO-Perhaps make these shorter to match LW_UNICODE_STRING_MAX_CHARS.
#define LW_ANSI_STRING_MAX_CHARS LW_MAXUSHORT
#define LW_ANSI_STRING_MAX_BYTES LW_MAXUSHORT

// Works on any character size strings/cstrings:
#define LW_RTL_STRING_IS_NULL_OR_EMPTY(String) (!(String) || !((String)->Length))
#define LwRtlCStringIsNullOrEmpty(String) (!(String) || !(*(String)))

#define LW_RTL_CONSTANT_STRING(StringLiteral) \
    { \
        sizeof(StringLiteral) - sizeof((StringLiteral)[0]), \
        sizeof(StringLiteral), \
        (StringLiteral) \
    }

#define LW_RTL_STRING_NUM_CHARS(String) \
    ( (String)->Length / sizeof((String)->Buffer[0]) )

#define LW_RTL_STRING_LAST_CHAR(String) \
    ( (String)->Buffer[LW_RTL_STRING_NUM_CHARS(String) - 1] )

#define LW_RTL_STRING_IS_NULL_TERMINATED(String) \
    ((String) && \
     (String)->Buffer && \
     ((String)->MaximumLength > (String)->Length) && \
     !(String)->Buffer[LW_RTL_STRING_NUM_CHARS(String)])

// This works for CHAR and WCHAR since WCHAR is native byte order.
#define LwRtlIsDecimalDigit(Character) \
    ( ((Character) >= '0') && ((Character) <= '9') )

// This works for CHAR and WCHAR since WCHAR is native byte order.
#define LwRtlDecimalDigitValue(Character) \
    ((Character) - '0')

#define LW_RTL_MAKE_CUSTOM_FREE(FreeFunction, PointerToPointer) \
    do { \
        if (*(PointerToPointer)) \
        { \
            (FreeFunction)(*(PointerToPointer)); \
            *(PointerToPointer) = NULL; \
        } \
    } while (0)

#define LW_RTL_MAKE_CUSTOM_FREE_SECURE_STRING(FreeFunction, PointerToPointer, CharType) \
    do { \
        if (*(PointerToPointer)) \
        { \
            CharType* pOverwrite; \
            for (pOverwrite = *(PointerToPointer); *pOverwrite; pOverwrite++) \
            { \
                *pOverwrite = 0; \
            } \
            (FreeFunction)(*(PointerToPointer)); \
            *(PointerToPointer) = NULL; \
        } \
    } while (0)

#ifndef LW_STRICT_NAMESPACE

typedef LW_UNICODE_STRING UNICODE_STRING;
typedef LW_PUNICODE_STRING PUNICODE_STRING;

#define UNICODE_STRING_MAX_CHARS LW_UNICODE_STRING_MAX_CHARS
#define UNICODE_STRING_MAX_BYTES LW_UNICODE_STRING_MAX_BYTES

typedef LW_ANSI_STRING ANSI_STRING;
typedef LW_PANSI_STRING PANSI_STRING;

#define ANSI_STRING_MAX_CHARS LW_ANSI_STRING_MAX_CHARS
#define ANSI_STRING_MAX_BYTES LW_ANSI_STRING_MAX_BYTES

#define RTL_STRING_IS_NULL_OR_EMPTY(String) LW_RTL_STRING_IS_NULL_OR_EMPTY(String) 
#define RtlCStringIsNullOrEmpty(String) LwRtlCStringIsNullOrEmpty(String)

#define RTL_CONSTANT_STRING(StringLiteral) LW_RTL_CONSTANT_STRING(StringLiteral)

#define RTL_STRING_NUM_CHARS(String) LW_RTL_STRING_NUM_CHARS(String)
#define RTL_STRING_LAST_CHAR(String) LW_RTL_STRING_LAST_CHAR(String)
#define RTL_STRING_IS_NULL_TERMINATED(String) LW_RTL_STRING_IS_NULL_TERMINATED(String)

#define RtlIsDecimalDigit(Character) LwRtlIsDecimalDigit(Character)
#define RtlDecimalDigitValue(Character) LwRtlDecimalDigitValue(Character)

#endif /* LW_STRICT_NAMESPACE */

#ifdef _DCE_IDL_

cpp_quote("#endif")

#endif

#endif
