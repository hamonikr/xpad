/*

Copyright (c) 2001-2007 Michael Terry
Copyright (c) 2009 Paul Ivanov
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

#include "xpad-pad-group.h"
#include "xpad-pad.h"

struct XpadPadGroupPrivate
{
	GSList *pads;
};

G_DEFINE_TYPE_WITH_PRIVATE (XpadPadGroup, xpad_pad_group, G_TYPE_OBJECT)

static void xpad_pad_group_save_unsaved_all (XpadPadGroup *group);

enum {
	PROP_0
};

enum
{
	PAD_ADDED,
	PAD_REMOVED,
	LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };

XpadPadGroup *
xpad_pad_group_new (void)
{
	return XPAD_PAD_GROUP (g_object_new (XPAD_TYPE_PAD_GROUP, NULL));
}

static void
xpad_pad_group_init (XpadPadGroup *group)
{
	group->priv = xpad_pad_group_get_instance_private (group);

	group->priv->pads = NULL;
}

static void
xpad_pad_group_class_init (XpadPadGroupClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	signals[PAD_ADDED] =
		g_signal_new ("pad_added",
		              G_OBJECT_CLASS_TYPE (object_class),
		              G_SIGNAL_RUN_FIRST,
		              G_STRUCT_OFFSET (XpadPadGroupClass, pad_added),
		              NULL, NULL,
		              g_cclosure_marshal_VOID__OBJECT,
		              G_TYPE_NONE,
		              1,
		              GTK_TYPE_WIDGET);

	signals[PAD_REMOVED] =
		g_signal_new ("pad_removed",
		              G_OBJECT_CLASS_TYPE (object_class),
		              G_SIGNAL_RUN_FIRST,
		              G_STRUCT_OFFSET (XpadPadGroupClass, pad_added),
		              NULL, NULL,
		              g_cclosure_marshal_VOID__OBJECT,
		              G_TYPE_NONE,
		              1,
		              GTK_TYPE_WIDGET);
}

GSList *
xpad_pad_group_get_pads (XpadPadGroup *group)
{
	return g_slist_copy (group->priv->pads);
}

/* Add a pad to this group */
void
xpad_pad_group_add (XpadPadGroup *group, GtkWidget *pad)
{
	g_object_ref(pad);

	group->priv->pads = g_slist_append (group->priv->pads, XPAD_PAD (pad));
	g_signal_connect_swapped (pad, "destroy", G_CALLBACK (xpad_pad_group_remove), group);

	g_signal_emit (group, signals[PAD_ADDED], 0, pad);
}

/* Remove a pad from this group */
void
xpad_pad_group_remove (XpadPadGroup *group, GtkWidget *pad)
{
	group->priv->pads = g_slist_remove (group->priv->pads, XPAD_PAD (pad));
	g_clear_object(&pad);

	g_signal_emit (group, signals[PAD_REMOVED], 0, pad);
}

/* Delete all the current pads in the group */
void
xpad_pad_group_destroy_pads (XpadPadGroup *group)
{
	xpad_pad_group_save_unsaved_all(group);

	/* Remove (and thus disable) all the key accelerators before the pads get destroyed, preventing a call to a non-existing accelerator */
	g_slist_foreach (group->priv->pads, (GFunc) xpad_pad_remove_accelerator_group, NULL);
	g_slist_foreach (group->priv->pads, (GFunc) gtk_widget_destroy, NULL);
}

guint
xpad_pad_group_num_pads (XpadPadGroup *group)
{
	return g_slist_length (group->priv->pads);
}

guint
xpad_pad_group_num_visible_pads (XpadPadGroup *group)
{
	guint num = 0;
	if (group)
	{
		GSList *i;
		for (i = group->priv->pads; i; i = i->next)
		{
			if (gtk_widget_get_visible(GTK_WIDGET(i->data)))
				num ++;
		}
		g_slist_free(i);
	}
	return num;
}

static void xpad_pad_group_save_unsaved_all (XpadPadGroup *group) {
	if (group)
		g_slist_foreach (group->priv->pads, (GFunc) xpad_pad_save_unsaved, NULL);
}

void
xpad_pad_group_close_all (XpadPadGroup *group)
{
	if (group)
		g_slist_foreach (group->priv->pads, (GFunc) xpad_pad_close, NULL);
}

void
xpad_pad_group_show_all (XpadPadGroup *group)
{
	if (group)
		g_slist_foreach (group->priv->pads, (GFunc) gtk_widget_show, NULL);
}

/* If one of the pads is not visible show them all, else hide them all. */
void
xpad_pad_group_toggle_hide (XpadPadGroup *group)
{
	if (!group) {
		return;
	}

	GSList *nextPad = g_slist_nth (group->priv->pads, 0);

	while (nextPad != NULL) {
		if (!gtk_widget_get_visible (GTK_WIDGET(nextPad->data))) {
			xpad_pad_group_show_all (group);
			return;
		}

		nextPad = g_slist_next (nextPad);
	}

	xpad_pad_group_close_all (group);
}


void
xpad_pad_group_update_sticky (XpadPadGroup *group, gboolean is_sticky)
{
	if (group) {
		g_slist_foreach (group->priv->pads, (GFunc) xpad_pad_set_sticky, GINT_TO_POINTER(is_sticky));
	}
}
