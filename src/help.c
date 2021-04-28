/*

Copyright (c) 2001-2007 Michael Terry
Copyright (c) 2013-2014 Arthur Borsboom

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

*/

#include "../config.h"

#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include "help.h"
#include "xpad-app.h"

GtkWindow *help_window = NULL;

static void help_close ()
{
	help_window = NULL;
}

void show_help ()
{
	if (help_window == NULL) {

		GtkWidget *helptext, *scrolled_window, *button;
		GtkBox *vbox;
		gchar *helptextbuf = NULL;
		gboolean success;
		GError *error = NULL;

		/* Load help text from file */
		success = g_file_get_contents (HELP_FILE, &helptextbuf, NULL, &error);

		if (!success) {
			xpad_app_error (NULL, _("Error showing the help"), g_strdup_printf (_("Could not find the help file %s\n%s"), HELP_FILE, error->message));
			return;
		}

		/* Set layout of help text */
		helptext = gtk_label_new ("");
		gtk_label_set_markup (GTK_LABEL (helptext), helptextbuf);
		g_free (helptextbuf);
		gtk_widget_set_margin_top (helptext, 12);
		gtk_widget_set_margin_bottom (helptext, 12);
		gtk_widget_set_margin_start (helptext, 12);
		gtk_widget_set_margin_end (helptext, 12);
		gtk_label_set_line_wrap (GTK_LABEL (helptext), TRUE);

		/* Create a box and stuff the text and buttons in */
		vbox = GTK_BOX (gtk_box_new (GTK_ORIENTATION_VERTICAL, 18));
		gtk_box_set_homogeneous (vbox, FALSE);
		gtk_box_pack_start (vbox, helptext, TRUE, TRUE, 10);
		button = gtk_button_new_from_icon_name ("gtk-close", GTK_ICON_SIZE_BUTTON);
		gtk_button_set_label (GTK_BUTTON (button), _("Close"));
		gtk_box_pack_start (vbox, button, FALSE, FALSE, 5);

		/* Initiliaze help window */
		help_window = GTK_WINDOW (gtk_window_new (GTK_WINDOW_TOPLEVEL));
		gtk_window_set_title (help_window, _("Help"));
		gtk_window_set_position (help_window, GTK_WIN_POS_CENTER);
		gtk_window_resize (help_window, 800, 1000);

		/* Add scrollbars */
		scrolled_window = gtk_scrolled_window_new (NULL, NULL);
		gtk_container_add (GTK_CONTAINER (scrolled_window), GTK_WIDGET (vbox));
		gtk_scrolled_window_set_vadjustment (GTK_SCROLLED_WINDOW (scrolled_window), 0);
		gtk_container_add (GTK_CONTAINER (help_window), scrolled_window);

		g_signal_connect (help_window, "destroy", G_CALLBACK (help_close), NULL);
		g_signal_connect_swapped (GTK_BUTTON (button), "clicked", G_CALLBACK (gtk_widget_destroy), help_window);

		gtk_widget_show_all (GTK_WIDGET (help_window));
		gtk_label_set_selectable (GTK_LABEL (helptext), TRUE);
	}
	else
		gtk_window_present (help_window);
}
