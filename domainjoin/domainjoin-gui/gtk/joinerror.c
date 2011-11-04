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
#include "joinerror.h"
#include "common.h"

#include <glade/glade.h>
#include <stdarg.h>
#include <ctdef.h>
#include <ctstrutils.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define abort() \
    do \
    { \
        fprintf(stderr, "%s:%i: abort()", __FILE__, __LINE__); \
        abort(); \
    } while (0) \


struct JoinErrorDialog
{
    GtkDialog* dialog;
    GtkLabel* error_short;
    GtkLabel* error_long;
    GtkTextView* error_details;
};

JoinErrorDialog*
joinerror_new(GtkWindow* parent, LWException* exc)
{
    GladeXML* xml = glade_xml_new (DOMAINJOIN_XML, "JoinErrorDialog", NULL);
    JoinErrorDialog* dialog = g_new0(JoinErrorDialog, 1);

    if(!xml || !dialog)
        goto cleanup;

    dialog->dialog = GTK_DIALOG(glade_xml_get_widget(xml, "JoinErrorDialog"));
    g_assert(dialog->dialog != NULL);
    g_object_ref(G_OBJECT(dialog->dialog));

    gtk_window_set_transient_for (GTK_WINDOW(dialog->dialog), parent);

    dialog->error_short = GTK_LABEL(glade_xml_get_widget(xml, "JoinErrorShort"));
    g_assert(dialog->error_short != NULL);
    g_object_ref(G_OBJECT(dialog->error_short));

    if (exc->shortMsg)
    {
        char* markup;

        if (CTAllocateStringPrintf(&markup,
                    "<span weight=\"bold\" size=\"x-large\">%s</span>",
                    exc->shortMsg))
            abort();

        gtk_label_set_markup(dialog->error_short, markup);
    }
    else
    {
        gtk_label_set_markup(dialog->error_short, 
                 "<span weight=\"bold\" size=\"x-large\">Join error encountered</span>");
    }

    dialog->error_long = GTK_LABEL(glade_xml_get_widget(xml, "JoinErrorLong"));
    g_assert(dialog->error_long != NULL);
    g_object_ref(G_OBJECT(dialog->error_long));

    if (exc->longMsg)
    {
        gtk_label_set_text(dialog->error_long, exc->longMsg);
    }
    else
    {
        gtk_label_set_text(dialog->error_long,
                           "An unexpected or internal error was encountered "
                           "during the domain join.  Please contact Likewise technical "
                           "support for assistance.");
    }

    dialog->error_details = GTK_TEXT_VIEW(glade_xml_get_widget(xml, "JoinErrorDetails"));
    g_assert(dialog->error_details != NULL);
    g_object_ref(G_OBJECT(dialog->error_details));

    {
        char* details;
        GtkTextIter iter;
        GtkTextBuffer* buffer;
        LWStackFrame* stack;

        buffer = gtk_text_view_get_buffer(dialog->error_details);

        if (CTAllocateStringPrintf(&details,
                                   "Error code: %s (0x%.8x)\n\n"
                                   "Backtrace:",
                                   LwWin32ExtErrorToName(exc->code), exc->code))
            abort();

        gtk_text_buffer_set_text(
                gtk_text_view_get_buffer(dialog->error_details), details, -1);

        gtk_text_buffer_get_end_iter(buffer, &iter);

        CTFreeString(details);

        for (stack = &exc->stack; stack; stack = stack->down)
        {
            if (CTAllocateStringPrintf(&details,
                                       "\n    %s:%i",
                                       stack->file, stack->line))
                abort();

            gtk_text_buffer_insert(buffer, &iter, details, -1);

            CTFreeString(details);
        }
    }

cleanup:
    if (xml)
    {
        g_object_unref(xml);
        xml = NULL;
    }
    return dialog;
}

int joinerror_run(JoinErrorDialog* dialog)
{
    return gtk_dialog_run(dialog->dialog);
}

void
joinerror_delete(JoinErrorDialog* dialog)
{
    g_object_unref(G_OBJECT(dialog->dialog));
    g_object_unref(G_OBJECT(dialog->error_short));
    g_object_unref(G_OBJECT(dialog->error_long));
    g_object_unref(G_OBJECT(dialog->error_details));

    gtk_widget_destroy(GTK_WIDGET(dialog->dialog));

    g_free(dialog);
}
