/* GIMP LiquidRescale Plug-in
 * Copyright (C) 2007-2009 Carlo Baldassi (the "Author") <carlobaldassi@gmail.com>.
 * All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the Licence, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org.licences/>.
 */

#include "config.h"

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include <math.h>
#include <string.h>
#include <signal.h>
#include <sys/time.h>

#include <lqr.h>

#include "plugin-intl.h"
#include "main.h"
#include "render.h"
#include "interface_I.h"
#include "preview.h"
#include "layers_combo.h"
#include "altsizeentry.h"
#include "altcoordinates.h"


/***  Constants  ***/

#define SCALE_WIDTH         (80)
#define SPIN_BUTTON_WIDTH   (75)
#define MAX_COEFF	  (3000)
#define MAX_RIGIDITY      (1000)
#define MAX_DELTA_X         (10)
#define MAX_STRING_SIZE   (2048)
#define ALARM_DELAY          (4)


/***  Local functions declariations  ***/

/* Callbacks */
static void callback_dialog_I_response (GtkWidget * dialog, gint response_id,
				      gpointer data);
//static void callback_noninter_button (GtkWidget * button, gpointer data);

static void callback_resetvalues_button (GtkWidget * button, gpointer data);
static void callback_flatten_button (GtkWidget * button, gpointer data);
static void callback_dump_button (GtkWidget * button, gpointer data);
static void callback_show_info_button (GtkWidget * button, gpointer data);

//static void callback_set_disc_warning (GtkWidget * dummy, gpointer data);
static void callback_size_changed (GtkWidget * size_entry, gpointer data);
static void set_alarm (glong dseconds);
static void alarm_handler (int signum);
static void set_info_label_text (InterfaceIData * p_data);
static void callback_alarm_triggered (GtkWidget * size_entry, gpointer data);
//static void callback_res_order_changed (GtkWidget * res_order, gpointer data);

/***  Local variables  ***/

gint dialog_I_response = GTK_RESPONSE_OK;

PlugInUIVals *ui_state;
PlugInVals *state;
PlugInDialogVals *dialog_state;
gboolean features_are_sensitive;
InterfaceIData interface_I_data;
//volatile sig_atomic_t interface_locked = 0;

GtkWidget *dlg;
GtkTooltips *dlg_tips;
GtkWidget *coordinates;


/***  Public functions  ***/

