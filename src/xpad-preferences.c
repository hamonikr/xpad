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

#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include "xpad-preferences.h"
#include "xpad-app.h"
#include "xpad-pad-group.h"

struct XpadPreferencesPrivate
{
	XpadSettings *settings;

	GtkWidget *notebook;

	GtkWidget *height;
	GtkWidget *width;
	GtkWidget *fontcheck;
	GtkWidget *antifontcheck;
	GtkWidget *fontbutton;
	GtkWidget *colorcheck;
	GtkWidget *anticolorcheck;
	GtkWidget *colorbox;
	GtkWidget *textbutton;
	GtkWidget *backbutton;
	GtkWidget *autostart_xpad;
	GtkWidget *autostart_wait_systray;
	GtkWidget *autostart_delay;
	GtkWidget *autostart_new_pad;
	GtkWidget *autostart_sticky;
	GtkWidget *autostart_display_pads;
	GtkWidget *tray_enabled;
	GtkWidget *tray_click_configuration;
	GtkWidget *editcheck;
	GtkWidget *confirmcheck;
	GtkWidget *has_decorations;
	GtkWidget *hide_from_taskbar;
	GtkWidget *hide_from_task_switcher;
	GtkWidget *has_toolbar;
	GtkWidget *autohide_toolbar;
	GtkWidget *has_scrollbar;
	GtkWidget *line_numbering;

	gulong fontcheck_handler;
	gulong font_handler;
	gulong colorcheck_handler;
	gulong text_handler;
	gulong back_handler;
	gulong new_pad_height_handler;
	gulong new_pad_width_handler;
	gulong autostart_xpad_handler;
	gulong autostart_wait_systray_handler;
	gulong autostart_delay_handler;
	gulong autostart_new_pad_handler;
	gulong autostart_sticky_handler;
	gulong autostart_display_pads_handler;
	gulong tray_enabled_handler;
	gulong tray_click_handler;
	gulong editcheck_handler;
	gulong confirmcheck_handler;
	gulong has_decorations_handler;
	gulong hide_from_taskbar_handler;
	gulong hide_from_task_switcher_handler;
	gulong has_toolbar_handler;
	gulong autohide_toolbar_handler;
	gulong has_scrollbar_handler;
	gulong line_numbering_handler;

	gulong notify_font_handler;
	gulong notify_text_handler;
	gulong notify_back_handler;
	gulong notify_autostart_xpad_handler;
	gulong notify_autostart_wait_systray_handler;
	gulong notify_autostart_delay_handler;
	gulong notify_autostart_new_pad_handler;
	gulong notify_autostart_sticky_handler;
	gulong notify_autostart_display_pads_handler;
	gulong notify_tray_enabled_handler;
	gulong notify_tray_click_handler;
	gulong notify_edit_handler;
	gulong notify_confirm_handler;
	gulong notify_has_decorations_handler;
	gulong notify_hide_from_taskbar_handler;
	gulong notify_hide_from_task_switcher_handler;
	gulong notify_has_toolbar_handler;
	gulong notify_autohide_toolbar_handler;
	gulong notify_has_scrollbar_handler;
	gulong notify_line_numbering_handler;
};

G_DEFINE_TYPE_WITH_PRIVATE (XpadPreferences, xpad_preferences, GTK_TYPE_WINDOW)

static void xpad_preferences_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec);
static void xpad_preferences_get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec);
static void xpad_preferences_constructed (GObject *object);
static void xpad_preferences_finalize (GObject *object);

static void change_font_check (GtkToggleButton *button, XpadPreferences *pref);
static void change_font_face (GtkFontChooser *button, XpadPreferences *pref);
static void change_color_check (GtkToggleButton *button, XpadPreferences *pref);
static void change_text_color (GtkColorChooser *chooser, XpadPreferences *pref);
static void change_back_color (GtkColorChooser *chooser, XpadPreferences *pref);
static void change_new_pad_height (GtkSpinButton *button, XpadPreferences *pref);
static void change_new_pad_width (GtkSpinButton *button, XpadPreferences *pref);
static void change_autostart_xpad (GtkToggleButton *button, XpadPreferences *pref);
static void change_autostart_wait_systray (GtkToggleButton *button, XpadPreferences *pref);
static void change_autostart_delay (GtkComboBox *box, XpadPreferences *pref);
static void change_autostart_new_pad (GtkToggleButton *button, XpadPreferences *pref);
static void change_autostart_sticky (GtkToggleButton *button, XpadPreferences *pref);
static void change_autostart_display_pads (GtkComboBox *box, XpadPreferences *pref);
static void change_tray_enabled (GtkToggleButton *button, XpadPreferences *pref);
static void change_tray_click (GtkComboBox *box, XpadPreferences *pref);
static void change_edit_check (GtkToggleButton *button, XpadPreferences *pref);
static void change_confirm_check (GtkToggleButton *button, XpadPreferences *pref);
static void change_has_decorations (GtkToggleButton *button, XpadPreferences *pref);
static void change_hide_from_taskbar (GtkToggleButton *button, XpadPreferences *pref);
static void change_hide_from_task_switcher (GtkToggleButton *button, XpadPreferences *pref);
static void change_has_toolbar (GtkToggleButton *button, XpadPreferences *pref);
static void change_autohide_toolbar (GtkToggleButton *button, XpadPreferences *pref);
static void change_has_scrollbar (GtkToggleButton *button, XpadPreferences *pref);
static void change_line_numbering (GtkToggleButton *button, XpadPreferences *pref);

static void notify_fontname (XpadPreferences *pref);
static void notify_text_color (XpadPreferences *pref);
static void notify_back_color (XpadPreferences *pref);
static void notify_autostart_xpad (XpadPreferences *pref);
static void notify_autostart_wait_systray (XpadPreferences *pref);
static void notify_autostart_delay (XpadPreferences *pref);
static void notify_autostart_new_pad (XpadPreferences *pref);
static void notify_autostart_sticky (XpadPreferences *pref);
static void notify_autostart_display_pads (XpadPreferences *pref);
static void notify_tray_enabled (XpadPreferences *pref);
static void notify_tray_click (XpadPreferences *pref);
static void notify_edit (XpadPreferences *pref);
static void notify_confirm (XpadPreferences *pref);
static void notify_has_decorations (XpadPreferences *pref);
static void notify_hide_from_taskbar (XpadPreferences *pref);
static void notify_hide_from_task_switcher (XpadPreferences *pref);
static void notify_has_toolbar (XpadPreferences *pref);
static void notify_autohide_toolbar (XpadPreferences *pref);
static void notify_has_scrollbar (XpadPreferences *pref);
static void notify_line_numbering (XpadPreferences *pref);

static GtkWidget * create_label (const gchar *label_text);

static GtkWidget *_xpad_preferences = NULL;

enum
{
	PROP_0,
	PROP_SETTINGS,
	N_PROPERTIES
};

static GParamSpec *obj_prop[N_PROPERTIES] = { NULL, };

void
xpad_preferences_open (XpadSettings *settings)
{
	if (!_xpad_preferences)
	{
		_xpad_preferences = GTK_WIDGET (g_object_new (XPAD_TYPE_PREFERENCES, "settings", settings, NULL));
		g_signal_connect_swapped (_xpad_preferences, "destroy", G_CALLBACK (g_nullify_pointer), &_xpad_preferences);
	}

	gtk_window_present (GTK_WINDOW (_xpad_preferences));
}

static void
xpad_preferences_class_init (XpadPreferencesClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

	gobject_class->constructed = xpad_preferences_constructed;
	gobject_class->set_property = xpad_preferences_set_property;
	gobject_class->get_property = xpad_preferences_get_property;
	gobject_class->finalize = xpad_preferences_finalize;

	obj_prop[PROP_SETTINGS] = g_param_spec_pointer ("settings", "Xpad settings", "Xpad global settings", G_PARAM_READWRITE | G_PARAM_CONSTRUCT);

	g_object_class_install_properties (gobject_class, N_PROPERTIES, obj_prop);
}

