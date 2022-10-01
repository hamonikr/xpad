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

#include <string.h>
#include <gtk/gtk.h>
#include <glib/gi18n.h>

#include "xpad-app.h"
#include "xpad-settings.h"
#include "fio.h"

struct XpadSettingsPrivate
{
	guint width;
	guint height;
	gboolean hide_from_taskbar;
	gboolean hide_from_task_switcher;
	gboolean has_decorations;
	gboolean confirm_destroy;
	gboolean edit_lock;
	gboolean tray_enabled;
	guint tray_click_configuration;
	gboolean has_toolbar;
	gboolean autohide_toolbar;
	gboolean has_scrollbar;
	GdkRGBA *text;
	GdkRGBA *back;
	const gchar *fontname;
	GSList *toolbar_buttons;
	gboolean autostart_wait_systray;
	guint autostart_delay;
	gboolean autostart_new_pad;
	gboolean autostart_sticky;
	guint autostart_display_pads;
	gboolean line_numbering;
};

G_DEFINE_TYPE_WITH_PRIVATE (XpadSettings, xpad_settings, G_TYPE_OBJECT)

#define DEFAULTS_FILENAME	"default-style"

enum
{
	CHANGE_BUTTONS,
	LAST_SIGNAL
};

enum
{
	PROP_0,
	PROP_WIDTH,
	PROP_HEIGHT,
	PROP_HAS_DECORATIONS,
	PROP_CONFIRM_DESTROY,
	PROP_EDIT_LOCK,
	PROP_TRAY_ENABLED,
	PROP_TRAY_CLICK_CONFIGURATION,
	PROP_HAS_TOOLBAR,
	PROP_AUTOHIDE_TOOLBAR,
	PROP_HAS_SCROLLBAR,
	PROP_TEXT_COLOR,
	PROP_BACK_COLOR,
	PROP_FONTNAME,
	PROP_AUTOSTART_XPAD,
	PROP_AUTOSTART_WAIT_SYSTRAY,
	PROP_AUTOSTART_DELAY,
	PROP_AUTOSTART_NEW_PAD,
	PROP_AUTOSTART_STICKY,
	PROP_AUTOSTART_DISPLAY_PADS,
	PROP_HIDE_FROM_TASKBAR,
	PROP_HIDE_FROM_TASK_SWITCHER,
	PROP_LINE_NUMBERING,
	N_PROPERTIES
};

static GParamSpec *obj_prop[N_PROPERTIES] = { NULL, };
static guint signals[LAST_SIGNAL] = { 0 };

/* A pleasant light yellow background color, similar to commercial sticky notes, with black text. */
static const GdkRGBA text_color_default = {0, 0, 0, 1};
static const GdkRGBA back_color_default = {1, 0.933334350586, 0.6, 1};
static const gchar *font_default = "Sans 9";

static void load_from_file (XpadSettings *settings, const gchar *filename);
static void save_to_file (XpadSettings *settings, const gchar *filename);
static void xpad_settings_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec);
static void xpad_settings_get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec);
static void xpad_settings_finalize (GObject *object);

XpadSettings *
xpad_settings_new (void)
{
	return g_object_new (XPAD_TYPE_SETTINGS, NULL);
}

