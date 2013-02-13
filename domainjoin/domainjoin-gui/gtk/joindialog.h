/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software    2004-2008
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

#ifndef __JOINDIALOG_H__
#define __JOINDIALOG_H__

#include "common.h"

#define JOINDIALOG_CLOSE 0
#define JOINDIALOG_JOIN 1

struct JoinDialog;

typedef struct JoinDialog JoinDialog;

JoinDialog* joindialog_new(const PJOINSTATE pJoinState);
int joindialog_run(JoinDialog* dialog);
const char* joindialog_get_computer_name(JoinDialog* dialog);
const char* joindialog_get_domain_name(JoinDialog* dialog);
gboolean  joindialog_get_ou_active(JoinDialog* dialog);
const char* joindialog_get_ou_name(JoinDialog* dialog);
gboolean joindialog_get_modify_hosts(JoinDialog* dialog);
const char* joindialog_get_default_prefix(JoinDialog* dialog);
GtkWindow* joindialog_get_gtk_window(JoinDialog* dialog);
void joindialog_delete(JoinDialog* dialog);

#endif