gint
dialog_I (gint32 image_ID, gint32 layer_ID,
	PlugInVals * vals,
	PlugInImageVals * image_vals,
	PlugInDrawableVals * drawable_vals, PlugInUIVals * ui_vals,
        PlugInColVals * col_vals, PlugInDialogVals * dialog_vals)
{
  struct sigaction alarm_action, old_alarm_action;
  gint orig_width, orig_height;
  GtkWidget *main_hbox;
  GtkWidget *vbox;
  GtkWidget *vbox2;
  GtkWidget *vbox3;
  GtkWidget *hbox;
  GtkWidget *hbox2;
  GtkWidget *frame;
  //GtkWidget *noninter_button;
  GtkWidget *resetvalues_event_box;
  GtkWidget *resetvalues_button;
  GtkWidget *resetvalues_icon;

  GtkWidget *flatten_event_box;
  GtkWidget *flatten_button;
  GtkWidget *flatten_icon;
  GtkWidget *show_info_event_box;
  GtkWidget *show_info_button;
  GtkWidget *show_info_icon;
  GtkWidget *dump_event_box;
  GtkWidget *dump_button;
  GtkWidget *dump_icon;
  //GtkWidget *lastvalues_event_box;
  //GtkWidget *lastvalues_button;
  //GtkWidget *lastvalues_icon;
  gboolean has_mask = FALSE;
  GimpUnit unit;
  gdouble xres, yres;
  GtkWidget * v_separator;
  GtkWidget *info_title_label;
  GtkWidget * info_label;

  CarverData * carver_data;

  state = g_new (PlugInVals, 1);
  memcpy (state, vals, sizeof (PlugInVals));

  ui_state = g_new (PlugInUIVals, 1);
  memcpy (ui_state, ui_vals, sizeof (PlugInUIVals));

  dialog_state = dialog_vals;

  orig_width = gimp_drawable_width (layer_ID);
  orig_height = gimp_drawable_height (layer_ID);

  g_assert (gimp_drawable_is_layer (layer_ID) == TRUE);

  drawable_vals->layer_ID = layer_ID;
  interface_I_data.image_ID = image_ID;
  interface_I_data.drawable_vals = drawable_vals;
  interface_I_data.orig_width = orig_width;
  interface_I_data.orig_height = orig_height;
  interface_I_data.col_vals = col_vals;


  if (gimp_layer_get_mask (layer_ID) != -1)
    {
      has_mask = TRUE;
    }

  dlg = gtk_dialog_new_with_buttons (_("GIMP LiquidRescale Plug-In"), 
			 NULL, 0,
			 //GIMP_STOCK_RESET, RESPONSE_RESET,
			 //GTK_STOCK_REFRESH, RESPONSE_REFRESH,
			 GTK_STOCK_GO_BACK, RESPONSE_NONINTERACTIVE,
			 GTK_STOCK_CLOSE, GTK_RESPONSE_OK, NULL);

  gtk_window_set_resizable (GTK_WINDOW (dlg), FALSE);

  if (dialog_state->has_pos)
    {
      //printf("move window, x,y=%i,%i\n", dialog_state->x, dialog_state->y); fflush(stdout);
      gtk_window_move (GTK_WINDOW(dlg), dialog_state->x, dialog_state->y);
      dialog_state->has_pos = FALSE;
    }

  g_signal_connect (dlg, "response", G_CALLBACK (callback_dialog_I_response),
		    (gpointer) (NULL));

  dlg_tips = gtk_tooltips_new ();

  main_hbox = gtk_hbox_new (FALSE, 12);
  gtk_container_set_border_width (GTK_CONTAINER (main_hbox), 12);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dlg)->vbox), main_hbox);

  vbox = gtk_vbox_new (FALSE, 12);
  gtk_box_pack_start (GTK_BOX (main_hbox), vbox, TRUE, TRUE, 0);
  gtk_widget_show (vbox);

  /*  New size  */

  frame = gimp_frame_new (_("Set width and height"));
  gtk_box_pack_start (GTK_BOX (vbox), frame, TRUE, TRUE, 0);
  gtk_widget_show (frame);

  //interface_I_data.size_frame = frame;

  hbox = gtk_hbox_new (FALSE, 4);
  gtk_container_add (GTK_CONTAINER (frame), hbox);
  gtk_widget_show (hbox);

  vbox3 = gtk_vbox_new (FALSE, 4);
  gtk_box_pack_start (GTK_BOX (hbox), vbox3, FALSE, FALSE, 0);
  gtk_widget_show (vbox3);

  unit = gimp_image_get_unit (image_ID);
  gimp_image_get_resolution (image_ID, &xres, &yres);

  coordinates =
    alt_coordinates_new (unit, "%p", TRUE, TRUE, SPIN_BUTTON_WIDTH,
			  ALT_SIZE_ENTRY_UPDATE_SIZE, ui_state->chain_active,
			  TRUE, _("Width:"), state->new_width, xres, 2,
			  GIMP_MAX_IMAGE_SIZE, 0, orig_width,
			  _("Height:"), state->new_height, yres, 2,
			  GIMP_MAX_IMAGE_SIZE, 0, orig_height);

  interface_I_data.coordinates = coordinates;

  g_signal_connect (ALT_SIZE_ENTRY (coordinates), "value-changed",
		    G_CALLBACK (callback_size_changed),
		    (gpointer) & interface_I_data);

  g_signal_connect (ALT_SIZE_ENTRY (coordinates), "refval-changed",
		    G_CALLBACK (callback_size_changed),
		    (gpointer) & interface_I_data);

  sigaction(SIGALRM, NULL, &old_alarm_action);
  alarm_action = old_alarm_action;
  alarm_action.sa_handler = alarm_handler;
  sigaction(SIGALRM, &alarm_action, NULL);

  g_signal_connect (ALT_SIZE_ENTRY (coordinates), "coordinates-alarm",
                    G_CALLBACK (callback_alarm_triggered),
                    (gpointer) & interface_I_data);

  gtk_box_pack_start (GTK_BOX (vbox3), coordinates, FALSE, FALSE, 0);
  gtk_widget_show (coordinates);

  /* Reset size button */

  vbox2 = gtk_vbox_new (FALSE, 4);
  gtk_box_pack_end (GTK_BOX (hbox), vbox2, FALSE, FALSE, 0);
  gtk_widget_show (vbox2);

  resetvalues_event_box = gtk_event_box_new ();
  gtk_box_pack_start (GTK_BOX (vbox2), resetvalues_event_box, FALSE, FALSE,
		      0);
  gtk_widget_show (resetvalues_event_box);

  gimp_help_set_help_data (resetvalues_event_box,
			   _
			   ("Reset width and height to their original values"),
			   NULL);

  resetvalues_button = gtk_button_new ();
  resetvalues_icon =
    gtk_image_new_from_stock (GIMP_STOCK_RESET, GTK_ICON_SIZE_MENU);
  gtk_container_add (GTK_CONTAINER (resetvalues_button), resetvalues_icon);
  gtk_widget_show (resetvalues_icon);
  gtk_container_add (GTK_CONTAINER (resetvalues_event_box),
		     resetvalues_button);
  gtk_widget_show (resetvalues_button);

  g_signal_connect (resetvalues_button, "clicked",
		    G_CALLBACK (callback_resetvalues_button),
		    (gpointer) & interface_I_data);

  //interface_I_data.resetvalues_button = resetvalues_button;

  /* Map info */

  v_separator = gtk_vseparator_new();
  gtk_box_pack_start (GTK_BOX (main_hbox), v_separator, TRUE, TRUE, 0);
  gtk_widget_show(v_separator);

  vbox = gtk_vbox_new (FALSE, 4);
  gtk_box_pack_start (GTK_BOX (main_hbox), vbox, FALSE, FALSE, 0);
  gtk_widget_show (vbox);

  hbox2 = gtk_hbox_new (FALSE, 4);
  gtk_box_pack_start (GTK_BOX (vbox), hbox2, FALSE, FALSE, 0);
  gtk_widget_show (hbox2);

  info_title_label = gtk_label_new ("");
  gtk_label_set_markup(GTK_LABEL(info_title_label), _("<b>Map</b>"));
  gtk_box_pack_start (GTK_BOX (hbox2), info_title_label, FALSE, FALSE, 0);
  gtk_widget_show (info_title_label);

  hbox = gtk_hbox_new (FALSE, 12);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  vbox2 = gtk_vbox_new (FALSE, 4);
  gtk_box_pack_start (GTK_BOX (hbox), vbox2, FALSE, FALSE, 0);
  gtk_widget_show (vbox2);

  show_info_event_box = gtk_event_box_new ();
  gtk_box_pack_start (GTK_BOX (vbox2), show_info_event_box, FALSE, FALSE,
		      0);
  gtk_widget_show (show_info_event_box);

  gimp_help_set_help_data (show_info_event_box,
			   _
			   ("Show/hide internal map information"),
			   NULL);

  show_info_button = gtk_toggle_button_new ();
  show_info_icon =
    gtk_image_new_from_stock (GTK_STOCK_INFO, GTK_ICON_SIZE_MENU);
  gtk_container_add (GTK_CONTAINER (show_info_button), show_info_icon);
  gtk_widget_show (show_info_icon);
  gtk_container_add (GTK_CONTAINER (show_info_event_box),
		     show_info_button);
  gtk_widget_show (show_info_button);

  g_signal_connect (show_info_button, "toggled",
		    G_CALLBACK (callback_show_info_button),
		    (gpointer) & interface_I_data);


  flatten_event_box = gtk_event_box_new ();
  gtk_box_pack_start (GTK_BOX (vbox2), flatten_event_box, FALSE, FALSE,
		      0);
  gtk_widget_show (flatten_event_box);

  gimp_help_set_help_data (flatten_event_box,
			   _
			   ("Reset the internal map"),
			   NULL);

  flatten_button = gtk_button_new ();
  flatten_icon =
    gtk_image_new_from_stock (GIMP_STOCK_RESET, GTK_ICON_SIZE_MENU);
  gtk_container_add (GTK_CONTAINER (flatten_button), flatten_icon);
  gtk_widget_show (flatten_icon);
  gtk_container_add (GTK_CONTAINER (flatten_event_box),
		     flatten_button);
  gtk_widget_show (flatten_button);

  g_signal_connect (flatten_button, "clicked",
		    G_CALLBACK (callback_flatten_button),
		    (gpointer) & interface_I_data);

  dump_event_box = gtk_event_box_new ();
  gtk_box_pack_start (GTK_BOX (vbox2), dump_event_box, FALSE, FALSE,
		      0);
  gtk_widget_show (dump_event_box);

  gimp_help_set_help_data (dump_event_box,
			   _
			   ("Dump the internal map on a new layer (RGB images only)"),
			   NULL);

  dump_button = gtk_button_new ();
  dump_icon =
    gtk_image_new_from_stock (GIMP_STOCK_VISIBLE, GTK_ICON_SIZE_MENU);
  gtk_container_add (GTK_CONTAINER (dump_button), dump_icon);
  gtk_widget_show (dump_icon);
  gtk_container_add (GTK_CONTAINER (dump_event_box),
		     dump_button);
  gtk_widget_show (dump_button);

  g_signal_connect (dump_button, "clicked",
		    G_CALLBACK (callback_dump_button),
		    (gpointer) & interface_I_data);

  gtk_widget_set_sensitive(dump_button, FALSE);
  interface_I_data.dump_button = dump_button;

  info_label = gtk_label_new("");
  //set_info_label_text (info_label, orig_width, orig_height, 0, 0, state->enl_step / 100);
  gtk_label_set_selectable(GTK_LABEL(info_label), TRUE);
  //gtk_container_add (GTK_CONTAINER (info_frame), info_label);
  gtk_box_pack_start (GTK_BOX (hbox), info_label, TRUE, TRUE, 0);
  gtk_label_set_justify(GTK_LABEL (info_label), GTK_JUSTIFY_LEFT);
  gtk_widget_show (info_label);

  //interface_I_data.info_frame = info_frame;
  interface_I_data.info_label = info_label;

  callback_show_info_button(show_info_button, (gpointer) &interface_I_data);


  /*
  noninter_button = gtk_button_new_with_mnemonic ("_Non-interactive");

  g_signal_connect (GTK_BUTTON (noninter_button), "clicked",
                    G_CALLBACK (callback_noninter_button), (gpointer) dlg);

  gtk_box_pack_start (GTK_BOX (vbox2), noninter_button, FALSE, FALSE, 0);
  gtk_widget_show (noninter_button);
  */

  /* Initialize the carver */

  if ((state->pres_layer_ID == -1) || (ui_state->pres_status == FALSE))
    {
      state->pres_layer_ID = 0;
    }
  if ((state->disc_layer_ID == -1) || (ui_state->disc_status == FALSE))
    {
      state->disc_layer_ID = 0;
    }
  if ((state->rigmask_layer_ID == -1) || (ui_state->rigmask_status == FALSE))
    {
      state->rigmask_layer_ID = 0;
    }
  carver_data = render_init_carver(image_ID, state, drawable_vals, TRUE);
  if (carver_data == NULL)
    {
      return RESPONSE_FATAL;
    }
  interface_I_data.carver_data = carver_data;

  set_info_label_text (&interface_I_data);

  set_alarm (ALARM_DELAY);
  //callback_alarm_triggered (coordinates, (gpointer) & interface_I_data);

  /*  Show the main containers  */

  gtk_widget_show (main_hbox);
  gtk_widget_show (dlg);
  gtk_main ();


  lqr_carver_destroy (carver_data->carver);

  if ((dialog_I_response == GTK_RESPONSE_OK) || (dialog_I_response == RESPONSE_NONINTERACTIVE))
    {
      /*  Save ui values  */
      ui_state->chain_active =
	gimp_chain_button_get_active (GIMP_COORDINATES_CHAINBUTTON
				      (coordinates));
      /*
      state->new_width =
	ROUND (alt_size_entry_get_refval (ALT_SIZE_ENTRY (coordinates), 0));
      state->new_height =
	ROUND (alt_size_entry_get_refval (ALT_SIZE_ENTRY (coordinates), 1));
        */

      /* save all */
      memcpy (vals, state, sizeof (PlugInVals));
      memcpy (ui_vals, ui_state, sizeof (PlugInUIVals));
    }

  gtk_widget_destroy (dlg);
  
  sigaction(SIGALRM, &old_alarm_action, NULL);

  return dialog_I_response;
}


