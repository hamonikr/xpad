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

#include "xpad-toolbar.h"
#include "xpad-app.h"
#include "xpad-grip-tool-item.h"

struct XpadToolbarPrivate
{
	gboolean move_removed;
	guint move_index;
	guint move_motion_handler;
	guint move_button_release_handler;
	guint move_key_press_handler;
	XpadPad *pad;
};

G_DEFINE_TYPE_WITH_PRIVATE (XpadToolbar, xpad_toolbar, GTK_TYPE_TOOLBAR)

enum {
	XPAD_BUTTON_TYPE_SEPARATOR,
	XPAD_BUTTON_TYPE_BUTTON,
	XPAD_BUTTON_TYPE_TOGGLE
};

typedef struct
{
	const gchar *name;
	const gchar *icon_name;
	guint signal;
	guint type;
	const gchar *desc;
	const gchar *menu_desc;
} XpadToolbarButton;

enum
{
	ACTIVATE_NEW,
	ACTIVATE_CLOSE,
	ACTIVATE_UNDO,
	ACTIVATE_REDO,
	ACTIVATE_CUT,
	ACTIVATE_COPY,
	ACTIVATE_PASTE,
	ACTIVATE_SEARCH,
	ACTIVATE_DELETE,
	ACTIVATE_CLEAR,
	ACTIVATE_PREFERENCES,
	ACTIVATE_PROPERTIES,
	ACTIVATE_QUIT,
	POPUP,
	POPDOWN,
	LAST_SIGNAL
};

static const XpadToolbarButton buttons[] =
{
	{"Clear", "edit-clear", ACTIVATE_CLEAR, XPAD_BUTTON_TYPE_BUTTON, N_("Clear Pad Contents"), N_("Add C_lear button")},
	{"Close", "window-close", ACTIVATE_CLOSE, XPAD_BUTTON_TYPE_BUTTON, N_("Close and Save Pad"), N_("Add _Close button")},
	{"Copy", "edit-copy", ACTIVATE_COPY, XPAD_BUTTON_TYPE_BUTTON, N_("Copy to Clipboard"), N_("Add C_opy button")},
	{"Cut", "edit-cut", ACTIVATE_CUT, XPAD_BUTTON_TYPE_BUTTON, N_("Cut to Clipboard"), N_("Add C_ut button")},
	{"Delete", "edit-delete", ACTIVATE_DELETE, XPAD_BUTTON_TYPE_BUTTON, N_("Delete Pad"), N_("Add _Delete button")},
	{"Find", "edit-find", ACTIVATE_SEARCH, XPAD_BUTTON_TYPE_BUTTON, N_("Find text"), N_("Add _Find button")},
	{"New", "document-new", ACTIVATE_NEW, XPAD_BUTTON_TYPE_BUTTON, N_("Open New Pad"), N_("Add _New button")},
	{"Paste", "edit-paste", ACTIVATE_PASTE, XPAD_BUTTON_TYPE_BUTTON, N_("Paste from Clipboard"), N_("Add Pa_ste button")},
	{"Preferences", "preferences-system", ACTIVATE_PREFERENCES, XPAD_BUTTON_TYPE_BUTTON, N_("Edit Preferences"), N_("Add Pr_eferences button")},
	{"Properties", "document-properties", ACTIVATE_PROPERTIES, XPAD_BUTTON_TYPE_BUTTON, N_("Edit Pad Properties"), N_("Add Proper_ties button")},
	{"Redo", "edit-redo", ACTIVATE_REDO, XPAD_BUTTON_TYPE_BUTTON, N_("Redo"), N_("Add _Redo button")},
	{"Quit", "application-exit", ACTIVATE_QUIT, XPAD_BUTTON_TYPE_BUTTON, N_("Close All Pads"), N_("Add Close _All button")},
	{"Undo", "edit-undo", ACTIVATE_UNDO, XPAD_BUTTON_TYPE_BUTTON, N_("Undo"), N_("Add _Undo button")},
	{"Separator", NULL, 0, XPAD_BUTTON_TYPE_SEPARATOR, NULL, N_("Add Se_parator")}
};

