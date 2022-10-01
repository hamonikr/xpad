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

#include <gtk/gtk.h>
#include <gtksourceview/gtksource.h>

#include "xpad-text-view.h"
#include "xpad-text-buffer.h"
#include "xpad-pad.h"
#include "xpad-toolbar.h"

struct XpadTextViewPrivate
{
	gboolean follow_font_style;
	gboolean follow_color_style;
	gulong notify_text_handler;
	gulong notify_back_handler;
	gulong notify_font_handler;
	GtkCssProvider *font_provider;
	XpadTextBuffer *buffer;
	XpadSettings *settings;
	XpadPad *pad;
};

G_DEFINE_TYPE_WITH_PRIVATE (XpadTextView, xpad_text_view, GTK_SOURCE_TYPE_VIEW)

static void xpad_text_view_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec);
static void xpad_text_view_get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec);
static void xpad_text_view_constructed (GObject *object);
static void xpad_text_view_dispose (GObject *object);
static void xpad_text_view_finalize (GObject *object);
static void xpad_text_view_realize (XpadTextView *widget);
static gboolean xpad_text_view_button_press_event (GtkWidget *widget, GdkEventButton *event, XpadSettings *settings);
static gboolean xpad_text_view_focus_out_event (GtkWidget *widget, GdkEventFocus *event, XpadSettings *settings);
static void xpad_text_view_notify_edit_lock (XpadTextView *view);
static void xpad_text_view_notify_editable (XpadTextView *view);
static void xpad_text_view_notify_fontname (XpadTextView *view);
static void xpad_text_view_notify_colors (XpadTextView *view);
static void xpad_text_view_notify_line_numbering (XpadTextView *view);

enum
{
	PROP_0,
	PROP_SETTINGS,
	PROP_PAD,
	PROP_FOLLOW_FONT_STYLE,
	PROP_FOLLOW_COLOR_STYLE,
	N_PROPERTIES
};

static GParamSpec *obj_prop[N_PROPERTIES] = { NULL, };

GtkWidget *
xpad_text_view_new (XpadSettings *settings, XpadPad *pad)
{
	return GTK_WIDGET (g_object_new (XPAD_TYPE_TEXT_VIEW, "settings", settings, "pad", pad, "follow-font-style", TRUE, "follow-color-style", TRUE, NULL));
}

static void
xpad_text_view_class_init (XpadTextViewClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

	gobject_class->constructed = xpad_text_view_constructed;
	gobject_class->dispose = xpad_text_view_dispose;
	gobject_class->finalize = xpad_text_view_finalize;
	gobject_class->set_property = xpad_text_view_set_property;
	gobject_class->get_property = xpad_text_view_get_property;

	obj_prop[PROP_SETTINGS] = g_param_spec_pointer ("settings", "Xpad settings", "Xpad global settings", G_PARAM_READWRITE | G_PARAM_CONSTRUCT);
	obj_prop[PROP_PAD] = g_param_spec_pointer ("pad", "Pad", "Pad connected to this textview", G_PARAM_READWRITE | G_PARAM_CONSTRUCT);
	obj_prop[PROP_FOLLOW_FONT_STYLE] = g_param_spec_boolean ("follow-font-style", "Follow font style", "Whether to use the default xpad font style", TRUE, G_PARAM_READWRITE | G_PARAM_CONSTRUCT);
	obj_prop[PROP_FOLLOW_COLOR_STYLE] = g_param_spec_boolean ("follow-color-style", "Follow color style", "Whether to use the default xpad color style", TRUE, G_PARAM_READWRITE | G_PARAM_CONSTRUCT);

	g_object_class_install_properties (gobject_class, N_PROPERTIES, obj_prop);
}

static void
xpad_text_view_init (XpadTextView *view)
{
	view->priv = xpad_text_view_get_instance_private (view);
}