/***  Private functions  ***/

/* Callbacks */

static void
callback_dialog_I_response (GtkWidget * dialog, gint response_id, gpointer data)
{
  //ResponseData * r_data = RESPONSE_DATA (data);
  switch (response_id)
    {
      /*
      case RESPONSE_REFRESH:
        break;
      */
      case RESPONSE_NONINTERACTIVE:
        gtk_window_get_position(GTK_WINDOW(dialog), &(dialog_state->x), &(dialog_state->y));
        dialog_state->has_pos = TRUE;
        //printf("state set, x,y=%i,%i\n", dialog_state->x, dialog_state->y); fflush(stdout);
      default:
        dialog_I_response = response_id;
        gtk_main_quit ();
        break;
    }
}

static void
callback_size_changed (GtkWidget * size_entry, gpointer data)
{
  set_alarm(ALARM_DELAY);
}

static void
set_alarm (glong dseconds)
{
  struct itimerval old, new;
  long int seconds = (dseconds * 100000) / 1000000;
  long int useconds = (dseconds * 100000) % 1000000;
  new.it_interval.tv_usec = 0;
  new.it_interval.tv_sec = 0;
  new.it_value.tv_usec = useconds;
  new.it_value.tv_sec = seconds;
  setitimer (ITIMER_REAL, &new, &old);
}

