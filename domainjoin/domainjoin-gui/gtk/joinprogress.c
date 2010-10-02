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
#include <lw/attrs.h>

#include "joinprogress.h"
#include "common.h"

#include <ctdef.h>
#include <ctstrutils.h>

#include <stdlib.h>

struct JoinProgressDialog
{
    GtkDialog* dialog;
    GtkLabel* status;
    GtkLabel* title;
    GtkProgressBar* progress;
    GtkButton* close;
    LWException* error;
};

typedef struct
{
    JoinProgressDialog* dialog;
    gdouble ratio;
    char* info;
} StageUpdate;

typedef struct
{
    JoinProgressDialog* dialog;
    LWException* error;
} RaisedError;

static gboolean
update_stage(gpointer data)
{
    StageUpdate* update = (StageUpdate*) data;

    gdk_threads_enter();

    gtk_progress_bar_set_text(update->dialog->progress, update->info);

    if (update->ratio >= 0.0)
    {
        gtk_progress_bar_set_fraction(update->dialog->progress, update->ratio);
    }
    else
    {
        gtk_progress_bar_pulse(update->dialog->progress);
    }

    gdk_threads_leave();

    g_free(update->info);
    g_free(update);

    return FALSE;
}

static gboolean
raise_error(gpointer data)
{
    RaisedError* raised = (RaisedError*) data;

    gdk_threads_enter();

    raised->dialog->error = raised->error;

    gtk_label_set_markup(raised->dialog->status, "<b>Failed</b>");
    gtk_widget_show(GTK_WIDGET(raised->dialog->status));
    gtk_widget_set_sensitive(GTK_WIDGET(raised->dialog->close), TRUE);
    gtk_dialog_response(raised->dialog->dialog, JOINPROGRESS_ERROR);
    
    gdk_threads_leave();

    g_free(raised);

    return FALSE;
}

static gboolean
done(gpointer data)
{
    JoinProgressDialog* dialog = (JoinProgressDialog*) data;

    gdk_threads_enter();

    gtk_label_set_markup(dialog->status, "<b>Succeeded</b>");
    gtk_widget_show(GTK_WIDGET(dialog->status));

    gtk_progress_bar_set_text(dialog->progress, "Done");
    gtk_progress_bar_set_fraction(dialog->progress, 1.0);

    gtk_widget_set_sensitive(GTK_WIDGET(dialog->close), TRUE);

    gdk_threads_leave();

    return FALSE;
}

JoinProgressDialog*
joinprogress_new(GtkWindow* parent, const char* title)
{
    GladeXML* xml = glade_xml_new (DOMAINJOIN_XML, "JoinProgressDialog", NULL);
    JoinProgressDialog* dialog = g_new0(JoinProgressDialog, 1);
    char* title_markup;

    if(!xml || !dialog)
        goto cleanup;

    dialog->dialog = GTK_DIALOG(glade_xml_get_widget(xml, "JoinProgressDialog"));
    g_assert(dialog->dialog != NULL);
    g_object_ref(G_OBJECT(dialog->dialog));

    gtk_window_set_transient_for (GTK_WINDOW(dialog->dialog), parent);

    dialog->status = GTK_LABEL(glade_xml_get_widget(xml, "JoinProgressStatus"));
    g_assert(dialog->status != NULL);
    g_object_ref(G_OBJECT(dialog->status));

    dialog->title = GTK_LABEL(glade_xml_get_widget(xml, "JoinProgressTitle"));
    g_assert(dialog->title != NULL);
    g_object_ref(G_OBJECT(dialog->title));

    if (CTAllocateStringPrintf(&title_markup, "<span weight=\"bold\" size=\"x-large\">%s</span>", title))
        return NULL;

    gtk_label_set_markup(dialog->title, title_markup);

    CTFreeString(title_markup);

    dialog->progress = GTK_PROGRESS_BAR(glade_xml_get_widget(xml, "JoinProgressBar"));
    g_assert(dialog->progress != NULL);
    g_object_ref(G_OBJECT(dialog->progress));

    dialog->close = GTK_BUTTON(glade_xml_get_widget(xml, "JoinProgressClose"));
    g_assert(dialog->close != NULL);
    g_object_ref(G_OBJECT(dialog->close));

cleanup:
    if (xml)
    {
        g_object_unref(xml);
        xml = NULL;
    }
    return dialog;
}

void
joinprogress_done(JoinProgressDialog* dialog)
{
    g_idle_add(done, dialog);
}

void
joinprogress_raise_error(JoinProgressDialog* dialog, LWException* error)
{
    RaisedError* raised = g_new0(RaisedError, 1);

    if (!raised)
	return;

    raised->dialog = dialog;
    raised->error = error;
    
    g_idle_add(raise_error, raised);
}

LWException*
joinprogress_get_error(JoinProgressDialog* dialog)
{
    return dialog->error;
}

void
joinprogress_update(JoinProgressDialog* dialog, gdouble ratio, const char* description)
{
    StageUpdate* update = g_new0(StageUpdate, 1);

    if (!update)
	return;

    update->dialog = dialog;
    update->ratio = ratio;
    update->info = g_strdup(description);
    
    if (!update->info)
	return;

    g_idle_add(update_stage, update);
}

void joinprogress_delete(JoinProgressDialog* dialog)
{
    g_object_unref(G_OBJECT(dialog->dialog));
    g_object_unref(G_OBJECT(dialog->status));
    g_object_unref(G_OBJECT(dialog->progress));
    g_object_unref(G_OBJECT(dialog->close));

    gtk_widget_destroy(GTK_WIDGET(dialog->dialog));

    free(dialog);
}

int
joinprogress_run(JoinProgressDialog* dialog)
{
    return gtk_dialog_run(dialog->dialog);
}

GtkWindow*
joinprogress_get_gtk_window(JoinProgressDialog* dialog)
{
    return GTK_WINDOW(dialog->dialog);
}