static void
xpad_text_view_constructed (GObject *object)
{
	XpadTextView *view = XPAD_TEXT_VIEW (object);

	view->priv->buffer = xpad_text_buffer_new (view->priv->pad);

	gtk_text_view_set_buffer (GTK_TEXT_VIEW (view), GTK_TEXT_BUFFER (view->priv->buffer));
	gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (view), GTK_WRAP_WORD);
	gtk_text_view_set_top_margin (GTK_TEXT_VIEW (view), 5);
	gtk_text_view_set_bottom_margin (GTK_TEXT_VIEW (view), 5);
	gtk_text_view_set_left_margin (GTK_TEXT_VIEW (view), 5);
	gtk_text_view_set_right_margin (GTK_TEXT_VIEW (view), 5);

	gchar *widget_name = g_strdup_printf ("%p", (void *) view);
	gtk_widget_set_name (GTK_WIDGET (view), widget_name);
	g_free (widget_name);
	xpad_text_view_notify_line_numbering(view);

	/* Add CSS style class, so the styling can be overridden by a GTK theme */
	GtkStyleContext *context = gtk_widget_get_style_context(GTK_WIDGET (view));
	gtk_style_context_add_class(context, "XpadTextView");

	/* Signals */
	g_signal_connect (view, "button-press-event", G_CALLBACK (xpad_text_view_button_press_event), view->priv->settings);
	g_signal_connect_after (view, "focus-out-event", G_CALLBACK (xpad_text_view_focus_out_event), view->priv->settings);
	g_signal_connect (view, "realize", G_CALLBACK (xpad_text_view_realize), NULL);
	g_signal_connect (view, "notify::editable", G_CALLBACK (xpad_text_view_notify_editable), NULL);
	g_signal_connect_swapped (view->priv->settings, "notify::edit-lock", G_CALLBACK (xpad_text_view_notify_edit_lock), view);
	g_signal_connect_swapped (view->priv->settings, "notify::line-numbering", G_CALLBACK (xpad_text_view_notify_line_numbering), view);

	view->priv->notify_font_handler = g_signal_connect_swapped (view->priv->settings, "notify::fontname", G_CALLBACK (xpad_text_view_notify_fontname), view);
	view->priv->notify_text_handler = g_signal_connect_swapped (view->priv->settings, "notify::text-color", G_CALLBACK (xpad_text_view_notify_colors), view);
	view->priv->notify_back_handler = g_signal_connect_swapped (view->priv->settings, "notify::back-color", G_CALLBACK (xpad_text_view_notify_colors), view);

	g_signal_handler_block (view->priv->settings, view->priv->notify_font_handler);
}

static void
xpad_text_view_dispose (GObject *object)
{
	XpadTextView *view = XPAD_TEXT_VIEW (object);

	g_clear_object (&view->priv->buffer);
	g_clear_object (&view->priv->pad);
	g_clear_object (&view->priv->settings);
	g_clear_object (&view->priv->font_provider);

	G_OBJECT_CLASS (xpad_text_view_parent_class)->dispose (object);
}

static void
xpad_text_view_finalize (GObject *object)
{
	XpadTextView *view = XPAD_TEXT_VIEW (object);

	if (view->priv->settings)
		g_signal_handlers_disconnect_matched (view->priv->settings, G_SIGNAL_MATCH_DATA, 0, 0, NULL, NULL, view);

	G_OBJECT_CLASS (xpad_text_view_parent_class)->finalize (object);
}

static void
xpad_text_view_realize (XpadTextView *view)
{
	gboolean edit_lock;
	g_object_get (view->priv->settings, "edit-lock", &edit_lock, NULL);
	gtk_text_view_set_editable (GTK_TEXT_VIEW (view), !edit_lock);
}

