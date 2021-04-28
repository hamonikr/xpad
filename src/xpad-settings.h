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

#ifndef __XPAD_SETTINGS_H__
#define __XPAD_SETTINGS_H__

#include <gtk/gtk.h>

#include "xpad-styling-helpers.h"

G_BEGIN_DECLS

#define XPAD_TYPE_SETTINGS          (xpad_settings_get_type ())
#define XPAD_SETTINGS(o)            (G_TYPE_CHECK_INSTANCE_CAST ((o), XPAD_TYPE_SETTINGS, XpadSettings))
#define XPAD_SETTINGS_CLASS(k)      (G_TYPE_CHECK_CLASS_CAST((k), XPAD_TYPE_SETTINGS, XpadSettingsClass))
#define XPAD_IS_SETTINGS(o)         (G_TYPE_CHECK_INSTANCE_TYPE ((o), XPAD_TYPE_SETTINGS))
#define XPAD_IS_SETTINGS_CLASS(k)   (G_TYPE_CHECK_CLASS_TYPE ((k), XPAD_TYPE_SETTINGS))
#define XPAD_SETTINGS_GET_CLASS(o)  (G_TYPE_INSTANCE_GET_CLASS ((o), XPAD_TYPE_SETTINGS, XpadSettingsClass))

typedef struct XpadSettingsClass XpadSettingsClass;
typedef struct XpadSettingsPrivate XpadSettingsPrivate;
typedef struct XpadSettings XpadSettings;

struct XpadSettings
{
	GObject parent;
	XpadSettingsPrivate *priv;
};

struct XpadSettingsClass
{
	GObjectClass parent_class;
	
	void (*change_buttons) (XpadSettings *settings);
};

GType xpad_settings_get_type (void);

XpadSettings *xpad_settings_new (void);

void xpad_settings_add_toolbar_button (XpadSettings *settings, const gchar *button);
gboolean xpad_settings_remove_all_toolbar_buttons (XpadSettings *settings);
gboolean xpad_settings_remove_last_toolbar_button (XpadSettings *settings);
const GSList *xpad_settings_get_toolbar_buttons (XpadSettings *settings);

G_END_DECLS

#endif /* __XPAD_SETTINGS_H__ */