static void
xpad_settings_class_init (XpadSettingsClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

	gobject_class->finalize = xpad_settings_finalize;
	gobject_class->set_property = xpad_settings_set_property;
	gobject_class->get_property = xpad_settings_get_property;

	obj_prop[PROP_WIDTH] = g_param_spec_uint ("width", "Default width of pads", "Window width of pads on creation", 0, G_MAXUINT, 300, G_PARAM_READWRITE);
	obj_prop[PROP_HEIGHT] = g_param_spec_uint ("height", "Default height of pads", "Window height of pads on creation", 0, G_MAXUINT, 300, G_PARAM_READWRITE);
	obj_prop[PROP_HAS_DECORATIONS] = g_param_spec_boolean ("has-decorations", "Has decorations", "Whether pads have window decorations", FALSE, G_PARAM_READWRITE);
	obj_prop[PROP_CONFIRM_DESTROY] = g_param_spec_boolean ("confirm-destroy", "Confirm destroy", "Ask for delete confirmation", TRUE, G_PARAM_READWRITE);
	obj_prop[PROP_EDIT_LOCK] = g_param_spec_boolean ("edit-lock", "Edit lock", "Toggle read-only mode", FALSE, G_PARAM_READWRITE);
	obj_prop[PROP_TRAY_ENABLED] = g_param_spec_boolean ("tray-enabled", "Enable tray icon", "Enable or disable the systray icon", TRUE, G_PARAM_READWRITE);
	obj_prop[PROP_TRAY_CLICK_CONFIGURATION] = g_param_spec_uint ("tray-click-configuration", "Tray click configuration", "Configure tray left click", 0, G_MAXUINT, 0, G_PARAM_READWRITE);
	obj_prop[PROP_HAS_TOOLBAR] = g_param_spec_boolean ("has-toolbar", "Has toolbar", "Whether pads have toolbars", TRUE, G_PARAM_READWRITE);
	obj_prop[PROP_AUTOHIDE_TOOLBAR] = g_param_spec_boolean ("autohide-toolbar", "Autohide toolbar", "Hide toolbars when not used", TRUE, G_PARAM_READWRITE);
	obj_prop[PROP_HAS_SCROLLBAR] = g_param_spec_boolean ("has-scrollbar", "Has scrollbar", "Whether pads have scrollbars", TRUE, G_PARAM_READWRITE);
	obj_prop[PROP_FONTNAME] = g_param_spec_string ("fontname", "Font name", "Default name of pad font", NULL, G_PARAM_READWRITE);
	obj_prop[PROP_TEXT_COLOR] = g_param_spec_boxed ("text-color", "Text color", "Default color of pad text", GDK_TYPE_RGBA, G_PARAM_READWRITE);
	obj_prop[PROP_BACK_COLOR] = g_param_spec_boxed ("back-color", "Back color", "Default color of pad background", GDK_TYPE_RGBA, G_PARAM_READWRITE);
	obj_prop[PROP_AUTOSTART_XPAD] = g_param_spec_boolean ("autostart-xpad", "Automatically start xpad", "Start Xpad after login", FALSE, G_PARAM_READWRITE);
	obj_prop[PROP_AUTOSTART_WAIT_SYSTRAY] = g_param_spec_boolean ("autostart-wait-systray", "Wait for systray", "Whether to wait for the systray after login", TRUE, G_PARAM_READWRITE);
	obj_prop[PROP_AUTOSTART_NEW_PAD] = g_param_spec_boolean ("autostart-new-pad", "Start a new pad", "Whether to create a new pad on startup", FALSE, G_PARAM_READWRITE);
	obj_prop[PROP_AUTOSTART_STICKY] = g_param_spec_boolean ("autostart-sticky", "Stick to desktop", "Whether pads are sticky on creation", FALSE, G_PARAM_READWRITE);
	obj_prop[PROP_AUTOSTART_DELAY] = g_param_spec_uint ("autostart-delay", "Delay autostart of Xpad", "Number of seconds to wait before start of Xpad", 0, G_MAXUINT, 0, G_PARAM_READWRITE);
	obj_prop[PROP_AUTOSTART_DISPLAY_PADS] = g_param_spec_uint ("autostart-display-pads", "Autostart display pads", "Show/hide/restore pads at start", 0, G_MAXUINT, 2, G_PARAM_READWRITE);
	obj_prop[PROP_HIDE_FROM_TASKBAR] = g_param_spec_boolean ("hide-from-taskbar", "Hide from taskbar", "Hide the pads from the task bar", FALSE, G_PARAM_READWRITE);
	obj_prop[PROP_HIDE_FROM_TASK_SWITCHER] = g_param_spec_boolean ("hide-from-task-switcher", "Hide from task switcher", "Hide the pads from the task or workspace switcher", FALSE, G_PARAM_READWRITE);
	obj_prop[PROP_LINE_NUMBERING] = g_param_spec_boolean ("line-numbering", "Toggle line numbering", "Enable or disable the line numbering", FALSE, G_PARAM_READWRITE);

	g_object_class_install_properties (gobject_class, N_PROPERTIES, obj_prop);

	signals[CHANGE_BUTTONS] = g_signal_new ("change_buttons", G_OBJECT_CLASS_TYPE (gobject_class), G_SIGNAL_RUN_LAST, G_STRUCT_OFFSET (XpadSettingsClass, change_buttons), NULL, NULL, g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);
}

