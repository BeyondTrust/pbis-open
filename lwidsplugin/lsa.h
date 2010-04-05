/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */
 
/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        lsa.h - Abreviated version of lsa.h from the lsass componet.
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *        This version is used to just define errors to Mac DS plugin.
 *
 *        Public Client API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */
#ifndef __LSAERROR_H__
#define __LSAERROR_H__

#ifndef LSA_ERRORS_DEFINED

#define LSA_ERRORS_DEFINED 1

#define LSA_ERROR_SUCCESS                                   0x0000
#define LSA_ERROR_INVALID_CACHE_PATH                        0x8000 // 32768
#define LSA_ERROR_INVALID_CONFIG_PATH                       0x8001 // 32769
#define LSA_ERROR_INVALID_PREFIX_PATH                       0x8002 // 32770
#define LSA_ERROR_INSUFFICIENT_BUFFER                       0x8003 // 32771
#define LSA_ERROR_OUT_OF_MEMORY                             0x8004 // 32772
#define LSA_ERROR_INVALID_MESSAGE                           0x8005 // 32773
#define LSA_ERROR_UNEXPECTED_MESSAGE                        0x8006 // 32774
#define LSA_ERROR_NO_SUCH_USER                              0x8007 // 32775
#define LSA_ERROR_DATA_ERROR                                0x8008 // 32776
#define LSA_ERROR_NOT_IMPLEMENTED                           0x8009 // 32777
#define LSA_ERROR_NO_CONTEXT_ITEM                           0x800A // 32778
#define LSA_ERROR_NO_SUCH_GROUP                             0x800B // 32779
#define LSA_ERROR_REGEX_COMPILE_FAILED                      0x800C // 32780
#define LSA_ERROR_NSS_EDIT_FAILED                           0x800D // 32781
#define LSA_ERROR_NO_HANDLER                                0x800E // 32782
#define LSA_ERROR_INTERNAL                                  0x800F // 32783
#define LSA_ERROR_NOT_HANDLED                               0x8010 // 32784
#define LSA_ERROR_INVALID_DNS_RESPONSE                      0x8011 // 32785
#define LSA_ERROR_DNS_RESOLUTION_FAILED                     0x8012 // 32786
#define LSA_ERROR_FAILED_TIME_CONVERSION                    0x8013 // 32787
#define LSA_ERROR_INVALID_SID                               0x8014 // 32788
#define LSA_ERROR_PASSWORD_MISMATCH                         0x8015 // 32789
#define LSA_ERROR_UNEXPECTED_DB_RESULT                      0x8016 // 32790
#define LSA_ERROR_PASSWORD_EXPIRED                          0x8017 // 32791
#define LSA_ERROR_ACCOUNT_EXPIRED                           0x8018 // 32792
#define LSA_ERROR_USER_EXISTS                               0x8019 // 32793
#define LSA_ERROR_GROUP_EXISTS                              0x801A // 32794
#define LSA_ERROR_INVALID_GROUP_INFO_LEVEL                  0x801B // 32795
#define LSA_ERROR_INVALID_USER_INFO_LEVEL                   0x801C // 32796
#define LSA_ERROR_UNSUPPORTED_USER_LEVEL                    0x801D // 32797
#define LSA_ERROR_UNSUPPORTED_GROUP_LEVEL                   0x801E // 32798
#define LSA_ERROR_INVALID_LOGIN_ID                          0x801F // 32799
#define LSA_ERROR_INVALID_HOMEDIR                           0x8020 // 32800
#define LSA_ERROR_INVALID_GROUP_NAME                        0x8021 // 32801
#define LSA_ERROR_NO_MORE_GROUPS                            0x8022 // 32802
#define LSA_ERROR_NO_MORE_USERS                             0x8023 // 32803
#define LSA_ERROR_FAILED_ADD_USER                           0x8024 // 32804
#define LSA_ERROR_FAILED_ADD_GROUP                          0x8025 // 32805
#define LSA_ERROR_INVALID_LSA_CONNECTION                    0x8026 // 32806
#define LSA_ERROR_INVALID_AUTH_PROVIDER                     0x8027 // 32807
#define LSA_ERROR_INVALID_PARAMETER                         0x8028 // 32808
#define LSA_ERROR_LDAP_NO_PARENT_DN                         0x8029 // 32809
#define LSA_ERROR_LDAP_ERROR                                0x802A // 32810
#define LSA_ERROR_NO_SUCH_DOMAIN                            0x802B // 32811
#define LSA_ERROR_LDAP_FAILED_GETDN                         0x802C // 32812
#define LSA_ERROR_DUPLICATE_DOMAINNAME                      0x802D // 32813
#define LSA_ERROR_KRB5_CALL_FAILED                          0x802E // 32814
#define LSA_ERROR_GSS_CALL_FAILED                           0x802F // 32815
#define LSA_ERROR_FAILED_FIND_DC                            0x8030 // 32816
#define LSA_ERROR_NO_SUCH_CELL                              0x8031 // 32817
#define LSA_ERROR_GROUP_IN_USE                              0x8032 // 32818
#define LSA_ERROR_FAILED_CREATE_HOMEDIR                     0x8033 // 32819
#define LSA_ERROR_PASSWORD_TOO_WEAK                         0x8034 // 32820
#define LSA_ERROR_INVALID_SID_REVISION                      0x8035 // 32821
#define LSA_ERROR_ACCOUNT_LOCKED                            0x8036 // 32822
#define LSA_ERROR_ACCOUNT_DISABLED                          0x8037 // 32823
#define LSA_ERROR_USER_CANNOT_CHANGE_PASSWD                 0x8038 // 32824
#define LSA_ERROR_LOAD_LIBRARY_FAILED                       0x8039 // 32825
#define LSA_ERROR_LOOKUP_SYMBOL_FAILED                      0x803A // 32826
#define LSA_ERROR_INVALID_EVENTLOG                          0x803B // 32827
#define LSA_ERROR_INVALID_CONFIG                            0x803C // 32828
#define LSA_ERROR_UNEXPECTED_TOKEN                          0x803D // 32829
#define LSA_ERROR_LDAP_NO_RECORDS_FOUND                     0x803E // 32830
#define LSA_ERROR_DUPLICATE_USERNAME                        0x803F // 32831
#define LSA_ERROR_DUPLICATE_GROUPNAME                       0x8040 // 32832
#define LSA_ERROR_DUPLICATE_CELLNAME                        0x8041 // 32833
#define LSA_ERROR_STRING_CONV_FAILED                        0x8042 // 32834
#define LSA_ERROR_INVALID_ACCOUNT                           0x8043 // 32835
#define LSA_ERROR_INVALID_PASSWORD                          0x8044 // 32836
#define LSA_ERROR_QUERY_CREATION_FAILED                     0x8045 // 32837
#define LSA_ERROR_NO_SUCH_OBJECT                            0x8046 // 32838
#define LSA_ERROR_DUPLICATE_USER_OR_GROUP                   0x8047 // 32839
#define LSA_ERROR_INVALID_KRB5_CACHE_TYPE                   0x8048 // 32840
#define LSA_ERROR_NOT_JOINED_TO_AD                          0x8049 // 32841
#define LSA_ERROR_FAILED_TO_SET_TIME                        0x804A // 32842
#define LSA_ERROR_NO_NETBIOS_NAME                           0x804B // 32843
#define LSA_ERROR_INVALID_NETLOGON_RESPONSE                 0x804C // 32844
#define LSA_ERROR_INVALID_OBJECTGUID                        0x804D // 32845
#define LSA_ERROR_INVALID_DOMAIN                            0x804E // 32846
#define LSA_ERROR_NO_DEFAULT_REALM                          0x804F // 32847
#define LSA_ERROR_NOT_SUPPORTED                             0x8050 // 32848
#define LSA_ERROR_LOGON_FAILURE                             0x8051 // 32849
#define LSA_ERROR_NO_SITE_INFORMATION                       0x8052 // 32850
#define LSA_ERROR_INVALID_LDAP_STRING                       0x8053 // 32851
#define LSA_ERROR_INVALID_LDAP_ATTR_VALUE                   0x8054 // 32852
#define LSA_ERROR_NULL_BUFFER                               0x8055 // 32853
#define LSA_ERROR_CLOCK_SKEW                                0x8056 // 32854
#define LSA_ERROR_KRB5_NO_KEYS_FOUND                        0x8057 // 32855
#define LSA_ERROR_SERVICE_NOT_AVAILABLE                     0x8058 // 32856
#define LSA_ERROR_INVALID_SERVICE_RESPONSE                  0x8059 // 32857
#define LSA_ERROR_NSS_ERROR                                 0x805A // 32858
#define LSA_ERROR_AUTH_ERROR                                0x805B // 32859
#define LSA_ERROR_INVALID_LDAP_DN                           0x805C // 32860
#define LSA_ERROR_NOT_MAPPED                                0x805D // 32861
#define LSA_ERROR_RPC_NETLOGON_FAILED                       0x805E // 32862
#define LSA_ERROR_ENUM_DOMAIN_TRUSTS_FAILED                 0x805F // 32863
#define LSA_ERROR_RPC_LSABINDING_FAILED                     0x8060 // 32864
#define LSA_ERROR_RPC_OPENPOLICY_FAILED                     0x8061 // 32865
#define LSA_ERROR_RPC_LSA_LOOKUPNAME2_FAILED                0x8062 // 32866
#define LSA_ERROR_RPC_SET_SESS_CREDS_FAILED                 0x8063 // 32867
#define LSA_ERROR_RPC_REL_SESS_CREDS_FAILED                 0x8064 // 32868
#define LSA_ERROR_RPC_CLOSEPOLICY_FAILED                    0x8065 // 32869
#define LSA_ERROR_RPC_LSA_LOOKUPNAME2_NOT_FOUND             0x8066 // 32870
#define LSA_ERROR_RPC_LSA_LOOKUPNAME2_FOUND_DUPLICATES      0x8067 // 32871
#define LSA_ERROR_NO_TRUSTED_DOMAIN_FOUND                   0x8068 // 32872
#define LSA_ERROR_INCOMPATIBLE_MODES_BETWEEN_TRUSTEDDOMAINS 0x8069 // 32873
#define LSA_ERROR_DCE_CALL_FAILED                           0x806A // 32874
#define LSA_ERROR_FAILED_TO_LOOKUP_DC                       0x806B // 32875
#define LSA_ERROR_INVALID_NSS_ARTEFACT_INFO_LEVEL           0x806C // 32876
#define LSA_ERROR_UNSUPPORTED_NSS_ARTEFACT_LEVEL            0x806D // 32877
#define LSA_ERROR_INVALID_USER_NAME                         0x806E // 32878
#define LSA_ERROR_INVALID_LOG_LEVEL                         0x806F // 32879
#define LSA_ERROR_INVALID_METRIC_TYPE                       0x8070 // 32880
#define LSA_ERROR_INVALID_METRIC_PACK                       0x8071 // 32881
#define LSA_ERROR_INVALID_METRIC_INFO_LEVEL                 0x8072 // 32882
#define LSA_ERROR_FAILED_STARTUP_PREREQUISITE_CHECK         0x8073 // 32883
#define LSA_ERROR_MAC_FLUSH_DS_CACHE_FAILED                 0x8074 // 32884
#define LSA_ERROR_LSA_SERVER_UNREACHABLE                    0x8075 // 32885
#define LSA_ERROR_INVALID_NSS_ARTEFACT_TYPE                 0x8076 // 32886
#define LSA_ERROR_INVALID_AGENT_VERSION                     0x8077 // 32887
#define LSA_ERROR_DOMAIN_IS_OFFLINE                         0x8078 // 32888
#define LSA_ERROR_INVALID_HOMEDIR_TEMPLATE                  0x8079 // 32889
#define LSA_ERROR_RPC_PARSE_SID_STRING                      0x807A // 32890
#define LSA_ERROR_RPC_LSA_LOOKUPSIDS_FAILED                 0x807B // 32891
#define LSA_ERROR_RPC_LSA_LOOKUPSIDS_NOT_FOUND              0x807C // 32892
#define LSA_ERORR_RPC_LSA_LOOKUPSIDS_FOUND_DUPLICATES       0x807D // 32893
#define LSA_ERROR_PASSWORD_RESTRICTION                      0x807E // 32894
#define LSA_ERROR_OBJECT_NOT_ENABLED                        0x807F // 32895
#define LSA_ERROR_NO_MORE_NSS_ARTEFACTS                     0x8080 // 32896
#define LSA_ERROR_INVALID_NSS_MAP_NAME                      0x8081 // 32897
#define LSA_ERROR_INVALID_NSS_KEY_NAME                      0x8082 // 32898
#define LSA_ERROR_NO_SUCH_NSS_KEY                           0x8083 // 32899
#define LSA_ERROR_NO_SUCH_NSS_MAP                           0x8084 // 32900
#define LSA_ERROR_RPC_ERROR                                 0x8085 // 32901
#define LSA_ERROR_LDAP_SERVER_UNAVAILABLE                   0x8086 // 32902
#define LSA_ERROR_CREATE_KEY_FAILED                         0x8087 // 32903
#define LSA_ERROR_CANNOT_DETECT_USER_PROCESSES              0x8088 // 32904
#define LSA_ERROR_TRACE_NOT_INITIALIZED                     0x8089 // 32905
#define LSA_ERROR_NO_SUCH_TRACE_FLAG                        0x808A // 32906
#define LSA_ERROR_DCERPC_ERROR                              0x808B // 32907
#define LSA_ERROR_INVALID_RPC_SERVER                        0x808C // 32908
#define LSA_ERROR_RPC_SERVER_REGISTRATION_ERROR             0x808D // 32909
#define LSA_ERROR_RPC_SERVER_RUNTIME_ERROR                  0x808E // 32910
#define LSA_ERROR_DOMAIN_IN_USE                             0x808F // 32911
#define LSA_ERROR_SAM_DATABASE_ERROR                        0x8090 // 32912
#define LSA_ERROR_SAM_INIT_ERROR                            0x8091 // 32913
#define LSA_ERROR_OBJECT_IN_USE                             0x8092 // 32914
#define LSA_ERROR_NO_SUCH_ATTRIBUTE                         0x8093 // 32915
#define LSA_ERROR_GET_DC_NAME_FAILED                        0x8094 // 32916
#define LSA_ERROR_INVALID_ATTRIBUTE_VALUE                   0x8095 // 32917
#define LSA_ERROR_NO_ATTRIBUTE_VALUE                        0x8096 // 32918
#define LSA_ERROR_SENTINEL                                  0x8097 // 32919

#define LSA_ERROR_MASK(_e_)                                 (_e_ & 0x8000)

/* WARNINGS */
#define LSA_WARNING_CONTINUE_NEEDED                         0x7001

#define LWPS_ERROR_INVALID_ACCOUNT                          0x4016 // 16406

#endif /* LSA_ERRORS_DEFINED */


#endif /* __LSAERROR_H__ */
