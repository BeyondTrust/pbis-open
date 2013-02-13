/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

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

#ifndef __LW_ERROR_H__
#define __LW_ERROR_H__

#include <lw/types.h>
#include <lw/attrs.h>
#include <lw/winerror.h>
#include <lw/ntstatus.h>

#include <lwmsg/status.h>

/** Success */
#define LW_ERROR_SUCCESS                                   0

#define LW_ERROR_INVALID_CACHE_PATH                        40001
#define LW_ERROR_INVALID_CONFIG_PATH                       40002
#define LW_ERROR_INVALID_PREFIX_PATH                       40003
#define LW_ERROR_INSUFFICIENT_BUFFER                       40004
#define LW_ERROR_OUT_OF_MEMORY                             40005
#define LW_ERROR_INVALID_MESSAGE                           40006
#define LW_ERROR_UNEXPECTED_MESSAGE                        40007
#define LW_ERROR_NO_SUCH_USER                              40008
#define LW_ERROR_DATA_ERROR                                40009
#define LW_ERROR_NOT_IMPLEMENTED                           40010
#define LW_ERROR_NO_CONTEXT_ITEM                           40011
#define LW_ERROR_NO_SUCH_GROUP                             40012
#define LW_ERROR_REGEX_COMPILE_FAILED                      40013
#define LW_ERROR_NSS_EDIT_FAILED                           40014
#define LW_ERROR_NO_HANDLER                                40015
#define LW_ERROR_INTERNAL                                  40016
#define LW_ERROR_NOT_HANDLED                               40017
#define LW_ERROR_INVALID_DNS_RESPONSE                      40018
#define LW_ERROR_DNS_RESOLUTION_FAILED                     40019
#define LW_ERROR_FAILED_TIME_CONVERSION                    40020
#define LW_ERROR_INVALID_SID                               40021
#define LW_ERROR_PASSWORD_MISMATCH                         40022
#define LW_ERROR_UNEXPECTED_DB_RESULT                      40023
#define LW_ERROR_PASSWORD_EXPIRED                          40024
#define LW_ERROR_ACCOUNT_EXPIRED                           40025
#define LW_ERROR_USER_EXISTS                               40026
#define LW_ERROR_GROUP_EXISTS                              40027
#define LW_ERROR_INVALID_GROUP_INFO_LEVEL                  40028
#define LW_ERROR_INVALID_USER_INFO_LEVEL                   40029
#define LW_ERROR_UNSUPPORTED_USER_LEVEL                    40030
#define LW_ERROR_UNSUPPORTED_GROUP_LEVEL                   40031
#define LW_ERROR_INVALID_LOGIN_ID                          40032
#define LW_ERROR_INVALID_HOMEDIR                           40033
#define LW_ERROR_INVALID_GROUP_NAME                        40034
#define LW_ERROR_NO_MORE_GROUPS                            40035
#define LW_ERROR_NO_MORE_USERS                             40036
#define LW_ERROR_FAILED_ADD_USER                           40037
#define LW_ERROR_FAILED_ADD_GROUP                          40038
#define LW_ERROR_INVALID_LSA_CONNECTION                    40039
#define LW_ERROR_INVALID_AUTH_PROVIDER                     40040
#define LW_ERROR_INVALID_PARAMETER                         40041
#define LW_ERROR_LDAP_NO_PARENT_DN                         40042
#define LW_ERROR_LDAP_ERROR                                40043
#define LW_ERROR_NO_SUCH_DOMAIN                            40044
#define LW_ERROR_LDAP_FAILED_GETDN                         40045
#define LW_ERROR_DUPLICATE_DOMAINNAME                      40046
#define LW_ERROR_KRB5_CALL_FAILED                          40047
#define LW_ERROR_GSS_CALL_FAILED                           40048
#define LW_ERROR_FAILED_FIND_DC                            40049
#define LW_ERROR_NO_SUCH_CELL                              40050
#define LW_ERROR_GROUP_IN_USE                              40051
#define LW_ERROR_FAILED_CREATE_HOMEDIR                     40052
#define LW_ERROR_PASSWORD_TOO_WEAK                         40053
#define LW_ERROR_INVALID_SID_REVISION                      40054
#define LW_ERROR_ACCOUNT_LOCKED                            40055
#define LW_ERROR_ACCOUNT_DISABLED                          40056
#define LW_ERROR_USER_CANNOT_CHANGE_PASSWD                 40057
#define LW_ERROR_LOAD_LIBRARY_FAILED                       40058
#define LW_ERROR_LOOKUP_SYMBOL_FAILED                      40059
#define LW_ERROR_INVALID_EVENTLOG                          40060
#define LW_ERROR_INVALID_CONFIG                            40061
#define LW_ERROR_UNEXPECTED_TOKEN                          40062
#define LW_ERROR_LDAP_NO_RECORDS_FOUND                     40063
#define LW_ERROR_DUPLICATE_USERNAME                        40064
#define LW_ERROR_DUPLICATE_GROUPNAME                       40065
#define LW_ERROR_DUPLICATE_CELLNAME                        40066
#define LW_ERROR_STRING_CONV_FAILED                        40067
#define LW_ERROR_INVALID_ACCOUNT                           40068
#define LW_ERROR_INVALID_PASSWORD                          40069
#define LW_ERROR_QUERY_CREATION_FAILED                     40070
#define LW_ERROR_NO_SUCH_OBJECT                            40071
#define LW_ERROR_DUPLICATE_USER_OR_GROUP                   40072
#define LW_ERROR_INVALID_KRB5_CACHE_TYPE                   40073
#define LW_ERROR_NOT_JOINED_TO_AD                          40074
#define LW_ERROR_FAILED_TO_SET_TIME                        40075
#define LW_ERROR_NO_NETBIOS_NAME                           40076
#define LW_ERROR_INVALID_NETLOGON_RESPONSE                 40077
#define LW_ERROR_INVALID_OBJECTGUID                        40078
#define LW_ERROR_INVALID_DOMAIN                            40079
#define LW_ERROR_NO_DEFAULT_REALM                          40080
#define LW_ERROR_NOT_SUPPORTED                             40081
#define LW_ERROR_LOGON_FAILURE                             40082
#define LW_ERROR_NO_SITE_INFORMATION                       40083
#define LW_ERROR_INVALID_LDAP_STRING                       40084
#define LW_ERROR_INVALID_LDAP_ATTR_VALUE                   40085
#define LW_ERROR_NULL_BUFFER                               40086
#define LW_ERROR_CLOCK_SKEW                                40087
#define LW_ERROR_KRB5_NO_KEYS_FOUND                        40088
#define LW_ERROR_SERVICE_NOT_AVAILABLE                     40089
#define LW_ERROR_INVALID_SERVICE_RESPONSE                  40090
#define LW_ERROR_NSS_ERROR                                 40091
#define LW_ERROR_AUTH_ERROR                                40092
#define LW_ERROR_INVALID_LDAP_DN                           40093
#define LW_ERROR_NOT_MAPPED                                40094
#define LW_ERROR_RPC_NETLOGON_FAILED                       40095
#define LW_ERROR_ENUM_DOMAIN_TRUSTS_FAILED                 40096
#define LW_ERROR_RPC_LSABINDING_FAILED                     40097
#define LW_ERROR_RPC_OPENPOLICY_FAILED                     40098
#define LW_ERROR_RPC_LSA_LOOKUPNAME2_FAILED                40099
#define LW_ERROR_RPC_SET_SESS_CREDS_FAILED                 40100
#define LW_ERROR_RPC_REL_SESS_CREDS_FAILED                 40101
#define LW_ERROR_RPC_CLOSEPOLICY_FAILED                    40102
#define LW_ERROR_RPC_LSA_LOOKUPNAME2_NOT_FOUND             40103
#define LW_ERROR_RPC_LSA_LOOKUPNAME2_FOUND_DUPLICATES      40104
#define LW_ERROR_NO_TRUSTED_DOMAIN_FOUND                   40105
#define LW_ERROR_INCOMPATIBLE_MODES_BETWEEN_TRUSTEDDOMAINS 40106
#define LW_ERROR_DCE_CALL_FAILED                           40107
#define LW_ERROR_FAILED_TO_LOOKUP_DC                       40108
#define LW_ERROR_INVALID_NSS_ARTEFACT_INFO_LEVEL           40109
#define LW_ERROR_UNSUPPORTED_NSS_ARTEFACT_LEVEL            40110
#define LW_ERROR_INVALID_USER_NAME                         40111
#define LW_ERROR_INVALID_LOG_LEVEL                         40112
#define LW_ERROR_INVALID_METRIC_TYPE                       40113
#define LW_ERROR_INVALID_METRIC_PACK                       40114
#define LW_ERROR_INVALID_METRIC_INFO_LEVEL                 40115
#define LW_ERROR_FAILED_STARTUP_PREREQUISITE_CHECK         40116
#define LW_ERROR_MAC_FLUSH_DS_CACHE_FAILED                 40117
#define LW_ERROR_LSA_SERVER_UNREACHABLE                    40118
#define LW_ERROR_INVALID_NSS_ARTEFACT_TYPE                 40119
#define LW_ERROR_INVALID_AGENT_VERSION                     40120
#define LW_ERROR_DOMAIN_IS_OFFLINE                         40121
#define LW_ERROR_INVALID_HOMEDIR_TEMPLATE                  40122
#define LW_ERROR_RPC_PARSE_SID_STRING                      40123
#define LW_ERROR_RPC_LSA_LOOKUPSIDS_FAILED                 40124
#define LW_ERROR_RPC_LSA_LOOKUPSIDS_NOT_FOUND              40125
#define LW_ERROR_RPC_LSA_LOOKUPSIDS_FOUND_DUPLICATES       40126
#define LW_ERROR_PASSWORD_RESTRICTION                      40127
#define LW_ERROR_OBJECT_NOT_ENABLED                        40128
#define LW_ERROR_NO_MORE_NSS_ARTEFACTS                     40129
#define LW_ERROR_INVALID_NSS_MAP_NAME                      40130
#define LW_ERROR_INVALID_NSS_KEY_NAME                      40131
#define LW_ERROR_NO_SUCH_NSS_KEY                           40132
#define LW_ERROR_NO_SUCH_NSS_MAP                           40133
#define LW_ERROR_RPC_ERROR                                 40134
#define LW_ERROR_LDAP_SERVER_UNAVAILABLE                   40135
#define LW_ERROR_CREATE_KEY_FAILED                         40136
#define LW_ERROR_CANNOT_DETECT_USER_PROCESSES              40137
#define LW_ERROR_TRACE_NOT_INITIALIZED                     40138
#define LW_ERROR_NO_SUCH_TRACE_FLAG                        40139
#define LW_ERROR_DCERPC_ERROR                              40140
#define LW_ERROR_INVALID_RPC_SERVER                        40141
#define LW_ERROR_RPC_SERVER_REGISTRATION_ERROR             40142
#define LW_ERROR_RPC_SERVER_RUNTIME_ERROR                  40143
#define LW_ERROR_DOMAIN_IN_USE                             40144
#define LW_ERROR_SAM_DATABASE_ERROR                        40145
#define LW_ERROR_SAM_INIT_ERROR                            40146
#define LW_ERROR_OBJECT_IN_USE                             40147
#define LW_ERROR_NO_SUCH_ATTRIBUTE                         40148
#define LW_ERROR_GET_DC_NAME_FAILED                        40149
#define LW_ERROR_INVALID_ATTRIBUTE_VALUE                   40150
#define LW_ERROR_NO_ATTRIBUTE_VALUE                        40151
#define LW_ERROR_MEMBER_IN_LOCAL_GROUP                     40152
#define LW_ERROR_MEMBER_NOT_IN_LOCAL_GROUP                 40153
// Obsolete LW_ERROR_KRB5_S_PRINCIPAL_UNKNOWN              40154
#define LW_ERROR_INVALID_GROUP                             40155
#define LW_WARNING_CONTINUE_NEEDED                         40157
#define LW_ERROR_ACCESS_DENIED                             40158
#define LW_ERROR_NO_SUCH_PROCESS                           40159
#define LW_ERROR_INTERRUPTED                               40160
#define LW_ERROR_GENERIC_IO                                40161
#define LW_ERROR_INVALID_HANDLE                            40162