static void
xpad_settings_init (XpadSettings *settings)
{
	settings->priv = xpad_settings_get_instance_private (settings);

	/*
	 * FIXME: Although the default values below have been set in the xpad_settings_class_init above,
	 * these default values are not applied to the private variables of this instance.
	 * I haven't found a way to resolve this yet. So, for now we have a double
	 * administration of defaults values. It would be awesome to reduce this to 1 administration.
	 *
	 * Found the explanation, but not a solution:
	 * http://blogs.gnome.org/desrt/2012/02/26/a-gentle-introduction-to-gobject-construction/
	 */
	settings->priv->width = 300;
	settings->priv->height = 300;
	settings->priv->has_decorations = FALSE;
	settings->priv->hide_from_taskbar = FALSE;
	settings->priv->hide_from_task_switcher = FALSE;
	settings->priv->confirm_destroy = TRUE;
	settings->priv->edit_lock = FALSE;
	settings->priv->tray_enabled = TRUE;
	settings->priv->tray_click_configuration = 1;
	settings->priv->has_toolbar = TRUE;
	settings->priv->autohide_toolbar = TRUE;
	settings->priv->has_scrollbar = TRUE;
	settings->priv->fontname = g_strdup (font_default);
	settings->priv->text = gdk_rgba_copy (&text_color_default);
	settings->priv->back = gdk_rgba_copy (&back_color_default);
	settings->priv->autostart_wait_systray = TRUE;
	settings->priv->autostart_new_pad = FALSE;
	settings->priv->autostart_sticky = FALSE;
	settings->priv->autostart_delay = 0;
	settings->priv->autostart_display_pads = 2;
	settings->priv->line_numbering = FALSE;

	settings->priv->toolbar_buttons = NULL;
	settings->priv->toolbar_buttons = g_slist_append (settings->priv->toolbar_buttons, g_strdup ("New"));
	settings->priv->toolbar_buttons = g_slist_append (settings->priv->toolbar_buttons, g_strdup ("Delete"));
	settings->priv->toolbar_buttons = g_slist_append (settings->priv->toolbar_buttons, g_strdup ("Separator"));
	settings->priv->toolbar_buttons = g_slist_append (settings->priv->toolbar_buttons, g_strdup ("Cut"));
	settings->priv->toolbar_buttons = g_slist_append (settings->priv->toolbar_buttons, g_strdup ("Copy"));
	settings->priv->toolbar_buttons = g_slist_append (settings->priv->toolbar_buttons, g_strdup ("Paste"));
	settings->priv->toolbar_buttons = g_slist_append (settings->priv->toolbar_buttons, g_strdup ("Separator"));
	settings->priv->toolbar_buttons = g_slist_append (settings->priv->toolbar_buttons, g_strdup ("Undo"));
	settings->priv->toolbar_buttons = g_slist_append (settings->priv->toolbar_buttons, g_strdup ("Redo"));
	settings->priv->toolbar_buttons = g_slist_append (settings->priv->toolbar_buttons, g_strdup ("Separator"));
	settings->priv->toolbar_buttons = g_slist_append (settings->priv->toolbar_buttons, g_strdup ("Find"));

	load_from_file (settings, DEFAULTS_FILENAME);
}

static void
xpad_settings_finalize (GObject *object)
{
	XpadSettings *settings = XPAD_SETTINGS (object);

	g_slist_free (settings->priv->toolbar_buttons);

	if (settings->priv->text)
		gdk_rgba_free (settings->priv->text);
	if (settings->priv->back)
		gdk_rgba_free (settings->priv->back);

	G_OBJECT_CLASS (xpad_settings_parent_class)->finalize (object);
}