static void
xpad_text_view_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
	XpadTextView *view = XPAD_TEXT_VIEW (object);

	switch (prop_id)
	{
	case PROP_SETTINGS:
		view->priv->settings = g_value_get_pointer (value);
		g_object_ref (view->priv->settings);
		break;

	case PROP_PAD:
		view->priv->pad = g_value_get_pointer (value);
		g_object_ref (view->priv->pad);
		break;

	case PROP_FOLLOW_FONT_STYLE:
		view->priv->follow_font_style = g_value_get_boolean (value);
		if (view->priv->follow_font_style) {
			xpad_text_view_notify_fontname (view);
			if (view->priv->notify_font_handler != 0) {
				g_signal_handler_unblock (view->priv->settings, view->priv->notify_font_handler);
			}
		}
		else
			g_signal_handler_block (view->priv->settings, view->priv->notify_font_handler);
		break;

	case PROP_FOLLOW_COLOR_STYLE:
		view->priv->follow_color_style = g_value_get_boolean (value);
		xpad_text_view_notify_colors (view);
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
xpad_text_view_get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
	XpadTextView *view = XPAD_TEXT_VIEW (object);

	switch (prop_id)
	{
	case PROP_SETTINGS:
		g_value_set_pointer (value, view->priv->settings);
		break;

	case PROP_PAD:
		g_value_set_pointer (value, view->priv->pad);
		break;

	case PROP_FOLLOW_FONT_STYLE:
		g_value_set_boolean (value, view->priv->follow_font_style);
		break;

	case PROP_FOLLOW_COLOR_STYLE:
		g_value_set_boolean (value, view->priv->follow_color_style);
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static gboolean
xpad_text_view_focus_out_event (GtkWidget *widget, GdkEventFocus *event, XpadSettings *settings)
{
	/* A dirty way to silence the compiler for these unused variables. */
	(void) event;

	gboolean edit_lock;
	g_object_get (settings, "edit-lock", &edit_lock, NULL);

	if (edit_lock)
	{
		gtk_text_view_set_editable (GTK_TEXT_VIEW (widget), FALSE);
		return TRUE;
	}

	return FALSE;
}

static gboolean
xpad_text_view_button_press_event (GtkWidget *widget, GdkEventButton *event, XpadSettings *settings)
{
	gboolean edit_lock;
	g_object_get (settings, "edit-lock", &edit_lock, NULL);

	if (event->button == 1 &&
	    edit_lock &&
	    !gtk_text_view_get_editable (GTK_TEXT_VIEW (widget)))
	{
		if (event->type == GDK_2BUTTON_PRESS)
		{
			gtk_text_view_set_editable (GTK_TEXT_VIEW (widget), TRUE);
			return TRUE;
		}
		else if (event->type == GDK_BUTTON_PRESS)
		{
			gtk_window_begin_move_drag (GTK_WINDOW (gtk_widget_get_toplevel (widget)), (gint) event->button, (gint) event->x_root, (gint) event->y_root, event->time);
			return TRUE;
		}
	}

	return FALSE;
}

static void
xpad_text_view_notify_edit_lock (XpadTextView *view)
{
	/* chances are good that they don't have the text view focused while it changed, so make non-editable if edit lock turned on */
	gboolean edit_lock;
	g_object_get (view->priv->settings, "edit-lock", &edit_lock, NULL);
	gtk_text_view_set_editable (GTK_TEXT_VIEW (view), !edit_lock);
}

static void
xpad_text_view_notify_editable (XpadTextView *view)
{
	GdkCursor *cursor;
	gboolean editable;
	GdkDisplay *display;
	GdkWindow *view_window;
	GtkSourceView *view_tv = GTK_SOURCE_VIEW (view);

	editable = gtk_text_view_get_editable (GTK_TEXT_VIEW (view_tv));
	gtk_text_view_set_cursor_visible (GTK_TEXT_VIEW (view_tv), editable);

	view_window = gtk_text_view_get_window (GTK_TEXT_VIEW (view_tv), GTK_TEXT_WINDOW_TEXT);
	display = gdk_window_get_display(view_window);
	cursor = editable ? gdk_cursor_new_for_display (display, GDK_XTERM) : NULL;

	/* Only set for pads which are currently visible */
	if (view_window != NULL)
		gdk_window_set_cursor (view_window, cursor);

	g_clear_object (&cursor);
}

static void
xpad_text_view_notify_fontname (XpadTextView *view)
{
	gchar *font;
	g_object_get (view->priv->settings, "fontname", &font, NULL);

	PangoFontDescription *fontdesc = font ? pango_font_description_from_string (font) : NULL;
	xpad_text_view_set_font (GTK_WIDGET (view), fontdesc);
	if (fontdesc)
		pango_font_description_free (fontdesc);

	g_free (font);
}

/* Update the colors of the textview */
static void
xpad_text_view_notify_colors (XpadTextView *view)
{
	if (view->priv->follow_color_style) {
		/* Set the colors of this individual pad to the global setting preference. */
		GdkRGBA *text_color, *back_color;

		GtkWidget *view_widget = GTK_WIDGET (view);

		/* Set the colors to the global preferences colors */
		g_object_get (view->priv->settings, "text-color", &text_color, "back-color", &back_color, NULL);
		xpad_text_view_set_colors(view_widget, text_color, back_color);
		gdk_rgba_free (text_color);
		gdk_rgba_free (back_color);
	}
}

/* Set the foreground and background color of the visible part of the pad, which is the text view */
void
xpad_text_view_set_colors (GtkWidget *view, GdkRGBA *text_color, GdkRGBA *back_color) {
	gchar *text_color_string = text_color ? gdk_rgba_to_string (text_color) : "@theme-fg_color";
	gchar *back_color_string = back_color ? gdk_rgba_to_string (back_color) : "@theme_bg_color";

	gchar *cssStyling = g_strconcat(
			"textview, textview text {caret-color: ", text_color_string,
			"; color: ", text_color_string,
			"; background-color: ", back_color_string,
			";}\n", NULL);

	if (text_color) {
		g_free (text_color_string);
	}

	if (back_color) {
		g_free (back_color_string);
	}

	/*
	 * TODO: If the background color is close to the text selection background color (blue-ish),
	 * then change the color for selected text by adding this to the CSS.
	 * */

	GtkStyleContext *context = gtk_widget_get_style_context (view);
	GtkCssProvider *provider = gtk_css_provider_new ();
	gtk_css_provider_load_from_data (provider, cssStyling, -1, NULL);
	gtk_style_context_add_provider (context, GTK_STYLE_PROVIDER (provider), GTK_STYLE_PROVIDER_PRIORITY_SETTINGS);

	g_free(cssStyling);
	g_clear_object (&provider);
}

/* Set the font of the pad, which is in the text view */
void
xpad_text_view_set_font (GtkWidget *view, PangoFontDescription *desc) {
	GtkStyleContext *context = gtk_widget_get_style_context (view);

	if (desc == NULL && XPAD_TEXT_VIEW (view)->priv->font_provider) {
		/* Remove font provider */
		gtk_style_context_remove_provider (context, GTK_STYLE_PROVIDER (XPAD_TEXT_VIEW (view)->priv->font_provider));
		g_clear_object (&XPAD_TEXT_VIEW (view)->priv->font_provider);
	} else {
		/* Add/replace font provider */
		gchar *font_description;
		gchar *cssStyling;

		font_description = pango_font_description_to_css(desc);
		cssStyling = g_strconcat("textview, textview text ", font_description, "\n", NULL);
		g_free (font_description);

		if (XPAD_TEXT_VIEW (view)->priv->font_provider) {
			gtk_style_context_remove_provider (context, GTK_STYLE_PROVIDER (XPAD_TEXT_VIEW (view)->priv->font_provider));
			g_clear_object (&XPAD_TEXT_VIEW (view)->priv->font_provider);
		}

		GtkCssProvider *provider = gtk_css_provider_new ();
		gtk_css_provider_load_from_data (provider, cssStyling, -1, NULL);
		gtk_style_context_add_provider (context, GTK_STYLE_PROVIDER (provider), GTK_STYLE_PROVIDER_PRIORITY_SETTINGS);
		XPAD_TEXT_VIEW (view)->priv->font_provider = provider;

		g_free (cssStyling);
	}
}

static void
xpad_text_view_notify_line_numbering (XpadTextView *view)
{
	gboolean line_numbering;
	g_object_get (view->priv->settings, "line-numbering", &line_numbering, NULL);
	gtk_source_view_set_show_line_numbers (GTK_SOURCE_VIEW (view), line_numbering);
}

