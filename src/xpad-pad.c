/*

Copyright (c) 2001-2007 Michael Terry
Copyright (c) 2009 Paul Ivanov
Copyright (c) 2011 Dennis Hilmar
Copyright (c) 2011 OBATA Akio
Copyright (c) 2013-2015 Arthur Borsboom
Copyright (c) 2019 Siergiej Riaguzow

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
#include <gtksourceview/gtksource.h>

#include "xpad-pad.h"
#include "xpad-app.h"
#include "xpad-pad-properties.h"
#include "xpad-periodic.h"
#include "xpad-preferences.h"
#include "xpad-search-bar.h"
#include "xpad-text-buffer.h"
#include "xpad-text-view.h"
#include "xpad-toolbar.h"
#include "xpad-tray.h"
#include "fio.h"
#include "help.h"

struct XpadPadPrivate
{
	/* saved values */
	gint x, y;
	guint width, height;
	gboolean location_valid;
	gchar *infoname;
	gchar *contentname;
	gboolean sticky;

	/* selected child widgets */
	GtkOverlay *text_with_search_overlay;

	/*
	 * TODO: Why everything is a GtkWidget? declare as proper child types for readability
	 * see long comment in xpad-toolbar.h. Even if class creators may return GtkWidget*
	 * what is the point of storing them as such. This drastrically reduces readability
	 */

	GtkWidget *textview;
	GtkWidget *scrollbar;
	XpadSearchBar *searchbar;

	/* toolbar stuff */
	GtkWidget *toolbar;
	guint toolbar_timeout;
	guint toolbar_height;
	gboolean toolbar_expanded;
	gboolean toolbar_pad_resized;

	/* properties window */
	GtkWidget *properties;

	/* preferences/xpad global settings */
	XpadSettings *settings;

	/* menus */
	GtkWidget *menu;
	GtkWidget *highlight_menu;

	gboolean unsaved_content;
	gboolean unsaved_info;

	GtkClipboard *clipboard;
	GtkAccelGroup *accel_group;

	XpadPadGroup *group;
};

G_DEFINE_TYPE_WITH_PRIVATE (XpadPad, xpad_pad, GTK_TYPE_WINDOW)

enum
{
	CLOSED,
	LAST_SIGNAL
};

enum
{
  PROP_0,
  PROP_GROUP,
  PROP_SETTINGS,
  N_PROPERTIES
};

static GParamSpec *obj_prop[N_PROPERTIES] = { NULL, };
static guint signals[LAST_SIGNAL] = { 0 };

static void xpad_pad_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec);
static void xpad_pad_get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec);
static void xpad_pad_constructed (GObject *object);
static void xpad_pad_dispose (GObject *object);
static void xpad_pad_finalize (GObject *object);
static void xpad_pad_load_info (XpadPad *pad, gboolean *show);
static GtkWidget *menu_get_popup_highlight (XpadPad *pad, GtkAccelGroup *accel_group);
static GtkWidget *menu_get_popup_no_highlight (XpadPad *pad, GtkAccelGroup *accel_group);
static void xpad_pad_show (XpadPad *pad);
static gboolean xpad_pad_configure_event (XpadPad *pad, GdkEventConfigure *event);
static gboolean xpad_pad_toolbar_size_allocate (XpadPad *pad, GtkAllocation *event);
static gboolean xpad_pad_delete_event (XpadPad *pad, GdkEvent *event);
static gboolean xpad_pad_popup_menu (XpadPad *pad);
static void menu_popup (XpadPad *pad);
static void menu_popdown (XpadPad *pad);
static gboolean xpad_pad_button_press_event (XpadPad *pad, GdkEventButton *event);
static void xpad_pad_text_changed (XpadPad *pad, GtkSourceBuffer *buffer);
static void xpad_pad_notify_has_scrollbar (XpadPad *pad);
static void xpad_pad_notify_has_decorations (XpadPad *pad);
static void xpad_pad_notify_has_toolbar (XpadPad *pad);
static void xpad_pad_notify_autohide_toolbar (XpadPad *pad);
static void xpad_pad_notify_hide_from_taskbar (XpadPad *pad);
static void xpad_pad_notify_hide_from_task_switcher (XpadPad *pad);
static void xpad_pad_hide_toolbar (XpadPad *pad);
static void xpad_pad_show_toolbar (XpadPad *pad);
static void xpad_pad_popup (XpadPad *pad, GdkEventButton *event);
static void xpad_pad_spawn (XpadPad *pad);
static void xpad_pad_clear (XpadPad *pad);
static void xpad_pad_search (XpadPad *pad);
static void xpad_pad_undo (XpadPad *pad);
static void xpad_pad_redo (XpadPad *pad);
static void xpad_pad_cut (XpadPad *pad);
static void xpad_pad_copy (XpadPad *pad);
static void xpad_pad_paste (XpadPad *pad);
static void xpad_pad_delete (XpadPad *pad);
static void xpad_pad_open_properties (XpadPad *pad);
static void xpad_pad_open_preferences (XpadPad *pad);
static void xpad_pad_close_all (XpadPad *pad);
static void xpad_pad_sync_title (XpadPad *pad);
static void xpad_pad_stick_unstick (XpadPad *pad);
static gboolean xpad_pad_leave_notify_event (GtkWidget *pad, GdkEventCrossing *event);
static gboolean xpad_pad_enter_notify_event (GtkWidget *pad, GdkEventCrossing *event);

/* Create a new empty pad. */
GtkWidget *
xpad_pad_new (XpadPadGroup *group, XpadSettings *settings)
{
	GtkWidget *pad = GTK_WIDGET (g_object_new (XPAD_TYPE_PAD, "group", group, "settings", settings, NULL));

	xpad_pad_save_info_delayed (XPAD_PAD (pad));

	return pad;
}

/* Create a new pad based on the provided info-xxxxx file from the config directory and return this pad */
GtkWidget *
xpad_pad_new_with_info (XpadPadGroup *group, XpadSettings *settings, const gchar *info_filename, gboolean *show)
{
	GtkWidget *pad = GTK_WIDGET (g_object_new (XPAD_TYPE_PAD, "group", group, "settings", settings, NULL));

	XPAD_PAD (pad)->priv->infoname = g_strdup (info_filename);
	xpad_pad_load_info (XPAD_PAD (pad), show);
	xpad_pad_load_content (XPAD_PAD (pad));
	gtk_window_set_role (GTK_WINDOW (pad), XPAD_PAD (pad)->priv->infoname);

	return pad;
}

/* Create a new pad based on the provided filename from the command line */
GtkWidget *
xpad_pad_new_from_file (XpadPadGroup *group, XpadSettings *settings, const gchar *filename)
{
	GtkWidget *pad = NULL;
	gchar *content;

	content = fio_get_file (filename, CURRENT_WORK_DIR);

	if (!content) {
		gchar *usertext = g_strdup_printf (_("Could not read file %s."), filename);
		xpad_app_error (NULL, usertext, NULL);
		g_free (usertext);
	} else {
		GtkSourceBuffer *buffer;

		pad = xpad_pad_new (group, settings);

		buffer = GTK_SOURCE_BUFFER (gtk_text_view_get_buffer (GTK_TEXT_VIEW (XPAD_PAD (pad)->priv->textview)));

		xpad_text_buffer_freeze_undo (XPAD_TEXT_BUFFER (buffer));

		g_signal_handlers_block_by_func (buffer, xpad_pad_text_changed, pad);
		xpad_text_buffer_set_text_with_tags (XPAD_TEXT_BUFFER (buffer), content ? content : "");
		g_signal_handlers_unblock_by_func (buffer, xpad_pad_text_changed, pad);

		g_free (content);

		xpad_text_buffer_thaw_undo (XPAD_TEXT_BUFFER (buffer));

		xpad_pad_text_changed(XPAD_PAD(pad), buffer);
	}

	return pad;
}

/* Class pad - constructor */
static void
xpad_pad_class_init (XpadPadClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

	gobject_class->constructed = xpad_pad_constructed;
	gobject_class->set_property = xpad_pad_set_property;
	gobject_class->get_property = xpad_pad_get_property;
	gobject_class->dispose = xpad_pad_dispose;
	gobject_class->finalize = xpad_pad_finalize;

	signals[CLOSED] =
		g_signal_new ("closed",
						  G_OBJECT_CLASS_TYPE (gobject_class),
						  G_SIGNAL_RUN_FIRST,
						  G_STRUCT_OFFSET (XpadPadClass, closed),
						  NULL, NULL,
						  g_cclosure_marshal_VOID__VOID,
						  G_TYPE_NONE,
						  0);

	/* Properties */
	obj_prop[PROP_GROUP] = g_param_spec_pointer ("group", "Pad group", "Pad group for this pad", G_PARAM_READWRITE | G_PARAM_CONSTRUCT);
	obj_prop[PROP_SETTINGS] = g_param_spec_pointer ("settings", "Xpad settings", "Xpad global settings", G_PARAM_READWRITE | G_PARAM_CONSTRUCT);

	g_object_class_install_properties (gobject_class, N_PROPERTIES, obj_prop);
}