void xpad_settings_add_toolbar_button (XpadSettings *settings, const gchar *button)
{
	settings->priv->toolbar_buttons = g_slist_append (settings->priv->toolbar_buttons, g_strdup (button));

	save_to_file (settings, DEFAULTS_FILENAME);

	g_signal_emit (settings, signals[CHANGE_BUTTONS], 0);
}

static void xpad_settings_remove_toolbar_list_element (XpadSettings *settings, GSList *element)
{
	g_free (element->data);
	settings->priv->toolbar_buttons = g_slist_delete_link (settings->priv->toolbar_buttons, element);
}

gboolean xpad_settings_remove_all_toolbar_buttons (XpadSettings *settings)
{
	if (settings->priv->toolbar_buttons == NULL)
		return FALSE;

	while (settings->priv->toolbar_buttons)
	{
		g_free (settings->priv->toolbar_buttons->data);
		settings->priv->toolbar_buttons = 
			g_slist_delete_link (settings->priv->toolbar_buttons,
					settings->priv->toolbar_buttons);
	}

	settings->priv->toolbar_buttons = NULL;

	g_signal_emit (settings, signals[CHANGE_BUTTONS], 0);

	return TRUE;
}

gboolean xpad_settings_remove_last_toolbar_button (XpadSettings *settings)
{
	GSList *element;

	element = g_slist_last (settings->priv->toolbar_buttons);

	if (!element) {
		g_slist_free(element);
		return FALSE;
	}

	xpad_settings_remove_toolbar_list_element (settings, element);

	save_to_file (settings, DEFAULTS_FILENAME);

	g_signal_emit (settings, signals[CHANGE_BUTTONS], 0);

	return TRUE;
}

const GSList *xpad_settings_get_toolbar_buttons (XpadSettings *settings)
{
	return settings->priv->toolbar_buttons;
}

