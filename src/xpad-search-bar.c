/*

Copyright (c) 2001-2007 Michael Terry
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

#include <gtk/gtk.h>
#include <gtksourceview/gtksource.h>

#include "xpad-search-bar.h"

struct XpadSearchBarPrivate
{
	GtkSourceBuffer *source_buffer;
	GtkSearchEntry *search_entry;
	GtkSourceSearchContext *search_context;
};

G_DEFINE_TYPE_WITH_PRIVATE (XpadSearchBar, xpad_search_bar, GTK_TYPE_SEARCH_BAR)

enum
{
	PROP_0,
	PROP_SOURCE_BUFFER,
	LAST_PROP
};

const gchar *EMPTY_STRING = "";

static void xpad_search_bar_constructed (GObject *object);
static void xpad_search_bar_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec);
static void xpad_search_bar_get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec);
static void xpad_search_bar_dispose (GObject *object);
static void xpad_search_bar_finalize (GObject *object);
static void xpad_search_bar_previous_match_cb(XpadSearchBar *searchbar);
static void xpad_search_bar_next_match_cb(XpadSearchBar *searchbar);
static void xpad_search_bar_search_changed_cb(XpadSearchBar *searchbar);
static void xpad_search_bar_stop_search_cb(XpadSearchBar *searchbar);
static void reset_search_string(XpadSearchBar *searchbar, const gchar *search_string);
static void set_not_found_state (XpadSearchBar *searchbar, gboolean is_search_string_not_found);

XpadSearchBar *
xpad_search_bar_new (GtkSourceBuffer *buffer)
{
	XpadSearchBar *searchbar = g_object_new (XPAD_TYPE_SEARCH_BAR, "source_buffer", buffer, NULL);
	return searchbar;
}

void
xpad_search_bar_show (XpadSearchBar *searchbar)
{
	reset_search_string(searchbar, EMPTY_STRING);
	gtk_source_search_context_set_highlight (searchbar->priv->search_context, TRUE);
	gtk_search_bar_set_search_mode (GTK_SEARCH_BAR(searchbar), TRUE);
}

void
xpad_search_bar_hide (XpadSearchBar *searchbar)
{
	reset_search_string(searchbar, EMPTY_STRING);
	gtk_source_search_context_set_highlight (searchbar->priv->search_context, FALSE);
	gtk_search_bar_set_search_mode (GTK_SEARCH_BAR(searchbar), FALSE);
}

static void
xpad_search_bar_class_init (XpadSearchBarClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

	gobject_class->constructed = xpad_search_bar_constructed;
	gobject_class->set_property = xpad_search_bar_set_property;
	gobject_class->get_property = xpad_search_bar_get_property;
	gobject_class->dispose = xpad_search_bar_dispose;
	gobject_class->finalize = xpad_search_bar_finalize;

	g_object_class_install_property (gobject_class,
					 PROP_SOURCE_BUFFER,
					 g_param_spec_pointer ("source_buffer",
									"GtkSourceBuffer",
									"GtkSourceBuffer associated with this search bar",
									 G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
}


static void
xpad_search_bar_init (XpadSearchBar *searchbar)
{
	searchbar->priv = xpad_search_bar_get_instance_private (searchbar);
	searchbar->priv->search_entry = NULL;
	searchbar->priv->search_context = NULL;
}

static void
xpad_search_bar_constructed (GObject *object)
{
	XpadSearchBar *searchbar = XPAD_SEARCH_BAR (object);

	/* Horizontal box with search_entry and up/down buttons */
	GtkBox *hbox = GTK_BOX (gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0));
	gtk_box_set_homogeneous (hbox, FALSE);
	gtk_container_add (GTK_CONTAINER (searchbar), GTK_WIDGET (hbox));

	/* search_entry */
	searchbar->priv->search_entry = GTK_SEARCH_ENTRY (gtk_search_entry_new ());
	gtk_box_pack_start (hbox, GTK_WIDGET (searchbar->priv->search_entry), TRUE, TRUE, 0);

	/* previous match button */
	GtkButton *prev_search_btn = GTK_BUTTON (gtk_button_new_from_icon_name ("go-up-symbolic", GTK_ICON_SIZE_SMALL_TOOLBAR));
	gtk_box_pack_start (hbox, GTK_WIDGET (prev_search_btn), FALSE, FALSE, 0);

	/* next match button */
	GtkButton *forward_search_btn = GTK_BUTTON (gtk_button_new_from_icon_name ("go-down-symbolic", GTK_ICON_SIZE_SMALL_TOOLBAR));
	gtk_box_pack_start (hbox, GTK_WIDGET (forward_search_btn), FALSE, FALSE, 0);

	/*
	 * this will allow to for example press Esc without moving the mouse exactly
	 * Sinto the search field to remove the search bar
	 */
	gtk_search_bar_connect_entry (GTK_SEARCH_BAR (searchbar), GTK_ENTRY (searchbar->priv->search_entry));

	/* connecting to GtkSearchEntry signals for actual searching */
	g_signal_connect_swapped (searchbar->priv->search_entry, "stop-search", G_CALLBACK (xpad_search_bar_stop_search_cb), searchbar);
	g_signal_connect_swapped (searchbar->priv->search_entry, "search-changed", G_CALLBACK (xpad_search_bar_search_changed_cb), searchbar);
	g_signal_connect_swapped (searchbar->priv->search_entry, "previous-match", G_CALLBACK (xpad_search_bar_previous_match_cb), searchbar);
	g_signal_connect_swapped (searchbar->priv->search_entry, "next-match", G_CALLBACK (xpad_search_bar_next_match_cb), searchbar);
	g_signal_connect_swapped (searchbar->priv->search_entry, "activate", G_CALLBACK (xpad_search_bar_next_match_cb), searchbar);
	g_signal_connect_swapped (prev_search_btn, "clicked", G_CALLBACK (xpad_search_bar_previous_match_cb), searchbar);
	g_signal_connect_swapped (forward_search_btn, "clicked", G_CALLBACK (xpad_search_bar_next_match_cb), searchbar);

	/*
	 * also allow to close by a button on a searchbar (which is implemented in GtkSearchBar itself)
	 * (in gedit search entry dissappears when cursor is moved to text buffer, choosing not to do
	 * the same since it is actually irritating, in case you want to search something, change something,
	 * search again for the same thing. Not adding search and replace since xpad is not a full-blown
	 * text editor, but not hiding search entry when cursor is moved to text buffer is at least a
	 * step in this direction
	 */
	gtk_search_bar_set_show_close_button (GTK_SEARCH_BAR(searchbar), TRUE);

	/* Searchbar should appear in right-top corner of the overlay with text
	 * to not cover the text which is searched (assuming that first
	 * several lines most probably are short enough to not be covered)
	 */
	gtk_widget_set_halign (GTK_WIDGET (searchbar), GTK_ALIGN_END);
	gtk_widget_set_valign (GTK_WIDGET (searchbar), GTK_ALIGN_START);

	/* Create GtkSearchContext with source buffer as parameter, this will be used for the actual search */
	searchbar->priv->search_context = gtk_source_search_context_new (searchbar->priv->source_buffer, NULL);

	GtkSourceSearchSettings *settings = gtk_source_search_context_get_settings (searchbar->priv->search_context);

	/* Setting wrap-around to true to ease life. This means that next/prev buttons in searchbar
	 * don't need to be greyed out in case we've reached end of document with forward search
	 * (or beginnig with backward search), they will just wrap arround. If pressing enter
	 * in search entry (which is set to forward search) or pressing next/prev buttons in searchbar
	 * will not find anything we then highlight search entry by red background. This is
	 * similar to gedit where search entry is highlighted in the same way but not after
	 * enter/next/prev are pressed but with complicated logic to do it on the fly (as buffer is
	 * scanned asynchronously there may be a situation when gtk_source_search_context_get_occurrences_count
	 * does not return actual count since it is not counted yet and then they wait for several amount
	 * of time to retry). To avoid this complex logic, wrap-around is set to true to never grey out next/prev
	 * buttons and search entry is highlighted red only if enter/next/prev was pressed and nothing was found
	 * (any change inside search entry will remove this highlight, same for initiating another search)
	 */
	gtk_source_search_settings_set_wrap_around (settings, TRUE);
}