static void
xpad_pad_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
	XpadPad *pad = XPAD_PAD (object);

	switch (prop_id)
	{
	case PROP_GROUP:
		pad->priv->group = g_value_get_pointer (value);
		g_object_ref (pad->priv->group);
		if (pad->priv->group)
			xpad_pad_group_add (pad->priv->group, GTK_WIDGET (pad));
		break;

	case PROP_SETTINGS:
		pad->priv->settings = g_value_get_pointer (value);
		g_object_ref (pad->priv->settings);
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
xpad_pad_get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
	XpadPad *pad = XPAD_PAD (object);

	switch (prop_id)
	{
	case PROP_GROUP:
		g_value_set_pointer (value, pad->priv->group);
		break;

	case PROP_SETTINGS:
		g_value_set_pointer (value, pad->priv->settings);
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

/* Class pad - initializer */
static void
xpad_pad_init (XpadPad *pad)
{
	pad->priv = xpad_pad_get_instance_private (pad);

	pad->priv->x = 0;
	pad->priv->y = 0;
	pad->priv->location_valid = FALSE;
	pad->priv->infoname = NULL;
	pad->priv->contentname = NULL;
	pad->priv->text_with_search_overlay = NULL;
	pad->priv->textview = NULL;
	pad->priv->scrollbar = NULL;
	pad->priv->searchbar = NULL;
	pad->priv->toolbar = NULL;
	pad->priv->toolbar_timeout = 0;
	pad->priv->toolbar_height = 0;
	pad->priv->toolbar_expanded = FALSE;
	pad->priv->toolbar_pad_resized = TRUE;
	pad->priv->properties = NULL;
	pad->priv->unsaved_content = FALSE;
	pad->priv->unsaved_info = FALSE;
}

static void xpad_pad_constructed (GObject *object)
{
	XpadPad *pad = XPAD_PAD (object);

	gboolean decorations, hide_from_taskbar, hide_from_task_switcher;

	g_object_get (pad->priv->settings,
			"width", &pad->priv->width,
			"height", &pad->priv->height,
			"autostart-sticky", &pad->priv->sticky, NULL);

	GtkWindow *pad_window = GTK_WINDOW (pad);

	/* textview in scrollbar */
	pad->priv->textview = GTK_WIDGET (XPAD_TEXT_VIEW (xpad_text_view_new (pad->priv->settings, pad)));

	pad->priv->scrollbar = GTK_WIDGET (g_object_new (GTK_TYPE_SCROLLED_WINDOW,
		"hadjustment", NULL,
		"hscrollbar-policy", GTK_POLICY_NEVER,
		"shadow-type", GTK_SHADOW_NONE,
		"vadjustment", NULL,
		"vscrollbar-policy", GTK_POLICY_NEVER,
		"child", pad->priv->textview,
		NULL));

	/* searchbar */
	GtkSourceBuffer *buffer = GTK_SOURCE_BUFFER (gtk_text_view_get_buffer (GTK_TEXT_VIEW (pad->priv->textview)));
	pad->priv->searchbar = xpad_search_bar_new (buffer);

	/* overlay with scrollbar (with text) and searchbar */
	pad->priv->text_with_search_overlay = GTK_OVERLAY (gtk_overlay_new ());
	gtk_overlay_add_overlay (pad->priv->text_with_search_overlay, pad->priv->scrollbar);
	gtk_overlay_add_overlay (pad->priv->text_with_search_overlay, GTK_WIDGET (pad->priv->searchbar));

	/* toolbar */
	pad->priv->toolbar = GTK_WIDGET (xpad_toolbar_new (pad));

	pad->priv->accel_group = gtk_accel_group_new ();
	gtk_window_add_accel_group (pad_window, pad->priv->accel_group);
	pad->priv->menu = menu_get_popup_no_highlight (pad, pad->priv->accel_group);
	pad->priv->highlight_menu = menu_get_popup_highlight (pad, pad->priv->accel_group);
	gtk_accel_group_connect (pad->priv->accel_group, GDK_KEY_Q, GDK_CONTROL_MASK, 0, g_cclosure_new_swap (G_CALLBACK (xpad_app_quit), pad, NULL));

	/* GtkBox with overlay and toolbar */
	GtkBox *vbox = GTK_BOX (gtk_box_new (GTK_ORIENTATION_VERTICAL, 0));
	gtk_box_set_homogeneous (vbox, FALSE);
	gtk_box_pack_start (vbox, GTK_WIDGET (pad->priv->text_with_search_overlay), TRUE, TRUE, 0);
	gtk_box_pack_start (vbox, pad->priv->toolbar, FALSE, FALSE, 0);

	gtk_container_child_set (GTK_CONTAINER (vbox), pad->priv->toolbar, "expand", FALSE, NULL);

	g_object_get (pad->priv->settings, "has-decorations", &decorations, NULL);
	g_object_get (pad->priv->settings, "hide-from-taskbar", &hide_from_taskbar, NULL);
	g_object_get (pad->priv->settings, "hide-from-task-switcher", &hide_from_task_switcher, NULL);
	gtk_window_set_decorated (pad_window, decorations);
	gtk_window_set_default_size (pad_window, (gint) pad->priv->width, (gint) pad->priv->height);
	gtk_window_set_gravity (pad_window, GDK_GRAVITY_STATIC); /* static gravity makes saving pad x,y work */
	gtk_window_set_skip_taskbar_hint (pad_window, hide_from_taskbar);
	gtk_window_set_skip_pager_hint (pad_window, hide_from_task_switcher);
	gtk_window_set_position (pad_window, GTK_WIN_POS_MOUSE);

	g_object_set (G_OBJECT (pad), "child", vbox, NULL);

	xpad_pad_notify_has_scrollbar (pad);

	pad->priv->clipboard = gtk_clipboard_get (GDK_SELECTION_CLIPBOARD);

	xpad_pad_sync_title (pad);

	/* Add CSS style class, so the styling can be overridden by a GTK theme */
	GtkStyleContext *context = gtk_widget_get_style_context(GTK_WIDGET (pad));
	gtk_style_context_add_class(context, "XpadPad");

	gtk_widget_show_all (GTK_WIDGET (vbox));

	gtk_widget_hide (pad->priv->toolbar);
	xpad_pad_notify_has_toolbar (pad);

	/* Set up signals */
	gtk_widget_add_events (GTK_WIDGET (pad), GDK_BUTTON_PRESS_MASK | GDK_PROPERTY_CHANGE_MASK);
	gtk_widget_add_events (pad->priv->toolbar, GDK_ALL_EVENTS_MASK);
	g_signal_connect_swapped (GTK_TEXT_VIEW (pad->priv->textview), "button-press-event", G_CALLBACK (xpad_pad_button_press_event), pad);
	g_signal_connect_swapped (GTK_TEXT_VIEW (pad->priv->textview), "popup-menu", G_CALLBACK (xpad_pad_popup_menu), pad);
	g_signal_connect_swapped (pad->priv->toolbar, "size-allocate", G_CALLBACK (xpad_pad_toolbar_size_allocate), pad);
	g_signal_connect (pad, "button-press-event", G_CALLBACK (xpad_pad_button_press_event), NULL);
	g_signal_connect (pad, "configure-event", G_CALLBACK (xpad_pad_configure_event), NULL);
	g_signal_connect (pad, "delete-event", G_CALLBACK (xpad_pad_delete_event), NULL);
	g_signal_connect (pad, "popup-menu", G_CALLBACK (xpad_pad_popup_menu), NULL);
	g_signal_connect (pad, "show", G_CALLBACK (xpad_pad_show), NULL);
	g_signal_connect_swapped (gtk_text_view_get_buffer (GTK_TEXT_VIEW (pad->priv->textview)), "changed", G_CALLBACK (xpad_pad_text_changed), pad);

	g_signal_connect (pad, "enter-notify-event", G_CALLBACK (xpad_pad_enter_notify_event), NULL);
	g_signal_connect (pad, "leave-notify-event", G_CALLBACK (xpad_pad_leave_notify_event), NULL);

	g_signal_connect_swapped (pad->priv->settings, "notify::hide-from-taskbar", G_CALLBACK (xpad_pad_notify_hide_from_taskbar), pad);
	g_signal_connect_swapped (pad->priv->settings, "notify::hide-from-task-switcher", G_CALLBACK (xpad_pad_notify_hide_from_task_switcher), pad);
	g_signal_connect_swapped (pad->priv->settings, "notify::has-decorations", G_CALLBACK (xpad_pad_notify_has_decorations), pad);
	g_signal_connect_swapped (pad->priv->settings, "notify::has-toolbar", G_CALLBACK (xpad_pad_notify_has_toolbar), pad);
	g_signal_connect_swapped (pad->priv->settings, "notify::autohide-toolbar", G_CALLBACK (xpad_pad_notify_autohide_toolbar), pad);
	g_signal_connect_swapped (pad->priv->settings, "notify::has-scrollbar", G_CALLBACK (xpad_pad_notify_has_scrollbar), pad);
	g_signal_connect_swapped (gtk_text_view_get_buffer (GTK_TEXT_VIEW (pad->priv->textview)), "notify::has-selection", G_CALLBACK (xpad_pad_notify_has_selection), pad);
	g_signal_connect_swapped (pad->priv->clipboard, "owner-change", G_CALLBACK (xpad_pad_notify_clipboard_owner_changed), pad);

	g_signal_connect_swapped (pad->priv->toolbar, "activate-new", G_CALLBACK (xpad_pad_spawn), pad);
	g_signal_connect_swapped (pad->priv->toolbar, "activate-clear", G_CALLBACK (xpad_pad_clear), pad);
	g_signal_connect_swapped (pad->priv->toolbar, "activate-close", G_CALLBACK (xpad_pad_close), pad);
	g_signal_connect_swapped (pad->priv->toolbar, "activate-undo", G_CALLBACK (xpad_pad_undo), pad);
	g_signal_connect_swapped (pad->priv->toolbar, "activate-redo", G_CALLBACK (xpad_pad_redo), pad);
	g_signal_connect_swapped (pad->priv->toolbar, "activate-cut", G_CALLBACK (xpad_pad_cut), pad);
	g_signal_connect_swapped (pad->priv->toolbar, "activate-copy", G_CALLBACK (xpad_pad_copy), pad);
	g_signal_connect_swapped (pad->priv->toolbar, "activate-paste", G_CALLBACK (xpad_pad_paste), pad);
	g_signal_connect_swapped (pad->priv->toolbar, "activate-search", G_CALLBACK (xpad_pad_search), pad);
	g_signal_connect_swapped (pad->priv->toolbar, "activate-delete", G_CALLBACK (xpad_pad_delete), pad);
	g_signal_connect_swapped (pad->priv->toolbar, "activate-properties", G_CALLBACK (xpad_pad_open_properties), pad);
	g_signal_connect_swapped (pad->priv->toolbar, "activate-preferences", G_CALLBACK (xpad_pad_open_preferences), pad);
	g_signal_connect_swapped (pad->priv->toolbar, "activate-quit", G_CALLBACK (xpad_pad_close_all), pad);

	g_signal_connect_swapped (pad->priv->toolbar, "popup", G_CALLBACK (menu_popup), pad);
	g_signal_connect_swapped (pad->priv->toolbar, "popdown", G_CALLBACK (menu_popdown), pad);

	g_signal_connect_swapped (pad->priv->menu, "deactivate", G_CALLBACK (menu_popdown), pad);
	g_signal_connect_swapped (pad->priv->highlight_menu, "deactivate", G_CALLBACK (menu_popdown), pad);
}

static void
xpad_pad_dispose (GObject *object)
{
	XpadPad *pad = XPAD_PAD (object);

	xpad_pad_remove_accelerator_group (pad);

	g_clear_object(&pad->priv->group);

	if (GTK_IS_WIDGET(pad->priv->menu)) {
		gtk_widget_destroy (pad->priv->menu);
		pad->priv->menu = NULL;
	}

	if (GTK_IS_WIDGET(pad->priv->highlight_menu)) {
		gtk_widget_destroy (pad->priv->highlight_menu);
		pad->priv->highlight_menu = NULL;
	}

	if (XPAD_IS_PAD_PROPERTIES (pad->priv->properties)) {
		gtk_widget_destroy (pad->priv->properties);
		pad->priv->properties = NULL;
	}

	gtk_clipboard_clear (pad->priv->clipboard);

	/* For some reason the toolbar handler does not get automatically disconnected (or not at the right moment), leading to errors after deleting a pad. This manual disconnect prevents this error. */
	if (XPAD_IS_TOOLBAR (pad->priv->toolbar)) {
		g_signal_handlers_disconnect_matched (pad->priv->toolbar, G_SIGNAL_MATCH_DATA, 0, 0, NULL, NULL, pad);
		gtk_widget_destroy(pad->priv->toolbar);
		pad->priv->toolbar = NULL;
	}

	G_OBJECT_CLASS (xpad_pad_parent_class)->dispose (object);
}

static void
xpad_pad_finalize (GObject *object)
{
	XpadPad *pad = XPAD_PAD (object);

	if (pad->priv->settings) {
		g_signal_handlers_disconnect_matched (pad->priv->settings, G_SIGNAL_MATCH_DATA, 0, 0, NULL, NULL, pad);
		g_clear_object(&pad->priv->settings);
	}

	g_free (pad->priv->infoname);
	g_free (pad->priv->contentname);

	G_OBJECT_CLASS (xpad_pad_parent_class)->finalize (object);
}

static void
xpad_pad_show (XpadPad *pad)
{
	/*
	 * Some wm's might not acknowledge our request for a specific
	 * location before we are shown.  What we do here is a little gimpy
	 * and not very respectful of wms' sovereignty, but it has the effect
	 * of making pads' locations very dependable.  We just move the pad
	 * again here after being shown.  This may create a visual effect if
	 * the wm did ignore us, but is better than being in the wrong
	 * place, I guess.
	 */
	if (pad->priv->location_valid)
		gtk_window_move (GTK_WINDOW (pad), pad->priv->x, pad->priv->y);

	xpad_pad_stick_unstick (pad);

	/* Show the pad and set the cursor into the pad */
	gtk_window_present (GTK_WINDOW (pad));
	gtk_widget_grab_focus (GTK_WIDGET (pad->priv->textview));

	/* Save the new visibility status to the disk */
	xpad_pad_save_info_delayed (pad);
}

void
xpad_pad_set_sticky (XpadPad *pad, gboolean is_sticky)
{
	pad->priv->sticky = is_sticky;
	xpad_pad_stick_unstick (pad);
	xpad_pad_save_info_delayed (pad);
}

static void
xpad_pad_stick_unstick (XpadPad *pad)
{
	if (pad->priv->sticky)
		gtk_window_stick (GTK_WINDOW (pad));
	else
		gtk_window_unstick (GTK_WINDOW (pad));
}


static gboolean toolbar_timeout (XpadPad *pad)
{
	if (!pad || !pad->priv || !pad->priv->toolbar_timeout)
		return FALSE;

	gboolean has_toolbar, autohide_toolbar;
	g_object_get (pad->priv->settings, "has-toolbar", &has_toolbar, "autohide-toolbar", &autohide_toolbar, NULL);

	if (pad->priv->toolbar_timeout && autohide_toolbar && has_toolbar)
		xpad_pad_hide_toolbar (pad);

	pad->priv->toolbar_timeout = 0;

	return FALSE;
}

static void
xpad_pad_notify_has_decorations (XpadPad *pad)
{
	GtkWidget *pad_widget = GTK_WIDGET (pad);
	GtkWindow *pad_window = GTK_WINDOW (pad);
	gboolean decorations;
	g_object_get (pad->priv->settings, "has-decorations", &decorations, NULL);

	/*
	 *  There are two modes of operation:  a normal mode and a 'stealth' mode.
	 *  If decorations are disabled, we also don't show up in the taskbar or pager.
	 */
	gtk_window_set_decorated (pad_window, decorations);

	/*
	 * reshow_with_initial_size() seems to set the window back to a never-shown state.
	 * This is good, as some WMs don't like us changing the above parameters mid-run,
	 * even if we do a hide/show cycle.
	 */
	gtk_window_set_default_size (pad_window, (gint) pad->priv->width, (gint) pad->priv->height);
	gtk_widget_hide (pad_widget);
	gtk_widget_unrealize (pad_widget);
	gtk_widget_show (pad_widget);
}

static void
xpad_pad_notify_hide_from_taskbar (XpadPad *pad)
{
	gboolean hide;
	g_object_get (pad->priv->settings, "hide-from-taskbar", &hide, NULL);
	gtk_window_set_skip_taskbar_hint (GTK_WINDOW (pad), hide);
}

static void
xpad_pad_notify_hide_from_task_switcher (XpadPad *pad)
{
	gboolean hide;
	g_object_get (pad->priv->settings, "hide-from-task-switcher", &hide, NULL);
	gtk_window_set_skip_pager_hint (GTK_WINDOW (pad), hide);
}

static void
xpad_pad_notify_has_toolbar (XpadPad *pad)
{
	gboolean has_toolbar, autohide_toolbar;
	g_object_get (pad->priv->settings, "has-toolbar", &has_toolbar, "autohide-toolbar", &autohide_toolbar, NULL);

	if (has_toolbar && !autohide_toolbar)
		xpad_pad_show_toolbar (pad);
	else
		xpad_pad_hide_toolbar (pad);
}

static void
xpad_pad_notify_autohide_toolbar (XpadPad *pad)
{
	gboolean autohide_toolbar;
	g_object_get (pad->priv->settings, "autohide-toolbar", &autohide_toolbar, NULL);

	if (autohide_toolbar)
	{
		/* Likely not to be in pad when turning setting on */
		if (!pad->priv->toolbar_timeout)
			pad->priv->toolbar_timeout = g_timeout_add (1000, (GSourceFunc) toolbar_timeout, pad);
	}
	else
	{
		gboolean has_toolbar;
		g_object_get (pad->priv->settings, "has-toolbar", &has_toolbar, NULL);

		if (has_toolbar)
			xpad_pad_show_toolbar(pad);
	}
}

static void
xpad_pad_notify_has_scrollbar (XpadPad *pad)
{
	gboolean has_scrollbar;
	g_object_get (pad->priv->settings, "has-scrollbar", &has_scrollbar, NULL);

	if (has_scrollbar)
		gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (pad->priv->scrollbar),
			GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	else
	{
		GtkAdjustment *v, *h;

		gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (pad->priv->scrollbar),
			GTK_POLICY_NEVER, GTK_POLICY_NEVER);

		/* now we need to adjust view so that user can see whole pad */
		h = gtk_scrolled_window_get_hadjustment (GTK_SCROLLED_WINDOW (pad->priv->scrollbar));
		v = gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW (pad->priv->scrollbar));

		gtk_adjustment_set_value (h, 0);
		gtk_adjustment_set_value (v, 0);
	}
}