static void
xpad_settings_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
	XpadSettings *settings = XPAD_SETTINGS (object);

	switch (prop_id)
	{

	case PROP_WIDTH:
		settings->priv->width = g_value_get_uint (value);
		break;

	case PROP_HEIGHT:
		settings->priv->height = g_value_get_uint (value);
		break;

	case PROP_HAS_DECORATIONS:
		settings->priv->has_decorations = g_value_get_boolean (value);
		break;

	case PROP_CONFIRM_DESTROY:
		settings->priv->confirm_destroy = g_value_get_boolean (value);
		break;

	case PROP_EDIT_LOCK:
		settings->priv->edit_lock = g_value_get_boolean (value);
		break;

	case PROP_TRAY_ENABLED:
		settings->priv->tray_enabled = g_value_get_boolean (value);
		break;

	case PROP_TRAY_CLICK_CONFIGURATION:
		settings->priv->tray_click_configuration = g_value_get_uint(value);
		break;

	case PROP_HAS_TOOLBAR:
		settings->priv->has_toolbar = g_value_get_boolean (value);
		break;

	case PROP_AUTOHIDE_TOOLBAR:
		settings->priv->autohide_toolbar = g_value_get_boolean (value);
		break;

	case PROP_HAS_SCROLLBAR:
		settings->priv->has_scrollbar = g_value_get_boolean (value);
		break;

	case PROP_TEXT_COLOR:
		if (settings->priv->text) {
			gdk_rgba_free (settings->priv->text);
		}

		if (g_value_get_boxed (value)) {
			settings->priv->text = gdk_rgba_copy (g_value_get_boxed (value));
		} else {
			settings->priv->text = NULL;
		}

		break;

	case PROP_BACK_COLOR:
		if (settings->priv->back) {
			gdk_rgba_free (settings->priv->back);
		}

		if (g_value_get_boxed (value)) {
			settings->priv->back = gdk_rgba_copy (g_value_get_boxed (value));
		} else {
			settings->priv->back = NULL;
		}

		break;

	case PROP_FONTNAME:
		if (value)
			settings->priv->fontname = g_value_dup_string (value);
		else
			settings->priv->fontname = NULL;
		break;

	case PROP_AUTOSTART_XPAD:
		if (g_value_get_boolean (value)) {
			/* Copy the xpad.desktop file to the autostart folder and enable/disable the wait for systray preference */
			const gchar *xpad_desktop_filename = "xpad.desktop";

			gboolean success;
			gchar *source_dir, *destination_dir;
			GFile *source, *destination;
			GError *error = NULL;

			source_dir = g_strdup_printf ("%s/applications", DATADIR);
			destination_dir = g_strdup_printf ("%s/.config/autostart", g_get_home_dir());

			if (g_mkdir_with_parents (destination_dir, 0700) != 0) {
				xpad_app_error (NULL, _("Error enabling Xpad autostart"), g_strdup_printf (_("Could not create directory %s\n%s."), destination_dir, error->message));
				break;
			}

			source = g_file_new_build_filename (source_dir, xpad_desktop_filename, NULL);
			destination = g_file_new_build_filename (destination_dir, xpad_desktop_filename, NULL);

			success = g_file_copy (source, destination, G_FILE_COPY_OVERWRITE, NULL, NULL, NULL, &error);

			if (!success)
				xpad_app_error (NULL, _("Error enabling Xpad autostart"), g_strdup_printf (_("Could not copy %s to %s\n%s"), xpad_desktop_filename, destination_dir, error->message));

			g_free (source_dir);
			g_free (destination_dir);
		}
		else {
			/* Remove the xpad.desktop file from the autostart folder and enable/disable the wait for systray preference */
			gboolean success;
			char *filename;
			GFile *file;
			GError *error = NULL;

			filename = g_strdup_printf ("%s/.config/autostart/xpad.desktop", g_getenv ("HOME"));
			file = g_file_new_for_path (filename);
			success = g_file_delete (file, NULL, &error);

			if (!success)
				xpad_app_error (NULL, _("Error disabling Xpad autostart"), g_strdup_printf (_("Could not delete %s\n%s"), filename, error->message));
		}
		break;

	case PROP_AUTOSTART_WAIT_SYSTRAY:
		settings->priv->autostart_wait_systray = g_value_get_boolean (value);
		break;

	case PROP_AUTOSTART_NEW_PAD:
		settings->priv->autostart_new_pad = g_value_get_boolean (value);
		break;

	case PROP_AUTOSTART_STICKY:
		settings->priv->autostart_sticky = g_value_get_boolean (value);
		break;

	case PROP_AUTOSTART_DELAY:
		settings->priv->autostart_delay = g_value_get_uint (value);
		break;

	case PROP_AUTOSTART_DISPLAY_PADS:
		settings->priv->autostart_display_pads = g_value_get_uint (value);
		break;

	case PROP_HIDE_FROM_TASKBAR:
		settings->priv->hide_from_taskbar = g_value_get_boolean (value);
		break;

	case PROP_HIDE_FROM_TASK_SWITCHER:
		settings->priv->hide_from_task_switcher = g_value_get_boolean (value);
		break;

	case PROP_LINE_NUMBERING:
		settings->priv->line_numbering = g_value_get_boolean (value);
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		return;
	}
	save_to_file (settings, DEFAULTS_FILENAME);
}

