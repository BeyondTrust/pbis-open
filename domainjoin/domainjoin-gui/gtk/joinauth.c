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

#include "DomainJoinConfig.h"
#include <gtk/gtk.h>
#include <glade/glade.h>

#include "joinauth.h"
#include "common.h"

struct JoinAuthDialog
{
    GtkDialog* dialog;
    GtkEntry* user;
    GtkEntry* password;
};

JoinAuthDialog*
joinauth_new(PJOINSTATE pJoinState, GtkWindow* parent)
{
    GladeXML* xml = glade_xml_new (DOMAINJOIN_XML, "JoinAuthDialog", NULL);
    JoinAuthDialog* dialog = g_new0(JoinAuthDialog, 1);

    if(!xml || !dialog)
        goto cleanup;

    dialog->dialog = GTK_DIALOG(glade_xml_get_widget(xml, "JoinAuthDialog"));
    g_assert(dialog->dialog != NULL);
    g_object_ref(G_OBJECT(dialog->dialog));

    gtk_window_set_transient_for (GTK_WINDOW(dialog->dialog), parent);

    dialog->user = GTK_ENTRY(glade_xml_get_widget(xml, "JoinUserEntry"));
    g_assert(dialog->user != NULL);
    g_object_ref(G_OBJECT(dialog->user));

    dialog->password = GTK_ENTRY(glade_xml_get_widget(xml, "JoinPasswordEntry"));
    g_assert(dialog->password != NULL);
    g_object_ref(G_OBJECT(dialog->password));

    if (pJoinState->user)
        gtk_entry_set_text(dialog->user, pJoinState->user);

cleanup:
    if (xml)
    {
        g_object_unref(xml);
        xml = NULL;
    }
    return dialog;
}

void
joinauth_delete(JoinAuthDialog* dialog)
{
    g_object_unref(G_OBJECT(dialog->dialog));
    g_object_unref(G_OBJECT(dialog->user));
    g_object_unref(G_OBJECT(dialog->password));
    gtk_widget_destroy(GTK_WIDGET(dialog->dialog));
}

int
joinauth_run(JoinAuthDialog* dialog)
{
    return gtk_dialog_run(dialog->dialog);
}

const char*
joinauth_get_user(JoinAuthDialog* dialog)
{
    return gtk_entry_get_text(dialog->user);
}

const char*
joinauth_get_password(JoinAuthDialog* dialog)
{
    return gtk_entry_get_text(dialog->password);
}