static void
xpad_search_bar_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
	XpadSearchBar *searchbar = XPAD_SEARCH_BAR (object);

	switch (prop_id)
	{
	case PROP_SOURCE_BUFFER:
		if (G_VALUE_HOLDS_POINTER (value) && G_IS_OBJECT (g_value_get_pointer (value))) {
			g_clear_object (&searchbar->priv->source_buffer);
			searchbar->priv->source_buffer = g_value_get_pointer (value);
			g_object_ref (searchbar->priv->source_buffer);
		}

		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	 }
}

static void
xpad_search_bar_get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
	XpadSearchBar *searchbar = XPAD_SEARCH_BAR (object);

	switch (prop_id)
	{
	case PROP_SOURCE_BUFFER:
		g_value_set_pointer (value, searchbar->priv->source_buffer);
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
xpad_search_bar_dispose (GObject *object)
{
	XpadSearchBar *searchbar = XPAD_SEARCH_BAR (object);

	g_clear_object (&searchbar->priv->source_buffer);
	g_clear_object (&searchbar->priv->search_context);

	G_OBJECT_CLASS (xpad_search_bar_parent_class)->dispose (object);
}

static void
xpad_search_bar_finalize (GObject *object)
{
	G_OBJECT_CLASS (xpad_search_bar_parent_class)->finalize (object);
}

static void
xpad_search_bar_previous_match_cb(XpadSearchBar *searchbar)
{
	/*
	 * reset error highlighting because the text which was not found
	 * might have just appeared (user might have enterd it)
	 */
	set_not_found_state (searchbar, FALSE);

	GtkTextBuffer *buffer = GTK_TEXT_BUFFER (searchbar->priv->source_buffer);

	/*
	 * See comment in xpad_search_bar_next_match_cb, this is exactly the same
	 * but as we are searching backwards this time we look for the beginning
	 * of selection (or in case there is no selection - for the position of cursor
	 * which is equal to both beginning and end of selection)
	 */
	GtkTextIter start;
	gtk_text_buffer_get_selection_bounds (buffer, &start, NULL);

	GtkTextIter match_start;
	GtkTextIter match_end;
	gboolean *has_wrapped_around = FALSE;
	gboolean found = gtk_source_search_context_backward (searchbar->priv->search_context, &start, &match_start, &match_end, has_wrapped_around);

	if (found)
	{
		gtk_text_buffer_select_range (GTK_TEXT_BUFFER (buffer),
						  &match_start,
						  &match_end);
	}
	else
	{
		set_not_found_state (searchbar, TRUE);
	}
}

static void
xpad_search_bar_next_match_cb(XpadSearchBar *searchbar)
{
	/* reset error highlighting because the text which was not found
	 * might have just appeared (user might have enterd it)
	 */
	set_not_found_state (searchbar, FALSE);

	GtkTextBuffer *buffer = GTK_TEXT_BUFFER (searchbar->priv->source_buffer);

	/* In case something was selected by ongoing search (pressing next -> next -> next
	 * start will point to the end of selection and next search will start from there,
	 * otherwise (user has moved cursor away from search entry into the text buffer),
	 * start will then point to the exact location of the cursor and next search will
	 * begin from there. This allows to handle situation when user changes text, goes
	 * back to search entry, searches several times, again changes text and so on
	 */
	GtkTextIter start;
	gtk_text_buffer_get_selection_bounds (buffer, NULL, &start);

	GtkTextIter match_start;
	GtkTextIter match_end;
	gboolean *has_wrapped_around = FALSE;
	gboolean found = gtk_source_search_context_forward (searchbar->priv->search_context, &start, &match_start, &match_end, has_wrapped_around);

	if (found)
	{
		gtk_text_buffer_select_range (GTK_TEXT_BUFFER (buffer),
						  &match_start,
						  &match_end);
	}
	else
	{
		set_not_found_state (searchbar, TRUE);
	}
}

static void
xpad_search_bar_search_changed_cb(XpadSearchBar *searchbar)
{
	const gchar *text_to_search = gtk_entry_get_text (GTK_ENTRY (searchbar->priv->search_entry));
	gchar *unescaped_text_to_search = gtk_source_utils_unescape_search_text (text_to_search);

	reset_search_string (searchbar, unescaped_text_to_search);

	g_free (unescaped_text_to_search);
	gtk_source_search_context_set_highlight (searchbar->priv->search_context, TRUE);
}

static void
xpad_search_bar_stop_search_cb(XpadSearchBar *searchbar)
{
	xpad_search_bar_hide (searchbar);
}

static void
reset_search_string(XpadSearchBar *searchbar, const gchar *search_string)
{
	GtkSourceSearchSettings *settings = gtk_source_search_context_get_settings (searchbar->priv->search_context);
	gtk_source_search_settings_set_search_text (settings, search_string);
	set_not_found_state (searchbar, FALSE);
}

static void
set_not_found_state (XpadSearchBar *searchbar, gboolean is_search_string_not_found)
{
	GtkStyleContext *context = gtk_widget_get_style_context (GTK_WIDGET (searchbar->priv->search_entry));

	if (is_search_string_not_found)
	{
		gtk_style_context_add_class (context, GTK_STYLE_CLASS_ERROR);
	}
	else
	{
		gtk_style_context_remove_class (context, GTK_STYLE_CLASS_ERROR);
	}
}