static void
xpad_settings_get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
	XpadSettings *settings = XPAD_SETTINGS (object);

	switch (prop_id)
	{

	case PROP_WIDTH:
		g_value_set_uint (value, settings->priv->width);
		break;

	case PROP_HEIGHT:
		g_value_set_uint (value, settings->priv->height);
		break;

	case PROP_HAS_DECORATIONS:
		g_value_set_boolean (value, settings->priv->has_decorations);
		break;

	case PROP_CONFIRM_DESTROY:
		g_value_set_boolean (value, settings->priv->confirm_destroy);
		break;

	case PROP_EDIT_LOCK:
		g_value_set_boolean (value, settings->priv->edit_lock);
		break;

	case PROP_TRAY_ENABLED:
		g_value_set_boolean (value, settings->priv->tray_enabled);
		break;

	case PROP_TRAY_CLICK_CONFIGURATION:
		g_value_set_uint (value, settings->priv->tray_click_configuration);
		break;

	case PROP_HAS_TOOLBAR:
		g_value_set_boolean (value, settings->priv->has_toolbar);
		break;

	case PROP_AUTOHIDE_TOOLBAR:
		g_value_set_boolean (value, settings->priv->autohide_toolbar);
		break;

	case PROP_HAS_SCROLLBAR:
		g_value_set_boolean (value, settings->priv->has_scrollbar);
		break;

	case PROP_TEXT_COLOR:
		if (settings->priv->text)
			g_value_set_static_boxed (value, settings->priv->text);
		else
			value = NULL;
		break;

	case PROP_BACK_COLOR:
		if (settings->priv->back)
			g_value_set_static_boxed (value, settings->priv->back);
		else
			value = NULL;
		break;

	case PROP_FONTNAME:
		if (settings->priv->fontname)
			g_value_set_string (value, settings->priv->fontname);
		else
			value = NULL;
		break;

	case PROP_AUTOSTART_XPAD:
		g_value_set_boolean (value, g_file_test (g_strdup_printf ("%s/.config/autostart/xpad.desktop", g_get_home_dir()), G_FILE_TEST_EXISTS));
		break;

	case PROP_AUTOSTART_WAIT_SYSTRAY:
		g_value_set_boolean (value, settings->priv->autostart_wait_systray);
		break;

	case PROP_AUTOSTART_NEW_PAD:
		g_value_set_boolean (value, settings->priv->autostart_new_pad);
		break;

	case PROP_AUTOSTART_STICKY:
		g_value_set_boolean (value, settings->priv->autostart_sticky);
		break;

	case PROP_AUTOSTART_DELAY:
		g_value_set_uint (value, settings->priv->autostart_delay);
		break;

	case PROP_AUTOSTART_DISPLAY_PADS:
		g_value_set_uint (value, settings->priv->autostart_display_pads);
		break;

	case PROP_HIDE_FROM_TASKBAR:
		g_value_set_boolean (value, settings->priv->hide_from_taskbar);
		break;

	case PROP_HIDE_FROM_TASK_SWITCHER:
		g_value_set_boolean (value, settings->priv->hide_from_task_switcher);
		break;

	case PROP_LINE_NUMBERING:
		g_value_set_boolean (value, settings->priv->line_numbering);
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		return;
	}
}

static void
load_from_file (XpadSettings *settings, const gchar *filename)
{
	gchar *buttons = NULL, *text_color_string = NULL, *background_color_string = NULL;
	GdkRGBA text_color = text_color_default, back_color = back_color_default;
	gboolean use_text = TRUE, use_back = TRUE;

	/* get all the values from the default-style text file in the forms of booleans, ints or strings. */
	if (fio_get_values_from_file (filename,
		"b|decorations", &settings->priv->has_decorations,
		"u|height", &settings->priv->height,
		"u|width", &settings->priv->width,
		"b|confirm_destroy", &settings->priv->confirm_destroy,
		"b|edit_lock", &settings->priv->edit_lock,
		"b|sticky_on_start", &settings->priv->autostart_sticky,
		"b|tray_enabled", &settings->priv->tray_enabled,
		"u|tray_click_configuration", &settings->priv->tray_click_configuration,
		"s|back", &background_color_string,
		"b|use_back", &use_back,
		"s|text", &text_color_string,
		"b|use_text", &use_text,
		"s|fontname", &settings->priv->fontname,
		"b|toolbar", &settings->priv->has_toolbar,
		"b|auto_hide_toolbar", &settings->priv->autohide_toolbar,
		"b|scrollbar", &settings->priv->has_scrollbar,
		"s|buttons", &buttons,
		"b|autostart_wait_systray", &settings->priv->autostart_wait_systray,
		"u|autostart_delay", &settings->priv->autostart_delay,
		"b|autostart_new_pad", &settings->priv->autostart_new_pad,
		"u|autostart_display_pads", &settings->priv->autostart_display_pads,
		"b|hide_from_taskbar", &settings->priv->hide_from_taskbar,
		"b|hide_from_task_switcher", &settings->priv->hide_from_task_switcher,
		"b|line_numbering", &settings->priv->line_numbering,
		NULL))
		return;

	/* Remove the existing value, since we are going to overwrite it */
	if (settings->priv->text) {
		gdk_rgba_free (settings->priv->text);
	}

	if (use_text) {
		/* The user prefers a custom text color */
		if (text_color_string != NULL) {
			/* If parsing succeeds, then text_color is overwritten.
			 * If it fails, the default remains.
			 */
			gdk_rgba_parse (&text_color, text_color_string);
		}

		settings->priv->text = gdk_rgba_copy (&text_color);
	} else {
		/* The user prefers the text color of the GTK theme */
		settings->priv->text = NULL;
	}

	/* Remove the existing value, since we are going to overwrite it */
	if (settings->priv->back) {
		gdk_rgba_free (settings->priv->back);
	}

	if (use_back) {
		/* The user prefers a custom background color */
		if (background_color_string != NULL) {
			/* If parsing succeeds, then back_color is overwritten.
			 * If it fails, the default remains.
			 */
			gdk_rgba_parse (&back_color, background_color_string);
		}

		settings->priv->back = gdk_rgba_copy (&back_color);
	} else {
		/* The user prefers the background color of the GTK theme*/
		settings->priv->back = NULL;
	}

	/* If the String value of NULL has been retrieved, then the user wants to follow the GTK theme font */
    if (settings->priv->fontname && strcmp (settings->priv->fontname, "NULL") == 0) {
    	settings->priv->fontname = NULL;
    }

	if (buttons) {
		gint i;
		gchar **button_names;

		button_names = g_strsplit (buttons, ",", 0);

		while (settings->priv->toolbar_buttons)
		{
			g_free (settings->priv->toolbar_buttons->data);
			settings->priv->toolbar_buttons = 
				g_slist_delete_link (settings->priv->toolbar_buttons,
				settings->priv->toolbar_buttons);
		}

		for (i = 0; button_names[i]; ++i)
			settings->priv->toolbar_buttons = 
				g_slist_append (settings->priv->toolbar_buttons,
				g_strstrip (button_names[i])); /* takes ownership of string */

		g_free (button_names);
		g_free (buttons);
	}

	g_free(text_color_string);
	g_free(background_color_string);
}