#define LW_ERROR_ERRNO_ENXIO                               40163
#define LW_ERROR_ERRNO_E2BIG                               40164
#define LW_ERROR_ERRNO_ENOEXEC                             40165
#define LW_ERROR_ERRNO_ECHILD                              40166
#define LW_ERROR_ERRNO_EAGAIN                              40167
#define LW_ERROR_ERRNO_EFAULT                              40168
#define LW_ERROR_ERRNO_ENOTBLK                             40169
#define LW_ERROR_ERRNO_EBUSY                               40170
#define LW_ERROR_ERRNO_EEXIST                              40171
#define LW_ERROR_ERRNO_EXDEV                               40172
#define LW_ERROR_ERRNO_ENODEV                              40173
#define LW_ERROR_ERRNO_ENOTDIR                             40174
#define LW_ERROR_ERRNO_EISDIR                              40175
#define LW_ERROR_ERRNO_ENFILE                              40176
#define LW_ERROR_ERRNO_EMFILE                              40177
#define LW_ERROR_ERRNO_ENOTTY                              40178
#define LW_ERROR_ERRNO_ETXTBSY                             40179
#define LW_ERROR_ERRNO_EFBIG                               40180
#define LW_ERROR_ERRNO_ENOSPC                              40181
#define LW_ERROR_ERRNO_ESPIPE                              40182
#define LW_ERROR_ERRNO_EROFS                               40183
#define LW_ERROR_ERRNO_EMLINK                              40184
#define LW_ERROR_ERRNO_EPIPE                               40185
#define LW_ERROR_ERRNO_EDOM                                40186
#define LW_ERROR_ERRNO_ERANGE                              40187
#define LW_ERROR_UNKNOWN                                   40188
#define LW_ERROR_ERRNO_ENAMETOOLONG                        40190
#define LW_ERROR_ERRNO_ENOLCK                              40191
#define LW_ERROR_ERRNO_ENOTEMPTY                           40193
#define LW_ERROR_ERRNO_ELOOP                               40194
#define LW_ERROR_ERRNO_ENOMSG                              40196
#define LW_ERROR_ERRNO_EIDRM                               40197
#define LW_ERROR_ERRNO_ECHRNG                              40198
#define LW_ERROR_ERRNO_EL2NSYNC                            40199
#define LW_ERROR_ERRNO_EL3HLT                              40200
#define LW_ERROR_ERRNO_EL3RST                              40201
#define LW_ERROR_ERRNO_ELNRNG                              40202
#define LW_ERROR_ERRNO_EUNATCH                             40203
#define LW_ERROR_ERRNO_ENOCSI                              40204
#define LW_ERROR_ERRNO_EL2HLT                              40205
#define LW_ERROR_ERRNO_EBADE                               40206
#define LW_ERROR_ERRNO_EBADR                               40207
#define LW_ERROR_ERRNO_EXFULL                              40208
#define LW_ERROR_ERRNO_ENOANO                              40209
#define LW_ERROR_ERRNO_EBADRQC                             40210
#define LW_ERROR_ERRNO_EBADSLT                             40211
#define LW_ERROR_ERRNO_EDEADLOCK                           40212
#define LW_ERROR_ERRNO_EBFONT                              40213
#define LW_ERROR_ERRNO_ENOSTR                              40214
#define LW_ERROR_ERRNO_ENODATA                             40215
#define LW_ERROR_ERRNO_ETIME                               40216
#define LW_ERROR_ERRNO_ENOSR                               40217
#define LW_ERROR_ERRNO_ENONET                              40218
#define LW_ERROR_ERRNO_ENOPKG                              40219
#define LW_ERROR_ERRNO_EREMOTE                             40220
#define LW_ERROR_ERRNO_ENOLINK                             40221
#define LW_ERROR_ERRNO_EADV                                40222
#define LW_ERROR_ERRNO_ESRMNT                              40223
#define LW_ERROR_ERRNO_ECOMM                               40224
#define LW_ERROR_ERRNO_EPROTO                              40225
#define LW_ERROR_ERRNO_EMULTIHOP                           40226
#define LW_ERROR_ERRNO_EDOTDOT                             40227
#define LW_ERROR_ERRNO_EBADMSG                             40228
#define LW_ERROR_ERRNO_EOVERFLOW                           40229
#define LW_ERROR_ERRNO_ENOTUNIQ                            40230
#define LW_ERROR_ERRNO_EBADFD                              40231
#define LW_ERROR_ERRNO_EREMCHG                             40232
#define LW_ERROR_ERRNO_ELIBACC                             40233
#define LW_ERROR_ERRNO_ELIBBAD                             40234
#define LW_ERROR_ERRNO_ELIBSCN                             40235
#define LW_ERROR_ERRNO_ELIBMAX                             40236
#define LW_ERROR_ERRNO_ELIBEXEC                            40237
#define LW_ERROR_ERRNO_EILSEQ                              40238
#define LW_ERROR_ERRNO_ERESTART                            40239
#define LW_ERROR_ERRNO_ESTRPIPE                            40240
#define LW_ERROR_ERRNO_EUSERS                              40241
#define LW_ERROR_ERRNO_ENOTSOCK                            40242
#define LW_ERROR_ERRNO_EDESTADDRREQ                        40243
#define LW_ERROR_ERRNO_EMSGSIZE                            40244
#define LW_ERROR_ERRNO_EPROTOTYPE                          40245
#define LW_ERROR_ERRNO_ENOPROTOOPT                         40246
#define LW_ERROR_ERRNO_EPROTONOSUPPORT                     40247
#define LW_ERROR_ERRNO_ESOCKTNOSUPPORT                     40248
#define LW_ERROR_ERRNO_EOPNOTSUPP                          40249
#define LW_ERROR_ERRNO_EPFNOSUPPORT                        40250
#define LW_ERROR_ERRNO_EAFNOSUPPORT                        40251
#define LW_ERROR_ERRNO_EADDRINUSE                          40252
#define LW_ERROR_ERRNO_EADDRNOTAVAIL                       40253
#define LW_ERROR_ERRNO_ENETDOWN                            40254
#define LW_ERROR_ERRNO_ENETUNREACH                         40255
#define LW_ERROR_ERRNO_ENETRESET                           40256
#define LW_ERROR_ERRNO_ECONNABORTED                        40257
#define LW_ERROR_ERRNO_ECONNRESET                          40258
#define LW_ERROR_ERRNO_ENOBUFS                             40259
#define LW_ERROR_ERRNO_EISCONN                             40260
#define LW_ERROR_ERRNO_ENOTCONN                            40261
#define LW_ERROR_ERRNO_ESHUTDOWN                           40262
#define LW_ERROR_ERRNO_ETOOMANYREFS                        40263
#define LW_ERROR_ERRNO_ETIMEDOUT                           40264
#define LW_ERROR_ERRNO_ECONNREFUSED                        40265
#define LW_ERROR_ERRNO_EHOSTDOWN                           40266
#define LW_ERROR_ERRNO_EHOSTUNREACH                        40267
#define LW_ERROR_ERRNO_EALREADY                            40268
#define LW_ERROR_ERRNO_EINPROGRESS                         40269
#define LW_ERROR_ERRNO_ESTALE                              40270
#define LW_ERROR_ERRNO_EUCLEAN                             40271
#define LW_ERROR_ERRNO_ENOTNAM                             40272
#define LW_ERROR_ERRNO_ENAVAIL                             40273
#define LW_ERROR_ERRNO_EISNAM                              40274
#define LW_ERROR_ERRNO_EREMOTEIO                           40275
#define LW_ERROR_ERRNO_EDQUOT                              40276
#define LW_ERROR_ERRNO_ENOMEDIUM                           40277
#define LW_ERROR_ERRNO_EMEDIUMTYPE                         40278
#define LW_ERROR_ERRNO_ECANCELED                           40279

