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

#include "xpad-grip-tool-item.h"

struct XpadGripToolItemPrivate
{
	GtkWidget *drawbox;
};

G_DEFINE_TYPE_WITH_PRIVATE(XpadGripToolItem, xpad_grip_tool_item, GTK_TYPE_TOOL_ITEM)

static gboolean xpad_grip_tool_item_event_box_draw (GtkWidget *widget, cairo_t *cr);
static void xpad_grip_tool_item_event_box_realize (GtkWidget *widget);
static gboolean xpad_grip_tool_item_button_pressed_event (GtkWidget *widget, GdkEventButton *event);

GtkToolItem *
xpad_grip_tool_item_new (void)
{
	return GTK_TOOL_ITEM (g_object_new (XPAD_TYPE_GRIP_TOOL_ITEM, NULL));
}

static void
xpad_grip_tool_item_class_init (XpadGripToolItemClass *klass)
{
}

static void
xpad_grip_tool_item_init (XpadGripToolItem *grip)
{
	/* Alignment of the grip tool has been removed, first since the function is deprecated and second,
	   it does not seem to make any difference. If it does, the old code can be enabled. Remove these
	   comments and the commented code in after March 2016 */

	/* GtkWidget *alignment; */
	/* gboolean right; */

	grip->priv = xpad_grip_tool_item_get_instance_private(grip);

	grip->priv->drawbox = gtk_drawing_area_new ();
	gtk_widget_add_events (grip->priv->drawbox, GDK_BUTTON_PRESS_MASK | GDK_EXPOSURE_MASK);
	g_signal_connect (grip->priv->drawbox, "button-press-event", G_CALLBACK (xpad_grip_tool_item_button_pressed_event), NULL);
	g_signal_connect (grip->priv->drawbox, "realize", G_CALLBACK (xpad_grip_tool_item_event_box_realize), NULL);
	g_signal_connect (grip->priv->drawbox, "draw", G_CALLBACK (xpad_grip_tool_item_event_box_draw), NULL);
	gtk_widget_set_size_request (grip->priv->drawbox, 18, 18);

	/* right = gtk_widget_get_direction (grip->priv->drawbox) == GTK_TEXT_DIR_LTR; */
	/* alignment = gtk_alignment_new (right ? 1 : 0, 1, 0, 0); */

	/* gtk_container_add (GTK_CONTAINER (alignment), grip->priv->drawbox); */
	/* gtk_container_add (GTK_CONTAINER (grip), alignment); */
	gtk_container_add (GTK_CONTAINER (grip), grip->priv->drawbox);
}

static gboolean
xpad_grip_tool_item_button_pressed_event (GtkWidget *widget, GdkEventButton *event)
{
	if (event->button == 1)
	{
		GdkWindowEdge edge;

		if (gtk_widget_get_direction (widget) == GTK_TEXT_DIR_LTR)
			edge = GDK_WINDOW_EDGE_SOUTH_EAST;
		else
			edge = GDK_WINDOW_EDGE_SOUTH_WEST;

		gtk_window_begin_resize_drag (GTK_WINDOW (gtk_widget_get_toplevel (widget)),
			edge, (gint) event->button, (gint) event->x_root, (gint) event->y_root, event->time);

		return TRUE;
	}

	return FALSE;
}

static void
xpad_grip_tool_item_event_box_realize (GtkWidget *widget)
{
	GdkDisplay *display = gtk_widget_get_display (widget);
	GdkCursorType cursor_type;
	GdkCursor *cursor;

	if (gtk_widget_get_direction (widget) == GTK_TEXT_DIR_LTR)
		cursor_type = GDK_BOTTOM_RIGHT_CORNER;
	else
		cursor_type = GDK_BOTTOM_LEFT_CORNER;

	cursor = gdk_cursor_new_for_display (display, cursor_type);
	gdk_window_set_cursor (gtk_widget_get_window(widget), cursor);
	g_clear_object (&cursor);
}

static gboolean
xpad_grip_tool_item_event_box_draw (GtkWidget *widget, cairo_t *cr)
{
	gtk_render_handle(gtk_widget_get_style_context(widget),
			cr,
			0, 0,
			gtk_widget_get_allocated_width(widget),
			gtk_widget_get_allocated_width(widget));
	
	return FALSE;
}
