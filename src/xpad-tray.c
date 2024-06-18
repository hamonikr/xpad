/*

Copyright (c) 2002 Jamis Buck
Copyright (c) 2003-2007 Michael Terry
Copyright (c) 2013-2014 Arthur Borsboom
Copyright (c) 2022-2024 Kevin Kim

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

#include "xpad-settings.h"
#include "xpad-tray.h"
#include "xpad-app.h"
#include "xpad-pad.h"
#include "xpad-pad-group.h"
#include "xpad-preferences.h"
#include "fio.h"
#include "help.h"

enum
{
	DO_NOTHING,
	TOGGLE_SHOW_ALL,
	LIST_OF_PADS,
	NEW_PAD
};

#define ICON_NAME "xpad"
#define TRAY_ICON "xpad-panel"

static GtkStatusIcon *status_icon = NULL;
static GtkWidget *tray_menu = NULL;

static void menu_spawn (XpadSettings *settings)
{
	GtkWidget *pad = xpad_pad_new (xpad_app_get_pad_group (), settings);
	gtk_widget_show (pad);
}

static GtkWidget* xpad_tray_create_menu(XpadSettings *settings) {
	XpadPadGroup *group = xpad_app_get_pad_group ();
	gboolean has_pads = xpad_pad_group_has_pads (group);

	GtkWidget *menu = gtk_menu_new ();
	GtkWidget *item;

	item = gtk_menu_item_new_with_mnemonic (_("_New"));
	g_signal_connect_swapped (item, "activate", G_CALLBACK (menu_spawn), settings);
	gtk_container_add (GTK_CONTAINER (menu), item);

	item = gtk_separator_menu_item_new ();
	gtk_container_add (GTK_CONTAINER (menu), item);

	item = gtk_menu_item_new_with_mnemonic (_("_Show All"));
	g_signal_connect_swapped (item, "activate", G_CALLBACK (xpad_pad_group_show_all), group);
	gtk_container_add (GTK_CONTAINER (menu), item);
	gtk_widget_set_sensitive (item, has_pads);

	item = gtk_menu_item_new_with_mnemonic (_("_Close All"));
	g_signal_connect_swapped (item, "activate", G_CALLBACK (xpad_pad_group_close_all), group);
	gtk_container_add (GTK_CONTAINER (menu), item);
	gtk_widget_set_sensitive (item, has_pads);

	item = gtk_separator_menu_item_new ();
	gtk_container_add (GTK_CONTAINER (menu), item);

	xpad_pad_append_pad_titles_to_menu (menu);

	item = gtk_separator_menu_item_new ();
	gtk_container_add (GTK_CONTAINER (menu), item);

	item = gtk_menu_item_new_with_mnemonic (_("_Preferences"));
	g_signal_connect_swapped (item, "activate", G_CALLBACK (xpad_preferences_open), settings);
	gtk_container_add (GTK_CONTAINER (menu), item);

	item = gtk_menu_item_new_with_mnemonic (_("_Help"));
	g_signal_connect_swapped (item, "activate", G_CALLBACK (show_help), settings);
	gtk_container_add (GTK_CONTAINER (menu), item);

	item = gtk_menu_item_new_with_mnemonic (_("_Quit"));
	g_signal_connect (item, "activate", G_CALLBACK (xpad_app_quit), NULL);
	gtk_container_add (GTK_CONTAINER (menu), item);

    gtk_widget_show_all (menu);

    return menu;
}

static char const* getIconName(void)
{
    char const* icon_name;

    GtkIconTheme* theme = gtk_icon_theme_get_default();

    /* If the tray's icon is a 48x48 file, use it. Otherwise, use the fallback builtin icon. */
    if (!gtk_icon_theme_has_icon(theme, TRAY_ICON)) {
        icon_name = ICON_NAME;
    } else {
        GtkIconInfo* icon_info = gtk_icon_theme_lookup_icon(theme, TRAY_ICON, 48, GTK_ICON_LOOKUP_USE_BUILTIN);
        gboolean const icon_is_builtin = gtk_icon_info_get_filename(icon_info) == NULL;
        g_object_unref(icon_info);
        icon_name = icon_is_builtin ? ICON_NAME : TRAY_ICON;
    }

    return icon_name;
}

static GtkStatusIcon* xpad_tray_status_icon_new (XpadSettings *settings)
{
	char const* icon_name = getIconName();
	GtkStatusIcon* icon = gtk_status_icon_new_from_icon_name(icon_name);
	gtk_status_icon_set_title(icon, g_get_application_name());
	gtk_status_icon_set_visible(icon, TRUE);
	return icon;
}

/* Create a fingerprint of all the pad titles. */
static gchar* xpad_tray_get_fingerprint () {
    /* Get all pads sorted by title. */
    GSList *pads = xpad_pad_group_get_pads_sorted_by_title(xpad_app_get_pad_group ());

    GString *pads_fingerprint = g_string_new(NULL);

    for (gint n = 1; pads; pads = pads->next, n++) {
        XpadPad *pad = pads->data;
        gchar *title = xpad_pad_get_title_for_menu(pad, n);
        g_string_append(pads_fingerprint, title);
        g_free (title);
    }

    g_slist_free (pads);

    /* Return the fingerprint. */
    return g_string_free (pads_fingerprint, FALSE);
}

static gboolean xpad_tray_update_menu (XpadSettings *settings) {
	/* Determine if the menu items have changed. */
	gchar *current_fingerprint = g_object_get_data (G_OBJECT (tray_menu), "pads-fingerprint");
	gchar *new_fingerprint = xpad_tray_get_fingerprint();

	int menu_changed = g_strcmp0 (current_fingerprint, new_fingerprint);
	g_free (new_fingerprint);

	/* If the menu did change, then set the new menu. */
	if (menu_changed != 0) {
		if (tray_menu) {
			gtk_widget_destroy(tray_menu);
		}
		tray_menu = xpad_tray_create_menu(settings);
		g_object_set_data_full(G_OBJECT(tray_menu), "pads-fingerprint", g_strdup(new_fingerprint), g_free);
	}

	return TRUE;
}

static void tray_icon_on_click(GtkStatusIcon *status_icon, guint button, guint activate_time, gpointer user_data)
{
    XpadSettings *settings = (XpadSettings *)user_data;
    if (tray_menu) {
        gtk_menu_popup_at_pointer(GTK_MENU(tray_menu), NULL);
    } else {
        tray_menu = xpad_tray_create_menu(settings);
        g_object_set_data_full(G_OBJECT(tray_menu), "pads-fingerprint", xpad_tray_get_fingerprint(), g_free);
        gtk_menu_popup_at_pointer(GTK_MENU(tray_menu), NULL);
    }
}

void xpad_tray_init (XpadSettings *settings) {
    gboolean tray_enabled;
    g_object_get (settings, "tray-enabled", &tray_enabled, NULL);

    if (tray_enabled) {
        status_icon = xpad_tray_status_icon_new(settings);
        g_signal_connect(status_icon, "activate", G_CALLBACK(tray_icon_on_click), settings);
        g_signal_connect(status_icon, "popup-menu", G_CALLBACK(tray_icon_on_click), settings);

		/* Refresh the menu every x seconds */
		guint refresh_each_seconds = 10;
		g_timeout_add_seconds(refresh_each_seconds, (GSourceFunc) xpad_tray_update_menu, settings);
    }
}

void xpad_tray_dispose (XpadSettings *settings) {
    if (status_icon)
        g_object_unref (status_icon);
}

gboolean xpad_tray_has_indicator ()
{
	if (status_icon)
		return TRUE;
	else
		return FALSE;
}
