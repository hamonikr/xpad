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

#ifndef __XPAD_SEARCH_BAR_H__
#define __XPAD_SEARCH_BAR_H__

#include <gtk/gtk.h>
#include <gtksourceview/gtksource.h>

G_BEGIN_DECLS

#define XPAD_TYPE_SEARCH_BAR          (xpad_search_bar_get_type ())
#define XPAD_SEARCH_BAR(o)            (G_TYPE_CHECK_INSTANCE_CAST ((o), XPAD_TYPE_SEARCH_BAR, XpadSearchBar))
#define XPAD_SEARCH_BAR_CLASS(k)      (G_TYPE_CHECK_CLASS_CAST((k), XPAD_TYPE_SEARCH_BAR, XpadSearchBarClass))
#define XPAD_IS_SEARCH_BAR(o)         (G_TYPE_CHECK_INSTANCE_TYPE ((o), XPAD_TYPE_SEARCH_BAR))
#define XPAD_IS_SEARCH_BAR_CLASS(k)   (G_TYPE_CHECK_CLASS_TYPE ((k), XPAD_TYPE_SEARCH_BAR))
#define XPAD_SEARCH_BAR_GET_CLASS(o)  (G_TYPE_INSTANCE_GET_CLASS ((o), XPAD_TYPE_SEARCH_BAR, XpadSearchBarClass))

typedef struct XpadSearchBarClass XpadSearchBarClass;
typedef struct XpadSearchBarPrivate XpadSearchBarPrivate;
typedef struct XpadSearchBar XpadSearchBar;

struct XpadSearchBar
{
	GtkSearchBar parent;
	XpadSearchBarPrivate *priv;
};

struct XpadSearchBarClass
{
	GtkSearchBarClass parent_class;
};

GType xpad_search_bar_get_type (void);

XpadSearchBar *xpad_search_bar_new (GtkSourceBuffer *bufer);

void xpad_search_bar_show (XpadSearchBar *searchbar);
void xpad_search_bar_hide (XpadSearchBar *searchbar);

G_END_DECLS

#endif /* __XPAD_SEARCH_BAR_H__ */