#define LW_ERROR_LDAP_SERVER_DOWN                          40286
#define LW_ERROR_LDAP_LOCAL_ERROR                          40287
#define LW_ERROR_LDAP_ENCODING_ERROR                       40288
#define LW_ERROR_LDAP_DECODING_ERROR                       40289
#define LW_ERROR_LDAP_TIMEOUT                              40290
#define LW_ERROR_LDAP_AUTH_UNKNOWN                         40291
#define LW_ERROR_LDAP_FILTER_ERROR                         40292
#define LW_ERROR_LDAP_USER_CANCELLED                       40293
#define LW_ERROR_LDAP_PARAM_ERROR                          40294
#define LW_ERROR_LDAP_NO_MEMORY                            40295
#define LW_ERROR_LDAP_CONNECT_ERROR                        40296
#define LW_ERROR_LDAP_NOT_SUPPORTED                        40297
#define LW_ERROR_LDAP_CONTROL_NOT_FOUND                    40298
#define LW_ERROR_LDAP_NO_RESULTS_RETURNED                  40299
#define LW_ERROR_LDAP_MORE_RESULTS_TO_RETURN               40300
#define LW_ERROR_LDAP_CLIENT_LOOP                          40301
#define LW_ERROR_LDAP_REFERRAL_LIMIT_EXCEEDED              40302
#define LW_ERROR_LDAP_OPERATIONS_ERROR                     40303
#define LW_ERROR_LDAP_PROTOCOL_ERROR                       40304
#define LW_ERROR_LDAP_TIMELIMIT_EXCEEDED                   40305
#define LW_ERROR_LDAP_SIZELIMIT_EXCEEDED                   40306
#define LW_ERROR_LDAP_COMPARE_FALSE                        40307
#define LW_ERROR_LDAP_COMPARE_TRUE                         40308
#define LW_ERROR_LDAP_STRONG_AUTH_NOT_SUPPORTED            40309
#define LW_ERROR_LDAP_STRONG_AUTH_REQUIRED                 40310
#define LW_ERROR_LDAP_PARTIAL_RESULTS                      40311
#define LW_ERROR_LDAP_NO_SUCH_ATTRIBUTE                    40312
#define LW_ERROR_LDAP_UNDEFINED_TYPE                       40313
#define LW_ERROR_LDAP_INAPPROPRIATE_MATCHING               40314
#define LW_ERROR_LDAP_CONSTRAINT_VIOLATION                 40315
#define LW_ERROR_LDAP_TYPE_OR_VALUE_EXISTS                 40316
#define LW_ERROR_LDAP_INVALID_SYNTAX                       40317
#define LW_ERROR_LDAP_NO_SUCH_OBJECT                       40318
#define LW_ERROR_LDAP_ALIAS_PROBLEM                        40319
#define LW_ERROR_LDAP_INVALID_DN_SYNTAX                    40320
#define LW_ERROR_LDAP_IS_LEAF                              40321
#define LW_ERROR_LDAP_ALIAS_DEREF_PROBLEM                  40322
#define LW_ERROR_LDAP_REFERRAL                             40323
#define LW_ERROR_LDAP_ADMINLIMIT_EXCEEDED                  40324
#define LW_ERROR_LDAP_UNAVAILABLE_CRITICAL_EXTENSION       40325
#define LW_ERROR_LDAP_CONFIDENTIALITY_REQUIRED             40326
#define LW_ERROR_LDAP_SASL_BIND_IN_PROGRESS                40327
#define LW_ERROR_LDAP_X_PROXY_AUTHZ_FAILURE                40328
#define LW_ERROR_LDAP_INAPPROPRIATE_AUTH                   40329
#define LW_ERROR_LDAP_INVALID_CREDENTIALS                  40330
#define LW_ERROR_LDAP_INSUFFICIENT_ACCESS                  40331
#define LW_ERROR_LDAP_BUSY                                 40332
#define LW_ERROR_LDAP_UNAVAILABLE                          40333
#define LW_ERROR_LDAP_UNWILLING_TO_PERFORM                 40334
#define LW_ERROR_LDAP_LOOP_DETECT                          40335
#define LW_ERROR_LDAP_NAMING_VIOLATION                     40336
#define LW_ERROR_LDAP_OBJECT_CLASS_VIOLATION               40337
#define LW_ERROR_LDAP_NOT_ALLOWED_ON_NONLEAF               40338
#define LW_ERROR_LDAP_NOT_ALLOWED_ON_RDN                   40339
#define LW_ERROR_LDAP_ALREADY_EXISTS                       40340
#define LW_ERROR_LDAP_NO_OBJECT_CLASS_MODS                 40341
#define LW_ERROR_LDAP_RESULTS_TOO_LARGE                    40342
#define LW_ERROR_LDAP_AFFECTS_MULTIPLE_DSAS                40343
#define LW_ERROR_LDAP_CUP_RESOURCES_EXHAUSTED              40344
#define LW_ERROR_LDAP_CUP_SECURITY_VIOLATION               40345
#define LW_ERROR_LDAP_CUP_INVALID_DATA                     40346
#define LW_ERROR_LDAP_CUP_UNSUPPORTED_SCHEME               40347
#define LW_ERROR_LDAP_CUP_RELOAD_REQUIRED                  40348
#define LW_ERROR_LDAP_CANCELLED                            40349
#define LW_ERROR_LDAP_NO_SUCH_OPERATION                    40350
#define LW_ERROR_LDAP_TOO_LATE                             40351
#define LW_ERROR_LDAP_CANNOT_CANCEL                        40352
#define LW_ERROR_LDAP_ASSERTION_FAILED                     40353