static void
xpad_preferences_init (XpadPreferences *pref)
{
	pref->priv = xpad_preferences_get_instance_private (pref);
}

static void xpad_preferences_constructed (GObject *object)
{
	XpadPreferences *pref = XPAD_PREFERENCES (object);

	GtkWidget *label, *label_height, *label_width;
	GtkBox *font_hbox, *vbox, *hbox, *view_vbox, *appearance_vbox, *start_vbox, *tray_vbox, *other_vbox, *pad_size_height_hbox, *pad_size_width_hbox, *pad_size_vbox;
	const GdkRGBA *text_color, *back_color;
	const gchar *fontname;
	GtkStyleContext *style;
	GtkSizeGroup *size_group_labels = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);
	GdkRGBA theme_text_color = {0, 0, 0, 0}, theme_background_color = {0, 0, 0, 0};
	guint tray_click_configuration, autostart_delay, autostart_display_pads, height, width;
	gboolean confirm_destroy, edit_lock, autostart_xpad, autostart_wait_systray, autostart_new_pad, autostart_sticky, has_decorations, hide_from_taskbar, hide_from_task_switcher, has_toolbar, autohide_toolbar, has_scrollbar, line_numbering;

	g_object_get (pref->priv->settings,
			"fontname", &fontname,
			"text-color", &text_color,
			"back-color", &back_color,
			"height", &height,
			"width", &width,
			"confirm-destroy", &confirm_destroy,
			"edit-lock", &edit_lock,
			"tray-click-configuration", &tray_click_configuration,
			"autostart-xpad", &autostart_xpad,
			"autostart-wait-systray", &autostart_wait_systray,
			"autostart-new-pad", &autostart_new_pad,
			"autostart-sticky", &autostart_sticky,
			"autostart-delay", &autostart_delay,
			"autostart-display-pads", &autostart_display_pads,
			"has-decorations", &has_decorations,
			"hide-from-taskbar", &hide_from_taskbar,
			"hide-from-task-switcher", &hide_from_task_switcher,
			"has-toolbar", &has_toolbar,
			"autohide-toolbar", &autohide_toolbar,
			"has-scrollbar", &has_scrollbar,
			"line-numbering", &line_numbering,
			NULL);

	/* create notebook to add pages */
	pref->priv->notebook = gtk_notebook_new ();

	/* View options */
	label = create_label (_("View"));

	view_vbox = GTK_BOX (gtk_box_new (GTK_ORIENTATION_VERTICAL, 20));
	gtk_box_set_homogeneous (view_vbox, FALSE);
        gtk_widget_set_margin_top (GTK_WIDGET (view_vbox), 12);
        gtk_widget_set_margin_bottom (GTK_WIDGET (view_vbox), 12);
        gtk_widget_set_margin_start (GTK_WIDGET (view_vbox), 12);
        gtk_widget_set_margin_end (GTK_WIDGET (view_vbox), 12);

	gtk_notebook_append_page (GTK_NOTEBOOK (pref->priv->notebook), GTK_WIDGET (view_vbox), label);

	pref->priv->has_toolbar = gtk_check_button_new_with_mnemonic (_("_Show toolbar"));
	gtk_box_pack_start (view_vbox, pref->priv->has_toolbar, FALSE, FALSE, 0);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (pref->priv->has_toolbar), has_toolbar);

	pref->priv->autohide_toolbar = gtk_check_button_new_with_mnemonic (_("_Autohide toolbar"));
	hbox = GTK_BOX (gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 20));
	gtk_box_pack_start (hbox, pref->priv->autohide_toolbar, FALSE, FALSE, 0);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (pref->priv->autohide_toolbar), autohide_toolbar);
	gtk_widget_set_sensitive (pref->priv->autohide_toolbar, has_toolbar);
	gtk_box_pack_start (view_vbox, GTK_WIDGET (hbox), FALSE, FALSE, 0);

	pref->priv->has_scrollbar = gtk_check_button_new_with_mnemonic (_("_Show scrollbar"));
	gtk_box_pack_start (view_vbox, pref->priv->has_scrollbar, FALSE, FALSE, 0);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (pref->priv->has_scrollbar), has_scrollbar);

	pref->priv->autostart_sticky = gtk_check_button_new_with_mnemonic (_("_Show notes on all workspaces"));
	gtk_box_pack_start (view_vbox, pref->priv->autostart_sticky, FALSE, FALSE, 0);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (pref->priv->autostart_sticky), autostart_sticky);

	pref->priv->has_decorations = gtk_check_button_new_with_mnemonic (_("_Show window decorations"));
	gtk_box_pack_start (view_vbox, pref->priv->has_decorations, FALSE, FALSE, 0);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (pref->priv->has_decorations), has_decorations);

	pref->priv->hide_from_taskbar = gtk_check_button_new_with_mnemonic (_("_Hide all notes from the taskbar and possibly the task switcher"));
	gtk_box_pack_start (view_vbox, pref->priv->hide_from_taskbar, FALSE, FALSE, 0);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (pref->priv->hide_from_taskbar), hide_from_taskbar);

	pref->priv->hide_from_task_switcher = gtk_check_button_new_with_mnemonic (_("_Hide all notes from the workspace switcher and possibly the task switcher"));
	gtk_box_pack_start (view_vbox, pref->priv->hide_from_task_switcher, FALSE, FALSE, 0);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (pref->priv->hide_from_task_switcher), hide_from_task_switcher);

	/* Layout options */
	label = create_label (_("Layout"));

	appearance_vbox = GTK_BOX (gtk_box_new (GTK_ORIENTATION_VERTICAL, 20));
	gtk_box_set_homogeneous (appearance_vbox, FALSE);
	gtk_widget_set_margin_top (GTK_WIDGET (appearance_vbox), 12);
	gtk_widget_set_margin_bottom (GTK_WIDGET (appearance_vbox), 12);
	gtk_widget_set_margin_start (GTK_WIDGET (appearance_vbox), 12);
	gtk_widget_set_margin_end (GTK_WIDGET (appearance_vbox), 12);

	gtk_notebook_append_page (GTK_NOTEBOOK (pref->priv->notebook), GTK_WIDGET (appearance_vbox), label);

	pref->priv->textbutton = gtk_color_button_new ();
	pref->priv->fontbutton = gtk_font_button_new ();
	pref->priv->backbutton = gtk_color_button_new ();
	pref->priv->colorbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);

	pref->priv->antifontcheck = gtk_radio_button_new_with_mnemonic (NULL, _("Use font from theme"));
	pref->priv->fontcheck = gtk_radio_button_new_with_mnemonic_from_widget (GTK_RADIO_BUTTON (pref->priv->antifontcheck), _("Use this font"));
	pref->priv->anticolorcheck = gtk_radio_button_new_with_mnemonic (NULL, _("Use colors from theme"));
	pref->priv->colorcheck = gtk_radio_button_new_with_mnemonic_from_widget (GTK_RADIO_BUTTON (pref->priv->anticolorcheck), _("Use these colors"));
	gtk_widget_set_margin_start (GTK_WIDGET (pref->priv->colorbox), 25);

	/* Size of new pads: input fields */
	label_height = gtk_label_new_with_mnemonic (_("Height new pad"));
	gtk_size_group_add_widget (size_group_labels, label_height);
	pref->priv->height = gtk_spin_button_new_with_range (10, 99999, 10);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON(pref->priv->height), height);
    gtk_widget_set_margin_start (pref->priv->height, 12);

	label_width = gtk_label_new_with_mnemonic (_("Width new pad"));
	gtk_size_group_add_widget (size_group_labels, label_width);
	pref->priv->width = gtk_spin_button_new_with_range (10, 99999, 10);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON(pref->priv->width), width);
    gtk_widget_set_margin_start (pref->priv->width, 12);

	/* Start adding the input pieces together in boxes */
	pad_size_height_hbox = GTK_BOX (gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6));
	gtk_box_pack_start (pad_size_height_hbox, label_height, FALSE, FALSE, 0);
	gtk_box_pack_start (pad_size_height_hbox, pref->priv->height, FALSE, FALSE, 0);
	pad_size_width_hbox = GTK_BOX (gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6));
	gtk_box_pack_start (pad_size_width_hbox, label_width, FALSE, FALSE, 0);
	gtk_box_pack_start (pad_size_width_hbox, pref->priv->width, FALSE, FALSE, 0);

	pad_size_vbox = GTK_BOX (gtk_box_new (GTK_ORIENTATION_VERTICAL, 6));
	gtk_box_pack_start (pad_size_vbox, GTK_WIDGET (pad_size_height_hbox), FALSE, TRUE, 0);
	gtk_box_pack_start (pad_size_vbox, GTK_WIDGET (pad_size_width_hbox), FALSE, TRUE, 0);

	font_hbox = GTK_BOX (gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6));
	gtk_box_pack_start (font_hbox, pref->priv->fontcheck, FALSE, TRUE, 0);
	gtk_box_pack_start (font_hbox, pref->priv->fontbutton, FALSE, TRUE, 0);

	hbox = GTK_BOX (gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 12));
	label = gtk_label_new_with_mnemonic (_("Text"));
	gtk_size_group_add_widget (size_group_labels, label);
	gtk_box_pack_start (hbox, label, FALSE, FALSE, 0);
	gtk_box_pack_start (hbox, pref->priv->textbutton, FALSE, TRUE, 0);
	g_object_set (G_OBJECT (pref->priv->colorbox), "child", hbox, NULL);

	hbox = GTK_BOX (gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 12));

	label = gtk_label_new_with_mnemonic (_("Background"));
	gtk_size_group_add_widget (size_group_labels, label);
	gtk_box_pack_start (hbox, label, FALSE, FALSE, 0);
	gtk_box_pack_start (hbox, pref->priv->backbutton, FALSE, TRUE, 0);
	g_object_set (G_OBJECT (pref->priv->colorbox), "child", hbox, NULL);

	style = gtk_widget_get_style_context (GTK_WIDGET(pref));
	gtk_style_context_get_color (style, GTK_STATE_FLAG_NORMAL, &theme_text_color);
	get_background_color (style, GTK_STATE_FLAG_NORMAL, &theme_background_color);

	if (fontname)
	{
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (pref->priv->fontcheck), TRUE);
		gtk_font_chooser_set_font (GTK_FONT_CHOOSER (pref->priv->fontbutton), fontname);
	}
	else
	{
		PangoFontDescription *font;

		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (pref->priv->antifontcheck), TRUE);
		gtk_widget_set_sensitive (pref->priv->fontbutton, FALSE);

		gtk_style_context_get (style, GTK_STATE_FLAG_NORMAL, GTK_STYLE_PROPERTY_FONT, &font, NULL);
		gtk_font_chooser_set_font (GTK_FONT_CHOOSER (pref->priv->fontbutton), pango_font_description_to_string(font));
		pango_font_description_free (font);
	}

	if (text_color)
		gtk_color_chooser_set_rgba (GTK_COLOR_CHOOSER (pref->priv->textbutton), text_color);
	else
		gtk_color_chooser_set_rgba (GTK_COLOR_CHOOSER (pref->priv->textbutton), &theme_text_color);

	if (back_color)
	{
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (pref->priv->colorcheck), TRUE);
		gtk_color_chooser_set_rgba (GTK_COLOR_CHOOSER (pref->priv->backbutton), back_color);
	}
	else
	{
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (pref->priv->anticolorcheck), TRUE);
		gtk_widget_set_sensitive (pref->priv->colorbox, FALSE);
		gtk_color_chooser_set_rgba (GTK_COLOR_CHOOSER (pref->priv->backbutton), &theme_background_color);
	}

	vbox = GTK_BOX (gtk_box_new (GTK_ORIENTATION_VERTICAL, 6));
	gtk_box_pack_start (vbox, pref->priv->antifontcheck, FALSE, FALSE, 0);
	gtk_box_pack_start (vbox, GTK_WIDGET (font_hbox), FALSE, FALSE, 0);
	gtk_box_pack_start (appearance_vbox, GTK_WIDGET (vbox), FALSE, FALSE, 0);

	vbox = GTK_BOX (gtk_box_new (GTK_ORIENTATION_VERTICAL, 6));
	gtk_box_pack_start (vbox, pref->priv->anticolorcheck, FALSE, FALSE, 0);
	gtk_box_pack_start (vbox, pref->priv->colorcheck, FALSE, FALSE, 0);
	gtk_box_pack_start (vbox, pref->priv->colorbox, FALSE, FALSE, 0);
	gtk_box_pack_start (appearance_vbox, GTK_WIDGET (vbox), FALSE, FALSE, 0);

	vbox = GTK_BOX (gtk_box_new (GTK_ORIENTATION_VERTICAL, 6));
	gtk_box_pack_start (vbox, GTK_WIDGET (pad_size_vbox), FALSE, FALSE, 0);
	gtk_box_pack_start (appearance_vbox, GTK_WIDGET (vbox), FALSE, FALSE, 0);

	gtk_color_chooser_set_use_alpha (GTK_COLOR_CHOOSER (pref->priv->textbutton), FALSE);
	gtk_color_chooser_set_use_alpha (GTK_COLOR_CHOOSER (pref->priv->backbutton), TRUE);

	gtk_color_button_set_title (GTK_COLOR_BUTTON (pref->priv->textbutton), _("Set Foreground Color"));
	gtk_color_button_set_title (GTK_COLOR_BUTTON (pref->priv->backbutton), _("Set Background Color"));
	gtk_font_button_set_title (GTK_FONT_BUTTON (pref->priv->fontbutton), _("Set Font"));

	/* Start options */
	label = create_label (_("Startup"));

	start_vbox = GTK_BOX (gtk_box_new (GTK_ORIENTATION_VERTICAL, 6));
	gtk_box_set_homogeneous (start_vbox, FALSE);
        gtk_widget_set_margin_top (GTK_WIDGET (start_vbox), 12);
        gtk_widget_set_margin_bottom (GTK_WIDGET (start_vbox), 12);
        gtk_widget_set_margin_start (GTK_WIDGET (start_vbox), 12);
        gtk_widget_set_margin_end (GTK_WIDGET (start_vbox), 12);

	gtk_notebook_append_page (GTK_NOTEBOOK (pref->priv->notebook), GTK_WIDGET (start_vbox), label);

	pref->priv->autostart_xpad = gtk_check_button_new_with_mnemonic (_("_Start Xpad automatically after login"));
	gtk_box_pack_start (start_vbox, pref->priv->autostart_xpad, FALSE, FALSE, 0);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (pref->priv->autostart_xpad), autostart_xpad);

	pref->priv->autostart_wait_systray = gtk_check_button_new_with_mnemonic (_("_Wait for systray (if possible)"));
	hbox = GTK_BOX (gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 20));
	gtk_box_pack_start (hbox, pref->priv->autostart_wait_systray, FALSE, FALSE, 0);

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (pref->priv->autostart_wait_systray), autostart_wait_systray);
	if (autostart_xpad)
		gtk_widget_set_sensitive (pref->priv->autostart_wait_systray, TRUE);
	else
		gtk_widget_set_sensitive (pref->priv->autostart_wait_systray, FALSE);
	gtk_box_pack_start (start_vbox, GTK_WIDGET (hbox), FALSE, FALSE, 0);

	pref->priv->autostart_new_pad = gtk_check_button_new_with_mnemonic (_("_Open a new empty pad"));
	gtk_box_pack_start (start_vbox, pref->priv->autostart_new_pad, FALSE, FALSE, 0);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (pref->priv->autostart_new_pad), autostart_new_pad);

	label = gtk_label_new (_("Delay in seconds"));
	pref->priv->autostart_delay = gtk_combo_box_text_new();
	guint i;
	for (i=0; i<15; i++)
		gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (pref->priv->autostart_delay), g_strdup_printf ("%i", i));
	gtk_combo_box_set_active (GTK_COMBO_BOX (pref->priv->autostart_delay), autostart_delay);
	hbox = GTK_BOX (gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 12));
	gtk_box_pack_start (hbox, label, FALSE, FALSE, 0);
	gtk_box_pack_start (hbox, pref->priv->autostart_delay, FALSE, TRUE, 0);
	gtk_box_pack_start (start_vbox, GTK_WIDGET (hbox), FALSE, FALSE, 0);

	label = gtk_label_new_with_mnemonic(_("Display pads"));
	pref->priv->autostart_display_pads = gtk_combo_box_text_new();
	gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (pref->priv->autostart_display_pads), _("Open all pads"));
	gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (pref->priv->autostart_display_pads), _("Hide all pads"));
	gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (pref->priv->autostart_display_pads), _("Restore to previous state") );
	gtk_combo_box_set_active (GTK_COMBO_BOX (pref->priv->autostart_display_pads), (guint) autostart_display_pads);
	hbox = GTK_BOX (gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 12));
	gtk_box_pack_start (hbox, label, FALSE, FALSE, 0);
	gtk_box_pack_start (hbox, pref->priv->autostart_display_pads, FALSE, TRUE, 0);
	gtk_box_pack_start (start_vbox, GTK_WIDGET (hbox), FALSE, FALSE, 0);

	/* Tray options */
	label = create_label (_("Tray"));

	tray_vbox = GTK_BOX (gtk_box_new (GTK_ORIENTATION_VERTICAL, 6));
	gtk_box_set_homogeneous (tray_vbox, FALSE);
        gtk_widget_set_margin_top (GTK_WIDGET (tray_vbox), 12);
        gtk_widget_set_margin_bottom (GTK_WIDGET (tray_vbox), 12);
        gtk_widget_set_margin_start (GTK_WIDGET (tray_vbox), 12);
        gtk_widget_set_margin_end (GTK_WIDGET (tray_vbox), 12);

	gtk_notebook_append_page (GTK_NOTEBOOK (pref->priv->notebook), GTK_WIDGET (tray_vbox), label);

	pref->priv->tray_enabled = gtk_check_button_new_with_mnemonic (_("_Enable tray icon"));
	gtk_box_pack_start (tray_vbox, pref->priv->tray_enabled, FALSE, FALSE, 0);

	label = gtk_label_new_with_mnemonic(_("Tray left mouse click behavior"));
	pref->priv->tray_click_configuration = gtk_combo_box_text_new();
	gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (pref->priv->tray_click_configuration), _("Do Nothing") );
	gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (pref->priv->tray_click_configuration), _("Toggle Show All") );
	gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (pref->priv->tray_click_configuration), _("List of Pads") );
	gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (pref->priv->tray_click_configuration), _("New Pad") );
	gtk_combo_box_set_active (GTK_COMBO_BOX (pref->priv->tray_click_configuration), tray_click_configuration);
	hbox = GTK_BOX (gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6));
	gtk_box_pack_start (hbox, label, FALSE, FALSE, 0);
	gtk_box_pack_start (hbox, pref->priv->tray_click_configuration, FALSE, TRUE, 0);
	gtk_box_pack_start (tray_vbox, GTK_WIDGET (hbox), FALSE, FALSE, 0);

	/* Other options */
	label = create_label (_("Other"));

	other_vbox = GTK_BOX (gtk_box_new (GTK_ORIENTATION_VERTICAL, 6));
	gtk_box_set_homogeneous (other_vbox, FALSE);
        gtk_widget_set_margin_top (GTK_WIDGET (other_vbox), 12);
        gtk_widget_set_margin_bottom (GTK_WIDGET (other_vbox), 12);
        gtk_widget_set_margin_start (GTK_WIDGET (other_vbox), 12);
        gtk_widget_set_margin_end (GTK_WIDGET (other_vbox), 12);

	gtk_notebook_append_page (GTK_NOTEBOOK (pref->priv->notebook), GTK_WIDGET (other_vbox), label);

	pref->priv->editcheck = gtk_check_button_new_with_mnemonic (_("_Make pads read-only (CTRL-J)"));
	pref->priv->confirmcheck = gtk_check_button_new_with_mnemonic (_("_Confirm pad deletion"));
	pref->priv->line_numbering = gtk_check_button_new_with_mnemonic (_("Enable _line numbering"));

	gtk_box_pack_start (other_vbox, pref->priv->editcheck, FALSE, FALSE, 0);
	gtk_box_pack_start (other_vbox, pref->priv->confirmcheck, FALSE, FALSE, 0);
	gtk_box_pack_start (other_vbox, pref->priv->line_numbering, FALSE, FALSE, 0);

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (pref->priv->editcheck), edit_lock);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (pref->priv->confirmcheck), confirm_destroy);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (pref->priv->line_numbering), line_numbering);

	/* Close button and window title */
	GtkWidget *button = gtk_button_new_from_icon_name ("gtk-close", GTK_ICON_SIZE_BUTTON);
	gtk_button_set_label (GTK_BUTTON (button), _("Close"));
	g_signal_connect_swapped (GTK_BUTTON (button), "clicked", G_CALLBACK (gtk_widget_destroy), GTK_WIDGET (pref));
	gtk_window_set_title (GTK_WINDOW (pref), _("Xpad Preferences"));

	/* Add preference tabs and the close button together */
	vbox = GTK_BOX (gtk_box_new (GTK_ORIENTATION_VERTICAL, 0));
 	gtk_container_add (GTK_CONTAINER (vbox), pref->priv->notebook);
 	gtk_container_add (GTK_CONTAINER (vbox), button);
 	gtk_container_add (GTK_CONTAINER (pref), GTK_WIDGET (vbox));

	/* Add CSS style class, so the styling can be overridden by a GTK theme */
	GtkStyleContext *context = gtk_widget_get_style_context(GTK_WIDGET (pref));
	gtk_style_context_add_class(context, "XpadPreferences");

 	/* Activate all handlers */
	pref->priv->has_decorations_handler = g_signal_connect (pref->priv->has_decorations, "toggled", G_CALLBACK (change_has_decorations), pref);
	pref->priv->hide_from_taskbar_handler = g_signal_connect (pref->priv->hide_from_taskbar, "toggled", G_CALLBACK (change_hide_from_taskbar), pref);
	pref->priv->hide_from_task_switcher_handler = g_signal_connect (pref->priv->hide_from_task_switcher, "toggled", G_CALLBACK (change_hide_from_task_switcher), pref);
	pref->priv->has_toolbar_handler = g_signal_connect (pref->priv->has_toolbar, "toggled", G_CALLBACK (change_has_toolbar), pref);
	pref->priv->autohide_toolbar_handler = g_signal_connect (pref->priv->autohide_toolbar, "toggled", G_CALLBACK (change_autohide_toolbar), pref);
	pref->priv->has_scrollbar_handler = g_signal_connect (pref->priv->has_scrollbar, "toggled", G_CALLBACK (change_has_scrollbar), pref);

	pref->priv->fontcheck_handler = g_signal_connect (pref->priv->fontcheck, "toggled", G_CALLBACK (change_font_check), pref);
	pref->priv->font_handler = g_signal_connect (pref->priv->fontbutton, "font-set", G_CALLBACK (change_font_face), pref);
	pref->priv->colorcheck_handler = g_signal_connect (pref->priv->colorcheck, "toggled", G_CALLBACK (change_color_check), pref);
	pref->priv->text_handler = g_signal_connect (pref->priv->textbutton, "color-set", G_CALLBACK (change_text_color), pref);
	pref->priv->back_handler = g_signal_connect (pref->priv->backbutton, "color-set", G_CALLBACK (change_back_color), pref);

	pref->priv->new_pad_height_handler = g_signal_connect (pref->priv->height, "value-changed", G_CALLBACK (change_new_pad_height), pref);
	pref->priv->new_pad_width_handler = g_signal_connect (pref->priv->width, "value-changed", G_CALLBACK (change_new_pad_width), pref);

	pref->priv->autostart_xpad_handler = g_signal_connect (pref->priv->autostart_xpad, "toggled", G_CALLBACK (change_autostart_xpad), pref);
	pref->priv->autostart_wait_systray_handler = g_signal_connect (pref->priv->autostart_wait_systray, "toggled", G_CALLBACK (change_autostart_wait_systray), pref);
	pref->priv->autostart_delay_handler = g_signal_connect(pref->priv->autostart_delay, "changed", G_CALLBACK(change_autostart_delay), pref);
	pref->priv->autostart_new_pad_handler = g_signal_connect (pref->priv->autostart_new_pad, "toggled", G_CALLBACK (change_autostart_new_pad), pref);
	pref->priv->autostart_sticky_handler = g_signal_connect (pref->priv->autostart_sticky, "toggled", G_CALLBACK (change_autostart_sticky), pref);
	pref->priv->autostart_display_pads_handler = g_signal_connect (pref->priv->autostart_display_pads, "changed", G_CALLBACK (change_autostart_display_pads), pref);

	pref->priv->tray_enabled_handler = g_signal_connect (pref->priv->tray_enabled, "toggled", G_CALLBACK (change_tray_enabled), pref);
	pref->priv->tray_click_handler = g_signal_connect(pref->priv->tray_click_configuration, "changed", G_CALLBACK(change_tray_click), pref);
	pref->priv->editcheck_handler = g_signal_connect (pref->priv->editcheck, "toggled", G_CALLBACK (change_edit_check), pref);
	pref->priv->confirmcheck_handler = g_signal_connect (pref->priv->confirmcheck, "toggled", G_CALLBACK (change_confirm_check), pref);
	pref->priv->line_numbering_handler = g_signal_connect (pref->priv->line_numbering, "toggled", G_CALLBACK (change_line_numbering), pref);

	pref->priv->notify_has_decorations_handler = g_signal_connect_swapped (pref->priv->settings, "notify::has-decorations", G_CALLBACK (notify_has_decorations), pref);
	pref->priv->notify_hide_from_taskbar_handler = g_signal_connect_swapped (pref->priv->settings, "notify::hide-from-taskbar", G_CALLBACK (notify_hide_from_taskbar), pref);
	pref->priv->notify_hide_from_task_switcher_handler = g_signal_connect_swapped (pref->priv->settings, "notify::hide-from-task-switcher", G_CALLBACK (notify_hide_from_task_switcher), pref);
	pref->priv->notify_has_toolbar_handler = g_signal_connect_swapped (pref->priv->settings, "notify::has-toolbar", G_CALLBACK (notify_has_toolbar), pref);
	pref->priv->notify_autohide_toolbar_handler = g_signal_connect_swapped (pref->priv->settings, "notify::autohide-toolbar", G_CALLBACK (notify_autohide_toolbar), pref);
	pref->priv->notify_has_scrollbar_handler = g_signal_connect_swapped (pref->priv->settings, "notify::has-scrollbar", G_CALLBACK (notify_has_scrollbar), pref);

	pref->priv->notify_font_handler = g_signal_connect_swapped (pref->priv->settings, "notify::fontname", G_CALLBACK (notify_fontname), pref);
	pref->priv->notify_text_handler = g_signal_connect_swapped (pref->priv->settings, "notify::text-color", G_CALLBACK (notify_text_color), pref);
	pref->priv->notify_back_handler = g_signal_connect_swapped (pref->priv->settings, "notify::back-color", G_CALLBACK (notify_back_color), pref);

	pref->priv->notify_autostart_xpad_handler = g_signal_connect_swapped (pref->priv->settings, "notify::autostart-xpad", G_CALLBACK (notify_autostart_xpad), pref);
	pref->priv->notify_autostart_wait_systray_handler = g_signal_connect_swapped (pref->priv->settings, "notify::autostart-wait-systray", G_CALLBACK (notify_autostart_wait_systray), pref);
	pref->priv->notify_autostart_delay_handler = g_signal_connect_swapped (pref->priv->settings, "notify::autostart-delay", G_CALLBACK(notify_autostart_delay), pref);
	pref->priv->notify_autostart_new_pad_handler = g_signal_connect_swapped (pref->priv->settings, "notify::autostart-new-pad", G_CALLBACK (notify_autostart_new_pad), pref);
	pref->priv->notify_autostart_sticky_handler = g_signal_connect_swapped (pref->priv->settings, "notify::autostart-sticky", G_CALLBACK (notify_autostart_sticky), pref);
	pref->priv->notify_autostart_display_pads_handler = g_signal_connect_swapped (pref->priv->settings, "notify::autostart-display-pads", G_CALLBACK (notify_autostart_display_pads), pref);
	pref->priv->notify_edit_handler = g_signal_connect_swapped (pref->priv->settings, "notify::edit-lock", G_CALLBACK (notify_edit), pref);
	pref->priv->notify_confirm_handler = g_signal_connect_swapped (pref->priv->settings, "notify::confirm-destroy", G_CALLBACK (notify_confirm), pref);
	pref->priv->notify_tray_enabled_handler = g_signal_connect_swapped (pref->priv->settings, "notify::tray-enabled", G_CALLBACK (notify_tray_enabled), pref);
	pref->priv->notify_tray_click_handler = g_signal_connect_swapped (pref->priv->settings, "notify::tray-click-configuration", G_CALLBACK(notify_tray_click), pref);
	pref->priv->notify_line_numbering_handler = g_signal_connect_swapped (pref->priv->settings, "notify::line-numbering", G_CALLBACK (notify_line_numbering), pref);

	g_clear_object (&size_group_labels);

	/* Initialize the GUI logic */
	g_object_notify (G_OBJECT (pref->priv->settings), "tray-enabled");

	/* Make the preference window visible */
	gtk_window_set_position (GTK_WINDOW (pref), GTK_WIN_POS_CENTER);
	gtk_widget_show_all (GTK_WIDGET (pref));
	gtk_window_set_resizable (GTK_WINDOW (pref), FALSE);
}

