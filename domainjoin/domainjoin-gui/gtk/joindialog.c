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
#include "joindialog.h"
#include "common.h"

#include <glade/glade.h>
#include <gtk/gtk.h>
#include <string.h>
#include <strings.h>

struct JoinDialog
{
    GtkDialog* dialog;
    GtkEntry* computer_entry;
    GtkEntry* domain_entry;
    GtkEntry* ou_entry;
    GtkEntry* prefix_entry;
    GtkCheckButton* prefix_check;
    GtkRadioButton* ou_specific;
    GtkCheckButton* modify_hosts;
};

static inline
size_t
min_size(size_t a, size_t b)
{
    return a < b ? a : b;
}

static void
ou_specific_toggled(GtkToggleButton* ou_specific, gpointer _data)
{
    JoinDialog* dialog = (JoinDialog*) _data;

    gtk_widget_set_sensitive(GTK_WIDGET(dialog->ou_entry),
                             gtk_toggle_button_get_active(ou_specific));
}

static void
prefix_check_toggled(GtkToggleButton* prefix_check, gpointer _data)
{
    JoinDialog* dialog = (JoinDialog*) _data;

    gtk_widget_set_sensitive(GTK_WIDGET(dialog->prefix_entry),
                             gtk_toggle_button_get_active(prefix_check));
}

static void
domain_changed_event(GtkEntry* entry, gpointer _data)
{
    JoinDialog* dialog = (JoinDialog*) _data;
    const gchar* domain_text = NULL;
    const gchar* prefix_text = NULL;
    gchar* dot = NULL;
    gchar* new_text = NULL;

    domain_text = gtk_entry_get_text(entry);
    prefix_text = gtk_entry_get_text(dialog->prefix_entry);

    if (!strncasecmp(domain_text, prefix_text, min_size(strlen(domain_text), strlen(prefix_text))))
    {
        new_text = g_utf8_strup(domain_text, -1);
        dot = strchr(new_text, '.');
        if (dot)
        {
            *dot = '\0';
        }
        gtk_entry_set_text(dialog->prefix_entry, new_text);
        g_free(new_text);
    }
}