static void
alarm_handler (int signum)
{
  //printf("ALARM!\n"); fflush(stdout);
  g_signal_emit_by_name (coordinates, "coordinates-alarm");
}

static void
callback_alarm_triggered (GtkWidget * size_entry, gpointer data)
{
  gint new_width, new_height;
  gboolean render_success;
  InterfaceIData *p_data = INTERFACE_I_DATA (data);
  CarverData *c_data = p_data->carver_data;
  //gtk_widget_set_sensitive (p_data->size_frame, FALSE);

  new_width =
    ROUND (alt_size_entry_get_refval (ALT_SIZE_ENTRY (size_entry), 0));
  new_height =
    ROUND (alt_size_entry_get_refval (ALT_SIZE_ENTRY (size_entry), 1));
  state->new_width = new_width;
  state->new_height = new_height;
  //printf("[w,h=%i,%i]\n", new_width, new_height); fflush(stdout);
  render_success = render_interactive (p_data->image_ID, state, p_data->drawable_vals, p_data->carver_data);
  p_data->drawable_vals->layer_ID = c_data->layer_ID;
  if (!render_success)
    {
      dialog_I_response = RESPONSE_FATAL;
      gtk_main_quit();
    }
  gimp_displays_flush();

  set_info_label_text (p_data);
  //set_info_label_text (p_data->info_label, c_data->ref_w, c_data->ref_h, c_data->orientation, c_data->depth, c_data->enl_step);
  //gtk_widget_set_sensitive (p_data->dump_button, (c_data->depth != 0));
  //gtk_widget_set_sensitive (p_data->size_frame, TRUE);

}