static GtkWidget * create_label (const gchar *label_text) {
	GtkWidget *label = GTK_WIDGET (g_object_new (GTK_TYPE_LABEL,
		"label", g_strconcat ("<b>", label_text, "</b>", NULL),
		"use-markup", TRUE,
		"xalign", 0.0,
		NULL));
	return label;
}

static void
xpad_preferences_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
	XpadPreferences *pref = XPAD_PREFERENCES (object);

	switch (prop_id)
	{
	case PROP_SETTINGS:
		pref->priv->settings = g_value_get_pointer (value);
		g_object_ref (pref->priv->settings);
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
xpad_preferences_get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
	XpadPreferences *pref = XPAD_PREFERENCES (object);

	switch (prop_id)
	{
	case PROP_SETTINGS:
		g_value_set_pointer (value, pref->priv->settings);
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
xpad_preferences_finalize (GObject *object)
{
	XpadPreferences *pref = XPAD_PREFERENCES (object);

	if (pref->priv->settings)
		g_signal_handlers_disconnect_matched (pref->priv->settings, G_SIGNAL_MATCH_DATA, 0, 0, NULL, NULL, pref);

	G_OBJECT_CLASS (xpad_preferences_parent_class)->finalize (object);
}

static void
change_font_check (GtkToggleButton *button, XpadPreferences *pref)
{
	g_signal_handler_block (pref->priv->settings, pref->priv->notify_font_handler);

	if (gtk_toggle_button_get_active (button)) {
		g_object_set (pref->priv->settings, "fontname", gtk_font_chooser_get_font (GTK_FONT_CHOOSER (pref->priv->fontbutton)), NULL);
	} else {
		g_object_set (pref->priv->settings, "fontname", NULL, NULL);
	}

	gtk_widget_set_sensitive (pref->priv->fontbutton, gtk_toggle_button_get_active (button));
	g_signal_handler_unblock (pref->priv->settings, pref->priv->notify_font_handler);
}

static void
change_font_face (GtkFontChooser *button, XpadPreferences *pref)
{
	g_signal_handler_block (pref->priv->settings, pref->priv->notify_font_handler);
	g_object_set (pref->priv->settings, "fontname", gtk_font_chooser_get_font (button), NULL);
	g_signal_handler_unblock (pref->priv->settings, pref->priv->notify_font_handler);
}

static void
change_color_check (GtkToggleButton *button, XpadPreferences *pref)
{
	g_signal_handler_block (pref->priv->settings, pref->priv->notify_text_handler);
	g_signal_handler_block (pref->priv->settings, pref->priv->notify_back_handler);

	if (gtk_toggle_button_get_active (button)) {
		GdkRGBA text_color, back_color;
		gtk_color_chooser_get_rgba (GTK_COLOR_CHOOSER (pref->priv->textbutton), &text_color);
		gtk_color_chooser_get_rgba (GTK_COLOR_CHOOSER (pref->priv->backbutton), &back_color);
		g_object_set (pref->priv->settings, "text-color", &text_color, "back-color", &back_color, NULL);
	} else {
		g_object_set (pref->priv->settings, "text-color", NULL, "back-color", NULL, NULL);
	}

	gtk_widget_set_sensitive (pref->priv->colorbox, gtk_toggle_button_get_active (button));

	g_signal_handler_unblock (pref->priv->settings, pref->priv->notify_text_handler);
	g_signal_handler_unblock (pref->priv->settings, pref->priv->notify_back_handler);
}

static void
change_text_color (GtkColorChooser *chooser, XpadPreferences *pref)
{
	GdkRGBA text_color = {0, 0, 0, 0};
	gtk_color_chooser_get_rgba (chooser, &text_color);

	g_signal_handler_block (pref->priv->settings, pref->priv->notify_text_handler);
	g_object_set (pref->priv->settings, "text-color", &text_color, NULL);
	g_signal_handler_unblock (pref->priv->settings, pref->priv->notify_text_handler);
}

static void
change_back_color (GtkColorChooser *chooser, XpadPreferences *pref)
{
	GdkRGBA back_color = {0, 0, 0, 0};
	gtk_color_chooser_get_rgba (chooser, &back_color);

	g_signal_handler_block (pref->priv->settings, pref->priv->notify_back_handler);
	g_object_set (pref->priv->settings, "back-color", &back_color, NULL);
	g_signal_handler_unblock (pref->priv->settings, pref->priv->notify_back_handler);
}

static void
change_autostart_xpad (GtkToggleButton *button, XpadPreferences *pref)
{
	g_object_set (pref->priv->settings, "autostart-xpad", gtk_toggle_button_get_active (button), NULL);
}

static void
change_autostart_wait_systray (GtkToggleButton *button, XpadPreferences *pref)
{
	GKeyFile *keyfile;
	GKeyFileFlags flags;
	GError *error = NULL;
	char *filename;
	gboolean wait_systray;

	wait_systray = gtk_toggle_button_get_active (button);

	/* Create a new GKeyFile object and a bitwise list of flags. */
	keyfile = g_key_file_new ();
	filename = g_strdup_printf ("%s/.config/autostart/xpad.desktop", g_get_home_dir());
	flags = G_KEY_FILE_KEEP_COMMENTS | G_KEY_FILE_KEEP_TRANSLATIONS;

	/* Load the GKeyFile from xpad.desktop or show an error message. */
	if (!g_key_file_load_from_file (keyfile, filename, flags, &error)) {
		gchar *errtext;
		errtext = g_strdup_printf (_("Could not load %s\n%s"), filename, error->message);
		xpad_app_error (NULL, _("Failed to change the time delay of starting the Xpad systray"), errtext);
		g_free (errtext);

		gtk_toggle_button_set_active (button, !wait_systray);
		return;
	}

	g_key_file_set_boolean (keyfile, "Desktop Entry", "X-LXQt-Need-Tray", wait_systray);

	if (!g_key_file_save_to_file (keyfile, filename, &error)) {
		gchar *errtext;
		errtext = g_strdup_printf (_("Could not save %s\n%s"), filename, error->message);
		xpad_app_error (NULL, _("Failed to change the time delay of starting the Xpad systray"), errtext);
		g_free (errtext);

		gtk_toggle_button_set_active (button, !wait_systray);
		return;
	}

	g_signal_handler_block (pref->priv->settings, pref->priv->notify_autostart_wait_systray_handler);
	g_object_set (pref->priv->settings, "autostart-wait-systray", wait_systray, NULL);
	g_signal_handler_unblock (pref->priv->settings, pref->priv->notify_autostart_wait_systray_handler);
}

static void
change_autostart_new_pad (GtkToggleButton *button, XpadPreferences *pref)
{
	g_signal_handler_block (pref->priv->settings, pref->priv->notify_autostart_new_pad_handler);
	g_object_set (pref->priv->settings, "autostart-new-pad", gtk_toggle_button_get_active (button), NULL);
	g_signal_handler_unblock (pref->priv->settings, pref->priv->notify_autostart_new_pad_handler);
}

static void
change_autostart_sticky (GtkToggleButton *button, XpadPreferences *pref)
{
	g_signal_handler_block (pref->priv->settings, pref->priv->notify_autostart_sticky_handler);
	gboolean is_sticky = gtk_toggle_button_get_active (button);
	g_object_set (pref->priv->settings, "autostart-sticky", is_sticky, NULL);
	xpad_pad_group_update_sticky (xpad_app_get_pad_group(), is_sticky);
	g_signal_handler_unblock (pref->priv->settings, pref->priv->notify_autostart_sticky_handler);
}

static void
change_autostart_delay (GtkComboBox *box, XpadPreferences *pref)
{
	g_signal_handler_block(pref->priv->settings, pref->priv->notify_autostart_delay_handler);
	g_object_set (pref->priv->settings, "autostart-delay", (guint) gtk_combo_box_get_active (box), NULL);
	g_signal_handler_unblock (pref->priv->settings, pref->priv->notify_autostart_delay_handler);
}

static void
change_autostart_display_pads (GtkComboBox *box, XpadPreferences *pref)
{
	g_signal_handler_block (pref->priv->settings, pref->priv->notify_autostart_display_pads_handler);
	g_object_set (pref->priv->settings, "autostart-display-pads", (guint) gtk_combo_box_get_active (box), NULL);
	g_signal_handler_unblock(pref->priv->settings, pref->priv->notify_autostart_display_pads_handler);
}

static void
change_tray_enabled (GtkToggleButton *button, XpadPreferences *pref)
{
	g_object_set (pref->priv->settings, "tray-enabled", gtk_toggle_button_get_active (button), NULL);
}

static void
change_tray_click (GtkComboBox *box, XpadPreferences *pref)
{
	g_signal_handler_block(pref->priv->settings, pref->priv->notify_tray_click_handler);
	g_object_set (pref->priv->settings, "tray-click-configuration", (guint) gtk_combo_box_get_active (box), NULL);
	g_signal_handler_unblock (pref->priv->settings, pref->priv->notify_tray_click_handler);
}

static void
change_edit_check (GtkToggleButton *button, XpadPreferences *pref)
{
	g_signal_handler_block (pref->priv->settings, pref->priv->notify_edit_handler);
	g_object_set (pref->priv->settings, "edit-lock", gtk_toggle_button_get_active (button), NULL);
	g_signal_handler_unblock (pref->priv->settings, pref->priv->notify_edit_handler);
}

static void
change_confirm_check (GtkToggleButton *button, XpadPreferences *pref)
{
	g_signal_handler_block (pref->priv->settings, pref->priv->notify_confirm_handler);
	g_object_set (pref->priv->settings, "confirm-destroy", gtk_toggle_button_get_active (button), NULL);
	g_signal_handler_unblock (pref->priv->settings, pref->priv->notify_confirm_handler);
}

static void
change_has_decorations (GtkToggleButton *button, XpadPreferences *pref)
{
	g_signal_handler_block (pref->priv->settings, pref->priv->notify_has_decorations_handler);
	g_object_set (pref->priv->settings, "has-decorations", gtk_toggle_button_get_active (button), NULL);
	g_signal_handler_unblock (pref->priv->settings, pref->priv->notify_has_decorations_handler);
}

static void
change_hide_from_taskbar (GtkToggleButton *button, XpadPreferences *pref)
{
	g_signal_handler_block (pref->priv->settings, pref->priv->notify_hide_from_taskbar_handler);
	g_object_set (pref->priv->settings, "hide-from-taskbar", gtk_toggle_button_get_active (button), NULL);
	g_signal_handler_unblock (pref->priv->settings, pref->priv->notify_hide_from_taskbar_handler);
}

static void
change_hide_from_task_switcher (GtkToggleButton *button, XpadPreferences *pref)
{
	g_signal_handler_block (pref->priv->settings, pref->priv->notify_hide_from_task_switcher_handler);
	g_object_set (pref->priv->settings, "hide-from-task-switcher", gtk_toggle_button_get_active (button), NULL);
	g_signal_handler_unblock (pref->priv->settings, pref->priv->notify_hide_from_task_switcher_handler);
}

static void
change_has_toolbar (GtkToggleButton *button, XpadPreferences *pref)
{
	g_object_set (pref->priv->settings, "has-toolbar", gtk_toggle_button_get_active (button), NULL);
}

static void
change_autohide_toolbar (GtkToggleButton *button, XpadPreferences *pref)
{
	g_signal_handler_block (pref->priv->settings, pref->priv->notify_autohide_toolbar_handler);
	g_object_set (pref->priv->settings, "autohide-toolbar", gtk_toggle_button_get_active (button), NULL);
	g_signal_handler_unblock (pref->priv->settings, pref->priv->notify_autohide_toolbar_handler);
}

static void
change_has_scrollbar (GtkToggleButton *button, XpadPreferences *pref)
{
	g_signal_handler_block (pref->priv->settings, pref->priv->notify_has_scrollbar_handler);
	g_object_set (pref->priv->settings, "has-scrollbar", gtk_toggle_button_get_active (button), NULL);
	g_signal_handler_unblock (pref->priv->settings, pref->priv->notify_has_scrollbar_handler);
}

static void
change_new_pad_height (GtkSpinButton *button, XpadPreferences *pref)
{
	g_object_set (pref->priv->settings, "height", gtk_spin_button_get_value_as_int(button), NULL);
}

static void
change_new_pad_width (GtkSpinButton *button, XpadPreferences *pref)
{
	g_object_set (pref->priv->settings, "width", gtk_spin_button_get_value_as_int(button), NULL);
}

static void
change_line_numbering (GtkToggleButton *button, XpadPreferences *pref)
{
	g_signal_handler_block (pref->priv->settings, pref->priv->notify_line_numbering_handler);
	g_object_set (pref->priv->settings, "line-numbering", gtk_toggle_button_get_active (button), NULL);
	g_signal_handler_unblock (pref->priv->settings, pref->priv->notify_line_numbering_handler);
}

static void
notify_fontname (XpadPreferences *pref)
{
	const gchar *fontname;
	g_object_get (pref->priv->settings, "fontname", &fontname, NULL);

	g_signal_handler_block (pref->priv->fontbutton, pref->priv->font_handler);
	g_signal_handler_block (pref->priv->fontcheck, pref->priv->fontcheck_handler);

	if (fontname)
	{
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (pref->priv->fontcheck), TRUE);
		gtk_widget_set_sensitive (pref->priv->fontbutton, TRUE);
		gtk_font_chooser_set_font (GTK_FONT_CHOOSER (pref->priv->fontbutton), fontname);
	}
	else
	{
		gtk_widget_set_sensitive (pref->priv->fontbutton, FALSE);
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (pref->priv->antifontcheck), TRUE);
	}

	g_signal_handler_unblock (pref->priv->fontcheck, pref->priv->fontcheck_handler);
	g_signal_handler_unblock (pref->priv->fontbutton, pref->priv->font_handler);
}