static guint
xpad_pad_text_and_toolbar_height (XpadPad *pad)
{
	cairo_rectangle_int_t rec;
	gint textx, texty, x, y;
	GtkTextIter iter;
	GtkSourceView *pad_textview = GTK_SOURCE_VIEW (pad->priv->textview);

	gtk_text_view_get_visible_rect (GTK_TEXT_VIEW (pad_textview), &rec);
	gtk_text_buffer_get_end_iter (gtk_text_view_get_buffer (GTK_TEXT_VIEW (pad_textview)), &iter);
	gtk_text_view_get_iter_location (GTK_TEXT_VIEW (pad_textview), &iter, &rec);
	gtk_text_view_buffer_to_window_coords (GTK_TEXT_VIEW (pad_textview),
		GTK_TEXT_WINDOW_WIDGET, rec.x + rec.width, rec.y + rec.height,
		&textx, &texty);
	gtk_widget_translate_coordinates(GTK_WIDGET (pad_textview), GTK_WIDGET (pad), textx, texty, &x, &y);

	/* Safe cast from gint to guint */
	if (y >= 0) {
		return (guint) y + pad->priv->toolbar_height + gtk_container_get_border_width (GTK_CONTAINER (pad_textview));
	}
	else {
		g_warning("There is a problem in the program Xpad. In function 'xpad_pad_toolbar_size_allocate' the variable 'event->height' is not a positive number. Please send a bugreport to https://bugs.launchpad.net/xpad/+filebug to help improve Xpad.");
		return 0;
	}
}

