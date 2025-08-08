/*

Copyright (c) 2002 Jamis Buck
Copyright (c) 2003-2007 Michael Terry
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

#include "xpad-settings.h"

#ifdef HAVE_APP_INDICATOR
#include <libayatana-appindicator/app-indicator.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <signal.h>

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

static AppIndicator *app_indicator = NULL;
static gboolean tray_initialized = FALSE;

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

static AppIndicator* xpad_tray_app_indicator_new (XpadSettings *settings)
{
	char const* icon_name = getIconName();
	
	g_debug("Creating app indicator with icon: %s", icon_name);
	
	/* Create a unique ID based on process ID to prevent conflicts */
	gchar* unique_id = g_strdup_printf("%s-%d", ICON_NAME, getpid());
	
	g_debug("Creating AppIndicator with unique ID: %s", unique_id);
	
	AppIndicator* indicator = app_indicator_new(unique_id, icon_name, APP_INDICATOR_CATEGORY_SYSTEM_SERVICES);
	
	if (indicator == NULL) {
		g_warning("Failed to create AppIndicator with ID: %s", unique_id);
		g_free(unique_id);
		return NULL;
	}
	
	g_debug("AppIndicator created successfully, setting status to ACTIVE");
	app_indicator_set_status(indicator, APP_INDICATOR_STATUS_ACTIVE);
	app_indicator_set_title(indicator, g_get_application_name());
	
	g_debug("AppIndicator setup completed");
	g_free(unique_id);
	return indicator;
}

/* Create a fingerprint of all the  pad titles. */
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
	GtkMenu *current_menu = app_indicator_get_menu (app_indicator);

	/* If there is no menu, then add it. */
	if (current_menu == NULL) {
		GtkWidget* new_menu = xpad_tray_create_menu(settings);
		app_indicator_set_menu(app_indicator, GTK_MENU(new_menu));
	} else {
		/* Determine if the menu items have changed. */
		gchar *current_fingerprint = g_object_get_data (G_OBJECT (current_menu), "pads-fingerprint");
		gchar *new_fingerprint = xpad_tray_get_fingerprint();

		int menu_changed = g_strcmp0 (current_fingerprint, new_fingerprint);
		g_free (new_fingerprint);

		/* If the menu did change, then set the new menu. */
		if (menu_changed != 0) {
			GtkWidget* new_menu = xpad_tray_create_menu(settings);
			app_indicator_set_menu(app_indicator, GTK_MENU(new_menu));
		}
	}

	return TRUE;
}

void xpad_tray_init (XpadSettings *settings) {
    gboolean tray_enabled;
    g_object_get (settings, "tray-enabled", &tray_enabled, NULL);

    g_debug("Tray initialization called, tray_enabled = %s", tray_enabled ? "TRUE" : "FALSE");
    
    if (!tray_enabled) {
        g_debug("Tray is disabled in settings, not initializing");
        return;
    }

    /* Check if tray is already initialized to prevent duplicates */
    if (tray_initialized) {
        g_debug("Tray already initialized, skipping duplicate initialization");
        return;
    }

    /* Check if there's already an indicator to prevent duplicates */
    if (app_indicator != NULL) {
        g_debug("Tray indicator already exists, skipping initialization");
        return;
    }

    /* Simple additional check: ensure we have a valid settings object */
    if (settings == NULL) {
        g_warning("Settings object is NULL, cannot initialize tray");
        return;
    }
    
    g_debug("Proceeding with tray indicator creation");
    
    app_indicator = xpad_tray_app_indicator_new(settings);
    
    /* Verify the indicator was created successfully */
    if (app_indicator == NULL) {
        g_warning("Failed to create tray indicator");
        return;
    }
    
    /* Mark as initialized */
    tray_initialized = TRUE;
    
    xpad_tray_update_menu(settings);

    /* Refresh the menu every x seconds */
    guint refresh_each_seconds = 10;
    g_timeout_add_seconds(refresh_each_seconds, (GSourceFunc) xpad_tray_update_menu, settings);
    
    g_debug("Tray indicator successfully initialized");
}

void xpad_tray_dispose (XpadSettings *settings) {
    if (app_indicator) {
        g_clear_object (&app_indicator);
        app_indicator = NULL;
    }
    
    /* Reset the initialization flag */
    tray_initialized = FALSE;
    
    g_debug("Tray indicator disposed");
}

gboolean xpad_tray_has_indicator ()
{
	if (app_indicator)
		return TRUE;
	else
		return FALSE;
}

/* Removed complex process scanning function for simplicity */

#else

void xpad_tray_init (XpadSettings *settings) {
}

void xpad_tray_dispose (XpadSettings *settings) {
}

gboolean xpad_tray_has_indicator ()
{
	return FALSE;
}

#endif