static void
notify_text_color (XpadPreferences *pref)
{
	const GdkRGBA *text_color;
	g_object_get (pref->priv->settings, "text-color", &text_color, NULL);

	g_signal_handler_block (pref->priv->textbutton, pref->priv->text_handler);
	g_signal_handler_block (pref->priv->colorcheck, pref->priv->colorcheck_handler);

	if (text_color)
	{
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (pref->priv->colorcheck), TRUE);
		gtk_widget_set_sensitive (pref->priv->colorbox, TRUE);
		gtk_color_chooser_set_rgba (GTK_COLOR_CHOOSER (pref->priv->textbutton), text_color);

	}
	else
	{
		gtk_widget_set_sensitive (pref->priv->colorbox, FALSE);
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (pref->priv->anticolorcheck), TRUE);
	}

	g_signal_handler_unblock (pref->priv->colorcheck, pref->priv->colorcheck_handler);
	g_signal_handler_unblock (pref->priv->textbutton, pref->priv->text_handler);
}

static void
notify_back_color (XpadPreferences *pref)
{
	const GdkRGBA *back_color;
	g_object_get (pref->priv->settings, "back-color", &back_color, NULL);

	g_signal_handler_block (pref->priv->backbutton, pref->priv->back_handler);
	g_signal_handler_block (pref->priv->colorcheck, pref->priv->colorcheck_handler);

	if (back_color)
	{
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (pref->priv->colorcheck), TRUE);
		gtk_widget_set_sensitive (pref->priv->colorbox, TRUE);
		gtk_color_chooser_set_rgba (GTK_COLOR_CHOOSER (pref->priv->backbutton), back_color);
	}
	else
	{
		gtk_widget_set_sensitive (pref->priv->colorbox, FALSE);
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (pref->priv->anticolorcheck), TRUE);
	}

	g_signal_handler_unblock (pref->priv->colorcheck, pref->priv->colorcheck_handler);
	g_signal_handler_unblock (pref->priv->backbutton, pref->priv->back_handler);
}