static void
xpad_pad_show_toolbar (XpadPad *pad)
{
	if (!gtk_widget_get_visible (pad->priv->toolbar))
	{
		GtkRequisition req;
		GtkWidget *pad_widget = GTK_WIDGET (pad);

		if (gtk_widget_get_window (pad_widget))
			gdk_window_freeze_updates (gtk_widget_get_window (pad_widget));
		gtk_widget_show (pad->priv->toolbar);
		if (!pad->priv->toolbar_height)
		{
			gtk_widget_get_preferred_size (pad->priv->toolbar, &req, NULL);
			/* safe cast from gint to guint */
			if (req.height >= 0) {
				pad->priv->toolbar_height = (guint) req.height;
			}
			else {
				g_warning ("There is a problem in the program Xpad. In function 'xpad_pad_show_toolbar' the variable 'req.height' is not a positive number. Please send a bugreport to https://bugs.launchpad.net/xpad/+filebug to help improve Xpad.");
				pad->priv->toolbar_height = 0;
			}
		}

		/* Do we have room for the toolbar without covering text? */
		if (xpad_pad_text_and_toolbar_height (pad) > pad->priv->height)
		{
			pad->priv->toolbar_expanded = TRUE;
			pad->priv->height += pad->priv->toolbar_height;
			gtk_window_resize (GTK_WINDOW (pad), (gint) pad->priv->width, (gint) pad->priv->height);
		}
		else
			pad->priv->toolbar_expanded = FALSE;

		pad->priv->toolbar_pad_resized = FALSE;

		if (gtk_widget_get_window (pad_widget))
			gdk_window_thaw_updates (gtk_widget_get_window (pad_widget));
	}
}

static void
xpad_pad_hide_toolbar (XpadPad *pad)
{
	if (gtk_widget_get_visible (pad->priv->toolbar))
	{
		GtkWidget *pad_widget = GTK_WIDGET (pad);
		if (gtk_widget_get_window (pad_widget))
			gdk_window_freeze_updates (gtk_widget_get_window (pad_widget));
		gtk_widget_hide (pad->priv->toolbar);

		if (pad->priv->toolbar_expanded ||
			 (pad->priv->toolbar_pad_resized && xpad_pad_text_and_toolbar_height (pad) >= pad->priv->height))
		{
				pad->priv->height -= pad->priv->toolbar_height;
				gtk_window_resize (GTK_WINDOW (pad), (gint) pad->priv->width, (gint) pad->priv->height);
				pad->priv->toolbar_expanded = FALSE;
		}
		if (gtk_widget_get_window (pad_widget))
			gdk_window_thaw_updates (gtk_widget_get_window (pad_widget));
	}
}

void
xpad_pad_notify_has_selection (XpadPad *pad)
{
	g_return_if_fail (pad);

	GtkSourceBuffer *buffer;
	buffer = GTK_SOURCE_BUFFER (gtk_text_view_get_buffer (GTK_TEXT_VIEW (pad->priv->textview)));
	gboolean has_selection = gtk_text_buffer_get_has_selection (GTK_TEXT_BUFFER (buffer));

	XpadToolbar *toolbar = XPAD_TOOLBAR (pad->priv->toolbar);
	if (toolbar == NULL)
		return;

	xpad_toolbar_enable_cut_button (toolbar, has_selection);
	xpad_toolbar_enable_copy_button (toolbar, has_selection);
}

void
xpad_pad_notify_clipboard_owner_changed (XpadPad *pad)
{
	g_return_if_fail (pad);

	/* safe cast to toolbar */
	if (XPAD_IS_TOOLBAR (pad->priv->toolbar)) {
		XpadToolbar *toolbar = XPAD_TOOLBAR (pad->priv->toolbar);
		g_return_if_fail (toolbar);

		GtkClipboard *clipboard = gtk_clipboard_get (GDK_SELECTION_CLIPBOARD);
		xpad_toolbar_enable_paste_button (toolbar, gtk_clipboard_wait_is_text_available (clipboard));
	}
}

void
xpad_pad_notify_undo_redo_changed (XpadPad *pad)
{
	g_return_if_fail (pad);

	XpadTextBuffer *buffer = NULL;
	buffer = XPAD_TEXT_BUFFER (gtk_text_view_get_buffer (GTK_TEXT_VIEW (pad->priv->textview)));
	g_return_if_fail (buffer);

	XpadToolbar *toolbar = NULL;
	toolbar = XPAD_TOOLBAR (pad->priv->toolbar);
	g_return_if_fail (toolbar);

	xpad_toolbar_enable_undo_button (toolbar, xpad_text_buffer_undo_available (buffer));
	xpad_toolbar_enable_redo_button (toolbar, xpad_text_buffer_redo_available (buffer));
}

static gboolean
xpad_pad_enter_notify_event (GtkWidget *pad, GdkEventCrossing *event)
{
	gboolean has_toolbar, autohide_toolbar;
	g_object_get (XPAD_PAD (pad)->priv->settings, "has-toolbar", &has_toolbar, "autohide-toolbar", &autohide_toolbar, NULL);

	if (has_toolbar && autohide_toolbar &&
		 event->detail != GDK_NOTIFY_INFERIOR &&
		 event->mode == GDK_CROSSING_NORMAL)
	{
		XPAD_PAD (pad)->priv->toolbar_timeout = 0;
		xpad_pad_show_toolbar (XPAD_PAD (pad));
	}

	return FALSE;
}

static gboolean
xpad_pad_leave_notify_event (GtkWidget *pad, GdkEventCrossing *event)
{
	gboolean has_toolbar, autohide_toolbar;
	g_object_get (XPAD_PAD (pad)->priv->settings, "has-toolbar", &has_toolbar, "autohide-toolbar", &autohide_toolbar, NULL);

	if (has_toolbar && autohide_toolbar &&
		 event->detail != GDK_NOTIFY_INFERIOR &&
		 event->mode == GDK_CROSSING_NORMAL)
	{
		if (!XPAD_PAD (pad)->priv->toolbar_timeout)
			XPAD_PAD (pad)->priv->toolbar_timeout = g_timeout_add (1000, (GSourceFunc) toolbar_timeout, pad);
	}

	return FALSE;
}

static void
xpad_pad_spawn (XpadPad *pad)
{
	GtkWidget *newpad = xpad_pad_new (pad->priv->group, pad->priv->settings);
	gtk_widget_show (newpad);
}

static void
xpad_pad_clear (XpadPad *pad)
{
	GtkTextBuffer *buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (pad->priv->textview));

	GtkTextIter start, end;
	gtk_text_buffer_get_bounds (GTK_TEXT_BUFFER (buffer), &start, &end);

	gtk_text_buffer_begin_user_action (buffer);
	gtk_text_buffer_delete (buffer, &start, &end);
	gtk_text_buffer_end_user_action (buffer);
}

void
xpad_pad_close (XpadPad *pad)
{
	gtk_widget_hide (GTK_WIDGET (pad));

	/*
	 * If no tray and this is the last pad, we don't want to record this
	 * pad as closed, we want to start with just this pad next open.  So
	 * quit before we record.
	 */
	if (!xpad_tray_has_indicator () &&
		 xpad_pad_group_num_visible_pads (pad->priv->group) == 0)
	{
		xpad_app_quit ();
		return;
	}

	if (pad->priv->properties)
		gtk_widget_destroy (pad->priv->properties);

	xpad_pad_save_info_delayed (pad);

	g_signal_emit (pad, signals[CLOSED], 0);
}

void
xpad_pad_search (XpadPad *pad)
{
	xpad_search_bar_show (pad->priv->searchbar);
}

void
xpad_pad_toggle(XpadPad *pad)
{
	 if (gtk_widget_get_visible (GTK_WIDGET(pad)))
		xpad_pad_close (pad);
	 else
		gtk_widget_show (GTK_WIDGET (pad));
}

