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

#ifndef __XPAD_STYLING_HELPERS_H__
#define __XPAD_STYLING_HELPERS_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

const GdkRGBA *get_theme_text_color();
const GdkRGBA *get_theme_back_color();
void get_background_color (GtkStyleContext *context, GtkStateFlags state, GdkRGBA *color);
gchar * pango_font_description_to_css (PangoFontDescription *desc);

G_END_DECLS

#endif /* __XPAD_STYLING_HELPERS_H__*/