static void xpad_toolbar_constructed (GObject *object);
static void xpad_toolbar_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec);
static void xpad_toolbar_get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec);
static void xpad_toolbar_dispose (GObject *object);
static const XpadToolbarButton *xpad_toolbar_button_lookup (XpadToolbar *toolbar, const gchar *name);
static GtkToolItem *xpad_toolbar_button_to_item (XpadToolbar *toolbar, const XpadToolbarButton *button);
static void xpad_toolbar_button_activated (GtkToolButton *button);
static void xpad_toolbar_change_buttons (XpadToolbar *toolbar);
static void xpad_toolbar_add_button (GtkMenuItem *menu_item, XpadSettings *settings);
static gboolean xpad_toolbar_popup_context_menu (GtkToolbar *toolbar, gint x, gint y, gint button);

static guint signals[LAST_SIGNAL] = { 0 };

enum
{
	PROP_0,
	PROP_PAD,
	LAST_PROP
};

GtkWidget *
xpad_toolbar_new (XpadPad *pad)
{
	return GTK_WIDGET (g_object_new (XPAD_TYPE_TOOLBAR, "pad", pad, NULL));
}

static void
xpad_toolbar_class_init (XpadToolbarClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
	GtkToolbarClass *gtktoolbar_class = GTK_TOOLBAR_CLASS (klass);

	gobject_class->constructed = xpad_toolbar_constructed;
	gobject_class->set_property = xpad_toolbar_set_property;
	gobject_class->get_property = xpad_toolbar_get_property;
	gobject_class->dispose = xpad_toolbar_dispose;
	gtktoolbar_class->popup_context_menu = xpad_toolbar_popup_context_menu;

	signals[ACTIVATE_NEW] = 
		g_signal_new ("activate-new",
		              G_OBJECT_CLASS_TYPE (gobject_class),
		              G_SIGNAL_RUN_LAST,
		              0,
		              NULL, NULL,
		              g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);

	signals[ACTIVATE_CLOSE] = 
		g_signal_new ("activate-close",
		              G_OBJECT_CLASS_TYPE (gobject_class),
		              G_SIGNAL_RUN_LAST,
		              0,
		              NULL, NULL,
		              g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);

	signals[ACTIVATE_UNDO] = 
		g_signal_new ("activate-undo",
		              G_OBJECT_CLASS_TYPE (gobject_class),
		              G_SIGNAL_RUN_LAST,
		              0,
		              NULL, NULL,
		              g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);

	signals[ACTIVATE_REDO] = 
		g_signal_new ("activate-redo",
		              G_OBJECT_CLASS_TYPE (gobject_class),
		              G_SIGNAL_RUN_LAST,
		              0,
		              NULL, NULL,
		              g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);

	signals[ACTIVATE_CUT] = 
		g_signal_new ("activate-cut",
		              G_OBJECT_CLASS_TYPE (gobject_class),
		              G_SIGNAL_RUN_LAST,
		              0,
		              NULL, NULL,
		              g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);

	signals[ACTIVATE_COPY] = 
		g_signal_new ("activate-copy",
		              G_OBJECT_CLASS_TYPE (gobject_class),
		              G_SIGNAL_RUN_LAST,
		              0,
		              NULL, NULL,
		              g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);

	signals[ACTIVATE_PASTE] = 
		g_signal_new ("activate-paste",
		              G_OBJECT_CLASS_TYPE (gobject_class),
		              G_SIGNAL_RUN_LAST,
		              0,
		              NULL, NULL,
		              g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);

	signals[ACTIVATE_SEARCH] =
		g_signal_new ("activate-search",
		              G_OBJECT_CLASS_TYPE (gobject_class),
		              G_SIGNAL_RUN_LAST,
		              0,
		              NULL, NULL,
		              g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);

	signals[ACTIVATE_QUIT] = 
		g_signal_new ("activate-quit",
		              G_OBJECT_CLASS_TYPE (gobject_class),
		              G_SIGNAL_RUN_LAST,
		              0,
		              NULL, NULL,
		              g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);

	signals[ACTIVATE_CLEAR] = 
		g_signal_new ("activate-clear",
		              G_OBJECT_CLASS_TYPE (gobject_class),
		              G_SIGNAL_RUN_LAST,
		              0,
		              NULL, NULL,
		              g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);
	
	signals[ACTIVATE_PROPERTIES] = 
		g_signal_new ("activate-properties",
		              G_OBJECT_CLASS_TYPE (gobject_class),
		              G_SIGNAL_RUN_LAST,
		              0,
		              NULL, NULL,
		              g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);

	signals[ACTIVATE_PREFERENCES] = 
		g_signal_new ("activate-preferences",
		              G_OBJECT_CLASS_TYPE (gobject_class),
		              G_SIGNAL_RUN_LAST,
		              0,
		              NULL, NULL,
		              g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);

	signals[ACTIVATE_DELETE] = 
		g_signal_new ("activate-delete",
		              G_OBJECT_CLASS_TYPE (gobject_class),
		              G_SIGNAL_RUN_LAST,
		              0,
		              NULL, NULL,
		              g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);

	signals[POPUP] = 
		g_signal_new ("popup",
		              G_OBJECT_CLASS_TYPE (gobject_class),
		              G_SIGNAL_RUN_LAST,
		              0,
		              NULL, NULL,
		              g_cclosure_marshal_VOID__OBJECT, G_TYPE_NONE, 1, GTK_TYPE_MENU);

	signals[POPDOWN] = 
		g_signal_new ("popdown",
		              G_OBJECT_CLASS_TYPE (gobject_class),
		              G_SIGNAL_RUN_LAST,
		              0,
		              NULL, NULL,
		              g_cclosure_marshal_VOID__OBJECT, G_TYPE_NONE, 1, GTK_TYPE_MENU);

	g_object_class_install_property (gobject_class,
					 PROP_PAD,
					 g_param_spec_pointer ("pad",
									"Pad",
									"Pad associated with this toolbar",
									G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY));
}