static void
save_to_file (XpadSettings *settings, const gchar *filename)
{
	gchar *buttons = g_strdup ("");
	GSList *tmp;

	tmp = settings->priv->toolbar_buttons;

	while (tmp)
	{
		gchar *tmpstr = buttons;

		if (tmp->next)
			buttons = g_strconcat (buttons, tmp->data, ", ", NULL);
		else
			buttons = g_strconcat (buttons, tmp->data, NULL);

		g_free (tmpstr);
		tmp = tmp->next;
	}

	fio_set_values_to_file (filename,
		"b|decorations", settings->priv->has_decorations,
		"u|height", settings->priv->height,
		"u|width", settings->priv->width,
		"b|confirm_destroy", settings->priv->confirm_destroy,
		"b|edit_lock", settings->priv->edit_lock,
		"b|sticky_on_start", settings->priv->autostart_sticky,
		"b|tray_enabled", settings->priv->tray_enabled,
		"u|tray_click_configuration", settings->priv->tray_click_configuration,
		"s|back", settings->priv->back ? gdk_rgba_to_string (settings->priv->back) : "NULL",
		"b|use_back", settings->priv->back ? TRUE : FALSE,
		"s|text", settings->priv->text ? gdk_rgba_to_string (settings->priv->text) : "NULL",
		"b|use_text", settings->priv->text ? TRUE : FALSE,
		"s|fontname", settings->priv->fontname ? settings->priv->fontname : "NULL",
		"b|toolbar", settings->priv->has_toolbar,
		"b|auto_hide_toolbar", settings->priv->autohide_toolbar,
		"b|scrollbar", settings->priv->has_scrollbar,
		"s|buttons", buttons,
		"b|autostart_wait_systray", settings->priv->autostart_wait_systray,
		"u|autostart_delay", settings->priv->autostart_delay,
		"b|autostart_new_pad", settings->priv->autostart_new_pad,
		"u|autostart_display_pads", settings->priv->autostart_display_pads,
		"b|hide_from_taskbar", settings->priv->hide_from_taskbar,
		"b|hide_from_task_switcher", settings->priv->hide_from_task_switcher,
		"b|line_numbering", settings->priv->line_numbering,
		NULL);

	g_free (buttons);
}