static gboolean
should_confirm_delete (XpadPad *pad)
{
	GtkSourceBuffer *buffer;
	GtkTextIter s, e;
	gchar *content;
	gboolean confirm;

	g_object_get (pad->priv->settings, "confirm-destroy", &confirm, NULL);
	if (!confirm)
		return FALSE;

	buffer = GTK_SOURCE_BUFFER (gtk_text_view_get_buffer (GTK_TEXT_VIEW (pad->priv->textview)));
	gtk_text_buffer_get_bounds (GTK_TEXT_BUFFER (buffer), &s, &e);
	content = gtk_text_buffer_get_text (GTK_TEXT_BUFFER (buffer), &s, &e, FALSE);

	confirm = strcmp (g_strstrip (content), "") != 0;

	g_free (content);

	return confirm;
}

static void
xpad_pad_delete (XpadPad *pad)
{
	g_return_if_fail (pad);

	/* With the delayed saving functionality, it is necessary to clear the unsaved flags to prevent usage of non-existing object information. */
	pad->priv->unsaved_info = FALSE;
	pad->priv->unsaved_content = FALSE;

	if (should_confirm_delete (pad))
	{
		GtkWidget *dialog;
		gint response;

		dialog = xpad_app_alert_dialog (GTK_WINDOW (pad), "dialog-warning", _("Delete this pad?"), _("All text of this pad will be irrevocably lost."));

		if (!dialog)
			return;

		gtk_dialog_add_buttons (GTK_DIALOG (dialog), _("_Delete"), GTK_RESPONSE_ACCEPT, _("_Cancel"), GTK_RESPONSE_REJECT, NULL);
		gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_REJECT);

		response = gtk_dialog_run (GTK_DIALOG (dialog));

		gtk_widget_destroy (dialog);

		if (response != GTK_RESPONSE_ACCEPT)
			return;
	}

	/* These two if statements actually erase the pad on the harddisk. */
	if (pad->priv->infoname)
		fio_remove_file (pad->priv->infoname);
	if (pad->priv->contentname)
		fio_remove_file (pad->priv->contentname);

	/* Remove the pad from the group and destroy it. */
	gtk_widget_destroy (GTK_WIDGET (pad));
}

static void
pad_properties_sync_title (XpadPad *pad)
{
	gchar *title;

	if (!pad->priv->properties)
		return;

	title = g_strdup_printf (_("'%s' Layout"), gtk_window_get_title (GTK_WINDOW (pad)));
	gtk_window_set_title (GTK_WINDOW (pad->priv->properties), title);
	g_free (title);
}

static void
pad_properties_destroyed (XpadPad *pad)
{
	if (!pad->priv->properties)
		return;

	g_signal_handlers_disconnect_by_func (pad, (gpointer) pad_properties_sync_title, NULL);
	pad->priv->properties = NULL;
}

static void
prop_notify_font (XpadPad *pad)
{
	XpadPadProperties *prop = XPAD_PAD_PROPERTIES (pad->priv->properties);

	gboolean follow_font_style;
	g_object_get (prop, "follow-font-style", &follow_font_style, NULL);
	g_object_set (XPAD_TEXT_VIEW (pad->priv->textview), "follow-font-style", follow_font_style, NULL);

	if (!follow_font_style)
	{
		gchar *fontname;
		g_object_get (prop, "fontname", &fontname, NULL);

		PangoFontDescription *fontdesc;
		fontdesc = fontname ? pango_font_description_from_string (fontname) : NULL;
		g_free(fontname);

		xpad_text_view_set_font (pad->priv->textview, fontdesc);
		if (fontdesc)
			pango_font_description_free (fontdesc);
	}

	xpad_pad_save_info_delayed (pad);
}

static void
prop_notify_colors (XpadPad *pad)
{
	XpadPadProperties *prop = XPAD_PAD_PROPERTIES (pad->priv->properties);

	gboolean follow_color_style;
	GdkRGBA *text_color, *back_color;

	g_object_get (prop, "follow-color-style", &follow_color_style, NULL);
	g_object_set (XPAD_TEXT_VIEW (pad->priv->textview), "follow-color-style", follow_color_style, NULL);

	if (follow_color_style) {
		/* Set the colors to the global preferences colors */
		g_object_get (pad->priv->settings, "text-color", &text_color, "back-color", &back_color, NULL);
	} else {
		/* Set the color to the individual pad properties colors */
		g_object_get (prop, "text-color", &text_color, "back-color", &back_color, NULL);
	}

	xpad_text_view_set_colors (pad->priv->textview, text_color, back_color);
	gdk_rgba_free (text_color);
	gdk_rgba_free (back_color);

	xpad_pad_save_info_delayed (pad);
}

static void
xpad_pad_open_properties (XpadPad *pad)
{
	gboolean follow_font_style, follow_color_style;
	GtkStyleContext *style = NULL;
	PangoFontDescription *font;
	GdkRGBA widget_text_color = {0, 0, 0, 0};
	GdkRGBA widget_background_color = {0, 0, 0, 0};

	if (pad->priv->properties)
	{
		gtk_window_present (GTK_WINDOW (pad->priv->properties));
		return;
	}

	pad->priv->properties = xpad_pad_properties_new ();

	gtk_window_set_transient_for (GTK_WINDOW (pad->priv->properties), GTK_WINDOW (pad));
	gtk_window_set_resizable (GTK_WINDOW (pad->priv->properties), FALSE);

	g_signal_connect_swapped (pad->priv->properties, "destroy", G_CALLBACK (pad_properties_destroyed), pad);
	g_signal_connect (pad, "notify::title", G_CALLBACK (pad_properties_sync_title), NULL);

	style = gtk_widget_get_style_context (pad->priv->textview);
	gtk_style_context_get(style, GTK_STATE_FLAG_NORMAL, GTK_STYLE_PROPERTY_FONT, &font, NULL);
	gtk_style_context_get_color (style, GTK_STATE_FLAG_NORMAL, &widget_text_color);
	get_background_color (style, GTK_STATE_FLAG_NORMAL, &widget_background_color);

	g_object_get (XPAD_TEXT_VIEW (pad->priv->textview), "follow-font-style", &follow_font_style, "follow-color-style", &follow_color_style, NULL);
	g_object_set (G_OBJECT (pad->priv->properties),
		"follow-font-style", follow_font_style,
		"follow-color-style", follow_color_style,
		"text-color", &widget_text_color,
		"back-color", &widget_background_color,
		"fontname", pango_font_description_to_string(font),
		NULL);
	pango_font_description_free (font);

	g_signal_connect_swapped (pad->priv->properties, "notify::follow-font-style", G_CALLBACK (prop_notify_font), pad);
	g_signal_connect_swapped (pad->priv->properties, "notify::follow-color-style", G_CALLBACK (prop_notify_colors), pad);
	g_signal_connect_swapped (pad->priv->properties, "notify::text-color", G_CALLBACK (prop_notify_colors), pad);
	g_signal_connect_swapped (pad->priv->properties, "notify::back-color", G_CALLBACK (prop_notify_colors), pad);
	g_signal_connect_swapped (pad->priv->properties, "notify::fontname", G_CALLBACK (prop_notify_font), pad);

	pad_properties_sync_title (pad);

	gtk_widget_show (pad->priv->properties);
}

static void
xpad_pad_open_preferences (XpadPad *pad)
{
	xpad_preferences_open (pad->priv->settings);
}

static void
xpad_pad_text_changed (XpadPad *pad, GtkSourceBuffer *buffer)
{
	/* A dirty way to silence the compiler for these unused variables. */
	(void) buffer;

	/* set title */
	xpad_pad_sync_title (pad);

	/* record change */
	xpad_pad_save_content_delayed(pad);
}

static gboolean
xpad_pad_toolbar_size_allocate (XpadPad *pad, GtkAllocation *event)
{
	/* safe cast from gint to guint */
	if (event->height >= 0) {
		pad->priv->toolbar_height = (guint) event->height;
	} else {
		g_warning("There is a problem in the program Xpad. In function 'xpad_pad_toolbar_size_allocate' the variable 'event->height' is not a positive number. Please send a bugreport to https://bugs.launchpad.net/xpad/+filebug to help improve Xpad.");
		pad->priv->toolbar_height = 0;
	}

	return FALSE;
}

static gboolean
xpad_pad_configure_event (XpadPad *pad, GdkEventConfigure *event)
{
	if (!gtk_widget_get_visible (GTK_WIDGET (pad)))
		return FALSE;

	int eWidth = event->width;
	int eHeight = event->height;

	/* safe cast from gint to guint */
	if (eWidth >= 0 && eHeight >=0 ) {
		/* If the width or height has changed, save it. */
		if (pad->priv->width != (guint) eWidth || pad->priv->height != (guint) eHeight) {
			pad->priv->toolbar_pad_resized = TRUE;
			pad->priv->width = (guint) eWidth;
			pad->priv->height = (guint) eHeight;
			xpad_pad_save_info_delayed(pad);
		}
	}

	/* If the location of the pad has changed, save it. */
	if (pad->priv->x != event->x || pad->priv->y != event->y) {
		pad->priv->x = event->x;
		pad->priv->y = event->y;
		pad->priv->location_valid = TRUE;
		xpad_pad_save_info_delayed(pad);
	}

	/*
	 * Sometimes when moving, if the toolbar tries to hide itself,
	 * the window manager will not resize it correctly.  So, we make
	 * sure not to end the timeout while moving.
	 */
	if (pad->priv->toolbar_timeout) {
		g_source_remove (pad->priv->toolbar_timeout);
		pad->priv->toolbar_timeout = g_timeout_add (1000, (GSourceFunc) toolbar_timeout, pad);
	}

	return FALSE;
}

