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

#include <stdlib.h>
#include <string.h>
#include <glib/gi18n.h>

#include "xpad-periodic.h"

#ifdef SHOW_DEBUG
#   define G_PRINT_DBG g_print
#else
#   define G_PRINT_DBG gprint_ignore
#endif

#define TIMEOUT_SECONDS     4

struct sigref_ {
	const char *        signame;
	XpadPeriodicFunc    func_ptr;
	gpointer            data;
};

typedef struct sigref_ Xpadsigref;

typedef struct {
	/************************
	count = a clock tick count
	after_id = the timeout id
	************************/
	int                 count;
	int                 after_id;

	/************************
	template = a list of signal names and function pointers
	template_len = the length of 'template'
	sigs = a list of signal names, function pointers and data
	sigs_len = the length of 'sigs'
	************************/
	Xpadsigref *        template;
	int                 template_len;
	Xpadsigref *        sigs;
	int                 sigs_len;
} XpadPeriodic;


/* prototypes */
static gint xppd_intercept (gpointer);
static gint gprint_ignore(const char *, ...);
static void xpad_periodic_signal (const char * cbname, void * xpad_pad);
static void xpad_periodic_error_exit (const char *, ...);

static gboolean str_equal (const char *, const char *);

/* global variables */
static XpadPeriodic xpptr [1];
static gboolean is_activated;

/* Functions start here */

gboolean xpad_periodic_init (void)
{
	memset(xpptr, 0, sizeof(*xpptr));
	xpptr->after_id = (gint) g_timeout_add_seconds(TIMEOUT_SECONDS, xppd_intercept, xpptr);

	/* Allocate space for the signal references. */
	int tlen = xpptr->template_len = 5;
	int slen = xpptr->sigs_len = 200;
	xpptr->template = g_malloc0((gsize) tlen * sizeof(Xpadsigref));
	xpptr->sigs = g_malloc0((gsize) slen * sizeof(Xpadsigref));
	is_activated = TRUE;

	return is_activated;
}

void xpad_periodic_close (void)
{
	is_activated = FALSE;
	if (xpptr->after_id) { g_source_remove((guint) xpptr->after_id); }
	/* Free the signal references memory. */
	g_free(xpptr->template);
	g_free(xpptr->sigs);
	/* Signal that this structure is now cleared. */
	memset(xpptr, 0, sizeof(*xpptr));
}

	/************************
	xppd_intercept - intercepts a timer tick

	This function intercepts a timer tick and iterates
	over the signal references. Any signal references that
	are fully stocked with signal names, function pointers
	and data pointers are invoked.

	IOW (In other words), the function pointer is called with the
	right data pointer.
	************************/

gint xppd_intercept (gpointer cdata)
{
	/* A dirty way to silence the compiler for these unused variables. */
	(void) cdata;

	int cnt=0;
	XpadPeriodicFunc fnptr=0;
	xpptr->count++; /* increment tick count */

	G_PRINT_DBG("xppd tick: %4d\n", xpptr->count);

	for (cnt = 0; cnt < xpptr->sigs_len; ++cnt) {
		Xpadsigref * sig_item = xpptr->sigs + cnt;
		if (sig_item->signame && sig_item->func_ptr && sig_item->data) {
			fnptr = sig_item->func_ptr;
			(*fnptr)(sig_item->data);
			G_PRINT_DBG("invoked %s : %p : %p\n", sig_item->signame,
				    sig_item->func_ptr, sig_item->data);
			memset(sig_item, 0, sizeof(*sig_item));
		}
	}

	return TRUE;
}

	/************************
	Xpad_periodic_set_callback():
	This function prepares a callback function to be invoked
	for an event name such as "save-content" or "save-info".

	cbname :        event name (or callback function name)
	func :          function address

	Returns true if a callback was registered.
	************************/

gboolean xpad_periodic_set_callback (const char * cbname, XpadPeriodicFunc func)
{
	int index = 0;
	gboolean isdone=FALSE;
	if (0 == func) { return FALSE; }
	if (0 == cbname || 0==*cbname) { return FALSE; }

	/* Find an open slot for signal (callback) references and
	insert this one. */
	for (index = 0; index < xpptr->template_len; ++index) {
		/* Store a pointer to the current signal item. */
		Xpadsigref * sig_item = xpptr->template + index;

		/* If it's empty, set it. */
		if (0 == sig_item->signame) {
			sig_item->signame = cbname;
			sig_item->func_ptr = func;
			isdone = TRUE;
			break;
		}
	}

	if (! isdone) {
		g_printerr("Failed to install signal callback: %s\n", cbname);
		exit(1);
	}

	return isdone;
}

void xpad_periodic_save_info_delayed (void * xpad_pad)
{
	if (is_activated) {
		xpad_periodic_signal("save-info", xpad_pad);
	}
}

void xpad_periodic_save_content_delayed (void * xpad_pad)
{
	if (is_activated) {
		xpad_periodic_signal("save-content", xpad_pad);
	}
}

static void xpad_periodic_signal (const char * cbname, void * xpad_pad)
{
	gboolean isdone = FALSE;
	int tnx = 0;
	int snx = 0;
	XpadPeriodicFunc func_ptr = 0;
	Xpadsigref * sig_item = 0;

	if (0 == cbname || 0 == *cbname) {
		return;
	}

	if (0 == xpad_pad) {
		return;
	}

	/* Get the callback function address */
	for (tnx = 0; tnx < xpptr->template_len; ++tnx) {
		if (str_equal(xpptr->template[tnx].signame, cbname)) {
			func_ptr = xpptr->template[tnx].func_ptr;
			break;
		}
	}

	/* If there is no callback address, we can't continue. */
	if (! func_ptr) {
		xpad_periodic_error_exit ("Can't find signal function address: %s\n", cbname);
	}

	/* Check that this event is not already present. 
	If it is present, don't do anything more. */
	for (snx = 0; snx < xpptr->sigs_len; ++snx) {
		sig_item = xpptr->sigs + snx;

		if (str_equal(sig_item->signame, cbname) && xpad_pad == sig_item->data) {
			G_PRINT_DBG("Already got signal for this pad: %s\n", cbname);
			return;
		}
	}

	/* Find a suitable slot for the signal reference and set it. */
	for (snx = 0; snx < xpptr->sigs_len; ++snx) {
		gint doadd = 0;
		sig_item = xpptr->sigs + snx;

		doadd += (str_equal(sig_item->signame, cbname) && xpad_pad == sig_item->data);
		doadd += (0 == sig_item->signame);

		if (doadd) {
			sig_item->signame = cbname;
			sig_item->func_ptr = func_ptr;
			sig_item->data = xpad_pad;
			isdone = TRUE;
			break;
		}
	}

	if (! isdone) {
		xpad_periodic_error_exit("Could not schedule event: %s\n", cbname);
	}
}

gboolean str_equal (const char * s1, const char * s2)
{
	if (0 == s1 || 0==s2) { return FALSE; }
	if (s1 == s2) { return TRUE; }
	return (0 == strcmp(s1, s2));
}

gint gprint_ignore (const char * fmt, ...)
{
	/* A dirty way to silence the compiler for these unused variables. */
	(void) fmt;

	return 0;
}

static void xpad_periodic_error_exit (const char * fmt, ...)
{
	va_list app;
	va_start(app, fmt);
	g_printerr(fmt, app);
	va_end(app);
	exit(1);
}
