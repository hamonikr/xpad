/*

Copyright (c) 2017-2022 Arthur Borsboom

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

void get_background_color (GtkStyleContext *context, GtkStateFlags state, GdkRGBA *color);

/* Replacement of gtk_style_context_get_background_color with local static
 * function get_background_color that does exactly same thing. */
void get_background_color (GtkStyleContext *context, GtkStateFlags state, GdkRGBA *color) {
	GdkRGBA *c;

	g_return_if_fail (color != NULL);
	g_return_if_fail (GTK_IS_STYLE_CONTEXT (context));

	gtk_style_context_get (context,
			state,
			"background-color", &c,
			NULL);

	*color = *c;
	gdk_rgba_free (c);
}

/*
 * This function is originally written in gtk/gtkfontbutton.c of gtk+ project.
 */
gchar * pango_font_description_to_css (PangoFontDescription *desc) {
	GString *s;
	PangoFontMask set;

	s = g_string_new ("{ ");

	if (desc != NULL) {
		set = pango_font_description_get_set_fields (desc);

		if (set & PANGO_FONT_MASK_FAMILY) {
		  g_string_append (s, "font-family: ");
		  g_string_append (s, pango_font_description_get_family (desc));
		  g_string_append (s, "; ");
		}

		if (set & PANGO_FONT_MASK_STYLE) {
		  switch (pango_font_description_get_style (desc))
			{
			case PANGO_STYLE_NORMAL:
			  g_string_append (s, "font-style: normal; ");
			  break;
			case PANGO_STYLE_OBLIQUE:
			  g_string_append (s, "font-style: oblique; ");
			  break;
			case PANGO_STYLE_ITALIC:
			  g_string_append (s, "font-style: italic; ");
			  break;
			}
		}

		if (set & PANGO_FONT_MASK_VARIANT) {
		  switch (pango_font_description_get_variant (desc))
			{
			case PANGO_VARIANT_NORMAL:
			  g_string_append (s, "font-variant: normal; ");
			  break;
			case PANGO_VARIANT_SMALL_CAPS:
			  g_string_append (s, "font-variant: small-caps; ");
			  break;
			}
		}

		if (set & PANGO_FONT_MASK_WEIGHT) {
		  switch (pango_font_description_get_weight (desc))
			{
			case PANGO_WEIGHT_THIN:
			  g_string_append (s, "font-weight: 100; ");
			  break;
			case PANGO_WEIGHT_ULTRALIGHT:
			  g_string_append (s, "font-weight: 200; ");
			  break;
			case PANGO_WEIGHT_LIGHT:
			case PANGO_WEIGHT_SEMILIGHT:
			  g_string_append (s, "font-weight: 300; ");
			  break;
			case PANGO_WEIGHT_BOOK:
			case PANGO_WEIGHT_NORMAL:
			  g_string_append (s, "font-weight: 400; ");
			  break;
			case PANGO_WEIGHT_MEDIUM:
			  g_string_append (s, "font-weight: 500; ");
			  break;
			case PANGO_WEIGHT_SEMIBOLD:
			  g_string_append (s, "font-weight: 600; ");
			  break;
			case PANGO_WEIGHT_BOLD:
			  g_string_append (s, "font-weight: 700; ");
			  break;
			case PANGO_WEIGHT_ULTRABOLD:
			  g_string_append (s, "font-weight: 800; ");
			  break;
			case PANGO_WEIGHT_HEAVY:
			case PANGO_WEIGHT_ULTRAHEAVY:
			  g_string_append (s, "font-weight: 900; ");
			  break;
			}
		}

		if (set & PANGO_FONT_MASK_STRETCH) {
		  switch (pango_font_description_get_stretch (desc))
			{
			case PANGO_STRETCH_ULTRA_CONDENSED:
			  g_string_append (s, "font-stretch: ultra-condensed; ");
			  break;
			case PANGO_STRETCH_EXTRA_CONDENSED:
			  g_string_append (s, "font-stretch: extra-condensed; ");
			  break;
			case PANGO_STRETCH_CONDENSED:
			  g_string_append (s, "font-stretch: condensed; ");
			  break;
			case PANGO_STRETCH_SEMI_CONDENSED:
			  g_string_append (s, "font-stretch: semi-condensed; ");
			  break;
			case PANGO_STRETCH_NORMAL:
			  g_string_append (s, "font-stretch: normal; ");
			  break;
			case PANGO_STRETCH_SEMI_EXPANDED:
			  g_string_append (s, "font-stretch: semi-expanded; ");
			  break;
			case PANGO_STRETCH_EXPANDED:
			  g_string_append (s, "font-stretch: expanded; ");
			  break;
			case PANGO_STRETCH_EXTRA_EXPANDED:
			  g_string_append (s, "font-stretch: extra-expanded; ");
			  break;
			case PANGO_STRETCH_ULTRA_EXPANDED:
			  g_string_append (s, "font-stretch: ultra-expanded; ");
			  break;
			}
		}

		if (set & PANGO_FONT_MASK_SIZE) {
			g_string_append_printf (s, "font-size: %dpt;", pango_font_description_get_size (desc) / PANGO_SCALE);
		}
	}

	g_string_append (s, " }");

	return g_string_free (s, FALSE);
}
