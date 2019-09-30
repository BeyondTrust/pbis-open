/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright © BeyondTrust Software 2004 - 2019
 * All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * BEYONDTRUST MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING TERMS AS
 * WELL. IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT WITH
 * BEYONDTRUST, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE TERMS OF THAT
 * SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE APACHE LICENSE,
 * NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU HAVE QUESTIONS, OR WISH TO REQUEST
 * A COPY OF THE ALTERNATE LICENSING TERMS OFFERED BY BEYONDTRUST, PLEASE CONTACT
 * BEYONDTRUST AT beyondtrust.com/contact
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
