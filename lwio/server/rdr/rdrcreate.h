/*
 * Copyright (c) Likewise Software.  All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.  You should have received a copy of the GNU General
 * Public License along with this program.  If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewise.com
 */

NTSTATUS
RdrCommonCreate(
    PRDR_IRP_CONTEXT pIrpContext,
    PIRP pIrp
    );

NTSTATUS
RdrAllocateIrpContext(
    PIRP pIrp,
    PRDR_IRP_CONTEXT * ppIrpContext
    );

NTSTATUS
RdrCommonCreateFile(
    PRDR_IRP_CONTEXT pIrpContext,
    PIRP pIrp
    );

NTSTATUS
RdrBuildAbsolutePathName(
    UNICODE_STRING RootPathName,
    UNICODE_STRING RelativePathName,
    UNICODE_STRING AbsolutePathName
    );

NTSTATUS
RdrGetFilePathName(
    IO_FILE_HANDLE hFileHandle,
    UNICODE_STRING PathName
    );

NTSTATUS
RdrCommonCreateDirectory(
    PRDR_IRP_CONTEXT pIrpContext,
    PIRP pIrp
    );

NTSTATUS
RdrCommonCreateFileSupersede(
    PRDR_IRP_CONTEXT pIrpContext,
    PIRP pIrp
    );

NTSTATUS
RdrCommonCreateFileCreate(
    PRDR_IRP_CONTEXT pIrpContext,
    PIRP pIrp
    );

NTSTATUS
RdrCommonCreateFileOpen(
    PRDR_IRP_CONTEXT pIrpContext,
    PIRP pIrp
    );

NTSTATUS
RdrCommonCreateFileOpenIf(
    PRDR_IRP_CONTEXT pIrpContext,
    PIRP pIrp
    );

NTSTATUS
RdrCommonCreateFileOverwrite(
    PRDR_IRP_CONTEXT pIrpContext,
    PIRP pIrp
    );

NTSTATUS
RdrCommonCreateFileOverwriteIf(
    PRDR_IRP_CONTEXT pIrpContext,
    PIRP pIrp
    );


NTSTATUS
RdrCommonCreateDirectory(
    PRDR_IRP_CONTEXT pIrpContext,
    PIRP pIrp
    );


NTSTATUS
RdrCommonCreateDirectoryFileSupersede(
    PRDR_IRP_CONTEXT pIrpContext,
    PIRP pIrp
    );

NTSTATUS
RdrCommonCreateDirectoryFileCreate(
    PRDR_IRP_CONTEXT pIrpContext,
    PIRP pIrp
    );

NTSTATUS
RdrCommonCreateDirectoryFileOpen(
    PRDR_IRP_CONTEXT pIrpContext,
    PIRP pIrp
    );

NTSTATUS
RdrCommonCreateDirectoryFileOpenIf(
    PRDR_IRP_CONTEXT pIrpContext,
    PIRP pIrp
    );

NTSTATUS
RdrCommonCreateDirectoryFileOverwrite(
    PRDR_IRP_CONTEXT pIrpContext,
    PIRP pIrp
    );

NTSTATUS
RdrCommonCreateDirectoryFileOverwriteIf(
    PRDR_IRP_CONTEXT pIrpContext,
    PIRP pIrp
    );