JoinDialog*
joindialog_new(
    const PJOINSTATE pJoinState
    )
{
    GladeXML* xml = glade_xml_new (DOMAINJOIN_XML, "JoinDialog", NULL);
    JoinDialog* dialog = g_new0(JoinDialog, 1);

    if (!xml || !dialog)
        goto cleanup;

    dialog->dialog = GTK_DIALOG(glade_xml_get_widget(xml, "JoinDialog"));
    g_assert(dialog->dialog != NULL);
    g_object_ref(G_OBJECT(dialog->dialog));

    // Computer name text field
    dialog->computer_entry = GTK_ENTRY(glade_xml_get_widget(xml, "ComputerEntry"));
    g_assert(dialog->computer_entry != NULL);
    g_object_ref(G_OBJECT(dialog->computer_entry));

    // Domain name text field
    dialog->domain_entry = GTK_ENTRY(glade_xml_get_widget(xml, "DomainEntry"));
    g_assert(dialog->domain_entry != NULL);
    g_object_ref(G_OBJECT(dialog->domain_entry));

    // Prefix check
    dialog->prefix_check = GTK_CHECK_BUTTON(glade_xml_get_widget(xml, "PrefixCheck"));
    g_assert(dialog->prefix_check != NULL);
    g_object_ref(G_OBJECT(dialog->prefix_check));

    // Prefix field
    dialog->prefix_entry = GTK_ENTRY(glade_xml_get_widget(xml, "PrefixEntry"));
    g_assert(dialog->prefix_entry != NULL);
    g_object_ref(G_OBJECT(dialog->prefix_entry));

    // OU text field
    dialog->ou_entry = GTK_ENTRY(glade_xml_get_widget(xml, "OUEntry"));
    g_assert(dialog->ou_entry != NULL);
    g_object_ref(G_OBJECT(dialog->ou_entry));

    // OU toggle button
    dialog->ou_specific = GTK_RADIO_BUTTON(glade_xml_get_widget(xml, "OUSpecific"));
    g_assert(dialog->ou_specific != NULL);
    g_object_ref(G_OBJECT(dialog->ou_specific));

    // Modify Hosts toggle button
    dialog->modify_hosts = GTK_CHECK_BUTTON(glade_xml_get_widget(xml, "JoinModifyHosts"));
    g_assert(dialog->modify_hosts != NULL);
    g_object_ref(G_OBJECT(dialog->modify_hosts));

    dialog_insert_likewise_logo(dialog->dialog);

    // Connect signals
    g_signal_connect(G_OBJECT(dialog->ou_specific), "toggled",
        G_CALLBACK(ou_specific_toggled), dialog);

    g_signal_connect(G_OBJECT(dialog->prefix_check), "toggled",
        G_CALLBACK(prefix_check_toggled), dialog);

    g_signal_connect(G_OBJECT(dialog->domain_entry), "changed",
                     G_CALLBACK(domain_changed_event), dialog);

    // Set values saved from previous iteration.
    if (pJoinState->computer)
        gtk_entry_set_text(dialog->computer_entry, pJoinState->computer);

    if (pJoinState->domain)
        gtk_entry_set_text(dialog->domain_entry, pJoinState->domain);

    if (pJoinState->prefix)
    {
        gtk_entry_set_text(dialog->prefix_entry, pJoinState->prefix);
    }

    if (pJoinState->ou)
    {
        gtk_entry_set_text(dialog->ou_entry, pJoinState->ou);

        if (pJoinState->ou_active)
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(dialog->ou_specific), TRUE);
    }

    if (pJoinState->noModifyHosts)
    {
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(dialog->modify_hosts), FALSE);
    }


cleanup:
    if (xml)
    {
        g_object_unref(xml);
        xml = NULL;
    }
    return dialog;
}

int
joindialog_run(JoinDialog* dialog)
{
    return gtk_dialog_run(dialog->dialog);
}

const char*
joindialog_get_computer_name(JoinDialog* dialog)
{
    return gtk_entry_get_text(dialog->computer_entry);
}

const char*
joindialog_get_domain_name(JoinDialog* dialog)
{
    return gtk_entry_get_text(dialog->domain_entry);
}

const char*
joindialog_get_default_prefix(JoinDialog* dialog)
{
    return gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(dialog->prefix_check)) ?
        gtk_entry_get_text(dialog->prefix_entry) : NULL;
}

gboolean
joindialog_get_ou_active(JoinDialog* dialog)
{
    return gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(dialog->ou_specific));
}

const char*
joindialog_get_ou_name(JoinDialog* dialog)
{
    return gtk_entry_get_text(dialog->ou_entry);
}

gboolean
joindialog_get_modify_hosts(JoinDialog* dialog)
{
    return gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(dialog->modify_hosts));
}

GtkWindow*
joindialog_get_gtk_window(JoinDialog* dialog)
{
    return GTK_WINDOW(dialog->dialog);
}

void
joindialog_delete(JoinDialog* dialog)
{
    g_object_unref(G_OBJECT(dialog->dialog));
    g_object_unref(G_OBJECT(dialog->computer_entry));
    g_object_unref(G_OBJECT(dialog->domain_entry));
    g_object_unref(G_OBJECT(dialog->prefix_entry));
    g_object_unref(G_OBJECT(dialog->prefix_check));
    g_object_unref(G_OBJECT(dialog->ou_entry));
    g_object_unref(G_OBJECT(dialog->ou_specific));
    g_object_unref(G_OBJECT(dialog->modify_hosts));

    gtk_widget_destroy(GTK_WIDGET(dialog->dialog));

    g_free(dialog);
}