static void
xpad_toolbar_init (XpadToolbar *toolbar)
{
	toolbar->priv = xpad_toolbar_get_instance_private(toolbar);

	toolbar->priv->move_motion_handler = 0;
	toolbar->priv->move_button_release_handler = 0;
	toolbar->priv->move_key_press_handler = 0;

	g_object_set (toolbar,
	              "icon-size", GTK_ICON_SIZE_MENU,
	              "show-arrow", FALSE,
	              "toolbar-style", GTK_TOOLBAR_ICONS,
	              NULL);
}

static void xpad_toolbar_constructed (GObject *object)
{
	XpadSettings *settings;
	XpadToolbar *toolbar = XPAD_TOOLBAR (object);

	xpad_toolbar_change_buttons (toolbar);

	/* Add CSS style class, so the styling can be overridden by a GTK theme */
	GtkStyleContext *context = gtk_widget_get_style_context(GTK_WIDGET (toolbar));
	gtk_style_context_add_class(context, "XpadToolbar");

	g_object_get (toolbar->priv->pad, "settings", &settings, NULL);
	g_signal_connect_swapped (settings, "change-buttons", G_CALLBACK (xpad_toolbar_change_buttons), toolbar);
}

static void
xpad_toolbar_dispose (GObject *object)
{
	XpadToolbar *toolbar = XPAD_TOOLBAR (object);

	g_clear_object (&toolbar->priv->pad);

	G_OBJECT_CLASS (xpad_toolbar_parent_class)->dispose (object);
}

void
xpad_toolbar_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
	XpadToolbar *toolbar = XPAD_TOOLBAR (object);

	switch (prop_id)
	{
	case PROP_PAD:
		g_clear_object (&toolbar->priv->pad);

		if (G_VALUE_HOLDS_POINTER (value) && G_IS_OBJECT (g_value_get_pointer (value))) {
			toolbar->priv->pad = g_value_get_pointer (value);
			g_object_ref (toolbar->priv->pad);
		}

		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	 }
}

