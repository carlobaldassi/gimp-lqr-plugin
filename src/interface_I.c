/* GIMP LiquidRescaling Plug-in
 * Copyright (C) 2007 Carlo Baldassi (the "Author") <carlobaldassi@yahoo.it>.
 * All Rights Reserved.
 *
 * This plugin implements the algorithm described in the paper
 * "Seam Carving for Content-Aware Image Resizing"
 * by Shai Avidan and Ariel Shamir
 * which can be found at http://www.faculty.idc.ac.il/arik/imret.pdf
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
#include <unistd.h>

#include <lqr.h>

#include "plugin-intl.h"
#include "main.h"
#include "render.h"
#include "interface_I.h"
#include "preview.h"
#include "layers_combo.h"


/***  Constants  ***/

#define SCALE_WIDTH         (80)
#define SPIN_BUTTON_WIDTH   (75)
#define MAX_COEFF	  (3000)
#define MAX_RIGIDITY      (1000)
#define MAX_DELTA_X         (10)
#define MAX_STRING_SIZE   (2048)
#define ALARM_DELAY          (1)


/***  Local functions declariations  ***/

/* Callbacks */
static void callback_dialog_I_response (GtkWidget * dialog, gint response_id,
				      gpointer data);
//static void callback_noninter_button (GtkWidget * button, gpointer data);

//static void callback_resetvalues_button (GtkWidget * button, gpointer data);

//static void callback_set_disc_warning (GtkWidget * dummy, gpointer data);
static void callback_size_changed (GtkWidget * size_entry, gpointer data);
static void alarm_handler (int signum);
static void callback_alarm_triggered (GtkWidget * size_entry, gpointer data);
//static void callback_res_order_changed (GtkWidget * res_order, gpointer data);

/***  Local variables  ***/

gint dialog_I_response = GTK_RESPONSE_CLOSE;

PlugInUIVals *ui_state;
PlugInVals *state;
PlugInDialogVals *dialog_state;
gboolean features_are_sensitive;
InterfaceIData interface_I_data;
//PreviewData preview_data;
//PresDiscStatus presdisc_status;
//ToggleData pres_toggle_data;
//ToggleData disc_toggle_data;
//ToggleData rigmask_toggle_data;
//GtkWidget *grad_func_combo_box;
//GtkWidget *res_order_combo_box;
//gboolean pres_info_show = FALSE;
//gboolean disc_info_show = FALSE;
//gboolean rigmask_info_show = FALSE;

GtkWidget *dlg;
GtkTooltips *dlg_tips;
GtkWidget *coordinates;


/***  Public functions  ***/