static void
notify_autostart_xpad (XpadPreferences *pref)
{
	gboolean value;
	g_object_get (pref->priv->settings, "autostart-xpad", &value, NULL);
	g_signal_handler_block (pref->priv->autostart_xpad, pref->priv->autostart_xpad_handler);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (pref->priv->autostart_xpad), value);
	gtk_widget_set_sensitive (pref->priv->autostart_wait_systray, value);
	if (value)
		change_autostart_wait_systray (GTK_TOGGLE_BUTTON (pref->priv->autostart_wait_systray), pref);
	g_signal_handler_unblock (pref->priv->autostart_xpad, pref->priv->autostart_xpad_handler);
}

static void
notify_autostart_wait_systray (XpadPreferences *pref)
{
	gboolean value;
	g_object_get (pref->priv->settings, "autostart-wait-systray", &value, NULL);
	g_signal_handler_block (pref->priv->autostart_wait_systray, pref->priv->autostart_wait_systray_handler);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (pref->priv->autostart_wait_systray), value);
	g_signal_handler_unblock (pref->priv->autostart_wait_systray, pref->priv->autostart_wait_systray_handler);
}

static void
notify_autostart_new_pad (XpadPreferences *pref)
{
	gboolean value;
	g_object_get (pref->priv->settings, "autostart-new-pad", &value, NULL);
	g_signal_handler_block (pref->priv->autostart_new_pad, pref->priv->autostart_new_pad_handler);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (pref->priv->autostart_new_pad), value);
	g_signal_handler_unblock (pref->priv->autostart_new_pad, pref->priv->autostart_new_pad_handler);
}

