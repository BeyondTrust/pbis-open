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

#ifndef __JOINPROGRESS_H__
#define __JOINPROGRESS_H__

#include <glade/glade.h>
#include <gtk/gtk.h>

#include <stdint.h>
#include <lwerror.h>
#include <lwexc.h>

#define JOINPROGRESS_CLOSE 0
#define JOINPROGRESS_ERROR 1

struct JoinProgressDialog;

typedef struct JoinProgressDialog JoinProgressDialog;

JoinProgressDialog* joinprogress_new(GtkWindow* parent, const char* title);
int joinprogress_run(JoinProgressDialog* dialog);
void joinprogress_update(JoinProgressDialog* dialog, gdouble ratio, const char* description);
void joinprogress_raise_error(JoinProgressDialog* dialog, LWException* exc);
LWException* joinprogress_get_error(JoinProgressDialog* dialog);
void joinprogress_done(JoinProgressDialog* dialog);
void joinprogress_delete(JoinProgressDialog* dialog);
GtkWindow* joinprogress_get_gtk_window(JoinProgressDialog* dialog);

#endif