static gboolean
xpad_pad_delete_event (XpadPad *pad, GdkEvent *event)
{
	/* A dirty way to silence the compiler for these unused variables. */
	(void) event;

	xpad_pad_close (pad);

	return TRUE;
}

static gboolean
xpad_pad_popup_menu (XpadPad *pad)
{
	xpad_pad_popup (pad, NULL);

	return TRUE;
}

static gboolean
xpad_pad_button_press_event (XpadPad *pad, GdkEventButton *event)
{
	if (event->type == GDK_BUTTON_PRESS)
	{
		switch (event->button)
		{
		case 1:
			if ((event->state & gtk_accelerator_get_default_mod_mask ()) == GDK_CONTROL_MASK)
			{
				gtk_window_begin_move_drag (GTK_WINDOW (pad), (gint) event->button, (gint) event->x_root, (gint) event->y_root, event->time);
				return TRUE;
			}
			break;

		case 3:
			if ((event->state & gtk_accelerator_get_default_mod_mask ()) == GDK_CONTROL_MASK)
			{
				GdkWindowEdge edge;

				if (gtk_widget_get_direction (GTK_WIDGET (pad)) == GTK_TEXT_DIR_LTR)
					edge = GDK_WINDOW_EDGE_SOUTH_EAST;
				else
					edge = GDK_WINDOW_EDGE_SOUTH_WEST;

				gtk_window_begin_resize_drag (GTK_WINDOW (pad), edge, (gint) event->button, (gint) event->x_root, (gint) event->y_root, event->time);
			}
			else
			{
				xpad_pad_popup (pad, event);
			}
			return TRUE;
		}
	}

	return FALSE;
}

static void
xpad_pad_sync_title (XpadPad *pad)
{
	GtkSourceBuffer *buffer;
	GtkTextIter s, e;
	gchar *content, *end;

	buffer = GTK_SOURCE_BUFFER (gtk_text_view_get_buffer (GTK_TEXT_VIEW (pad->priv->textview)));
	gtk_text_buffer_get_bounds (GTK_TEXT_BUFFER (buffer), &s, &e);
	content = gtk_text_buffer_get_text (GTK_TEXT_BUFFER (buffer), &s, &e, FALSE);
	end = g_utf8_strchr (content, -1, '\n');
	if (end)
		*end = '\0';

	gtk_window_set_title (GTK_WINDOW (pad), g_strstrip (content));

	g_free (content);
}

void
xpad_pad_load_content (XpadPad *pad)
{
	g_return_if_fail (pad);
	g_return_if_fail (pad->priv->contentname);

	gchar *content;
	GtkSourceBuffer *buffer;

	content = fio_get_file (pad->priv->contentname, CONFIG_DIR);

	buffer = GTK_SOURCE_BUFFER (gtk_text_view_get_buffer (GTK_TEXT_VIEW (pad->priv->textview)));

	xpad_text_buffer_freeze_undo (XPAD_TEXT_BUFFER (buffer));

	g_signal_handlers_block_by_func (buffer, xpad_pad_text_changed, pad);
	xpad_text_buffer_set_text_with_tags (XPAD_TEXT_BUFFER (buffer), content ? content : "");
	g_signal_handlers_unblock_by_func (buffer, xpad_pad_text_changed, pad);

	g_free (content);
	xpad_text_buffer_thaw_undo (XPAD_TEXT_BUFFER (buffer));

	xpad_pad_sync_title (pad);
	pad->priv->unsaved_content = FALSE;
}

void
xpad_pad_save_content (XpadPad *pad)
{
	g_return_if_fail (pad);

	gchar *content = NULL;
	XpadTextBuffer *buffer;

	if (!pad->priv->unsaved_content) {
		return;
	}

	/* create content file if it doesn't exist yet */
	if (!pad->priv->contentname)
	{
		pad->priv->contentname = fio_unique_name ("content-");
		if (!pad->priv->contentname)
			return;
	}

	if (GTK_IS_TEXT_VIEW(GTK_TEXT_VIEW (pad->priv->textview))) {
		buffer = XPAD_TEXT_BUFFER (gtk_text_view_get_buffer (GTK_TEXT_VIEW (pad->priv->textview)));
		content = xpad_text_buffer_get_text_with_tags (buffer);
	} else {
		g_warning("There is a problem in the program Xpad. In function 'xpad_pad_save_content' the variable 'pad->priv->textview' is not of type textview. Please send a bugreport to https://bugs.launchpad.net/xpad/+filebug to help improve Xpad.");
	}

	fio_set_file (pad->priv->contentname, content);

	pad->priv->unsaved_content = FALSE;
	g_free (content);
}

/* Extract all the metadata of a single pad from its info-xxxxx file and store it in the pad object */
static void
xpad_pad_load_info (XpadPad *pad, gboolean *show)
{
	gboolean locked = FALSE, follow_font = TRUE, follow_color = TRUE, hidden = FALSE;
	gboolean has_toolbar, autohide_toolbar;
	gchar *fontname = NULL, *text_color_string = NULL, *background_color_string = NULL;
	GdkRGBA text_color = {0, 0, 0, 0}, back_color = {0, 0, 0, 0};

	if (!pad->priv->infoname)
		return;

	if (fio_get_values_from_file (pad->priv->infoname,
		"i|width", &pad->priv->width,
		"i|height", &pad->priv->height,
		"i|x", &pad->priv->x,
		"i|y", &pad->priv->y,
		"b|locked", &locked,
		"b|follow_font", &follow_font,
		"b|follow_color", &follow_color,
		"b|sticky", &pad->priv->sticky,
		"b|hidden", &hidden,
		"s|back", &background_color_string,
		"s|text", &text_color_string,
		"s|fontname", &fontname,
		"s|content", &pad->priv->contentname,
		NULL))
		return;

	pad->priv->unsaved_info = FALSE;
	pad->priv->location_valid = TRUE;

	g_object_get (pad->priv->settings, "has-toolbar", &has_toolbar, "autohide-toolbar", &autohide_toolbar, NULL);

	if (has_toolbar && !autohide_toolbar)
	{
		pad->priv->toolbar_height = 0;
		xpad_pad_hide_toolbar (pad);
		xpad_pad_show_toolbar (pad); /* these will resize pad at correct height */
	}
	else
		gtk_window_resize (GTK_WINDOW (pad), (gint) pad->priv->width, (gint) pad->priv->height);
	gtk_window_move (GTK_WINDOW (pad), pad->priv->x, pad->priv->y);

	g_object_set (XPAD_TEXT_VIEW (pad->priv->textview), "follow-font-style", follow_font, "follow-color-style", follow_color, NULL);

	if (locked) {
		g_object_set (XPAD_TEXT_VIEW (pad->priv->textview), "follow-font-style", FALSE, "follow-color-style", FALSE, NULL);
	}

	if (!follow_font)
	{
		PangoFontDescription *font_desc = pango_font_description_from_string (fontname);
		xpad_text_view_set_font(pad->priv->textview, font_desc);
		pango_font_description_free (font_desc);
	}

	if (!follow_color)
	{
		/*
		 * If, for some reason, one of the colors could not be retrieved
		 * (for example due to the migration to the new GdkRGBA colors),
		 * set the color to the default.
		 */
		if (text_color_string == NULL || background_color_string == NULL) {
			text_color = (GdkRGBA) {0, 0, 0, 1};
			back_color = (GdkRGBA) {1, 0.933334350586, 0.6, 1};
		}
		else {
			/* If, for some reason, the parsing of the colors fail, set the color to the default. */
			if (!gdk_rgba_parse (&text_color, text_color_string) || !gdk_rgba_parse (&back_color, background_color_string)) {
				text_color = (GdkRGBA) {0, 0, 0, 1};
				back_color = (GdkRGBA) {1, 0.933334350586, 0.6, 1};
			}
		}

		/* Set the text and background color for this pad, as stated in its properties file. */
		xpad_text_view_set_colors (pad->priv->textview, &text_color, &back_color);
	}

	xpad_pad_stick_unstick (pad);

	if (show)
		*show = !hidden;

	g_free(text_color_string);
	g_free(background_color_string);
	g_free(fontname);
}

void
xpad_pad_save_info (XpadPad *pad)
{
	gboolean follow_font_style, follow_color_style;
	guint height = 0;
	GtkStyleContext *style = NULL;
	PangoFontDescription *font = NULL;
	GdkRGBA text_color = {0, 0, 0, 0}, back_color = {0, 0, 0, 0};

	g_return_if_fail (pad);

	if (!pad->priv->unsaved_info)
		return;

	/* Must create pad info file if it doesn't exist yet */
	if (!pad->priv->infoname)
	{
		pad->priv->infoname = fio_unique_name ("info-");
		if (!pad->priv->infoname)
			return;
		gtk_window_set_role (GTK_WINDOW (pad), pad->priv->infoname);
	}

	/* create content file if it doesn't exist yet */
	if (!pad->priv->contentname)
	{
		pad->priv->contentname = fio_unique_name ("content-");
		if (!pad->priv->contentname)
			return;
	}

	height = pad->priv->height;
	if (gtk_widget_get_visible (pad->priv->toolbar) && pad->priv->toolbar_expanded)
		height -= pad->priv->toolbar_height;

	style = gtk_widget_get_style_context (pad->priv->textview);

	gtk_style_context_get (style, GTK_STATE_FLAG_NORMAL, GTK_STYLE_PROPERTY_FONT, &font, NULL);
	gchar *font_string = pango_font_description_to_string (font);
	pango_font_description_free (font);

	gtk_style_context_get_color (style, GTK_STATE_FLAG_NORMAL, &text_color);
	gchar *text_color_string = gdk_rgba_to_string (&text_color);

	get_background_color (style, GTK_STATE_FLAG_NORMAL, &back_color);
	gchar *back_color_string = gdk_rgba_to_string (&back_color);

	g_object_get (XPAD_TEXT_VIEW (pad->priv->textview), "follow-font-style", &follow_font_style, "follow-color-style", &follow_color_style, NULL);

	fio_set_values_to_file (pad->priv->infoname,
		"i|width", pad->priv->width,
		"i|height", height,
		"i|x", pad->priv->x,
		"i|y", pad->priv->y,
		"b|follow_font", follow_font_style,
		"b|follow_color", follow_color_style,
		"b|sticky", pad->priv->sticky,
		"b|hidden", !gtk_widget_get_visible (GTK_WIDGET(pad)),
		"s|back", back_color_string,
		"s|text", text_color_string,
		"s|fontname", font_string,
		"s|content", pad->priv->contentname,
		NULL);

	g_free (text_color_string);
	g_free (back_color_string);
	g_free (font_string);
	pad->priv->unsaved_info = FALSE;
}