static void
notify_autostart_sticky (XpadPreferences *pref)
{
	gboolean value;
	g_object_get (pref->priv->settings, "autostart-sticky", &value, NULL);
	g_signal_handler_block (pref->priv->autostart_sticky, pref->priv->autostart_sticky_handler);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (pref->priv->autostart_sticky), value);
	g_signal_handler_unblock (pref->priv->autostart_sticky, pref->priv->autostart_sticky_handler);
}

static void
notify_autostart_delay (XpadPreferences *pref)
{
	guint value;
	g_object_get (pref->priv->settings, "autostart-delay", &value, NULL);
	g_signal_handler_block (pref->priv->autostart_delay, pref->priv->autostart_delay_handler);
	gtk_combo_box_set_active (GTK_COMBO_BOX (pref->priv->autostart_delay), value);
	g_signal_handler_unblock (pref->priv->autostart_delay, pref->priv->autostart_delay_handler);
}

static void
notify_autostart_display_pads (XpadPreferences *pref)
{
	guint value;
	g_object_get (pref->priv->settings, "autostart-display-pads", &value, NULL);
	g_signal_handler_block (pref->priv->autostart_display_pads, pref->priv->autostart_display_pads_handler);
	gtk_combo_box_set_active (GTK_COMBO_BOX (pref->priv->autostart_display_pads), value);
	g_signal_handler_unblock (pref->priv->autostart_display_pads, pref->priv->autostart_display_pads_handler);
}