#define LW_ERROR_UID_TOO_LOW                               40400
#define LW_ERROR_UID_TOO_HIGH                              40401
#define LW_ERROR_GID_TOO_LOW                               40402
#define LW_ERROR_GID_TOO_HIGH                              40403

/* range 40500 - 40600 are reserved for GSS specific errors */

#define LW_ERROR_BAD_MECH                                  40500
#define LW_ERROR_BAD_NAMETYPE                              40501
#define LW_ERROR_BAD_NAME                                  40502
#define LW_ERROR_INVALID_CONTEXT                           40503
#define LW_ERROR_INVALID_CREDENTIAL                        40504
#define LW_ERROR_NO_CONTEXT                                40505
#define LW_ERROR_NO_CRED                                   40506
#define LW_ERROR_INVALID_TOKEN                             40507
#define LW_ERROR_UNSUPPORTED_SUBPROTO                      40508
#define LW_ERROR_UNSUPPORTED_CRYPTO_OP                     40509

/* range 40601 - 40699 is reserved for lwtest specific errors */

#define LW_ERROR_TEST_FAILED                               40601
#define LW_ERROR_CSV_BAD_FORMAT                            40602
#define LW_ERROR_CSV_NO_SUCH_FIELD                         40603
#define LW_ERROR_TEST_SKIPPED                              40604

/*Range 40700 - 41200 is reserved for registry specific error*/

/* Range 41201 - 41700 is reserved for service manager errors */
#define LW_ERROR_INVALID_SERVICE_TRANSITION                41201
#define LW_ERROR_SERVICE_DEPENDENCY_UNMET                  41202
#define LW_ERROR_SERVICE_UNRESPONSIVE                      41203
#define LW_ERROR_NO_SUCH_SERVICE                           41204
#define LW_ERROR_DEPENDENT_SERVICE_STILL_RUNNING           41205

/* Range 41701 - 42499 is reserved for Kerberos errors.
 * Using the same spacing as the original error tables
 * to facilitate adding new errors.
 */

/* From krb5/src/lib/krb5/error_tables/asn1_err.et */

#define LW_ERROR_KRB5_ASN1_BAD_TIMEFORMAT                  41701
#define LW_ERROR_KRB5_ASN1_MISSING_FIELD                   41702
#define LW_ERROR_KRB5_ASN1_MISPLACED_FIELD                 41703
#define LW_ERROR_KRB5_ASN1_TYPE_MISMATCH                   41704
#define LW_ERROR_KRB5_ASN1_OVERFLOW                        41705
#define LW_ERROR_KRB5_ASN1_OVERRUN                         41706
#define LW_ERROR_KRB5_ASN1_BAD_ID                          41707
#define LW_ERROR_KRB5_ASN1_BAD_LENGTH                      41708
#define LW_ERROR_KRB5_ASN1_BAD_FORMAT                      41709
#define LW_ERROR_KRB5_ASN1_PARSE_ERROR                     41710
#define LW_ERROR_KRB5_ASN1_BAD_GMTIME                      41711
#define LW_ERROR_KRB5_ASN1_MISMATCH_INDEF                  41712
#define LW_ERROR_KRB5_ASN1_MISSING_EOC                     41713