static void
menu_about (XpadPad *pad)
{
	const gchar *artists[] = {"Michael Terry <mike@mterry.name>", NULL};
	const gchar *authors[] = {"Arthur Borsboom <arthurborsboom@gmail.com>", "Jeroen Vermeulen <jtv@xs4all.nl>", "Michael Terry <mike@mterry.name>", "Paul Ivanov <pivanov@berkeley.edu>", "Sachin Raut <great.sachin@gmail.com>", NULL};
	const gchar *comments = _("Sticky notes");
	const gchar *copyright = "© 2001-2014 Michael Terry";
	/* Translators: please translate this as your own name and optionally email
		like so: "Your Name <your@email.com>" */
	const gchar *translator_credits = _("translator-credits");
	const gchar *website = "https://launchpad.net/xpad";

	gtk_show_about_dialog (GTK_WINDOW (pad),
		"artists", artists,
		"authors", authors,
		"comments", comments,
		"copyright", copyright,
		"license-type", GTK_LICENSE_GPL_3_0,
		"logo-icon-name", PACKAGE,
		"translator-credits", translator_credits,
		"version", VERSION,
		"website", website,
		NULL);
}

static void
xpad_pad_cut (XpadPad *pad)
{
	gtk_text_buffer_cut_clipboard (
		gtk_text_view_get_buffer (GTK_TEXT_VIEW (pad->priv->textview)),
		gtk_clipboard_get (GDK_SELECTION_CLIPBOARD),
		TRUE);
}

static void
xpad_pad_copy (XpadPad *pad)
{
	gtk_text_buffer_copy_clipboard (
		gtk_text_view_get_buffer (GTK_TEXT_VIEW (pad->priv->textview)),
		gtk_clipboard_get (GDK_SELECTION_CLIPBOARD));
}

static void
xpad_pad_paste (XpadPad *pad)
{
	gtk_text_buffer_paste_clipboard (
		gtk_text_view_get_buffer (GTK_TEXT_VIEW (pad->priv->textview)),
		gtk_clipboard_get (GDK_SELECTION_CLIPBOARD),
		NULL,
		TRUE);
}

static void
xpad_pad_undo (XpadPad *pad)
{
	g_return_if_fail (pad->priv->textview);
	XpadTextBuffer *buffer = NULL;
	buffer = XPAD_TEXT_BUFFER (gtk_text_view_get_buffer (GTK_TEXT_VIEW (pad->priv->textview)));
	g_return_if_fail (buffer);
	xpad_text_buffer_undo (buffer);
}

static void
xpad_pad_redo (XpadPad *pad)
{
	g_return_if_fail (pad->priv->textview);
	XpadTextBuffer *buffer = NULL;
	buffer = XPAD_TEXT_BUFFER (gtk_text_view_get_buffer (GTK_TEXT_VIEW (pad->priv->textview)));
	g_return_if_fail (buffer);
	xpad_text_buffer_redo (buffer);
}

static void
xpad_pad_show_all (XpadPad *pad)
{
	xpad_pad_group_show_all (pad->priv->group);
}

static void
xpad_pad_close_all (XpadPad *pad)
{
	if (!pad->priv->group)
		return;

	/*
	 * The logic is different here depending on whether the tray is open.
	 * If it is open, we just close each pad individually.  If it isn't
	 * open, we do a quit.  This way, when xpad is run again, only the
	 * pads open during the last 'close all' will open again.
	 */
	if (xpad_tray_has_indicator ())
		xpad_pad_group_close_all (pad->priv->group);
	else
		xpad_app_quit ();
}

static void
menu_toggle_tag (XpadPad *pad, const gchar *name)
{
	g_return_if_fail (pad->priv->textview);
	XpadTextBuffer *buffer = NULL;
	buffer = XPAD_TEXT_BUFFER (gtk_text_view_get_buffer (GTK_TEXT_VIEW (pad->priv->textview)));
	xpad_text_buffer_toggle_tag (buffer, name);
	xpad_pad_save_content_delayed(pad);
}

static void
menu_bold (XpadPad *pad)
{
	menu_toggle_tag (pad, "bold");
}

static void
menu_italic (XpadPad *pad)
{
	menu_toggle_tag (pad, "italic");
}

static void
menu_underline (XpadPad *pad)
{
	menu_toggle_tag (pad, "underline");
}

static void
menu_strikethrough (XpadPad *pad)
{
	menu_toggle_tag (pad, "strikethrough");
}

/* FIXME: Accelerators are working but not visible for menu items with an image (icon). */
#define MENU_ADD(mnemonic, image, key, mask, callback) {\
	if (image) {\
		item = gtk_menu_item_new ();\
		GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 3);\
		gtk_container_add (GTK_CONTAINER (hbox), gtk_image_new_from_icon_name (image, GTK_ICON_SIZE_MENU));\
		gtk_container_add (GTK_CONTAINER (hbox), gtk_label_new_with_mnemonic (mnemonic));\
		gtk_container_add (GTK_CONTAINER (item), hbox);\
	}\
	else\
		item = gtk_menu_item_new_with_mnemonic (mnemonic);\
	g_signal_connect_swapped (item, "activate", G_CALLBACK (callback), pad);\
	if (key)\
		gtk_widget_add_accelerator (item, "activate", accel_group, key, mask, GTK_ACCEL_VISIBLE);\
	gtk_container_add (GTK_CONTAINER (menu), item);\
	}

#define MENU_ADD_CHECK(mnemonic, active, callback) {\
	item = gtk_check_menu_item_new_with_mnemonic (mnemonic);\
	gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (item), active);\
	g_signal_connect (item, "toggled", G_CALLBACK (callback), pad);\
	gtk_container_add (GTK_CONTAINER (menu), item);\
	}

#define MENU_ADD_SEP() {\
	item = gtk_separator_menu_item_new ();\
	gtk_container_add (GTK_CONTAINER (menu), item);\
	}

static GtkWidget *
menu_get_popup_no_highlight (XpadPad *pad, GtkAccelGroup *accel_group)
{
	GtkWidget *uppermenu, *menu, *item;

	/* Upper menu */
	uppermenu = gtk_menu_new ();
	gtk_menu_set_accel_group (GTK_MENU (uppermenu), accel_group);
	menu = uppermenu;
	MENU_ADD (_("_New"), "document-new", GDK_KEY_N, GDK_CONTROL_MASK, xpad_pad_spawn);
	MENU_ADD (_("_Delete"), "edit-delete", GDK_KEY_Delete, GDK_SHIFT_MASK, xpad_pad_delete);
	MENU_ADD (_("_Reload"), "reload-pad-content", GDK_KEY_F5, 0, xpad_pad_load_content);
	MENU_ADD (_("_Close"), "window-close", GDK_KEY_W, GDK_CONTROL_MASK, xpad_pad_close);

	/* Edit submenu */
	item = gtk_menu_item_new_with_mnemonic (_("_Edit"));
	gtk_container_add (GTK_CONTAINER (uppermenu), item);
	menu = gtk_menu_new ();
	gtk_menu_item_set_submenu (GTK_MENU_ITEM (item), menu);
	MENU_ADD (_("_Undo"), "edit-undo", GDK_KEY_Z, GDK_CONTROL_MASK, xpad_pad_undo);
	g_object_set_data (G_OBJECT (uppermenu), "undo", item);
	MENU_ADD (_("_Redo"), "edit-redo", GDK_KEY_Y, GDK_CONTROL_MASK, xpad_pad_redo);
	g_object_set_data (G_OBJECT (uppermenu), "redo", item);
	MENU_ADD_SEP();
	MENU_ADD (_("_Paste"), "edit-paste", 0, 0, xpad_pad_paste);
	g_object_set_data (G_OBJECT (uppermenu), "paste", item);
	MENU_ADD_SEP();
	MENU_ADD (_("_Find"), "edit-find", GDK_KEY_F, GDK_CONTROL_MASK, xpad_pad_search);
	MENU_ADD_SEP();
	MENU_ADD (_("_Layout"), "document-properties", 0, 0, xpad_pad_open_properties);
	MENU_ADD (_("_Read only"), "editable", 0, 0, xpad_pad_open_properties);

	menu = uppermenu;
	MENU_ADD_SEP();

	/* Notes submenu - The list of notes will get added in the prep function below */
	item = gtk_menu_item_new_with_mnemonic (_("_Notes"));
	gtk_container_add (GTK_CONTAINER (uppermenu), item);
	menu = gtk_menu_new ();
	gtk_menu_item_set_submenu (GTK_MENU_ITEM (item), menu);
	g_object_set_data (G_OBJECT (uppermenu), "notes-menu", menu);
	MENU_ADD (_("_Show All"), NULL, 0, 0, xpad_pad_show_all);
	MENU_ADD (_("_Close All"), NULL, 0, 0, xpad_pad_close_all);

	/* Help submenu */
	item = gtk_menu_item_new_with_mnemonic (_("_Help"));
	gtk_container_add (GTK_CONTAINER (uppermenu), item);
	menu = gtk_menu_new ();
	gtk_menu_item_set_submenu (GTK_MENU_ITEM (item), menu);
	MENU_ADD (_("_Help"), "help-browser", GDK_KEY_F1, 0, show_help);
	MENU_ADD (_("_About"), "help-about", 0, 0, menu_about);

	/* Upper menu */
	menu = uppermenu;
	MENU_ADD_SEP ();
	MENU_ADD (_("_Preferences"), "preferences-system", 0, 0, xpad_pad_open_preferences);

	gtk_widget_show_all (uppermenu);

	return uppermenu;
}