gint
dialog_I (gint32 image_ID, gint32 layer_ID,
	GimpDrawable * drawable,
	PlugInVals * vals,
	PlugInImageVals * image_vals,
	PlugInDrawableVals * drawable_vals, PlugInUIVals * ui_vals,
        PlugInDialogVals * dialog_vals)
{
  //GimpRGB saved_colour;
  //gint num_extra_layers;
  struct sigaction alarm_action, old_alarm_action;
  gint orig_width, orig_height;
  GtkWidget *main_hbox;
  GtkWidget *vbox;
  GtkWidget *vbox2;
  //GtkWidget *hbox;
  GtkWidget *frame;
  //GtkWidget *notebook;
  //gfloat wfactor, hfactor;
  //GtkWidget *preview_area;
  //GtkWidget *noninter_button;
  //GtkWidget *resetvalues_event_box;
  //GtkWidget *resetvalues_button;
  //GtkWidget *resetvalues_icon;
  //GtkWidget *lastvalues_event_box;
  //GtkWidget *lastvalues_button;
  //GtkWidget *lastvalues_icon;
  //GtkWidget *mode_event_box;
  //GtkWidget *oper_mode_combo_box;
  //GtkWidget *features_page;
  //GtkWidget *advanced_page;
  //GtkWidget *thispage;
  //GtkWidget *label;
  //GtkWidget *new_layer_button;
  //GtkWidget *resize_canvas_button;
  //GtkWidget *resize_aux_layers_button;
  gboolean has_mask = FALSE;
  GimpUnit unit;
  gdouble xres, yres;

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
  //preview_data.orig_layer_ID = layer_ID;
  interface_I_data.image_ID = image_ID;
  interface_I_data.drawable = drawable;
  interface_I_data.drawable_vals = drawable_vals;

  if (gimp_layer_get_mask (layer_ID) != -1)
    {
      has_mask = TRUE;
    }

  //num_extra_layers = count_extra_layers (image_ID, drawable);
  //features_are_sensitive = (num_extra_layers > 0 ? TRUE : FALSE);
  /*
  if (!features_are_sensitive)
    {
      ui_state->pres_status = FALSE;
      ui_state->disc_status = FALSE;
      preview_data.pres_combo_awaked = FALSE;
      preview_data.disc_combo_awaked = FALSE;
    }
    */

  /*
  dlg = gimp_dialog_new (_("GIMP LiquidRescale Plug-In"), PLUGIN_NAME "-I",
			 NULL, 0,
			 gimp_standard_help_func, "plug-in-lqr",
			 //GIMP_STOCK_RESET, RESPONSE_RESET,
			 //GTK_STOCK_REFRESH, RESPONSE_REFRESH,
			 GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
			 GTK_STOCK_OK, GTK_RESPONSE_OK, NULL);
			 */

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
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  vbox2 = gtk_vbox_new (FALSE, 4);
  gtk_container_set_border_width (GTK_CONTAINER (vbox2), 4);
  gtk_container_add (GTK_CONTAINER (frame), vbox2);
  gtk_widget_show (vbox2);

  /*
  hbox = gtk_hbox_new (FALSE, 4);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 4);
  gtk_box_pack_start (GTK_BOX (vbox2), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);
  */

  unit = gimp_image_get_unit (image_ID);
  gimp_image_get_resolution (image_ID, &xres, &yres);

  coordinates =
    gimp_coordinates_new (unit, "%p", TRUE, TRUE, SPIN_BUTTON_WIDTH,
			  GIMP_SIZE_ENTRY_UPDATE_SIZE, ui_state->chain_active,
			  TRUE, _("Width:"), state->new_width, xres, 2,
			  GIMP_MAX_IMAGE_SIZE, 0, orig_width,
			  _("Height:"), state->new_height, yres, 2,
			  GIMP_MAX_IMAGE_SIZE, 0, orig_height);

  interface_I_data.coordinates = (gpointer) coordinates; // USELESS

  g_signal_connect (GIMP_SIZE_ENTRY (coordinates), "value-changed",
		    G_CALLBACK (callback_size_changed),
		    (gpointer) & interface_I_data);

  g_signal_connect (GIMP_SIZE_ENTRY (coordinates), "refval-changed",
		    G_CALLBACK (callback_size_changed),
		    (gpointer) & interface_I_data);

  sigaction(SIGALRM, NULL, &old_alarm_action);
  alarm_action = old_alarm_action;
  alarm_action.sa_handler = alarm_handler;
  sigaction(SIGALRM, &alarm_action, NULL);

  g_signal_connect (GIMP_SIZE_ENTRY (coordinates), "coordinates-alarm",
                    G_CALLBACK (callback_alarm_triggered),
                    (gpointer) & interface_I_data);

  //gtk_box_pack_start (GTK_BOX (hbox), coordinates, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox2), coordinates, FALSE, FALSE, 0);
  gtk_widget_show (coordinates);

  /* Other buttons */

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
  carver_data = render_init_carver(image_ID, drawable, state, drawable_vals, TRUE);
  if (carver_data == NULL)
    {
      return RESPONSE_FATAL;
    }
  interface_I_data.carver_data = carver_data;

  /*  Show the main containers  */

  gtk_widget_show (main_hbox);
  gtk_widget_show (dlg);
  gtk_main ();


  lqr_carver_destroy (carver_data->carver);

  if (dialog_I_response == GTK_RESPONSE_OK)
    {
      /*  Save ui values  */
      ui_state->chain_active =
	gimp_chain_button_get_active (GIMP_COORDINATES_CHAINBUTTON
				      (coordinates));
      /*
      state->new_width =
	ROUND (gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (coordinates), 0));
      state->new_height =
	ROUND (gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (coordinates), 1));

      gimp_int_combo_box_get_active (GIMP_INT_COMBO_BOX (grad_func_combo_box),
				     &(state->grad_func));
      gimp_int_combo_box_get_active (GIMP_INT_COMBO_BOX (res_order_combo_box),
				     &(state->res_order));
      state->new_layer =
	gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (new_layer_button));
      state->resize_canvas =
	gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON
				      (resize_canvas_button));
      state->resize_aux_layers =
	gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON
				      (resize_aux_layers_button));
      state->output_seams =
	gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (out_seams_button));
	*/

      /* save vsmap colours */
      /*
      if (state->output_seams)
	{
	  gimp_color_button_get_color (GIMP_COLOR_BUTTON
				       (out_seams_col_button1), colour_start);
	  gimp_color_button_get_color (GIMP_COLOR_BUTTON
				       (out_seams_col_button2), colour_end);

	  col_vals->r1 = colour_start->r;
	  col_vals->g1 = colour_start->g;
	  col_vals->b1 = colour_start->b;

	  col_vals->r2 = colour_end->r;
	  col_vals->g2 = colour_end->g;
	  col_vals->b2 = colour_end->b;
	}
	*/

      /* save mask behaviour */
      /*
      if (has_mask == TRUE)
	{
	  gimp_int_combo_box_get_active (GIMP_INT_COMBO_BOX
					 (mask_behavior_combo_box),
					 &(state->mask_behavior));
	}
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

#if 0
static void
callback_set_disc_warning (GtkWidget * dummy, gpointer data)
{
  PreviewData *p_data = PREVIEW_DATA (data);
  gboolean issue_warn;
  gint old_w, old_h;
  gint new_w, new_h;

  if ((p_data->vals->no_disc_on_enlarge == FALSE) ||
      (p_data->ui_vals->disc_status == FALSE) ||
      (p_data->vals->disc_coeff == 0))
    {
      gtk_widget_hide (GTK_WIDGET (p_data->disc_warning_image));
    }
  else
    {
      old_w = p_data->old_width;
      old_h = p_data->old_height;
      new_w = p_data->vals->new_width;
      new_h = p_data->vals->new_height;
      issue_warn = FALSE;
      switch (p_data->vals->res_order)
	{
	case LQR_RES_ORDER_HOR:
	  if ((new_w > old_w) || ((new_w == old_w) && (new_h > old_h)))
	    {
	      issue_warn = TRUE;
	    }
	  break;
	case LQR_RES_ORDER_VERT:
	  if ((new_h > old_h) || ((new_h == old_h) && (new_w > old_w)))
	    {
	      issue_warn = TRUE;
	    }
	  break;
	}
      if (issue_warn == TRUE)
	{
	  gtk_widget_show (GTK_WIDGET (p_data->disc_warning_image));
	}
      else
	{
	  gtk_widget_hide (GTK_WIDGET (p_data->disc_warning_image));
	}
    }
}
#endif

static void
callback_size_changed (GtkWidget * size_entry, gpointer data)
{
  alarm(ALARM_DELAY);
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
  new_width =
    ROUND (gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (size_entry), 0));
  new_height =
    ROUND (gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (size_entry), 1));
  state->new_width = new_width;
  state->new_height = new_height;
  //printf("[w,h=%i,%i]\n", new_width, new_height); fflush(stdout);
  p_data->drawable = gimp_drawable_get(p_data->drawable_vals->layer_ID);
  render_success = render_interactive (p_data->image_ID, p_data->drawable, state, p_data->drawable_vals, p_data->carver_data);
  if (!render_success)
    {
      dialog_I_response = RESPONSE_FATAL;
      gtk_main_quit();
    }
  gimp_displays_flush();
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
#endif


/*
static void
callback_noninter_button (GtkWidget * button, gpointer data)
{
  gtk_dialog_response (GTK_DIALOG (data), RESPONSE_NONINTERACTIVE);
}
*/

