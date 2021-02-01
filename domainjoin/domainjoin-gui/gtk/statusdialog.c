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
#include "statusdialog.h"
#include "common.h"

#include <glade/glade.h>
#include <gtk/gtk.h>

struct StatusDialog
{
    GtkDialog* dialog;
    GtkEntry* computer_entry;
    GtkEntry* domain_entry;
};

StatusDialog*
statusdialog_new(const char* computer, const char* domain)
{
    GladeXML* xml = glade_xml_new (DOMAINJOIN_XML, "StatusDialog", NULL);
    StatusDialog* dialog = g_new0(StatusDialog, 1);

    if (!xml || !dialog)
    {
        goto cleanup;
    }

    dialog->dialog = GTK_DIALOG(glade_xml_get_widget(xml, "StatusDialog"));
    g_assert(dialog->dialog != NULL);
    g_object_ref(G_OBJECT(dialog->dialog));

    dialog->computer_entry = GTK_ENTRY(glade_xml_get_widget(xml, "StatusComputerEntry"));
    g_assert(dialog->computer_entry != NULL);
    g_object_ref(G_OBJECT(dialog->computer_entry));

    if (computer)
        gtk_entry_set_text(dialog->computer_entry, computer);

    dialog->domain_entry = GTK_ENTRY(glade_xml_get_widget(xml, "StatusDomainEntry"));
    g_assert(dialog->domain_entry != NULL);
    g_object_ref(G_OBJECT(dialog->domain_entry));

    if (domain)
        gtk_entry_set_text(dialog->domain_entry, domain);

    dialog_insert_likewise_logo(dialog->dialog);

cleanup:
    if (xml)
    {
        g_object_unref(xml);
        xml = NULL;
    }

    return dialog;
}

int
statusdialog_run(StatusDialog* dialog)
{
    return gtk_dialog_run(dialog->dialog);
}

GtkWindow*
statusdialog_get_gtk_window(StatusDialog* dialog)
{
    return GTK_WINDOW(dialog->dialog);
}

void
statusdialog_delete(StatusDialog* dialog)
{
    g_object_unref(G_OBJECT(dialog->dialog));
    g_object_unref(G_OBJECT(dialog->computer_entry));
    g_object_unref(G_OBJECT(dialog->domain_entry));

    gtk_widget_destroy(GTK_WIDGET(dialog->dialog));
    
    g_free(dialog);
}