/* From krb5/src/lib/krb5/error_tables/krb524_err.et */

#define LW_ERROR_KRB524_KRB4_DISABLED                      41721

/* From krb5/src/lib/krb5/error_tables/krb5_err.et */

#define LW_ERROR_KRB5KDC_ERR_NONE                          41731
#define LW_ERROR_KRB5KDC_ERR_NAME_EXP                      41732
#define LW_ERROR_KRB5KDC_ERR_SERVICE_EXP                   41733
#define LW_ERROR_KRB5KDC_ERR_BAD_PVNO                      41734
#define LW_ERROR_KRB5KDC_ERR_C_OLD_MAST_KVNO               41735
#define LW_ERROR_KRB5KDC_ERR_S_OLD_MAST_KVNO               41736
#define LW_ERROR_KRB5KDC_ERR_C_PRINCIPAL_UNKNOWN           41737
#define LW_ERROR_KRB5KDC_ERR_S_PRINCIPAL_UNKNOWN           41738
#define LW_ERROR_KRB5KDC_ERR_PRINCIPAL_NOT_UNIQUE          41739
#define LW_ERROR_KRB5KDC_ERR_NULL_KEY                      41740
#define LW_ERROR_KRB5KDC_ERR_CANNOT_POSTDATE               41741
#define LW_ERROR_KRB5KDC_ERR_POLICY                        41742
#define LW_ERROR_KRB5KDC_ERR_BADOPTION                     41743
#define LW_ERROR_KRB5KDC_ERR_ETYPE_NOSUPP                  41744
#define LW_ERROR_KRB5KDC_ERR_SUMTYPE_NOSUPP                41745
#define LW_ERROR_KRB5KDC_ERR_PADATA_TYPE_NOSUPP            41746
#define LW_ERROR_KRB5KDC_ERR_TRTYPE_NOSUPP                 41747
#define LW_ERROR_KRB5KDC_ERR_SERVICE_REVOKED               41749
#define LW_ERROR_KRB5KDC_ERR_TGT_REVOKED                   41750
#define LW_ERROR_KRB5KDC_ERR_CLIENT_NOTYET                 41751
#define LW_ERROR_KRB5KDC_ERR_SERVICE_NOTYET                41752
#define LW_ERROR_KRB5KDC_ERR_PREAUTH_REQUIRED              41755
#define LW_ERROR_KRB5KDC_ERR_SERVER_NOMATCH                41756

#define LW_ERROR_KRB5KDC_ERR_SVC_UNAVAILABLE               41759

#define LW_ERROR_KRB5KRB_AP_ERR_BAD_INTEGRITY              41761
#define LW_ERROR_KRB5KRB_AP_ERR_TKT_EXPIRED                41762
#define LW_ERROR_KRB5KRB_AP_ERR_TKT_NYV                    41763
#define LW_ERROR_KRB5KRB_AP_ERR_REPEAT                     41764
#define LW_ERROR_KRB5KRB_AP_ERR_NOT_US                     41765
#define LW_ERROR_KRB5KRB_AP_ERR_BADMATCH                   41766
#define LW_ERROR_KRB5KRB_AP_ERR_BADADDR                    41768
#define LW_ERROR_KRB5KRB_AP_ERR_BADVERSION                 41769
#define LW_ERROR_KRB5KRB_AP_ERR_MSG_TYPE                   41770
#define LW_ERROR_KRB5KRB_AP_ERR_MODIFIED                   41771
#define LW_ERROR_KRB5KRB_AP_ERR_BADORDER                   41772
#define LW_ERROR_KRB5KRB_AP_ERR_ILL_CR_TKT                 41773
#define LW_ERROR_KRB5KRB_AP_ERR_BADKEYVER                  41774
#define LW_ERROR_KRB5KRB_AP_ERR_NOKEY                      41775
#define LW_ERROR_KRB5KRB_AP_ERR_MUT_FAIL                   41776
#define LW_ERROR_KRB5KRB_AP_ERR_BADDIRECTION               41777
#define LW_ERROR_KRB5KRB_AP_ERR_METHOD                     41778
#define LW_ERROR_KRB5KRB_AP_ERR_BADSEQ                     41779
#define LW_ERROR_KRB5KRB_AP_ERR_INAPP_CKSUM                41780
#define LW_ERROR_KRB5KRB_AP_PATH_NOT_ACCEPTED              41781
#define LW_ERROR_KRB5KRB_ERR_RESPONSE_TOO_BIG              41782

#define LW_ERROR_KRB5KRB_ERR_GENERIC                       41789
#define LW_ERROR_KRB5KRB_ERR_FIELD_TOOLONG                 41790
#define LW_ERROR_KRB5KDC_ERR_CLIENT_NOT_TRUSTED            41791
#define LW_ERROR_KRB5KDC_ERR_KDC_NOT_TRUSTED               41792
#define LW_ERROR_KRB5KDC_ERR_INVALID_SIG                   41793
#define LW_ERROR_KRB5KDC_ERR_DH_KEY_PARAMETERS_NOT_ACCEPTED 49486
#define LW_ERROR_KRB5KDC_ERR_CERTIFICATE_MISMATCH          41795

#define LW_ERROR_KRB5KDC_ERR_CANT_VERIFY_CERTIFICATE       41799
#define LW_ERROR_KRB5KDC_ERR_INVALID_CERTIFICATE           41800
#define LW_ERROR_KRB5KDC_ERR_REVOKED_CERTIFICATE           41801
#define LW_ERROR_KRB5KDC_ERR_REVOCATION_STATUS_UNKNOWN     41802
#define LW_ERROR_KRB5KDC_ERR_REVOCATION_STATUS_UNAVAILABLE 41803
#define LW_ERROR_KRB5KDC_ERR_CLIENT_NAME_MISMATCH          41804
#define LW_ERROR_KRB5KDC_ERR_KDC_NAME_MISMATCH             41805
#define LW_ERROR_KRB5KDC_ERR_INCONSISTENT_KEY_PURPOSE      41806
#define LW_ERROR_KRB5KDC_ERR_DIGEST_IN_CERT_NOT_ACCEPTED   41807
#define LW_ERROR_KRB5KDC_ERR_PA_CHECKSUM_MUST_BE_INCLUDED  41808
#define LW_ERROR_KRB5KDC_ERR_DIGEST_IN_SIGNED_DATA_NOT_ACCEPTED 41809
#define LW_ERROR_KRB5KDC_ERR_PUBLIC_KEY_ENCRYPTION_NOT_SUPPORTED 41810

