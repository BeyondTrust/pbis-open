/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

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
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *
 * Abstract:
 *        Registry
 *
 * Authors: Adam Bernstein (abernstein@likewise.com)
 */

#ifndef INCLUDES_H_
#define INCLUDES_H_

#include "config.h"

#include "regsystem.h"
#include <locale.h>
#include <uuid/uuid.h>
#include <lw/base.h>
#include <lw/ntstatus.h>
#include <lw/rtlstring.h>
#include <lw/rtlmemory.h>

#include <lwmsg/lwmsg.h>

#include <reg/lwntreg.h>
#include <regdef.h>

#include <parse/includes.h>
#include "regutils.h"
#include "regparse_r.h"
#include "rsutils.h"
#include "regsrvutils.h"

#include "regserver.h"
#include "regipc.h"
#include "regprovspi.h"

#include "memstore_p.h"
#include "memdb_p.h"
#include "memstore.h"

#include "memapi.h"
#include "externs.h"




#endif /* INCLUDES_H_ */