void
xpad_toolbar_get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
	XpadToolbar *toolbar = XPAD_TOOLBAR (object);

	switch (prop_id)
	{
	case PROP_PAD:
		g_value_set_pointer (value, toolbar->priv->pad);
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static const XpadToolbarButton *
xpad_toolbar_button_lookup (XpadToolbar *toolbar, const gchar *name)
{
	/* A dirty way to silence the compiler for these unused variables. */
	(void) toolbar;

	guint i;
	for (i = 0; i < G_N_ELEMENTS (buttons); i++)
		if (!g_ascii_strcasecmp (name, buttons[i].name))
			return &buttons[i];

	return NULL;
}

static GtkToolItem *
xpad_toolbar_button_to_item (XpadToolbar *toolbar, const XpadToolbarButton *button)
{
	GtkToolItem *item = GTK_TOOL_ITEM (g_object_get_data (G_OBJECT (toolbar), button->name));

	if (item)
		return item;

	switch (button->type)
	{
	case XPAD_BUTTON_TYPE_BUTTON:
		item = gtk_tool_button_new (NULL, button->name);
		gtk_tool_button_set_icon_name (GTK_TOOL_BUTTON (item), button->icon_name);
		g_signal_connect (item, "clicked", G_CALLBACK (xpad_toolbar_button_activated), NULL);
		break;
	case XPAD_BUTTON_TYPE_TOGGLE:
		item = gtk_tool_button_new (NULL, button->name);
		gtk_tool_button_set_icon_name (GTK_TOOL_BUTTON (item), button->icon_name);
		g_signal_connect (item, "toggled", G_CALLBACK (xpad_toolbar_button_activated), NULL);
		break;
	case XPAD_BUTTON_TYPE_SEPARATOR:
		item = GTK_TOOL_ITEM (gtk_separator_tool_item_new ());
		break;
	default:
		return NULL;
	}

	g_object_set_data (G_OBJECT (item), "xpad-toolbar", toolbar);
	g_object_set_data (G_OBJECT (item), "xpad-tb", (gpointer) button);

	g_object_set_data (G_OBJECT (toolbar), button->name, item);

	if (button->desc)
		gtk_tool_item_set_tooltip_text (item, _(button->desc));

	return item;
}

static void
xpad_toolbar_button_activated (GtkToolButton *button)
{
	XpadToolbar *toolbar;
	const XpadToolbarButton *tb;

	toolbar = XPAD_TOOLBAR (g_object_get_data (G_OBJECT (button), "xpad-toolbar"));
	tb = (const XpadToolbarButton *) g_object_get_data (G_OBJECT (button), "xpad-tb");

	g_signal_emit (toolbar, signals[tb->signal], 0);
}

static void
xpad_toolbar_change_buttons (XpadToolbar *toolbar)
{
	GList *list, *temp;
	const GSList *slist, *stemp;
	guint i = 0, j = 0;
	GtkToolItem *item;
	XpadSettings *settings;

	list = gtk_container_get_children (GTK_CONTAINER (toolbar));

	for (temp = list; temp; temp = temp->next)
		gtk_widget_destroy (temp->data);

	g_list_free (list);
	g_list_free (temp);

	for (j = 0; j < G_N_ELEMENTS (buttons); j++)
		g_object_set_data (G_OBJECT (toolbar), buttons[j].name, NULL);

	g_object_get (toolbar->priv->pad, "settings", &settings, NULL);

	slist = xpad_settings_get_toolbar_buttons (settings);

	for (stemp = slist; stemp; stemp = stemp->next)
	{
		const XpadToolbarButton *button;

		button = xpad_toolbar_button_lookup (toolbar, stemp->data);

		if (button->type == XPAD_BUTTON_TYPE_SEPARATOR)
			item = gtk_separator_tool_item_new ();
		else if (button->type == XPAD_BUTTON_TYPE_BUTTON)
			item = xpad_toolbar_button_to_item (toolbar, button);
		else
			continue;

		if (item)
		{
			g_object_set_data (G_OBJECT (item), "xpad-button-num", GINT_TO_POINTER (i));
			gtk_toolbar_insert (GTK_TOOLBAR (toolbar), item, -1);
			gtk_widget_show_all (GTK_WIDGET (item));
			i++;
		}
	}

	item = gtk_separator_tool_item_new ();
	g_object_set_data (G_OBJECT (item), "xpad-button-num", GINT_TO_POINTER (i));
	gtk_separator_tool_item_set_draw (GTK_SEPARATOR_TOOL_ITEM (item), FALSE);
	gtk_tool_item_set_expand (item, TRUE);
	gtk_toolbar_insert (GTK_TOOLBAR (toolbar), item, -1);
	gtk_widget_show_all (GTK_WIDGET (item));
	i++;

	item = xpad_grip_tool_item_new ();
	g_object_set_data (G_OBJECT (item), "xpad-button-num", GINT_TO_POINTER (i));
	gtk_toolbar_insert (GTK_TOOLBAR (toolbar), item, -1);
	gtk_widget_show_all (GTK_WIDGET (item));
	i++;

	if (toolbar->priv->pad)
	{
		xpad_pad_notify_has_selection (toolbar->priv->pad);
		xpad_pad_notify_clipboard_owner_changed (toolbar->priv->pad);
	}
}

static void
xpad_toolbar_add_button (GtkMenuItem *menu_item, XpadSettings *settings)
{
	xpad_settings_add_toolbar_button (settings, gtk_menu_item_get_accel_path (menu_item) + 2);
}

static void
menu_deactivated (GtkWidget *menu, GtkToolbar *toolbar)
{
	g_signal_emit (toolbar, signals[POPDOWN], 0, menu);
}

static gboolean
xpad_toolbar_popup_context_menu (GtkToolbar *toolbar, gint x, gint y, gint button)
{
	/* A dirty way to silence the compiler for these unused variables. */
	(void) x;
	(void) y;

	guint i;
	gboolean is_button = FALSE;
	XpadSettings *settings;

	g_object_get (XPAD_TOOLBAR (toolbar)->priv->pad, "settings", &settings, NULL);
	const GSList *current_buttons = xpad_settings_get_toolbar_buttons (settings);
	GtkMenu *menu = GTK_MENU (gtk_menu_new ());

	for (i = 0; i < G_N_ELEMENTS (buttons); i++)
	{
		const GSList *j;
		GtkWidget *item;

		for (j = current_buttons; j; j = j->next)
			if (g_ascii_strcasecmp (j->data, "Separator") && !g_ascii_strcasecmp (j->data, buttons[i].name))
				break;

		if (j)
		{
			is_button = TRUE;
			continue;
		}

		item = gtk_menu_item_new_with_mnemonic (buttons[i].menu_desc);
		/* Ugly workaround by abusing the accel_path to get the variable passed to the next function */
		gtk_menu_item_set_accel_path (GTK_MENU_ITEM (item), g_strdup_printf ("</%s", buttons[i].name));
		g_signal_connect (item, "activate", G_CALLBACK (xpad_toolbar_add_button), settings);
		gtk_menu_attach (menu, item, 0, 1, i, i + 1);
		gtk_widget_show (item);
	}

	if (is_button)
	{
		GtkWidget *item;

		item = gtk_menu_item_new_with_mnemonic (N_("Remove All _Buttons"));
		g_signal_connect_swapped (item, "activate", G_CALLBACK (xpad_settings_remove_all_toolbar_buttons), settings);
		gtk_menu_attach (menu, item, 0, 1, i, i + 1);
		gtk_widget_show (item);

		i++;

		item = gtk_menu_item_new_with_mnemonic (N_("Remo_ve Last Button"));
		g_signal_connect_swapped (item, "activate", G_CALLBACK (xpad_settings_remove_last_toolbar_button), settings);
		gtk_menu_attach (menu, item, 0, 1, i, i + 1);
		gtk_widget_show (item);
	}

	g_signal_connect (menu, "deactivate", G_CALLBACK (menu_deactivated), toolbar);

	gtk_menu_popup_at_pointer (menu, NULL);

	g_signal_emit (toolbar, signals[POPUP], 0, menu);

	return TRUE;
}

static void
xpad_toolbar_enable_button (XpadToolbar *toolbar, const XpadToolbarButton *button, gboolean enable)
{
	g_return_if_fail (button);
	GtkToolItem *item = xpad_toolbar_button_to_item (toolbar, button);
	if (item)
		gtk_widget_set_sensitive (GTK_WIDGET (item), enable);
}

void
xpad_toolbar_enable_undo_button (XpadToolbar *toolbar, gboolean enable)
{
	const XpadToolbarButton *button = xpad_toolbar_button_lookup (toolbar, "Undo");
	xpad_toolbar_enable_button (toolbar, button, enable);
}

void
xpad_toolbar_enable_redo_button (XpadToolbar *toolbar, gboolean enable)
{
	const XpadToolbarButton *button = xpad_toolbar_button_lookup (toolbar, "Redo");
	xpad_toolbar_enable_button (toolbar, button, enable);
}

void
xpad_toolbar_enable_cut_button (XpadToolbar *toolbar, gboolean enable)
{
	const XpadToolbarButton *button = xpad_toolbar_button_lookup (toolbar, "Cut");
	xpad_toolbar_enable_button (toolbar, button, enable);
}

void
xpad_toolbar_enable_copy_button (XpadToolbar *toolbar, gboolean enable)
{
	const XpadToolbarButton *button = xpad_toolbar_button_lookup (toolbar, "Copy");
	xpad_toolbar_enable_button (toolbar, button, enable);
}

void
xpad_toolbar_enable_paste_button (XpadToolbar *toolbar, gboolean enable)
{
	const XpadToolbarButton *button = xpad_toolbar_button_lookup (toolbar, "Paste");
	xpad_toolbar_enable_button (toolbar, button, enable);
}