#define LW_ERROR_KRB5_ERR_RCSID                            41861
#define LW_ERROR_KRB5_LIBOS_BADLOCKFLAG                    41862
#define LW_ERROR_KRB5_LIBOS_CANTREADPWD                    41863
#define LW_ERROR_KRB5_LIBOS_PWDINTR                        41865
#define LW_ERROR_KRB5_PARSE_ILLCHAR                        41866
#define LW_ERROR_KRB5_PARSE_MALFORMED                      41867
#define LW_ERROR_KRB5_CONFIG_CANTOPEN                      41868
#define LW_ERROR_KRB5_CONFIG_BADFORMAT                     41869
#define LW_ERROR_KRB5_CONFIG_NOTENUFSPACE                  41870
#define LW_ERROR_KRB5_BADMSGTYPE                           41871
#define LW_ERROR_KRB5_CC_BADNAME                           41872
#define LW_ERROR_KRB5_CC_UNKNOWN_TYPE                      41873
#define LW_ERROR_KRB5_CC_NOTFOUND                          41874
#define LW_ERROR_KRB5_CC_END                               41875
#define LW_ERROR_KRB5_NO_TKT_SUPPLIED                      41876
#define LW_ERROR_KRB5KRB_AP_WRONG_PRINC                    41877
#define LW_ERROR_KRB5KRB_AP_ERR_TKT_INVALID                41878
#define LW_ERROR_KRB5_PRINC_NOMATCH                        41879
#define LW_ERROR_KRB5_KDCREP_MODIFIED                      41880
#define LW_ERROR_KRB5_KDCREP_SKEW                          41881
#define LW_ERROR_KRB5_IN_TKT_REALM_MISMATCH                41882
#define LW_ERROR_KRB5_PROG_ETYPE_NOSUPP                    41883
#define LW_ERROR_KRB5_PROG_KEYTYPE_NOSUPP                  41884
#define LW_ERROR_KRB5_WRONG_ETYPE                          41885
#define LW_ERROR_KRB5_PROG_SUMTYPE_NOSUPP                  41886
#define LW_ERROR_KRB5_REALM_UNKNOWN                        41887
#define LW_ERROR_KRB5_SERVICE_UNKNOWN                      41888
#define LW_ERROR_KRB5_NO_LOCALNAME                         41890
#define LW_ERROR_KRB5_MUTUAL_FAILED                        41891
#define LW_ERROR_KRB5_RC_TYPE_EXISTS                       41892
#define LW_ERROR_KRB5_RC_MALLOC                            41893
#define LW_ERROR_KRB5_RC_TYPE_NOTFOUND                     41894
#define LW_ERROR_KRB5_RC_UNKNOWN                           41895
#define LW_ERROR_KRB5_RC_REPLAY                            41896
#define LW_ERROR_KRB5_RC_IO                                41897
#define LW_ERROR_KRB5_RC_NOIO                              41898
#define LW_ERROR_KRB5_RC_PARSE                             41899
#define LW_ERROR_KRB5_RC_IO_EOF                            41900
#define LW_ERROR_KRB5_RC_IO_MALLOC                         41901
#define LW_ERROR_KRB5_RC_IO_PERM                           41902
#define LW_ERROR_KRB5_RC_IO_IO                             41903
#define LW_ERROR_KRB5_RC_IO_UNKNOWN                        41904
#define LW_ERROR_KRB5_RC_IO_SPACE                          41905
#define LW_ERROR_KRB5_TRANS_CANTOPEN                       41906
#define LW_ERROR_KRB5_TRANS_BADFORMAT                      41907
#define LW_ERROR_KRB5_LNAME_CANTOPEN                       41908
#define LW_ERROR_KRB5_LNAME_NOTRANS                        41909
#define LW_ERROR_KRB5_LNAME_BADFORMAT                      41910
#define LW_ERROR_KRB5_CRYPTO_INTERNAL                      41911
#define LW_ERROR_KRB5_KT_BADNAME                           41912
#define LW_ERROR_KRB5_KT_UNKNOWN_TYPE                      41913
#define LW_ERROR_KRB5_KT_NOTFOUND                          41914
#define LW_ERROR_KRB5_KT_END                               41915
#define LW_ERROR_KRB5_KT_NOWRITE                           41916
#define LW_ERROR_KRB5_KT_IOERR                             41917
#define LW_ERROR_KRB5_NO_TKT_IN_RLM                        41918
#define LW_ERROR_KRB5DES_BAD_KEYPAR                        41919
#define LW_ERROR_KRB5DES_WEAK_KEY                          41920
#define LW_ERROR_KRB5_BAD_ENCTYPE                          41921
#define LW_ERROR_KRB5_BAD_KEYSIZE                          41922
#define LW_ERROR_KRB5_BAD_MSIZE                            41923
#define LW_ERROR_KRB5_CC_TYPE_EXISTS                       41924
#define LW_ERROR_KRB5_KT_TYPE_EXISTS                       41925
#define LW_ERROR_KRB5_CC_IO                                41926
#define LW_ERROR_KRB5_FCC_PERM                             41927
#define LW_ERROR_KRB5_FCC_NOFILE                           41928
#define LW_ERROR_KRB5_FCC_INTERNAL                         41929
#define LW_ERROR_KRB5_CC_WRITE                             41930
#define LW_ERROR_KRB5_CC_NOMEM                             41931
#define LW_ERROR_KRB5_CC_FORMAT                            41932
#define LW_ERROR_KRB5_CC_NOT_KTYPE                         41933
#define LW_ERROR_KRB5_INVALID_FLAGS                        41934
#define LW_ERROR_KRB5_NO_2ND_TKT                           41935
#define LW_ERROR_KRB5_NOCREDS_SUPPLIED                     41936
#define LW_ERROR_KRB5_SENDAUTH_BADAUTHVERS                 41937
#define LW_ERROR_KRB5_SENDAUTH_BADAPPLVERS                 41938
#define LW_ERROR_KRB5_SENDAUTH_BADRESPONSE                 41939
#define LW_ERROR_KRB5_SENDAUTH_REJECTED                    41940
#define LW_ERROR_KRB5_PREAUTH_BAD_TYPE                     41941
#define LW_ERROR_KRB5_PREAUTH_NO_KEY                       41942
#define LW_ERROR_KRB5_PREAUTH_FAILED                       41943
#define LW_ERROR_KRB5_RCACHE_BADVNO                        41944
#define LW_ERROR_KRB5_CCACHE_BADVNO                        41945
#define LW_ERROR_KRB5_KEYTAB_BADVNO                        41946
#define LW_ERROR_KRB5_PROG_ATYPE_NOSUPP                    41947
#define LW_ERROR_KRB5_RC_REQUIRED                          41948
#define LW_ERROR_KRB5_ERR_BAD_HOSTNAME                     41949
#define LW_ERROR_KRB5_ERR_HOST_REALM_UNKNOWN               41950
#define LW_ERROR_KRB5_SNAME_UNSUPP_NAMETYPE                41951
#define LW_ERROR_KRB5KRB_AP_ERR_V4_REPLY                   41952
#define LW_ERROR_KRB5_REALM_CANT_RESOLVE                   41953
#define LW_ERROR_KRB5_TKT_NOT_FORWARDABLE                  41954
#define LW_ERROR_KRB5_FWD_BAD_PRINCIPAL                    41955
#define LW_ERROR_KRB5_GET_IN_TKT_LOOP                      41956
#define LW_ERROR_KRB5_CONFIG_NODEFREALM                    41957
#define LW_ERROR_KRB5_SAM_UNSUPPORTED                      41958
#define LW_ERROR_KRB5_SAM_INVALID_ETYPE                    41959
#define LW_ERROR_KRB5_SAM_NO_CHECKSUM                      41960
#define LW_ERROR_KRB5_SAM_BAD_CHECKSUM                     41961
#define LW_ERROR_KRB5_KT_NAME_TOOLONG                      41962
#define LW_ERROR_KRB5_KT_KVNONOTFOUND                      41963
#define LW_ERROR_KRB5_APPL_EXPIRED                         41964
#define LW_ERROR_KRB5_LIB_EXPIRED                          41965
#define LW_ERROR_KRB5_CHPW_PWDNULL                         41966
#define LW_ERROR_KRB5_CHPW_FAIL                            41967
#define LW_ERROR_KRB5_KT_FORMAT                            41968
#define LW_ERROR_KRB5_NOPERM_ETYPE                         41969
#define LW_ERROR_KRB5_CONFIG_ETYPE_NOSUPP                  41970
#define LW_ERROR_KRB5_OBSOLETE_FN                          41971
#define LW_ERROR_KRB5_EAI_FAIL                             41972
#define LW_ERROR_KRB5_EAI_NODATA                           41973
#define LW_ERROR_KRB5_EAI_NONAME                           41974
#define LW_ERROR_KRB5_EAI_SERVICE                          41975
#define LW_ERROR_KRB5_ERR_NUMERIC_REALM                    41976
#define LW_ERROR_KRB5_ERR_BAD_S2K_PARAMS                   41977
#define LW_ERROR_KRB5_ERR_NO_SERVICE                       41978
#define LW_ERROR_KRB5_CC_READONLY                          41979
#define LW_ERROR_KRB5_CC_NOSUPP                            41980
#define LW_ERROR_KRB5_DELTAT_BADFORMAT                     41981
#define LW_ERROR_KRB5_PLUGIN_NO_HANDLE                     41982
#define LW_ERROR_KRB5_PLUGIN_OP_NOTSUPP                    41983

