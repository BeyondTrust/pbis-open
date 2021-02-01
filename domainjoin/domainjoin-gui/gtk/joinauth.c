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