static void
//set_info_label_text (GtkWidget * label, gint ref_w, gint ref_h, gint orientation, gint depth, gfloat enl_step)
set_info_label_text (InterfaceIData * p_data)
{
  gchar label_text[MAX_STRING_SIZE];
  gchar text_refsizes[MAX_STRING_SIZE];
  gchar text_w[MAX_STRING_SIZE];
  gchar text_h[MAX_STRING_SIZE];
  gchar text_orientation[MAX_STRING_SIZE];
  gchar text_range[MAX_STRING_SIZE];
  gchar text_enl_step[MAX_STRING_SIZE];
  gint smin, smax;
  gint esmax;
  CarverData * c_data = p_data->carver_data;

  smin = c_data->orientation == 0 ? c_data->ref_w - c_data->depth : c_data->ref_h - c_data->depth;
  smax = c_data->orientation == 0 ? c_data->ref_w + c_data->depth : c_data->ref_h + c_data->depth;
  esmax = (gint) ((c_data->orientation ? c_data->ref_h : c_data->ref_w) * c_data->enl_step) - 1;
  esmax = MAX(1, esmax);

  snprintf(text_orientation, MAX_STRING_SIZE, _("Orientation"));
  snprintf(text_refsizes, MAX_STRING_SIZE, _("Reference size"));
  snprintf(text_w, MAX_STRING_SIZE, _("horizontal"));
  snprintf(text_h, MAX_STRING_SIZE, _("vertical"));
  snprintf(text_range, MAX_STRING_SIZE, _("Range"));
  snprintf(text_enl_step, MAX_STRING_SIZE, _("Next step at"));

  snprintf(label_text, MAX_STRING_SIZE,
      "<small>"
      "<small>"
      "<b>%s</b>\n  %s\n"
      "<b>%s</b>\n  %i\n"
      "<b>%s</b>\n  %i - %i\n"
      "<b>%s</b>\n  %i"
      "</small>"
      "</small>",
          text_orientation, c_data->orientation ? text_h : text_w,
          text_refsizes, c_data->orientation ? c_data->ref_h : c_data->ref_w,
          text_range, smin, smax,
          text_enl_step, esmax);
  label_text[MAX_STRING_SIZE - 1] = '\0';

  gtk_label_set_markup(GTK_LABEL(p_data->info_label), label_text);
  gtk_widget_set_sensitive (p_data->dump_button, (c_data->depth != 0) && (c_data->base_type == GIMP_RGB));

}

