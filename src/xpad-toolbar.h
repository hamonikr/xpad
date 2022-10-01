/*

Copyright (c) 2001-2007 Michael Terry

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

#ifndef __XPAD_TOOLBAR_H__
#define __XPAD_TOOLBAR_H__

#include "xpad-pad.h"

G_BEGIN_DECLS

#define XPAD_TYPE_TOOLBAR          (xpad_toolbar_get_type ())
#define XPAD_TOOLBAR(o)            (G_TYPE_CHECK_INSTANCE_CAST ((o), XPAD_TYPE_TOOLBAR, XpadToolbar))
#define XPAD_TOOLBAR_CLASS(k)      (G_TYPE_CHECK_CLASS_CAST((k), XPAD_TYPE_TOOLBAR, XpadToolbarClass))
#define XPAD_IS_TOOLBAR(o)         (G_TYPE_CHECK_INSTANCE_TYPE ((o), XPAD_TYPE_TOOLBAR))
#define XPAD_IS_TOOLBAR_CLASS(k)   (G_TYPE_CHECK_CLASS_TYPE ((k), XPAD_TYPE_TOOLBAR))
#define XPAD_TOOLBAR_GET_CLASS(o)  (G_TYPE_INSTANCE_GET_CLASS ((o), XPAD_TYPE_TOOLBAR, XpadToolbarClass))

typedef struct XpadToolbarClass XpadToolbarClass;
typedef struct XpadToolbarPrivate XpadToolbarPrivate;
typedef struct XpadToolbar XpadToolbar;

struct XpadToolbar
{
	GtkToolbar parent;
	XpadToolbarPrivate *priv;
};

struct XpadToolbarClass
{
	GtkToolbarClass parent_class;
};

GType xpad_toolbar_get_type (void);

/*
 * TODO: Why don't return XpadToolbar? Just because in GTK API it is like so?
 * (but at least in gedit e.g. fields of XXXPrivate structs are of proper child types
 * and not GtkWidget* everywhere)
 *
 * The only citation I've found on the topic is this one:
 * All of the _new() routines return a GtkWidget*, even though they allocate a subclass; this is for convenience.
 * (c) https://book.huihoo.com/gtk+-gnome-application-development/cha-gtk.html
 *
 * Who's convinience? If you will look at source code of gedit for example
 * all fields declared in a XXXPrivate has proper child type (what is
 * return from GTK _new is casted). This gives much more readability - maybe
 * with the drawback of having to cast things back to GTK_WIDGET to add to
 * a container and such.
 *
 * Anyway in Xpad part of _new functions return GtkWidget* and part
 * return proper pointer types. Also part of XXXPrivate has GtkWidget*
 * and part do not. Since Xpad is not an API I propose to return real
 * type and where is makes sense to store real types in XXXPrivate
 * (if this will result in too many casts, I will instead make everything
 * a GtkWidget for consistency). Currently it is a mess in Xpad (even without
 * arguing with GTK API choices). It should be at least consistent
 */
GtkWidget *xpad_toolbar_new (XpadPad *pad);

void xpad_toolbar_enable_undo_button (XpadToolbar *toolbar, gboolean enable);
void xpad_toolbar_enable_redo_button (XpadToolbar *toolbar, gboolean enable);
void xpad_toolbar_enable_cut_button (XpadToolbar *toolbar, gboolean enable);
void xpad_toolbar_enable_copy_button (XpadToolbar *toolbar, gboolean enable);
void xpad_toolbar_enable_paste_button (XpadToolbar *toolbar, gboolean enable);

G_END_DECLS

#endif /* __XPAD_TOOLBAR_H__ */
