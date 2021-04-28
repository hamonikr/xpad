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

#ifndef __FIO_H__
#define __FIO_H__

#include <glib.h>

typedef enum {CONFIG_DIR, CURRENT_WORK_DIR} DirectoryType;

G_BEGIN_DECLS

gchar *fio_get_file (const gchar *name, DirectoryType dirType);
gboolean fio_set_file (const gchar *name, const gchar *value);
void fio_remove_file (const gchar *filename);

gint fio_get_values_from_file (const gchar *filename, ...);
gint fio_set_values_to_file (const gchar *filename, ...);

gchar *str_replace_tokens (gchar **string, gchar obj, gchar *replacement);

gchar *fio_unique_name (const gchar *prefix);

G_END_DECLS

#endif /* __FIO_H__ */