static void
callback_resetvalues_button (GtkWidget * button, gpointer data)
{
  InterfaceIData *p_data = INTERFACE_I_DATA (data);

  alt_size_entry_set_refval (ALT_SIZE_ENTRY (p_data->coordinates), 0,
			      p_data->orig_width);
  alt_size_entry_set_refval (ALT_SIZE_ENTRY (p_data->coordinates), 1,
			      p_data->orig_height);
}

static void
callback_show_info_button (GtkWidget * button, gpointer data)
{
  InterfaceIData *p_data = INTERFACE_I_DATA (data);
  if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button)))
    {
      gtk_widget_show (p_data->info_label);
    }
  else
    {
      gtk_widget_hide (p_data->info_label);
    }
}


static void
callback_flatten_button (GtkWidget * button, gpointer data)
{
  gboolean render_success;
  InterfaceIData *p_data = INTERFACE_I_DATA (data);
  //CarverData * c_data = p_data->carver_data;

  render_success = render_flatten (p_data->image_ID, state, p_data->drawable_vals, p_data->carver_data);
  if (!render_success)
    {
      dialog_I_response = RESPONSE_FATAL;
      gtk_main_quit();
    }
  gimp_displays_flush();

  set_info_label_text (p_data);
  //set_info_label_text (p_data->info_label, c_data->ref_w, c_data->ref_h, c_data->orientation, c_data->depth, c_data->enl_step);
}


static void
callback_dump_button (GtkWidget * button, gpointer data)
{
  gboolean render_success;
  InterfaceIData *p_data = INTERFACE_I_DATA (data);
  //CarverData * c_data = p_data->carver_data;

  render_success = render_dump_vmap (p_data->image_ID, state, p_data->drawable_vals, p_data->col_vals, p_data->carver_data);
  if (!render_success)
    {
      dialog_I_response = RESPONSE_FATAL;
      gtk_main_quit();
    }
  gimp_displays_flush();

  //set_info_label_text (p_data->info_label, c_data->ref_w, c_data->ref_h, c_data->orientation, c_data->depth, c_data->enl_step);

}

#if 0
static void
callback_res_order_changed (GtkWidget * res_order, gpointer data)
{
  gint order;
  PreviewData *p_data = PREVIEW_DATA (data);
  gimp_int_combo_box_get_active (GIMP_INT_COMBO_BOX (res_order), &order);
  p_data->vals->res_order = order;
  callback_set_disc_warning (NULL, data);
}

static void
callback_noninter_button (GtkWidget * button, gpointer data)
{
  gtk_dialog_response (GTK_DIALOG (data), RESPONSE_NONINTERACTIVE);
}
#endif