static void
notify_tray_enabled (XpadPreferences *pref)
{
	gboolean value;
	g_object_get (pref->priv->settings, "tray-enabled", &value, NULL);
	g_signal_handler_block (pref->priv->tray_enabled, pref->priv->tray_enabled_handler);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (pref->priv->tray_enabled), value);
	gtk_widget_set_sensitive (pref->priv->tray_click_configuration, value);
	gtk_widget_set_sensitive (pref->priv->autostart_display_pads, value);
	if (!value)
		g_object_set (pref->priv->settings, "autostart-display-pads", 0, NULL);
	g_signal_handler_unblock (pref->priv->tray_enabled, pref->priv->tray_enabled_handler);
}

static void
notify_tray_click (XpadPreferences *pref)
{
	guint value;
	g_object_get (pref->priv->settings, "tray-click-configuration", &value, NULL);
	g_signal_handler_block (pref->priv->tray_click_configuration, pref->priv->tray_click_handler);
	gtk_combo_box_set_active (GTK_COMBO_BOX (pref->priv->tray_click_configuration), value);
	g_signal_handler_unblock (pref->priv->tray_click_configuration, pref->priv->tray_click_handler);
}

static void
notify_edit (XpadPreferences *pref)
{
	gboolean value;
	g_object_get (pref->priv->settings, "edit-lock", &value, NULL);
	g_signal_handler_block (pref->priv->editcheck, pref->priv->editcheck_handler);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (pref->priv->editcheck), value);
	g_signal_handler_unblock (pref->priv->editcheck, pref->priv->editcheck_handler);
}