static void
menu_prep_popup_no_highlight (XpadPad *pad, GtkWidget *uppermenu)
{
	GtkWidget *menu, *item;

	GtkClipboard *clipboard = gtk_clipboard_get (GDK_SELECTION_CLIPBOARD);

	XpadTextBuffer *buffer = XPAD_TEXT_BUFFER (gtk_text_view_get_buffer (GTK_TEXT_VIEW (pad->priv->textview)));

	item = g_object_get_data (G_OBJECT (uppermenu), "paste");
	if (item)
		gtk_widget_set_sensitive (item, gtk_clipboard_wait_is_text_available (clipboard));

	item = g_object_get_data (G_OBJECT (uppermenu), "undo");
	if (item)
		gtk_widget_set_sensitive (item, xpad_text_buffer_undo_available (buffer));

	item = g_object_get_data (G_OBJECT (uppermenu), "redo");
	if (item)
		gtk_widget_set_sensitive (item, xpad_text_buffer_redo_available (buffer));

	menu = g_object_get_data (G_OBJECT (uppermenu), "notes-menu");
	if (menu)
	{
		gint n = 1;

		/* Remove old notes */
		item = g_object_get_data (G_OBJECT (menu), "notes-sep");
		while (item)
		{
			gtk_container_remove (GTK_CONTAINER (menu), item);
			gchar *key = g_strdup_printf ("notes-%i", n++);
			item = g_object_get_data (G_OBJECT (menu), key);
			g_free (key);
		}

		MENU_ADD_SEP ();
		g_object_set_data (G_OBJECT (menu), "notes-sep", item);

		/* Add new notes */
		xpad_pad_append_pad_titles_to_menu (menu);
	}
	gtk_widget_show_all (menu);
}

void xpad_pad_append_pad_titles_to_menu (GtkWidget *menu)
{
	/* Get all pads sorted by title. */
	GSList *pads = xpad_pad_group_get_pads_sorted_by_title(xpad_app_get_pad_group ());

	/* Create a fingerprint of the pad titles for usage in the tray logic. */
	GString *pads_fingerprint = g_string_new(NULL);

	/* Add pads to the menu. */
	for (gint n = 1; pads; pads = pads->next, n++) {
		XpadPad *pad = pads->data;
		gchar *title = xpad_pad_get_title_for_menu(pad, n);
		g_string_append(pads_fingerprint, title);

		GtkWidget *item = gtk_menu_item_new_with_mnemonic (title);
		g_signal_connect_swapped (item, "activate", G_CALLBACK (gtk_window_present), pad);
		gtk_container_add (GTK_CONTAINER (menu), item);

		gchar *key = g_strdup_printf ("notes-%i", n);
		g_object_set_data (G_OBJECT (menu), key, item);
		g_free (key);

		g_free (title);
	}

	g_slist_free (pads);

	/* Add the fingerprint to the menu. */
	gchar *fingerprint = g_string_free (pads_fingerprint, FALSE);
	g_object_set_data (G_OBJECT (menu), "pads-fingerprint", fingerprint);
}

gchar* xpad_pad_get_title_for_menu(XpadPad *pad, gint pad_number) {
	gchar *tmp_title = g_strndup (gtk_window_get_title (GTK_WINDOW (pad)), 20);
	str_replace_tokens (&tmp_title, '_', "__");
	gchar *title;

	if (pad_number < 10) {
		title = g_strdup_printf ("_%i. %s", pad_number, tmp_title);
	} else {
		title = g_strdup_printf ("%i. %s", pad_number, tmp_title);
	}

	g_free (tmp_title);
	return title;
}

static GtkWidget *
menu_get_popup_highlight (XpadPad *pad, GtkAccelGroup *accel_group)
{
	GtkWidget *menu, *item;

	menu = gtk_menu_new ();
	gtk_menu_set_accel_group (GTK_MENU (menu), accel_group);

	MENU_ADD (_("Cu_t"), "edit-cut", 0, 0, xpad_pad_cut);
	MENU_ADD (_("_Copy"), "edit-copy", 0, 0, xpad_pad_copy);
	MENU_ADD (_("_Paste"), "edit-paste", 0, 0, xpad_pad_paste);
	g_object_set_data (G_OBJECT (menu), "paste", item);
	MENU_ADD_SEP ();
	MENU_ADD (_("_Bold"), "format-text-bold", GDK_KEY_b, GDK_CONTROL_MASK, menu_bold);
	MENU_ADD (_("_Italic"), "format-text-italic", GDK_KEY_i, GDK_CONTROL_MASK, menu_italic);
	MENU_ADD (_("_Underline"), "format-text-underline", GDK_KEY_u, GDK_CONTROL_MASK, menu_underline);
	MENU_ADD (_("_Strikethrough"), "format-text-strikethrough", 0, 0, menu_strikethrough);

	gtk_widget_show_all (menu);

	return menu;
}

static void
menu_prep_popup_highlight (XpadPad *pad, GtkWidget *menu)
{
	/* A dirty way to silence the compiler for these unused variables. */
	(void) pad;

	GtkWidget *item;
	GtkClipboard *clipboard;

	clipboard = gtk_clipboard_get (GDK_SELECTION_CLIPBOARD);

	item = g_object_get_data (G_OBJECT (menu), "paste");
	if (item)
		gtk_widget_set_sensitive (item, gtk_clipboard_wait_is_text_available (clipboard));
}

static void
menu_popup (XpadPad *pad)
{
	g_signal_handlers_block_matched (pad, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) xpad_pad_leave_notify_event, NULL);
	pad->priv->toolbar_timeout = 0;
}

static void
menu_popdown (XpadPad *pad)
{
	cairo_rectangle_int_t rect;

	/* We must check if we disabled off of pad and start the timeout if so. */
	rect.x = 10;
	rect.y = 10;
	rect.width = 1;
	rect.height = 1;

	if (!pad->priv->toolbar_timeout && !gtk_widget_intersect (GTK_WIDGET (pad), &rect, NULL))
		pad->priv->toolbar_timeout = g_timeout_add (1000, (GSourceFunc) toolbar_timeout, pad);

	g_signal_handlers_unblock_matched (pad, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) xpad_pad_leave_notify_event, NULL);
}

static void
xpad_pad_popup (XpadPad *pad, GdkEventButton *event)
{
	GtkSourceBuffer *buffer;
	GtkWidget *menu;

	buffer = GTK_SOURCE_BUFFER (gtk_text_view_get_buffer (GTK_TEXT_VIEW (pad->priv->textview)));

	if (gtk_text_buffer_get_selection_bounds (GTK_TEXT_BUFFER (buffer), NULL, NULL))
	{
		menu = pad->priv->highlight_menu;
		menu_prep_popup_highlight (pad, menu);
	}
	else
	{
		menu = pad->priv->menu;
		menu_prep_popup_no_highlight (pad, menu);
	}

	if (!menu)
		return;

	menu_popup (pad);

	if (event) {
		gtk_menu_popup_at_pointer (GTK_MENU (menu), (GdkEvent*) event);
	}
}

/* These functions below are used to reduce the amounts of writes, hence improve the performance. */
void xpad_pad_save_content_delayed (XpadPad *pad)
{
	pad->priv->unsaved_content = TRUE;
	xpad_periodic_save_content_delayed (pad);
}

void xpad_pad_save_info_delayed (XpadPad *pad)
{
	pad->priv->unsaved_info = TRUE;
	xpad_periodic_save_info_delayed (pad);
}

/* Save pad without delay, for example on application shutdown. */
void xpad_pad_save_unsaved (XpadPad *pad)
{
	if (pad->priv->unsaved_content)
		xpad_pad_save_content (pad);
	if (pad->priv->unsaved_info)
		xpad_pad_save_info (pad);
}

void xpad_pad_remove_accelerator_group (XpadPad *pad) {
	g_return_if_fail (pad);

	gtk_widget_add_events (GTK_WIDGET (pad), 0);

	if (pad->priv->toolbar) {
		gtk_widget_add_events (pad->priv->toolbar, 0);
	}

	if (pad->priv->accel_group) {
		gtk_window_remove_accel_group (GTK_WINDOW(pad), pad->priv->accel_group);
		g_clear_object (&pad->priv->accel_group);
	}
}
