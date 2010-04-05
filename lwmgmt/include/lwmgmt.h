/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */
 
/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        lwmgmt.h
 *
 * Abstract:
 *
 *        Likewise Management Service (LWMGMT)
 *
 *        Public header
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */
#ifndef __LWMGMT_H__
#define __LWMGMT_H__

#ifndef _WIN32
#include <lw/types.h>
#include <lw/attrs.h>
#endif

/* ERRORS */
#define LWMGMT_ERROR_SUCCESS                   0x0000
#define LWMGMT_ERROR_INVALID_CONFIG_PATH       0xD001 // 53249
#define LWMGMT_ERROR_INVALID_PREFIX_PATH       0xD002 // 53250
#define LWMGMT_ERROR_INSUFFICIENT_BUFFER       0xD003 // 53251
#define LWMGMT_ERROR_OUT_OF_MEMORY             0xD004 // 53252
#define LWMGMT_ERROR_INVALID_MESSAGE           0xD005 // 53253
#define LWMGMT_ERROR_UNEXPECTED_MESSAGE        0xD006 // 53254
#define LWMGMT_ERROR_NO_SUCH_USER              0xD007 // 53255
#define LWMGMT_ERROR_DATA_ERROR                0xD008 // 53256
#define LWMGMT_ERROR_NOT_IMPLEMENTED           0xD009 // 53257
#define LWMGMT_ERROR_NO_CONTEXT_ITEM           0xD00A // 53258
#define LWMGMT_ERROR_NO_SUCH_GROUP             0xD00B // 53259
#define LWMGMT_ERROR_REGEX_COMPILE_FAILED      0xD00C // 53260
#define LWMGMT_ERROR_NSS_EDIT_FAILED           0xD00D // 53261
#define LWMGMT_ERROR_NO_HANDLER                0xD00E // 53262
#define LWMGMT_ERROR_INTERNAL                  0xD00F // 53263
#define LWMGMT_ERROR_NOT_HANDLED               0xD010 // 53264
#define LWMGMT_ERROR_UNEXPECTED_DB_RESULT      0xD011 // 53265
#define LWMGMT_ERROR_INVALID_PARAMETER         0xD012 // 53266
#define LWMGMT_ERROR_LOAD_LIBRARY_FAILED       0xD013 // 53267
#define LWMGMT_ERROR_LOOKUP_SYMBOL_FAILED      0xD014 // 53268
#define LWMGMT_ERROR_INVALID_EVENTLOG          0xD015 // 53269
#define LWMGMT_ERROR_INVALID_CONFIG            0xD016 // 53270
#define LWMGMT_ERROR_STRING_CONV_FAILED        0xD017 // 53271
#define LWMGMT_ERROR_INVALID_DB_HANDLE         0xD018 // 53272
#define LWMGMT_ERROR_FAILED_CONVERT_TIME       0xD019 // 53273
#define LWMGMT_ERROR_RPC_EXCEPTION_UPON_RPC_BINDING 0xD01A // 53274
#define LWMGMT_ERROR_RPC_EXCEPTION_UPON_OPEN     0xD01B // 53275
#define LWMGMT_ERROR_RPC_EXCEPTION_UPON_CLOSE    0xD01C // 53276
#define LWMGMT_ERROR_RPC_EXCEPTION_UPON_COUNT    0xD01D // 53277
#define LWMGMT_ERROR_RPC_EXCEPTION_UPON_READ     0xD01E // 53278
#define LWMGMT_ERROR_RPC_EXCEPTION_UPON_WRITE    0xD01F // 53279
#define LWMGMT_ERROR_RPC_EXCEPTION_UPON_CLEAR    0xD020 // 53280
#define LWMGMT_ERROR_RPC_EXCEPTION_UPON_DELETE   0xD021 // 53281
#define LWMGMT_ERROR_RPC_EXCEPTION_UPON_REGISTER 0xD022 // 53282
#define LWMGMT_ERROR_RPC_EXCEPTION_UPON_UNREGISTER 0xD023 // 53283
#define LWMGMT_ERROR_RPC_EXCEPTION_UPON_LISTEN     0xD024 // 53284
#define LWMGMT_ERROR_KRB5_CALL_FAILED              0xD025 // 53285
#define LWMGMT_ERROR_KRB5_KT_BADNAME               0xD026 // 53286
#define LWMGMT_ERROR_KRB5_KT_NOTFOUND              0xD027 // 53287
#define LWMGMT_ERROR_KRB5_KT_END                   0xD028 // 53288
#define LWMGMT_ERROR_KRB5_KT_NOWRITE               0xD029 // 53289
#define LWMGMT_ERROR_KRB5_KT_IOERR                 0xD02A // 53290
#define LWMGMT_ERROR_KRB5_KT_NAME_TOOLONG          0xD02B // 53291
#define LWMGMT_ERROR_KRB5_KT_KVNONOTFOUND          0xD02C // 53292
#define LWMGMT_ERROR_KRB5_KT_FORMAT                0xD02D // 53293
#define LWMGMT_ERROR_KRB5_KT_BADVNO                0xD02E // 53294
#define LWMGMT_ERROR_KRB5_KT_EACCES                0xD02F // 53295
#define LWMGMT_ERROR_KRB5_KT_ENOENT                0xD030 // 53296
#define LWMGMT_ERROR_SENTINEL                      0xD031 // 53297

#define LWMGMT_ERROR_MASK(_e_)             (_e_ & 0xD000)


#endif /* __LWMGMT_H__ */
