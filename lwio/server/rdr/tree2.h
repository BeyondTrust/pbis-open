/*
 * Copyright Likewise Software
 * All rights reserved.
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
 * license@likewisesoftware.com
 */

/*
 * Module Name:
 *
 *        tree2.h
 *
 * Abstract:
 *
 *        LWIO Redirector
 *
 *        SMB2 tree management
 *
 * Author: Brian Koropoff (bkoropoff@likewise.com)
 *
 */

#ifndef __RDR_TREE2_H__
#define __RDR_TREE2_H_

VOID
RdrTree2Revive(
    PRDR_TREE2 pTree
    );

VOID
RdrTree2Release(
    PRDR_TREE2 pTree
    );

NTSTATUS
RdrTree2Create(
    PRDR_TREE2* ppTree
    );

NTSTATUS
RdrTree2Invalidate(
    PRDR_TREE2 pTree,
    NTSTATUS ntStatus
    );


#endif /* __RDR_TREE2_H_ */
