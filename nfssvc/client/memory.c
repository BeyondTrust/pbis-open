/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

/*
 * Copyright Likewise Software    2004-2008
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

#include "includes.h"


#define NtStatusToWin32(x) ((NET_API_STATUS)(x))

NET_API_STATUS NfsSvcInitMemory(void)
{
    NET_API_STATUS status = ERROR_SUCCESS;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    int locked = 0;

    GLOBAL_DATA_LOCK(locked);

    if (!bNfsSvcInitialised) {
        ntStatus = MemPtrListInit((PtrList**)&nfssvc_ptr_list);
        BAIL_ON_NT_STATUS(ntStatus);

        bNfsSvcInitialised = 1;
    }

cleanup:
    GLOBAL_DATA_UNLOCK(locked);

    if (status == ERROR_SUCCESS &&
        ntStatus != STATUS_SUCCESS)
    {
        status = LwNtStatusToWin32Error(ntStatus);
    }

    return status;

error:
    goto cleanup;
}


NET_API_STATUS NfsSvcDestroyMemory(void)
{
    NET_API_STATUS status = STATUS_SUCCESS;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    int locked = 0;

    GLOBAL_DATA_LOCK(locked);

    if (bNfsSvcInitialised && nfssvc_ptr_list) {
        ntStatus = MemPtrListDestroy((PtrList**)&nfssvc_ptr_list);
        BAIL_ON_NT_STATUS(ntStatus);

        bNfsSvcInitialised = 0;
    }

cleanup:
    GLOBAL_DATA_UNLOCK(locked);

    if (status == ERROR_SUCCESS &&
        ntStatus != STATUS_SUCCESS)
    {
        status = LwNtStatusToWin32Error(ntStatus);
    }

    return status;

error:
    goto cleanup;
}


NET_API_STATUS NfsSvcAllocateMemory(void **out, size_t size, void *dep)
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    ntStatus = MemPtrAllocate((PtrList*)nfssvc_ptr_list, out, size, dep);
    return LwNtStatusToWin32Error(ntStatus);
}


NET_API_STATUS NfsSvcFreeMemory(void *ptr)
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    ntStatus = MemPtrFree((PtrList*)nfssvc_ptr_list, ptr);
    return LwNtStatusToWin32Error(ntStatus);
}


NET_API_STATUS NfsSvcAddDepMemory(void *ptr, void *dep)
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    ntStatus = MemPtrAddDependant((PtrList*)nfssvc_ptr_list, ptr, dep);
    return LwNtStatusToWin32Error(ntStatus);
}

#define DUP_WC16S(ptr, str) do { \
    if (str) { \
        str = wc16sdup(str); \
        if (!(str)) { \
            status = ERROR_NOT_ENOUGH_MEMORY; \
            BAIL_ON_WIN_ERROR(status);  \
        } \
        status = NfsSvcAddDepMemory((void *)str, ptr); \
        BAIL_ON_WIN_ERROR(status); \
    } \
} while (0)

NET_API_STATUS NfsSvcCopyNetConnCtr(UINT32 level, nfssvc_NetConnCtr *ctr,
                                    UINT32 *entriesread, UINT8 **bufptr)
{
    NET_API_STATUS status = ERROR_SUCCESS;
    int i;
    int count = 0;
    void *ptr = NULL;

    BAIL_ON_INVALID_PTR(entriesread, status);
    BAIL_ON_INVALID_PTR(bufptr, status);
    BAIL_ON_INVALID_PTR(ctr, status);

    *entriesread = 0;
    *bufptr = NULL;

    switch (level) {
    case 0:
        if (ctr->ctr0) {
            PCONNECTION_INFO_0 a0;

            count = ctr->ctr0->count;

            status = NfsSvcAllocateMemory(&ptr,
                                          sizeof(CONNECTION_INFO_0) * count,
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            a0 = (PCONNECTION_INFO_0)ptr;

            for (i=0; i < count; i++) {
                 a0[i] = ctr->ctr0->array[i];
            }
        }
        break;
    case 1:
        if (ctr->ctr1) {
            PCONNECTION_INFO_1 a1;

            count = ctr->ctr1->count;

            status = NfsSvcAllocateMemory(&ptr,
                                          sizeof(CONNECTION_INFO_1) * count,
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            a1 = (PCONNECTION_INFO_1)ptr;

            for (i=0; i < count; i++) {
                 a1[i] = ctr->ctr1->array[i];
                 DUP_WC16S(a1, a1[i].coni1_username);
                 DUP_WC16S(a1, a1[i].coni1_netname);
            }
        }
        break;
    }

    *entriesread = count;
    *bufptr = (UINT8 *)ptr;

cleanup:
    return status;

error:
    if (ptr) {
        NfsSvcFreeMemory(ptr);
    }
    goto cleanup;
}


NET_API_STATUS NfsSvcCopyNetFileCtr(UINT32 level, nfssvc_NetFileCtr *ctr,
                                    UINT32 *entriesread, UINT8 **bufptr)
{
    NET_API_STATUS status = ERROR_SUCCESS;
    int i;
    int count = 0;
    void *ptr = NULL;

    BAIL_ON_INVALID_PTR(entriesread, status);
    BAIL_ON_INVALID_PTR(bufptr, status);
    BAIL_ON_INVALID_PTR(ctr, status);

    *entriesread = 0;
    *bufptr = NULL;

    switch (level) {
    case 2:
        if (ctr->ctr2) {
            PFILE_INFO_2 a2;

            count = ctr->ctr2->count;

            status = NfsSvcAllocateMemory(&ptr,
                                          sizeof(FILE_INFO_2) * count,
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            a2 = (PFILE_INFO_2)ptr;

            for (i=0; i < count; i++) {
                 a2[i] = ctr->ctr2->array[i];
            }
        }
        break;
    case 3:
        if (ctr->ctr3) {
            PFILE_INFO_3 a3;

            count = ctr->ctr3->count;

            status = NfsSvcAllocateMemory(&ptr,
                                          sizeof(FILE_INFO_3) * count,
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            a3 = (PFILE_INFO_3)ptr;

            for (i=0; i < count; i++) {
                 a3[i] = ctr->ctr3->array[i];
                 DUP_WC16S(a3, a3[i].fi3_path_name);
                 DUP_WC16S(a3, a3[i].fi3_username);
            }
        }
        break;
    }

    *entriesread = count;
    *bufptr = (UINT8 *)ptr;

cleanup:
    return status;

error:
    if (ptr) {
        NfsSvcFreeMemory(ptr);
    }
    goto cleanup;
}

NET_API_STATUS NfsSvcCopyNetFileInfo(UINT32 level, nfssvc_NetFileInfo *info,
                                     UINT8 **bufptr)
{
    NET_API_STATUS status = ERROR_SUCCESS;
    void *ptr = NULL;

    BAIL_ON_INVALID_PTR(bufptr, status);
    BAIL_ON_INVALID_PTR(info, status);

    *bufptr = NULL;

    switch (level) {
    case 2:
        if (info->info2) {
            PFILE_INFO_2 a2;

            status = NfsSvcAllocateMemory(&ptr,
                                          sizeof(FILE_INFO_2),
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            a2 = (PFILE_INFO_2)ptr;

            *a2 = *info->info2;
        }
        break;
    case 3:
        if (info->info3) {
            PFILE_INFO_3 a3;

            status = NfsSvcAllocateMemory(&ptr,
                                          sizeof(FILE_INFO_3),
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            a3 = (PFILE_INFO_3)ptr;

            *a3 = *info->info3;
            DUP_WC16S(a3, a3->fi3_path_name);
            DUP_WC16S(a3, a3->fi3_username);
        }
        break;
    }

    *bufptr = (UINT8 *)ptr;

cleanup:
    return status;

error:
    if (ptr) {
        NfsSvcFreeMemory(ptr);
    }
    goto cleanup;
}

NET_API_STATUS NfsSvcCopyNetSessCtr(UINT32 level, nfssvc_NetSessCtr *ctr,
                                    UINT32 *entriesread, UINT8 **bufptr)
{
    NET_API_STATUS status = ERROR_SUCCESS;
    int i;
    int count = 0;
    void *ptr = NULL;

    BAIL_ON_INVALID_PTR(entriesread, status);
    BAIL_ON_INVALID_PTR(bufptr, status);
    BAIL_ON_INVALID_PTR(ctr, status);

    *entriesread = 0;
    *bufptr = NULL;

    switch (level) {
    case 0:
        if (ctr->ctr0) {
            PSESSION_INFO_0 a0;

            count = ctr->ctr0->count;

            status = NfsSvcAllocateMemory(&ptr,
                                          sizeof(SESSION_INFO_0) * count,
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            a0 = (PSESSION_INFO_0)ptr;

            for (i=0; i < count; i++) {
                 a0[i] = ctr->ctr0->array[i];
                 DUP_WC16S(a0, a0[i].sesi0_cname);
            }
        }
        break;
    case 1:
        if (ctr->ctr1) {
            PSESSION_INFO_1 a1;

            count = ctr->ctr1->count;

            status = NfsSvcAllocateMemory(&ptr,
                                          sizeof(SESSION_INFO_1) * count,
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            a1 = (PSESSION_INFO_1)ptr;

            for (i=0; i < count; i++) {
                 a1[i] = ctr->ctr1->array[i];
                 DUP_WC16S(a1, a1[i].sesi1_cname);
                 DUP_WC16S(a1, a1[i].sesi1_username);
            }
        }
        break;
    case 2:
        if (ctr->ctr2) {
            PSESSION_INFO_2 a2;

            count = ctr->ctr2->count;

            status = NfsSvcAllocateMemory(&ptr,
                                          sizeof(SESSION_INFO_2) * count,
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            a2 = (PSESSION_INFO_2)ptr;

            for (i=0; i < count; i++) {
                 a2[i] = ctr->ctr2->array[i];
                 DUP_WC16S(a2, a2[i].sesi2_cname);
                 DUP_WC16S(a2, a2[i].sesi2_username);
                 DUP_WC16S(a2, a2[i].sesi2_cltype_name);
            }
        }
        break;
    case 10:
        if (ctr->ctr10) {
            PSESSION_INFO_10 a10;

            count = ctr->ctr10->count;

            status = NfsSvcAllocateMemory(&ptr,
                                          sizeof(SESSION_INFO_10) * count,
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            a10 = (PSESSION_INFO_10)ptr;

            for (i=0; i < count; i++) {
                 a10[i] = ctr->ctr10->array[i];
                 DUP_WC16S(a10, a10[i].sesi10_cname);
                 DUP_WC16S(a10, a10[i].sesi10_username);
            }
        }
        break;
    case 502:
        if (ctr->ctr502) {
            PSESSION_INFO_502 a502;

            count = ctr->ctr502->count;

            status = NfsSvcAllocateMemory(&ptr,
                                          sizeof(SESSION_INFO_502) * count,
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            a502 = (PSESSION_INFO_502)ptr;

            for (i=0; i < count; i++) {
                 a502[i] = ctr->ctr502->array[i];
                 DUP_WC16S(a502, a502[i].sesi502_cname);
                 DUP_WC16S(a502, a502[i].sesi502_username);
                 DUP_WC16S(a502, a502[i].sesi502_cltype_name);
                 DUP_WC16S(a502, a502[i].sesi502_transport);
            }
        }
        break;
    }

    *entriesread = count;
    *bufptr = (UINT8 *)ptr;

cleanup:
    return status;

error:
    if (ptr) {
        NfsSvcFreeMemory(ptr);
    }
    goto cleanup;
}

void *NfsSvcSecDescAllocFn(void *dep, size_t len)
{
    NET_API_STATUS status = ERROR_SUCCESS;
    void *ptr = NULL;

    status = NfsSvcAllocateMemory(&ptr, len, dep);
    BAIL_ON_WIN_ERROR(status);

cleanup:
    return ptr;

error:
    return NULL;
}

NET_API_STATUS NfsSvcCopyNetShareCtr(UINT32 level, nfssvc_NetShareCtr *ctr,
                                    UINT32 *entriesread, UINT8 **bufptr)
{
    NET_API_STATUS status = ERROR_SUCCESS;
    int i;
    int count = 0;
    void *ptr = NULL;

    BAIL_ON_INVALID_PTR(entriesread, status);
    BAIL_ON_INVALID_PTR(bufptr, status);
    BAIL_ON_INVALID_PTR(ctr, status);

    *entriesread = 0;
    *bufptr = NULL;

    switch (level) {
    case 0:
        if (ctr->ctr0) {
            PSHARE_INFO_0 a0;

            count = ctr->ctr0->count;

            status = NfsSvcAllocateMemory(&ptr,
                                          sizeof(SHARE_INFO_0) * count,
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            a0 = (PSHARE_INFO_0)ptr;

            for (i=0; i < count; i++) {
                 a0[i] = ctr->ctr0->array[i];
                 DUP_WC16S(a0, a0[i].shi0_netname);
            }
        }
        break;
    case 1:
        if (ctr->ctr1) {
            PSHARE_INFO_1 a1;

            count = ctr->ctr1->count;

            status = NfsSvcAllocateMemory(&ptr,
                                          sizeof(SHARE_INFO_1) * count,
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            a1 = (PSHARE_INFO_1)ptr;

            for (i=0; i < count; i++) {
                 a1[i] = ctr->ctr1->array[i];
                 DUP_WC16S(a1, a1[i].shi1_netname);
                 DUP_WC16S(a1, a1[i].shi1_remark);
            }
        }
        break;
    case 2:
        if (ctr->ctr2) {
            PSHARE_INFO_2 a2;

            count = ctr->ctr2->count;

            status = NfsSvcAllocateMemory(&ptr,
                                          sizeof(SHARE_INFO_2) * count,
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            a2 = (PSHARE_INFO_2)ptr;

            for (i=0; i < count; i++) {
                 a2[i] = ctr->ctr2->array[i];
                 DUP_WC16S(a2, a2[i].shi2_netname);
                 DUP_WC16S(a2, a2[i].shi2_remark);
                 DUP_WC16S(a2, a2[i].shi2_path);
                 DUP_WC16S(a2, a2[i].shi2_password);
            }
        }
        break;
    case 501:
        if (ctr->ctr501) {
            PSHARE_INFO_501 a501;

            count = ctr->ctr501->count;

            status = NfsSvcAllocateMemory(&ptr,
                                          sizeof(SHARE_INFO_501) * count,
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            a501 = (PSHARE_INFO_501)ptr;

            for (i=0; i < count; i++) {
                 a501[i] = ctr->ctr501->array[i];
                 DUP_WC16S(a501, a501[i].shi501_netname);
                 DUP_WC16S(a501, a501[i].shi501_remark);
            }
        }
        break;
    case 502:
        if (ctr->ctr502) {
            PSHARE_INFO_502 a502;

            count = ctr->ctr502->count;

            status = NfsSvcAllocateMemory(&ptr,
                                          sizeof(SHARE_INFO_502) * count,
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            a502 = (PSHARE_INFO_502)ptr;

            for (i=0; i < count; i++) {
                 PSHARE_INFO_502_I e;

                 e = &ctr->ctr502->array[i];

                 a502[i].shi502_netname      = e->shi502_netname;
                 a502[i].shi502_type         = e->shi502_type;
                 a502[i].shi502_remark       = e->shi502_remark;
                 a502[i].shi502_permissions  = e->shi502_permissions;
                 a502[i].shi502_max_uses     = e->shi502_max_uses;
                 a502[i].shi502_current_uses = e->shi502_current_uses;
                 a502[i].shi502_path         = e->shi502_path;
                 a502[i].shi502_password     = e->shi502_password;
                 a502[i].shi502_reserved     = e->shi502_reserved;

                 if (e->shi502_reserved)
                 {
                     status = NfsSvcAllocateMemory(OUT_PPVOID(&a502[i].shi502_security_descriptor),
                                                   e->shi502_reserved,
                                                   a502);
                     BAIL_ON_WIN_ERROR(status);
                 }

                 DUP_WC16S(a502, a502[i].shi502_netname);
                 DUP_WC16S(a502, a502[i].shi502_remark);
                 DUP_WC16S(a502, a502[i].shi502_path);
                 DUP_WC16S(a502, a502[i].shi502_password);
                 memcpy(a502[i].shi502_security_descriptor, e->shi502_security_descriptor, e->shi502_reserved);
            }
        }
        break;
    }

    *entriesread = count;
    *bufptr = (UINT8 *)ptr;
cleanup:
    return status;
error:
    if (ptr) {
        NfsSvcFreeMemory(ptr);
    }
    goto cleanup;
}

NET_API_STATUS NfsSvcCopyNetShareInfo(UINT32 level, nfssvc_NetShareInfo *info,
                                      UINT8 **bufptr)
{
    NET_API_STATUS status = ERROR_SUCCESS;
    void *ptr = NULL;

    BAIL_ON_INVALID_PTR(bufptr, status);
    *bufptr = NULL;

    BAIL_ON_INVALID_PTR(info, status);

    switch (level) {
    case 0:
        if (info->info0) {
            PSHARE_INFO_0 a0;

            status = NfsSvcAllocateMemory(&ptr,
                                          sizeof(SHARE_INFO_0),
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            a0 = (PSHARE_INFO_0)ptr;

            *a0 = *info->info0;
            DUP_WC16S(a0, a0->shi0_netname);
        }
        break;
    case 1:
        if (info->info1) {
            PSHARE_INFO_1 a1;

            status = NfsSvcAllocateMemory(&ptr,
                                          sizeof(SHARE_INFO_1),
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            a1 = (PSHARE_INFO_1)ptr;

            *a1 = *info->info1;
            DUP_WC16S(a1, a1->shi1_netname);
            DUP_WC16S(a1, a1->shi1_remark);
        }
        break;
    case 2:
        if (info->info2) {
            PSHARE_INFO_2 a2;

            status = NfsSvcAllocateMemory(&ptr,
                                          sizeof(SHARE_INFO_2),
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            a2 = (PSHARE_INFO_2)ptr;

            *a2 = *info->info2;
            DUP_WC16S(a2, a2->shi2_netname);
            DUP_WC16S(a2, a2->shi2_remark);
            DUP_WC16S(a2, a2->shi2_path);
            DUP_WC16S(a2, a2->shi2_password);
        }
        break;
    case 501:
        if (info->info501) {
            PSHARE_INFO_501 a501;

            status = NfsSvcAllocateMemory(&ptr,
                                          sizeof(SHARE_INFO_501),
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            a501 = (PSHARE_INFO_501)ptr;

            *a501 = *info->info501;
            DUP_WC16S(a501, a501->shi501_netname);
            DUP_WC16S(a501, a501->shi501_remark);
        }
        break;
    case 502:
        if (info->info502) {
            PSHARE_INFO_502 a502;
            PSHARE_INFO_502_I e;

            status = NfsSvcAllocateMemory(&ptr,
                                          sizeof(SHARE_INFO_502),
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            a502 = (PSHARE_INFO_502)ptr;

            e = info->info502;

            a502->shi502_netname      = e->shi502_netname;
            a502->shi502_type         = e->shi502_type;
            a502->shi502_remark       = e->shi502_remark;
            a502->shi502_permissions  = e->shi502_permissions;
            a502->shi502_max_uses     = e->shi502_max_uses;
            a502->shi502_current_uses = e->shi502_current_uses;
            a502->shi502_path         = e->shi502_path;
            a502->shi502_password     = e->shi502_password;
            a502->shi502_reserved     = e->shi502_reserved;

            if (e->shi502_reserved)
            {
                status = NfsSvcAllocateMemory(OUT_PPVOID(&a502->shi502_security_descriptor),
                                              e->shi502_reserved,
                                              a502);
                BAIL_ON_WIN_ERROR(status);

                memcpy(a502->shi502_security_descriptor,
                       e->shi502_security_descriptor,
                       e->shi502_reserved);
            }

            DUP_WC16S(a502, a502->shi502_netname);
            DUP_WC16S(a502, a502->shi502_remark);
            DUP_WC16S(a502, a502->shi502_path);
            DUP_WC16S(a502, a502->shi502_password);
        }
        break;
    }

    *bufptr = (UINT8 *)ptr;
cleanup:
    return status;
error:
    if (ptr) {
        NfsSvcFreeMemory(ptr);
    }
    goto cleanup;
}

NET_API_STATUS NfsSvcCopyNetNfsInfo(UINT32 level, nfssvc_NetNfsInfo *info,
                                      UINT8 **bufptr)
{
    NET_API_STATUS status = ERROR_SUCCESS;
    void *ptr = NULL;

    BAIL_ON_INVALID_PTR(bufptr, status);
    *bufptr = NULL;

    BAIL_ON_INVALID_PTR(info, status);

    switch (level) {
    case 100:
        if (info->info100) {
            PSERVER_INFO_100 a100;

            status = NfsSvcAllocateMemory(&ptr,
                                          sizeof(SERVER_INFO_100),
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            a100 = (PSERVER_INFO_100)ptr;

            *a100 = *info->info100;
            DUP_WC16S(a100, a100->sv100_name);
        }
        break;
    case 101:
        if (info->info101) {
            PSERVER_INFO_101 a101;

            status = NfsSvcAllocateMemory(&ptr,
                                          sizeof(SERVER_INFO_101),
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            a101 = (PSERVER_INFO_101)ptr;

            *a101 = *info->info101;
            DUP_WC16S(a101, a101->sv101_name);
            DUP_WC16S(a101, a101->sv101_comment);
        }
        break;
    case 102:
        if (info->info102) {
            PSERVER_INFO_102 a102;

            status = NfsSvcAllocateMemory(&ptr,
                                          sizeof(SERVER_INFO_102),
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            a102 = (PSERVER_INFO_102)ptr;

            *a102 = *info->info102;
            DUP_WC16S(a102, a102->sv102_name);
            DUP_WC16S(a102, a102->sv102_comment);
            DUP_WC16S(a102, a102->sv102_userpath);
        }
        break;
    case 402:
        if (info->info402) {
            PSERVER_INFO_402 a402;

            status = NfsSvcAllocateMemory(&ptr,
                                          sizeof(SERVER_INFO_402),
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            a402 = (PSERVER_INFO_402)ptr;

            *a402 = *info->info402;
            DUP_WC16S(a402, a402->sv402_alerts);
            DUP_WC16S(a402, a402->sv402_guestacct);
            DUP_WC16S(a402, a402->sv402_srvheuristics);
        }
        break;
    case 403:
        if (info->info403) {
            PSERVER_INFO_403 a403;

            status = NfsSvcAllocateMemory(&ptr,
                                          sizeof(SERVER_INFO_403),
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            a403 = (PSERVER_INFO_403)ptr;

            *a403 = *info->info403;
            DUP_WC16S(a403, a403->sv403_alerts);
            DUP_WC16S(a403, a403->sv403_guestacct);
            DUP_WC16S(a403, a403->sv403_srvheuristics);
            DUP_WC16S(a403, a403->sv403_autopath);
        }
        break;
    case 502:
        if (info->info502) {
            PSERVER_INFO_502 a502;

            status = NfsSvcAllocateMemory(&ptr,
                                          sizeof(SERVER_INFO_502),
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            a502 = (PSERVER_INFO_502)ptr;

            *a502 = *info->info502;
        }
        break;
    case 503:
        if (info->info503) {
            PSERVER_INFO_503 a503;

            status = NfsSvcAllocateMemory(&ptr,
                                          sizeof(SERVER_INFO_503),
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            a503 = (PSERVER_INFO_503)ptr;

            *a503 = *info->info503;
            DUP_WC16S(a503, a503->sv503_domain);
        }
        break;
    case 599:
        if (info->info599) {
            PSERVER_INFO_599 a599;

            status = NfsSvcAllocateMemory(&ptr,
                                          sizeof(SERVER_INFO_599),
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            a599 = (PSERVER_INFO_599)ptr;

            *a599 = *info->info599;
            DUP_WC16S(a599, a599->sv599_domain);
        }
        break;
    case 1005:
        if (info->info1005) {
            PSERVER_INFO_1005 a1005;

            status = NfsSvcAllocateMemory(&ptr,
                                          sizeof(SERVER_INFO_1005),
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            a1005 = (PSERVER_INFO_1005)ptr;

            *a1005 = *info->info1005;
            DUP_WC16S(a1005, a1005->sv1005_comment);
        }
        break;
    case 1010:
        if (info->info1010) {
            PSERVER_INFO_1010 a1010;

            status = NfsSvcAllocateMemory(&ptr,
                                          sizeof(SERVER_INFO_1010),
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            a1010 = (PSERVER_INFO_1010)ptr;

            *a1010 = *info->info1010;
        }
        break;
    case 1016:
        if (info->info1016) {
            PSERVER_INFO_1016 a1016;

            status = NfsSvcAllocateMemory(&ptr,
                                          sizeof(SERVER_INFO_1016),
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            a1016 = (PSERVER_INFO_1016)ptr;

            *a1016 = *info->info1016;
        }
        break;
    case 1017:
        if (info->info1017) {
            PSERVER_INFO_1017 a1017;

            status = NfsSvcAllocateMemory(&ptr,
                                          sizeof(SERVER_INFO_1017),
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            a1017 = (PSERVER_INFO_1017)ptr;

            *a1017 = *info->info1017;
        }
        break;
    case 1018:
        if (info->info1018) {
            PSERVER_INFO_1018 a1018;

            status = NfsSvcAllocateMemory(&ptr,
                                          sizeof(SERVER_INFO_1018),
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            a1018 = (PSERVER_INFO_1018)ptr;

            *a1018 = *info->info1018;
        }
        break;
    case 1107:
        if (info->info1107) {
            PSERVER_INFO_1107 a1107;

            status = NfsSvcAllocateMemory(&ptr,
                                          sizeof(SERVER_INFO_1107),
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            a1107 = (PSERVER_INFO_1107)ptr;

            *a1107 = *info->info1107;
        }
        break;
    case 1501:
        if (info->info1501) {
            PSERVER_INFO_1501 a1501;

            status = NfsSvcAllocateMemory(&ptr,
                                          sizeof(SERVER_INFO_1501),
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            a1501 = (PSERVER_INFO_1501)ptr;

            *a1501 = *info->info1501;
        }
        break;
    case 1502:
        if (info->info1502) {
            PSERVER_INFO_1502 a1502;

            status = NfsSvcAllocateMemory(&ptr,
                                          sizeof(SERVER_INFO_1502),
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            a1502 = (PSERVER_INFO_1502)ptr;

            *a1502 = *info->info1502;
        }
        break;
    case 1503:
        if (info->info1503) {
            PSERVER_INFO_1503 a1503;

            status = NfsSvcAllocateMemory(&ptr,
                                          sizeof(SERVER_INFO_1503),
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            a1503 = (PSERVER_INFO_1503)ptr;

            *a1503 = *info->info1503;
        }
        break;
    case 1506:
        if (info->info1506) {
            PSERVER_INFO_1506 a1506;

            status = NfsSvcAllocateMemory(&ptr,
                                          sizeof(SERVER_INFO_1506),
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            a1506 = (PSERVER_INFO_1506)ptr;

            *a1506 = *info->info1506;
        }
        break;
    case 1509:
        if (info->info1509) {
            PSERVER_INFO_1509 a1509;

            status = NfsSvcAllocateMemory(&ptr,
                                          sizeof(SERVER_INFO_1509),
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            a1509 = (PSERVER_INFO_1509)ptr;

            *a1509 = *info->info1509;
        }
        break;
    case 1510:
        if (info->info1510) {
            PSERVER_INFO_1510 a1510;

            status = NfsSvcAllocateMemory(&ptr,
                                          sizeof(SERVER_INFO_1510),
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            a1510 = (PSERVER_INFO_1510)ptr;

            *a1510 = *info->info1510;
        }
        break;
    case 1511:
        if (info->info1511) {
            PSERVER_INFO_1511 a1511;

            status = NfsSvcAllocateMemory(&ptr,
                                          sizeof(SERVER_INFO_1511),
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            a1511 = (PSERVER_INFO_1511)ptr;

            *a1511 = *info->info1511;
        }
        break;
    case 1512:
        if (info->info1512) {
            PSERVER_INFO_1512 a1512;

            status = NfsSvcAllocateMemory(&ptr,
                                          sizeof(SERVER_INFO_1512),
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            a1512 = (PSERVER_INFO_1512)ptr;

            *a1512 = *info->info1512;
        }
        break;
    case 1513:
        if (info->info1513) {
            PSERVER_INFO_1513 a1513;

            status = NfsSvcAllocateMemory(&ptr,
                                          sizeof(SERVER_INFO_1513),
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            a1513 = (PSERVER_INFO_1513)ptr;

            *a1513 = *info->info1513;
        }
        break;
    case 1514:
        if (info->info1514) {
            PSERVER_INFO_1514 a1514;

            status = NfsSvcAllocateMemory(&ptr,
                                          sizeof(SERVER_INFO_1514),
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            a1514 = (PSERVER_INFO_1514)ptr;

            *a1514 = *info->info1514;
        }
        break;
    case 1515:
        if (info->info1515) {
            PSERVER_INFO_1515 a1515;

            status = NfsSvcAllocateMemory(&ptr,
                                          sizeof(SERVER_INFO_1515),
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            a1515 = (PSERVER_INFO_1515)ptr;

            *a1515 = *info->info1515;
        }
        break;
    case 1516:
        if (info->info1516) {
            PSERVER_INFO_1516 a1516;

            status = NfsSvcAllocateMemory(&ptr,
                                          sizeof(SERVER_INFO_1516),
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            a1516 = (PSERVER_INFO_1516)ptr;

            *a1516 = *info->info1516;
        }
        break;
    case 1518:
        if (info->info1518) {
            PSERVER_INFO_1518 a1518;

            status = NfsSvcAllocateMemory(&ptr,
                                          sizeof(SERVER_INFO_1518),
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            a1518 = (PSERVER_INFO_1518)ptr;

            *a1518 = *info->info1518;
        }
        break;
    case 1520:
        if (info->info1520) {
            PSERVER_INFO_1520 a1520;

            status = NfsSvcAllocateMemory(&ptr,
                                          sizeof(SERVER_INFO_1520),
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            a1520 = (PSERVER_INFO_1520)ptr;

            *a1520 = *info->info1520;
        }
        break;
    case 1521:
        if (info->info1521) {
            PSERVER_INFO_1521 a1521;

            status = NfsSvcAllocateMemory(&ptr,
                                          sizeof(SERVER_INFO_1521),
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            a1521 = (PSERVER_INFO_1521)ptr;

            *a1521 = *info->info1521;
        }
        break;
    case 1522:
        if (info->info1522) {
            PSERVER_INFO_1522 a1522;

            status = NfsSvcAllocateMemory(&ptr,
                                          sizeof(SERVER_INFO_1522),
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            a1522 = (PSERVER_INFO_1522)ptr;

            *a1522 = *info->info1522;
        }
        break;
    case 1523:
        if (info->info1523) {
            PSERVER_INFO_1523 a1523;

            status = NfsSvcAllocateMemory(&ptr,
                                          sizeof(SERVER_INFO_1523),
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            a1523 = (PSERVER_INFO_1523)ptr;

            *a1523 = *info->info1523;
        }
        break;
    case 1524:
        if (info->info1524) {
            PSERVER_INFO_1524 a1524;

            status = NfsSvcAllocateMemory(&ptr,
                                          sizeof(SERVER_INFO_1524),
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            a1524 = (PSERVER_INFO_1524)ptr;

            *a1524 = *info->info1524;
        }
        break;
    case 1525:
        if (info->info1525) {
            PSERVER_INFO_1525 a1525;

            status = NfsSvcAllocateMemory(&ptr,
                                          sizeof(SERVER_INFO_1525),
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            a1525 = (PSERVER_INFO_1525)ptr;

            *a1525 = *info->info1525;
        }
        break;
    case 1528:
        if (info->info1528) {
            PSERVER_INFO_1528 a1528;

            status = NfsSvcAllocateMemory(&ptr,
                                          sizeof(SERVER_INFO_1528),
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            a1528 = (PSERVER_INFO_1528)ptr;

            *a1528 = *info->info1528;
        }
        break;
    case 1529:
        if (info->info1529) {
            PSERVER_INFO_1529 a1529;

            status = NfsSvcAllocateMemory(&ptr,
                                          sizeof(SERVER_INFO_1529),
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            a1529 = (PSERVER_INFO_1529)ptr;

            *a1529 = *info->info1529;
        }
        break;
    case 1530:
        if (info->info1530) {
            PSERVER_INFO_1530 a1530;

            status = NfsSvcAllocateMemory(&ptr,
                                          sizeof(SERVER_INFO_1530),
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            a1530 = (PSERVER_INFO_1530)ptr;

            *a1530 = *info->info1530;
        }
        break;
    case 1533:
        if (info->info1533) {
            PSERVER_INFO_1533 a1533;

            status = NfsSvcAllocateMemory(&ptr,
                                          sizeof(SERVER_INFO_1533),
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            a1533 = (PSERVER_INFO_1533)ptr;

            *a1533 = *info->info1533;
        }
        break;
    case 1534:
        if (info->info1534) {
            PSERVER_INFO_1534 a1534;

            status = NfsSvcAllocateMemory(&ptr,
                                          sizeof(SERVER_INFO_1534),
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            a1534 = (PSERVER_INFO_1534)ptr;

            *a1534 = *info->info1534;
        }
        break;
    case 1535:
        if (info->info1535) {
            PSERVER_INFO_1535 a1535;

            status = NfsSvcAllocateMemory(&ptr,
                                          sizeof(SERVER_INFO_1535),
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            a1535 = (PSERVER_INFO_1535)ptr;

            *a1535 = *info->info1535;
        }
        break;
    case 1536:
        if (info->info1536) {
            PSERVER_INFO_1536 a1536;

            status = NfsSvcAllocateMemory(&ptr,
                                          sizeof(SERVER_INFO_1536),
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            a1536 = (PSERVER_INFO_1536)ptr;

            *a1536 = *info->info1536;
        }
        break;
    case 1537:
        if (info->info1537) {
            PSERVER_INFO_1537 a1537;

            status = NfsSvcAllocateMemory(&ptr,
                                          sizeof(SERVER_INFO_1537),
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            a1537 = (PSERVER_INFO_1537)ptr;

            *a1537 = *info->info1537;
        }
        break;
    case 1538:
        if (info->info1538) {
            PSERVER_INFO_1538 a1538;

            status = NfsSvcAllocateMemory(&ptr,
                                          sizeof(SERVER_INFO_1538),
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            a1538 = (PSERVER_INFO_1538)ptr;

            *a1538 = *info->info1538;
        }
        break;
    case 1539:
        if (info->info1539) {
            PSERVER_INFO_1539 a1539;

            status = NfsSvcAllocateMemory(&ptr,
                                          sizeof(SERVER_INFO_1539),
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            a1539 = (PSERVER_INFO_1539)ptr;

            *a1539 = *info->info1539;
        }
        break;
    case 1540:
        if (info->info1540) {
            PSERVER_INFO_1540 a1540;

            status = NfsSvcAllocateMemory(&ptr,
                                          sizeof(SERVER_INFO_1540),
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            a1540 = (PSERVER_INFO_1540)ptr;

            *a1540 = *info->info1540;
        }
        break;
    case 1541:
        if (info->info1541) {
            PSERVER_INFO_1541 a1541;

            status = NfsSvcAllocateMemory(&ptr,
                                          sizeof(SERVER_INFO_1541),
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            a1541 = (PSERVER_INFO_1541)ptr;

            *a1541 = *info->info1541;
        }
        break;
    case 1542:
        if (info->info1542) {
            PSERVER_INFO_1542 a1542;

            status = NfsSvcAllocateMemory(&ptr,
                                          sizeof(SERVER_INFO_1542),
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            a1542 = (PSERVER_INFO_1542)ptr;

            *a1542 = *info->info1542;
        }
        break;
    case 1543:
        if (info->info1543) {
            PSERVER_INFO_1543 a1543;

            status = NfsSvcAllocateMemory(&ptr,
                                          sizeof(SERVER_INFO_1543),
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            a1543 = (PSERVER_INFO_1543)ptr;

            *a1543 = *info->info1543;
        }
        break;
    case 1544:
        if (info->info1544) {
            PSERVER_INFO_1544 a1544;

            status = NfsSvcAllocateMemory(&ptr,
                                          sizeof(SERVER_INFO_1544),
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            a1544 = (PSERVER_INFO_1544)ptr;

            *a1544 = *info->info1544;
        }
        break;
    case 1545:
        if (info->info1545) {
            PSERVER_INFO_1545 a1545;

            status = NfsSvcAllocateMemory(&ptr,
                                          sizeof(SERVER_INFO_1545),
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            a1545 = (PSERVER_INFO_1545)ptr;

            *a1545 = *info->info1545;
        }
        break;
    case 1546:
        if (info->info1546) {
            PSERVER_INFO_1546 a1546;

            status = NfsSvcAllocateMemory(&ptr,
                                          sizeof(SERVER_INFO_1546),
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            a1546 = (PSERVER_INFO_1546)ptr;

            *a1546 = *info->info1546;
        }
        break;
    case 1547:
        if (info->info1547) {
            PSERVER_INFO_1547 a1547;

            status = NfsSvcAllocateMemory(&ptr,
                                          sizeof(SERVER_INFO_1547),
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            a1547 = (PSERVER_INFO_1547)ptr;

            *a1547 = *info->info1547;
        }
        break;
    case 1548:
        if (info->info1548) {
            PSERVER_INFO_1548 a1548;

            status = NfsSvcAllocateMemory(&ptr,
                                          sizeof(SERVER_INFO_1548),
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            a1548 = (PSERVER_INFO_1548)ptr;

            *a1548 = *info->info1548;
        }
        break;
    case 1549:
        if (info->info1549) {
            PSERVER_INFO_1549 a1549;

            status = NfsSvcAllocateMemory(&ptr,
                                          sizeof(SERVER_INFO_1549),
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            a1549 = (PSERVER_INFO_1549)ptr;

            *a1549 = *info->info1549;
        }
        break;
    case 1550:
        if (info->info1550) {
            PSERVER_INFO_1550 a1550;

            status = NfsSvcAllocateMemory(&ptr,
                                          sizeof(SERVER_INFO_1550),
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            a1550 = (PSERVER_INFO_1550)ptr;

            *a1550 = *info->info1550;
        }
        break;
    case 1552:
        if (info->info1552) {
            PSERVER_INFO_1552 a1552;

            status = NfsSvcAllocateMemory(&ptr,
                                          sizeof(SERVER_INFO_1552),
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            a1552 = (PSERVER_INFO_1552)ptr;

            *a1552 = *info->info1552;
        }
        break;
    case 1553:
        if (info->info1553) {
            PSERVER_INFO_1553 a1553;

            status = NfsSvcAllocateMemory(&ptr,
                                          sizeof(SERVER_INFO_1553),
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            a1553 = (PSERVER_INFO_1553)ptr;

            *a1553 = *info->info1553;
        }
        break;
    case 1554:
        if (info->info1554) {
            PSERVER_INFO_1554 a1554;

            status = NfsSvcAllocateMemory(&ptr,
                                          sizeof(SERVER_INFO_1554),
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            a1554 = (PSERVER_INFO_1554)ptr;

            *a1554 = *info->info1554;
        }
        break;
    case 1555:
        if (info->info1555) {
            PSERVER_INFO_1555 a1555;

            status = NfsSvcAllocateMemory(&ptr,
                                          sizeof(SERVER_INFO_1555),
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            a1555 = (PSERVER_INFO_1555)ptr;

            *a1555 = *info->info1555;
        }
        break;
    case 1556:
        if (info->info1556) {
            PSERVER_INFO_1556 a1556;

            status = NfsSvcAllocateMemory(&ptr,
                                          sizeof(SERVER_INFO_1556),
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            a1556 = (PSERVER_INFO_1556)ptr;

            *a1556 = *info->info1556;
        }
        break;
    }

    *bufptr = (UINT8 *)ptr;

cleanup:
    return status;

error:
    if (ptr) {
        NfsSvcFreeMemory(ptr);
    }
    goto cleanup;
}

NET_API_STATUS NfsSvcCopyTIME_OF_DAY_INFO(PTIME_OF_DAY_INFO info,
                                      UINT8 **bufptr)
{
    NET_API_STATUS status = ERROR_SUCCESS;
    void *ptr = NULL;

    BAIL_ON_INVALID_PTR(bufptr, status);

    if (info) {
        PTIME_OF_DAY_INFO a;

        status = NfsSvcAllocateMemory(&ptr,
                                      sizeof(TIME_OF_DAY_INFO),
                                      NULL);
        BAIL_ON_WIN_ERROR(status);

        a = (PTIME_OF_DAY_INFO)ptr;

        *a = *info;
    }

    *bufptr = (UINT8 *)ptr;

cleanup:
    return status;

error:
    *bufptr = NULL;

    goto cleanup;
}

/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