/* From krb5/src/lib/krb5/error_tables/kv5m_err.et */

#define LW_ERROR_KRB5_KV5M_NONE                            42051
#define LW_ERROR_KRB5_KV5M_PRINCIPAL                       42052
#define LW_ERROR_KRB5_KV5M_DATA                            42053
#define LW_ERROR_KRB5_KV5M_KEYBLOCK                        42054
#define LW_ERROR_KRB5_KV5M_CHECKSUM                        42055
#define LW_ERROR_KRB5_KV5M_ENCRYPT_BLOCK                   42056
#define LW_ERROR_KRB5_KV5M_ENC_DATA                        42057
#define LW_ERROR_KRB5_KV5M_CRYPTOSYSTEM_ENTRY              42058
#define LW_ERROR_KRB5_KV5M_CS_TABLE_ENTRY                  42059
#define LW_ERROR_KRB5_KV5M_CHECKSUM_ENTRY                  42060
#define LW_ERROR_KRB5_KV5M_AUTHDATA                        42061
#define LW_ERROR_KRB5_KV5M_TRANSITED                       42062
#define LW_ERROR_KRB5_KV5M_ENC_TKT_PART                    42063
#define LW_ERROR_KRB5_KV5M_TICKET                          42064
#define LW_ERROR_KRB5_KV5M_AUTHENTICATOR                   42065
#define LW_ERROR_KRB5_KV5M_TKT_AUTHENT                     42066
#define LW_ERROR_KRB5_KV5M_CREDS                           42067
#define LW_ERROR_KRB5_KV5M_LAST_REQ_ENTRY                  42068
#define LW_ERROR_KRB5_KV5M_PA_DATA                         42069
#define LW_ERROR_KRB5_KV5M_KDC_REQ                         42070
#define LW_ERROR_KRB5_KV5M_ENC_KDC_REP_PART                42071
#define LW_ERROR_KRB5_KV5M_KDC_REP                         42072
#define LW_ERROR_KRB5_KV5M_ERROR                           42073
#define LW_ERROR_KRB5_KV5M_AP_REQ                          42074
#define LW_ERROR_KRB5_KV5M_AP_REP                          42075
#define LW_ERROR_KRB5_KV5M_AP_REP_ENC_PART                 42076
#define LW_ERROR_KRB5_KV5M_RESPONSE                        42077
#define LW_ERROR_KRB5_KV5M_SAFE                            42078
#define LW_ERROR_KRB5_KV5M_PRIV                            42079
#define LW_ERROR_KRB5_KV5M_PRIV_ENC_PART                   42080
#define LW_ERROR_KRB5_KV5M_CRED                            42081
#define LW_ERROR_KRB5_KV5M_CRED_INFO                       42082
#define LW_ERROR_KRB5_KV5M_CRED_ENC_PART                   42083
#define LW_ERROR_KRB5_KV5M_PWD_DATA                        42084
#define LW_ERROR_KRB5_KV5M_ADDRESS                         42085
#define LW_ERROR_KRB5_KV5M_KEYTAB_ENTRY                    42086
#define LW_ERROR_KRB5_KV5M_CONTEXT                         42087
#define LW_ERROR_KRB5_KV5M_OS_CONTEXT                      42088
#define LW_ERROR_KRB5_KV5M_ALT_METHOD                      42089
#define LW_ERROR_KRB5_KV5M_ETYPE_INFO_ENTRY                42090
#define LW_ERROR_KRB5_KV5M_DB_CONTEXT                      42091
#define LW_ERROR_KRB5_KV5M_AUTH_CONTEXT                    42092
#define LW_ERROR_KRB5_KV5M_KEYTAB                          42093
#define LW_ERROR_KRB5_KV5M_RCACHE                          42094
#define LW_ERROR_KRB5_KV5M_CCACHE                          42095
#define LW_ERROR_KRB5_KV5M_PREAUTH_OPS                     42096
#define LW_ERROR_KRB5_KV5M_SAM_CHALLENGE                   42097
#define LW_ERROR_KRB5_KV5M_SAM_CHALLENGE_2                 42098
#define LW_ERROR_KRB5_KV5M_SAM_KEY                         42099
#define LW_ERROR_KRB5_KV5M_ENC_SAM_RESPONSE_ENC            42100
#define LW_ERROR_KRB5_KV5M_ENC_SAM_RESPONSE_ENC_2          42102
#define LW_ERROR_KRB5_KV5M_SAM_RESPONSE                    42103
#define LW_ERROR_KRB5_KV5M_SAM_RESPONSE_2                  42104
#define LW_ERROR_KRB5_KV5M_PREDICTED_SAM_RESPONSE          42105
#define LW_ERROR_KRB5_KV5M_PASSWD_PHRASE_ELEMENT           42106
#define LW_ERROR_KRB5_KV5M_GSS_OID                         42107
#define LW_ERROR_KRB5_KV5M_GSS_QUEUE                       42108

/* From krb5/src/util/profile/prof_err.et */

