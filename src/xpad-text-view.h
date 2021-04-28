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

#ifndef __XPAD_TEXT_VIEW_H__
#define __XPAD_TEXT_VIEW_H__

#include <gtksourceview/gtksource.h>

#include "xpad-pad.h"

G_BEGIN_DECLS

#define XPAD_TYPE_TEXT_VIEW          (xpad_text_view_get_type ())
#define XPAD_TEXT_VIEW(o)            (G_TYPE_CHECK_INSTANCE_CAST ((o), XPAD_TYPE_TEXT_VIEW, XpadTextView))
#define XPAD_TEXT_VIEW_CLASS(k)      (G_TYPE_CHECK_CLASS_CAST((k), XPAD_TYPE_TEXT_VIEW, XpadTextViewClass))
#define XPAD_IS_TEXT_VIEW(o)         (G_TYPE_CHECK_INSTANCE_TYPE ((o), XPAD_TYPE_TEXT_VIEW))
#define XPAD_IS_TEXT_VIEW_CLASS(k)   (G_TYPE_CHECK_CLASS_TYPE ((k), XPAD_TYPE_TEXT_VIEW))
#define XPAD_TEXT_VIEW_GET_CLASS(o)  (G_TYPE_INSTANCE_GET_CLASS ((o), XPAD_TYPE_TEXT_VIEW, XpadTextViewClass))

typedef struct XpadTextViewClass XpadTextViewClass;
typedef struct XpadTextViewPrivate XpadTextViewPrivate;
typedef struct XpadTextView XpadTextView;

struct XpadTextView
{
	GtkSourceView parent;
	XpadTextViewPrivate *priv;
};

struct XpadTextViewClass
{
	GtkSourceViewClass parent_class;
};

GType xpad_text_view_get_type (void);

GtkWidget *xpad_text_view_new (XpadSettings *settings, XpadPad *pad);
void xpad_text_view_set_colors (GtkWidget *view, const GdkRGBA *text_color, const GdkRGBA *back_color);
void xpad_text_view_set_font (GtkWidget *view, PangoFontDescription *desc);

G_END_DECLS

#endif /* __XPAD_TEXT_VIEW_H__ */