static void
notify_confirm (XpadPreferences *pref)
{
	gboolean value;
	g_object_get (pref->priv->settings, "confirm-destroy", &value, NULL);
	g_signal_handler_block (pref->priv->confirmcheck, pref->priv->confirmcheck_handler);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (pref->priv->confirmcheck), value);
	g_signal_handler_unblock (pref->priv->confirmcheck, pref->priv->confirmcheck_handler);
}

static void
notify_has_decorations (XpadPreferences *pref)
{
	gboolean value;
	g_object_get (pref->priv->settings, "has-decorations", &value, NULL);
	g_signal_handler_block (pref->priv->has_decorations, pref->priv->has_decorations_handler);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (pref->priv->has_decorations), value);
	g_signal_handler_unblock (pref->priv->has_decorations, pref->priv->has_decorations_handler);
}

static void
notify_hide_from_taskbar (XpadPreferences *pref)
{
	gboolean value;
	g_object_get (pref->priv->settings, "hide-from-taskbar", &value, NULL);
	g_signal_handler_block (pref->priv->hide_from_taskbar, pref->priv->hide_from_taskbar_handler);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (pref->priv->hide_from_taskbar), value);
	g_signal_handler_unblock (pref->priv->hide_from_taskbar, pref->priv->hide_from_taskbar_handler);
}

static void
notify_hide_from_task_switcher (XpadPreferences *pref)
{
	gboolean value;
	g_object_get (pref->priv->settings, "hide-from-task-switcher", &value, NULL);
	g_signal_handler_block (pref->priv->hide_from_task_switcher, pref->priv->hide_from_task_switcher_handler);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (pref->priv->hide_from_task_switcher), value);
	g_signal_handler_unblock (pref->priv->hide_from_task_switcher, pref->priv->hide_from_task_switcher_handler);
}

static void
notify_has_toolbar (XpadPreferences *pref)
{
	gboolean value;
	g_object_get (pref->priv->settings, "has-toolbar", &value, NULL);
	g_signal_handler_block (pref->priv->has_toolbar, pref->priv->has_toolbar_handler);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (pref->priv->has_toolbar), value);
	gtk_widget_set_sensitive (pref->priv->autohide_toolbar, value);
	g_signal_handler_unblock (pref->priv->has_toolbar, pref->priv->has_toolbar_handler);
}

static void
notify_autohide_toolbar (XpadPreferences *pref)
{
	gboolean value;
	g_object_get (pref->priv->settings, "autohide-toolbar", &value, NULL);
	g_signal_handler_block (pref->priv->autohide_toolbar, pref->priv->autohide_toolbar_handler);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (pref->priv->autohide_toolbar), value);
	g_signal_handler_unblock (pref->priv->autohide_toolbar, pref->priv->autohide_toolbar_handler);
}

static void
notify_has_scrollbar (XpadPreferences *pref)
{
	gboolean value;
	g_object_get (pref->priv->settings, "has-scrollbar", &value, NULL);
	g_signal_handler_block (pref->priv->has_scrollbar, pref->priv->has_scrollbar_handler);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (pref->priv->has_scrollbar), value);
	g_signal_handler_unblock (pref->priv->has_scrollbar, pref->priv->has_scrollbar_handler);
}

static void
notify_line_numbering (XpadPreferences *pref)
{
	gboolean value;
	g_object_get (pref->priv->settings, "line-numbering", &value, NULL);
	g_signal_handler_block (pref->priv->line_numbering, pref->priv->line_numbering_handler);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (pref->priv->line_numbering), value);
	g_signal_handler_unblock (pref->priv->line_numbering, pref->priv->line_numbering_handler);
}
