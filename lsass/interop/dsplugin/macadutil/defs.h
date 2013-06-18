/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software
 * All rights reserved.
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

#ifndef __DEFS_H__
#define __DEFS_H__


#define MACHINE_GROUP_POLICY 1
#define USER_GROUP_POLICY    2
#define UNKNOWN_GROUP_POLICY 3

#define STATIC_PATH_BUFFER_SIZE 256

#define KRB5CCENVVAR "KRB5CCNAME"

#define DC_PREFIX "dc="

typedef gss_ctx_id_t CtxtHandle, *PCtxtHandle;

#define BAIL_ON_SEC_ERROR(dwMajorStatus, dwMinorStatus)       \
    if ((dwMajorStatus!= GSS_S_COMPLETE) &&                   \
        (dwMajorStatus != GSS_S_CONTINUE_NEEDED)) {           \
        LOG_ERROR("GSS API Error: Maj:%d Min: %d at %s:%d", dwMajorStatus, dwMinorStatus,  __FILE__, __LINE__); \
        dwError = MAC_AD_ERROR_GSS_API_FAILED;                \
        goto cleanup;                                         \
    }

#define BAIL_ON_KRB_ERROR(ctx, ret)                       \
    if (ret) {                                            \
        LOG_ERROR("KRB5 error at %s:%d. Error code: %d", __FILE__, __LINE__, ret); \
        switch (ret) {                                    \
        case ENOENT:                                      \
            dwError = ret;                                \
            break;                                        \
        case KRB5_LIBOS_BADPWDMATCH:                      \
            dwError = MAC_AD_ERROR_KRB5_PASSWORD_MISMATCH;\
            break;                                        \
        case KRB5KDC_ERR_KEY_EXP:                         \
            dwError = MAC_AD_ERROR_KRB5_PASSWORD_EXPIRED; \
            break;                                        \
        case KRB5KRB_AP_ERR_SKEW:                         \
            dwError = MAC_AD_ERROR_KRB5_CLOCK_SKEW;       \
            break;                                        \
        default:                                          \
            dwError = MAC_AD_ERROR_KRB5_ERROR;            \
        }                                                 \
        goto error;                                       \
    }

#define ADU_DISPLAY_NAME_ATTR            "displayName"
#define ADU_FLAGS_ATTR                   "flags"
#define ADU_FILESYS_PATH_ATTR            "gPCFileSysPath"
#define ADU_FUNCTIONALITY_VERSION_ATTR   "gPCFunctionalityVersion"
#define ADU_MACHINE_EXTENSION_NAMES_ATTR "gPCMachineExtensionNames"
#define ADU_USER_EXTENSION_NAMES_ATTR    "gPCUserExtensionNames"
#define ADU_WQL_FILTER_ATTR              "gPCWQLFilter"
#define ADU_VERSION_NUMBER_ATTR          "versionNumber"
#define ADU_GPLINK_ATTR                  "gPLink"
#define ADU_GPOPTIONS_ATTR               "gPOptions"
#define ADU_OBJECTGUID_ATTR              "objectGUID"

#define USER_MCX_CSE_GUID     "{07E500C4-20FD-4829-8F38-B5FF63FA0493}"
#define COMPUTER_MCX_CSE_GUID "{B9BF896E-F9EB-49b5-8E67-11E2EDAED06C}"


#endif /* __DEFS_H__ */