#define LW_ERROR_KRB5_PROF_VERSION                         42151
#define LW_ERROR_KRB5_PROF_MAGIC_NODE                      42152
#define LW_ERROR_KRB5_PROF_NO_SECTION                      42153
#define LW_ERROR_KRB5_PROF_NO_RELATION                     42154
#define LW_ERROR_KRB5_PROF_ADD_NOT_SECTION                 42155
#define LW_ERROR_KRB5_PROF_SECTION_WITH_VALUE              42156
#define LW_ERROR_KRB5_PROF_BAD_LINK_LIST                   42157
#define LW_ERROR_KRB5_PROF_BAD_GROUP_LVL                   42158
#define LW_ERROR_KRB5_PROF_BAD_PARENT_PTR                  42159
#define LW_ERROR_KRB5_PROF_MAGIC_ITERATOR                  42160
#define LW_ERROR_KRB5_PROF_SET_SECTION_VALUE               42161
#define LW_ERROR_KRB5_PROF_EINVAL                          42162
#define LW_ERROR_KRB5_PROF_READ_ONLY                       42163
#define LW_ERROR_KRB5_PROF_SECTION_NOTOP                   42164
#define LW_ERROR_KRB5_PROF_SECTION_SYNTAX                  42165
#define LW_ERROR_KRB5_PROF_RELATION_SYNTAX                 42166
#define LW_ERROR_KRB5_PROF_EXTRA_CBRACE                    42167
#define LW_ERROR_KRB5_PROF_MISSING_OBRACE                  42168
#define LW_ERROR_KRB5_PROF_MAGIC_PROFILE                   42169
#define LW_ERROR_KRB5_PROF_MAGIC_SECTION                   42170
#define LW_ERROR_KRB5_PROF_TOPSECTION_ITER_NOSUPP          42171
#define LW_ERROR_KRB5_PROF_INVALID_SECTION                 42172
#define LW_ERROR_KRB5_PROF_END_OF_SECTIONS                 42173
#define LW_ERROR_KRB5_PROF_BAD_NAMESET                     42174
#define LW_ERROR_KRB5_PROF_NO_PROFILE                      42175
#define LW_ERROR_KRB5_PROF_MAGIC_FILE                      42176
#define LW_ERROR_KRB5_PROF_FAIL_OPEN                       42177
#define LW_ERROR_KRB5_PROF_EXISTS                          42178
#define LW_ERROR_KRB5_PROF_BAD_BOOLEAN                     42179
#define LW_ERROR_KRB5_PROF_BAD_INTEGER                     42180
#define LW_ERROR_KRB5_PROF_MAGIC_FILE_DATA                 42181

/* Range 42500 - 42550 is reserved for domain join */

#define LW_ERROR_INVALID_OU                                42500
#define LW_ERROR_BAD_LICENSE_KEY                           42501
#define LW_ERROR_MODULE_NOT_ENABLED                        42502
#define LW_ERROR_MODULE_ALREADY_DONE                       42503
#define LW_ERROR_PAM_MISSING_SERVICE                       42504
#define LW_ERROR_PAM_BAD_CONF                              42505
#define LW_ERROR_SHOW_USAGE                                42506
#define LW_ERROR_DOMAINJOIN_WARNING                        42507
#define LW_ERROR_FAILED_ADMIN_PRIVS                        42508

// Range 42600 - 42560 is reserved for autoenroll
#define LW_ERROR_AUTOENROLL_FAILED                         42600
#define LW_ERROR_AUTOENROLL_HTTP_REQUEST_FAILED            42601
#define LW_ERROR_AUTOENROLL_SUBJECT_NAME_REQUIRED          42602

#define LW_ERROR_DNS_UPDATE_FAILED                         42700


//
// 49900-49999 reserved for internal PAM error code mapping
//

#define _LW_ERROR_PAM_BASE                                 49900
#define _LW_ERROR_PAM_MAX                                  49999

// 50000-50999 reserved for gpagentd
#define LW_ERROR_GP_REFRESH_FAILED                         50000
#define LW_ERROR_GP_LOGIN_POLICY_FAILED                    50001
#define LW_ERROR_GP_LOGOUT_POLICY_FAILED                   50002
#define LW_ERROR_GP_PATH_NOT_FOUND                         50003
#define LW_ERROR_GP_CREATE_FAILED                          50004
#define LW_ERROR_GP_READ_FAILED                            50005
#define LW_ERROR_GP_WRITE_FAILED                           50006
#define LW_ERROR_GP_QUERY_DIRECTORY                        50007
#define LW_ERROR_GP_FILE_COPY_FAILED                       50008
#define LW_ERROR_GP_LWIDATA_NOT_INITIALIZED                50009
#define LW_ERROR_GP_XPATH_CONTEXT_INIT_ERR                 50010
#define LW_ERROR_GP_XPATH_BAD_EXPRESSION                   50011
#define LW_ERROR_GP_XML_GPITEM_NOT_FOUND                   50012
#define LW_ERROR_GP_XML_NO_NODE_TEXT                       50013
#define LW_ERROR_GP_XML_TYPE_MISMATCH                      50014
#define LW_ERROR_GP_XML_NODE_NOT_FOUND                     50015
#define LW_ERROR_GP_GSS_CALL_FAILED                        50016
#define LW_ERROR_GP_NO_SMB_KRB5_SITE_KDC_INFO              50017
#define LW_ERROR_GP_NO_SMB_KRB5_SITE_INFO                  50018
#define LW_ERROR_GP_NOT_EXACTLY_ONE                        50019
#define LW_ERROR_GP_INVALID_GPLINK                         50020
#define LW_ERROR_GP_LICENSE_RESTRICTION                    50021
#define LW_ERROR_GP_PARSE_ERROR                            50022
#define LW_ERROR_GP_UNEXPECTED_ACTION_TYPE                 50023
#define LW_ERROR_GP_XML_FAILED_TO_WRITE_DOC                50024
#define LW_ERROR_GP_SETLOGLEVEL_FAILED                     50025
#define LW_ERROR_GP_DOMAIN_JOIN_FAILED                     50026
#define LW_ERROR_GP_DOMAIN_LEAVE_FAILED                    50027
#define LW_ERROR_GP_LOAD_EXTENSIONS_FAILED                 50028
#define LW_ERROR_GP_REFRESH_CONFIGURATION_FAILED           50029
#define LW_ERROR_GP_XML_PROPERTY_NOT_FOUND                 50030

LW_BEGIN_EXTERN_C

size_t
LwGetErrorString(
    LW_IN LW_DWORD dwError,
    LW_OUT LW_PSTR pszBuffer,
    LW_IN size_t stBufSize
    );

LW_DWORD
LwMapErrnoToLwError(
    LW_IN LW_DWORD dwErrno
    );

LW_DWORD
LwMapHErrnoToLwError(
    LW_IN LW_DWORD dwHErrno
    );

LW_DWORD
LwMapLdapErrorToLwError(
    LW_IN LW_DWORD dwErrno
    );

LW_DWORD
LwMapLwmsgStatusToLwError(
    LW_IN LWMsgStatus status
    );

LW_PCSTR
LwWin32ExtErrorToName(
    LW_WINERROR winerr
    );

LW_PCSTR
LwWin32ExtErrorToDescription(
    LW_WINERROR winerr
    );

LW_END_EXTERN_C


#endif /* __LW_ERROR_H__ */
