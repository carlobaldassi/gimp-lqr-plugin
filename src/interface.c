/* GIMP LiquidRescale Plug-in
 * Copyright (C) 2007-2010 Carlo Baldassi (the "Author") <carlobaldassi@gmail.com>.
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

#include <gtk/gtk.h>
#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>
#include "altsizeentry.h"
#include "altcoordinates.h"


#include <lqr.h>

#include <math.h>
#include <string.h>

#include "plugin-intl.h"
#include "main.h"
#include "interface.h"
#include "preview.h"
#include "layers_combo.h"


/***  Constants  ***/

#define SCALE_WIDTH         (80)
#define SPIN_BUTTON_WIDTH   (75)
#define BOX_INDENT          (22)
#define MAX_COEFF	  (3000)
#define MAX_RIGIDITY      (1000)
#define MAX_DELTA_X         (10)
#define MAX_STRING_SIZE   (2048)


/***  Local functions declariations  ***/

/* Callbacks */
static void callback_dialog_response (GtkWidget * dialog, gint response_id,
				      gpointer data);

static void callback_lastvalues_button (GtkWidget * button, gpointer data);
static void callback_resetvalues_button (GtkWidget * button, gpointer data);
static void callback_interactive_button (GtkWidget * button, gpointer data);

static void callback_set_disc_warning (GtkWidget * dummy, gpointer data);
static void callback_size_changed (GtkWidget * size_entry, gpointer data);
static void callback_res_order_changed (GtkWidget * res_order, gpointer data);
static void callback_output_target_changed (GtkWidget * res_order, gpointer data);
static void callback_scaleback_mode_changed (GtkWidget * res_order, gpointer data);
static void callback_expander_changed (GtkWidget * expander, gpointer data);

static void callback_out_seams_button (GtkWidget * button, gpointer data);
static void callback_out_seams_col_button1 (GtkWidget * button, gpointer data);
static void callback_out_seams_col_button2 (GtkWidget * button, gpointer data);
static void callback_scaleback_button (GtkWidget * button, gpointer data);
static void callback_resize_aux_layers_button_set_sensitive (GtkWidget *
							     button,
							     gpointer data);

/* Feature and advanced pages */
static GtkWidget *features_page_new (gint32 image_ID, gint32 layer_ID);
static GtkWidget *advanced_page_new (gint32 image_ID, gint32 layer_ID);
static void refresh_features_page (NotebookData * data);
static void refresh_advanced_page (NotebookData * data);


/***  Local variables  ***/

gint dialog_response = GTK_RESPONSE_CANCEL;

PlugInUIVals *ui_state;
PlugInVals *state;
PlugInDialogVals *dialog_state;
NotebookData *notebook_data;
gboolean features_are_sensitive;
PreviewData preview_data;
PresDiscStatus presdisc_status;
ToggleData pres_toggle_data;
ToggleData disc_toggle_data;
ToggleData rigmask_toggle_data;
GtkWidget *nrg_func_combo_box;
GtkWidget *res_order_combo_box;

GtkWidget *dlg;


/***  Public functions  ***/

gint
dialog (PlugInImageVals * image_vals,
	PlugInDrawableVals * drawable_vals,
	PlugInVals * vals,
        PlugInUIVals * ui_vals,
	PlugInColVals * col_vals,
        PlugInDialogVals * dialog_vals)
{
  gint32 image_ID;
  gint32 layer_ID;
  gint num_extra_layers;
  gint orig_width, orig_height;
  GtkWidget *main_hbox;
  GtkWidget *vbox;
  GtkWidget *vbox2;
  GtkWidget *vbox3;
  GtkWidget *hbox;
  GtkWidget *frame;
  GtkWidget *notebook;
  gfloat wfactor, hfactor;
  GtkWidget *preview_area;
  GtkWidget *filler;
  GtkWidget *pres_use_image;
  GtkWidget *disc_use_image;
  GtkWidget *rigmask_use_image;
  GtkWidget *coordinates;
  GtkWidget *resetvalues_event_box;
  GtkWidget *resetvalues_button;
  GtkWidget *resetvalues_icon;
  GtkWidget *lastvalues_event_box;
  GtkWidget *lastvalues_button;
  GtkWidget *lastvalues_icon;
  GtkWidget *interactive_event_box;
  GtkWidget *interactive_button;
  GtkWidget *interactive_hbox;
  GtkWidget *interactive_icon;
  GtkWidget *interactive_label;
  GtkWidget *scaleback_mode_alignment;
  GtkWidget *scaleback_mode_event_box;
  GtkWidget *scaleback_mode_label;
  GtkWidget *scaleback_mode_hbox;
  GtkWidget *scaleback_mode_combo_box;
  GtkWidget *features_page;
  GtkWidget *advanced_page;
  GtkWidget *thispage;
  GtkWidget *label;
  GtkWidget *output_target_event_box;
  GtkWidget *output_target_label;
  GtkWidget *output_target_hbox;
  GtkWidget *output_target_combo_box;
  GtkWidget *resize_canvas_button;
  GtkWidget *resize_aux_layers_button;
  GtkWidget *out_seams_hbox;
  GtkWidget *out_seams_button;
  GimpRGB *colour;
  GtkWidget *out_seams_col_button1;
  GtkWidget *out_seams_col_button2;
  GtkWidget *scaleback_button;
  GtkWidget *mask_behavior_combo_box = NULL;
  gboolean has_mask = FALSE;
  GimpUnit unit;
  gdouble xres, yres;

  image_ID = image_vals->image_ID;
  layer_ID = drawable_vals->layer_ID;

  IMAGE_CHECK (image_ID, FALSE);

  gimp_ui_init (PLUGIN_NAME, TRUE);

  dialog_state = dialog_vals;

  state = g_new (PlugInVals, 1);
  memcpy (state, vals, sizeof (PlugInVals));

  ui_state = g_new (PlugInUIVals, 1);
  memcpy (ui_state, ui_vals, sizeof (PlugInUIVals));

  notebook_data = g_new (NotebookData, 1);

  if (!gimp_drawable_is_valid(layer_ID))
    {
      layer_ID = gimp_image_get_active_layer (image_ID);
    }

  pres_toggle_data.ui_toggled = &(ui_state->pres_status);
  disc_toggle_data.ui_toggled = &(ui_state->disc_status);
  preview_data.pres_combo_awaked = FALSE;
  preview_data.disc_combo_awaked = FALSE;
  if (ui_state->pres_status == TRUE)
    {
      if (gimp_drawable_is_valid(state->pres_layer_ID) &&
          gimp_drawable_is_layer(state->pres_layer_ID) &&
          (state->pres_layer_ID != layer_ID))
        {
          preview_data.pres_combo_awaked = TRUE;
        }
      else
        {
          state->pres_layer_ID = 0;
          ui_state->pres_status = FALSE;
        }
    }
  if (ui_state->disc_status == TRUE)
    {
      if (gimp_drawable_is_valid(state->disc_layer_ID) &&
          gimp_drawable_is_layer(state->disc_layer_ID) &&
          (state->disc_layer_ID != layer_ID))
        {
          preview_data.disc_combo_awaked = TRUE;
        }
      else
        {
          state->disc_layer_ID = 0;
          ui_state->disc_status = FALSE;
        }
      preview_data.disc_combo_awaked = TRUE;
    }

  orig_width = gimp_drawable_width (layer_ID);
  orig_height = gimp_drawable_height (layer_ID);

  if (layer_ID != ui_state->last_layer_ID)
    {
      state->new_width = orig_width;
      state->new_height = orig_height;
    }

  g_assert (gimp_drawable_is_layer (layer_ID) == TRUE);

  drawable_vals->layer_ID = layer_ID;
  preview_data.orig_layer_ID = layer_ID;

  if (gimp_layer_get_mask (layer_ID) != -1)
    {
      has_mask = TRUE;
    }

  num_extra_layers = count_extra_layers (image_ID);
  features_are_sensitive = (num_extra_layers > 0 ? TRUE : FALSE);
  if (!features_are_sensitive)
    {
      ui_state->pres_status = FALSE;
      ui_state->disc_status = FALSE;
      preview_data.pres_combo_awaked = FALSE;
      preview_data.disc_combo_awaked = FALSE;
    }

  dlg = gimp_dialog_new (_("GIMP LiquidRescale Plug-In"), PLUGIN_NAME,
			 NULL, 0,
			 gimp_standard_help_func, "plug-in-lqr",
			 //"_Interactive", RESPONSE_INTERACTIVE,
			 GIMP_STOCK_RESET, RESPONSE_RESET,
			 GTK_STOCK_REFRESH, RESPONSE_REFRESH,
			 GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
			 GTK_STOCK_OK, GTK_RESPONSE_OK, NULL);

  gtk_window_set_resizable (GTK_WINDOW (dlg), FALSE);

  if (dialog_state->has_pos)
    {
      gtk_window_move (GTK_WINDOW(dlg), dialog_state->x, dialog_state->y);
      dialog_state->has_pos = FALSE;
    }

  g_signal_connect (dlg, "response", G_CALLBACK (callback_dialog_response),
		    (gpointer) (notebook_data));

  preview_data.dlg = dlg;

  main_hbox = gtk_hbox_new (FALSE, 12);
  gtk_container_set_border_width (GTK_CONTAINER (main_hbox), 12);
  gtk_container_add (GTK_CONTAINER (gtk_dialog_get_content_area(GTK_DIALOG (dlg))), main_hbox);

  vbox = gtk_vbox_new (FALSE, 12);
  gtk_box_pack_start (GTK_BOX (main_hbox), vbox, TRUE, TRUE, 0);
  gtk_widget_show (vbox);

  frame = gimp_frame_new (_("Selected layer"));
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  vbox2 = gtk_vbox_new (FALSE, 4);
  gtk_container_set_border_width (GTK_CONTAINER (vbox2), 0);
  gtk_container_add (GTK_CONTAINER (frame), vbox2);
  gtk_widget_show (vbox2);

  /* Preview */

  preview_data.image_ID = image_ID;
  preview_data.vals = state;
  preview_data.ui_vals = ui_state;
  wfactor = (gfloat) gimp_drawable_width (layer_ID) / PREVIEW_MAX_WIDTH;
  hfactor = (gfloat) gimp_drawable_height (layer_ID) / PREVIEW_MAX_HEIGHT;
  preview_data.factor = MAX (wfactor, hfactor);
  preview_data.factor = MAX (preview_data.factor, 1);


  preview_data.old_width = orig_width;
  preview_data.old_height = orig_height;
  gimp_drawable_offsets (layer_ID, &(preview_data.x_off),
			 &(preview_data.y_off));
  preview_data.width =
    gimp_drawable_width (preview_data.orig_layer_ID) / preview_data.factor;
  preview_data.height =
    gimp_drawable_height (preview_data.orig_layer_ID) / preview_data.factor;


  preview_data_create(image_ID, layer_ID, &preview_data);

  preview_build_pixbuf (&preview_data);

  preview_area = preview_area_create(&preview_data);

  gtk_box_pack_start (GTK_BOX (vbox2), preview_area, FALSE, FALSE, 0);

  gtk_widget_show (preview_area);

  hbox = gtk_hbox_new (FALSE, 10);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 4);
  gtk_box_pack_start (GTK_BOX (vbox2), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  filler = gtk_image_new ();
  gtk_box_pack_start (GTK_BOX (hbox), filler, TRUE, TRUE, 0);
  gtk_widget_show (filler);
  filler = gtk_image_new ();
  gtk_box_pack_end (GTK_BOX (hbox), filler, TRUE, TRUE, 0);
  gtk_widget_show (filler);


  pres_use_image = gtk_image_new_from_stock (GIMP_STOCK_CHANNEL_GREEN,
						  GTK_ICON_SIZE_MENU);

  gtk_box_pack_start (GTK_BOX (hbox), pres_use_image, FALSE, FALSE, 0);

  gtk_widget_show (pres_use_image);

  disc_use_image = gtk_image_new_from_stock (GIMP_STOCK_CHANNEL_RED,
						  GTK_ICON_SIZE_MENU);

  gtk_box_pack_start (GTK_BOX (hbox), disc_use_image, FALSE, FALSE, 0);

  gtk_widget_show (disc_use_image);

  rigmask_use_image = gtk_image_new_from_stock (GIMP_STOCK_CHANNEL_BLUE,
						  GTK_ICON_SIZE_MENU);

  gtk_widget_show (rigmask_use_image);

  gtk_box_pack_start (GTK_BOX (hbox), rigmask_use_image, FALSE, FALSE, 0);

  preview_data.pres_use_image = pres_use_image;
  preview_data.disc_use_image = disc_use_image;
  preview_data.rigmask_use_image = rigmask_use_image;

  g_signal_connect (G_OBJECT (preview_area), "expose_event",
		    G_CALLBACK (callback_preview_expose_event),
		    (gpointer) (&preview_data));


  /*  New size  */

  frame = gimp_frame_new (_("Select new width and height"));
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  vbox2 = gtk_vbox_new (FALSE, 4);
  gtk_container_set_border_width (GTK_CONTAINER (vbox2), 4);
  gtk_container_add (GTK_CONTAINER (frame), vbox2);
  gtk_widget_show (vbox2);

  hbox = gtk_hbox_new (FALSE, 4);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 4);
  gtk_box_pack_start (GTK_BOX (vbox2), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  unit = gimp_image_get_unit (image_ID);
  gimp_image_get_resolution (image_ID, &xres, &yres);

  coordinates =
    alt_coordinates_new (unit, "%p", TRUE, TRUE, SPIN_BUTTON_WIDTH,
			  ALT_SIZE_ENTRY_UPDATE_SIZE, ui_state->chain_active,
			  TRUE, _("Width:"), state->new_width, xres, 2,
			  GIMP_MAX_IMAGE_SIZE, 0, orig_width,
			  _("Height:"), state->new_height, yres, 2,
			  GIMP_MAX_IMAGE_SIZE, 0, orig_height);

  if (layer_ID != ui_state->last_layer_ID)
    {
      alt_size_entry_set_refval (ALT_SIZE_ENTRY (coordinates), 0,
				  state->new_width);
      alt_size_entry_set_refval (ALT_SIZE_ENTRY (coordinates), 1,
				  state->new_height);
    }

  g_signal_connect (ALT_SIZE_ENTRY (coordinates), "value-changed",
		    G_CALLBACK (callback_size_changed),
		    (gpointer) & preview_data);

  g_signal_connect (ALT_SIZE_ENTRY (coordinates), "refval-changed",
		    G_CALLBACK (callback_size_changed),
		    (gpointer) & preview_data);

  gtk_box_pack_start (GTK_BOX (hbox), coordinates, FALSE, FALSE, 0);
  gtk_widget_show (coordinates);

  preview_data.coordinates = (gpointer) coordinates;

  /* Aux buttons */

  vbox3 = gtk_vbox_new (FALSE, 1);
  gtk_box_pack_start (GTK_BOX (hbox), vbox3, FALSE, FALSE, 0);
  gtk_widget_show (vbox3);

  resetvalues_event_box = gtk_event_box_new ();
  gtk_box_pack_start (GTK_BOX (vbox3), resetvalues_event_box, FALSE, FALSE,
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
		    (gpointer) & preview_data);

  lastvalues_event_box = gtk_event_box_new ();
  gtk_box_pack_start (GTK_BOX (vbox3), lastvalues_event_box, FALSE, FALSE, 0);
  gtk_widget_show (lastvalues_event_box);

  gimp_help_set_help_data (lastvalues_event_box,
			   _("Set width and height to the last used values"),
			   NULL);

  lastvalues_button = gtk_button_new ();
  lastvalues_icon =
    gtk_image_new_from_stock (GTK_STOCK_REVERT_TO_SAVED, GTK_ICON_SIZE_MENU);
  gtk_container_add (GTK_CONTAINER (lastvalues_button), lastvalues_icon);
  gtk_widget_show (lastvalues_icon);
  gtk_container_add (GTK_CONTAINER (lastvalues_event_box), lastvalues_button);
  gtk_widget_show (lastvalues_button);

  g_signal_connect (lastvalues_button, "clicked",
		    G_CALLBACK (callback_lastvalues_button),
		    (gpointer) & preview_data);
  gtk_widget_set_sensitive (lastvalues_button,
			    ((ui_state->last_used_width != -1)
			     && (ui_state->last_used_height !=
				 -1)) ? TRUE : FALSE);

  vbox3 = gtk_vbox_new (FALSE, 4);
  gtk_box_pack_start (GTK_BOX (hbox), vbox3, FALSE, FALSE, 0);
  gtk_widget_show (vbox3);

  interactive_event_box = gtk_event_box_new ();
  gtk_box_pack_start (GTK_BOX (vbox3), interactive_event_box, FALSE, FALSE,
		      0);
  gtk_widget_show (interactive_event_box);

  gimp_help_set_help_data (interactive_event_box,
			   _("Switch to interactive mode. "
                           "Note that the current settings will be applied."),
			   NULL);

  interactive_button = gtk_button_new ();
  interactive_hbox = gtk_hbox_new (FALSE, 4);
  gtk_container_add (GTK_CONTAINER (interactive_button), interactive_hbox);
  gtk_widget_show (interactive_hbox);
  interactive_icon =
    gtk_image_new_from_stock (GTK_STOCK_EXECUTE, GTK_ICON_SIZE_LARGE_TOOLBAR);
  gtk_box_pack_start (GTK_BOX(interactive_hbox), interactive_icon, TRUE, FALSE, 0);
  gtk_widget_show (interactive_icon);
  interactive_label = gtk_label_new_with_mnemonic (_("_Interactive"));
  gtk_box_pack_start (GTK_BOX (interactive_hbox), interactive_label, TRUE, FALSE, 0);
  gtk_widget_show (interactive_label);
  gtk_container_add (GTK_CONTAINER (interactive_event_box),
		     interactive_button);
  gtk_widget_show (interactive_button);

  g_signal_connect (interactive_button, "clicked",
		    G_CALLBACK (callback_interactive_button),
		    (gpointer) dlg);

  /* Notebook */

  notebook = gtk_notebook_new ();
  gtk_box_pack_start (GTK_BOX (main_hbox), notebook, TRUE, TRUE, 5);
  gtk_widget_show (notebook);
  notebook_data->notebook = notebook;
  notebook_data->image_ID = image_ID;
  notebook_data->layer_ID = layer_ID;

  /* Fature masks page */

  features_page = features_page_new (image_ID, layer_ID);
  gtk_widget_show (features_page);
  notebook_data->features_page_ID =
    gtk_notebook_prepend_page_menu (GTK_NOTEBOOK (notebook), features_page,
				    notebook_data->label, NULL);

  /* Output settings page */

  label = gtk_label_new (_("Output"));

  thispage = gtk_vbox_new (FALSE, 12);
  gtk_container_set_border_width (GTK_CONTAINER (thispage), 12);
  gtk_notebook_append_page_menu (GTK_NOTEBOOK (notebook), thispage, label,
				 NULL);
  gtk_widget_show (thispage);

  /* Output checkboxes */

  vbox = gtk_vbox_new (FALSE, 4);
  gtk_box_pack_start (GTK_BOX (thispage), vbox, FALSE, FALSE, 0);
  gtk_widget_show (vbox);

  output_target_event_box = gtk_event_box_new ();
  gtk_box_pack_start (GTK_BOX (vbox), output_target_event_box, FALSE, FALSE, 0);
  gtk_widget_show (output_target_event_box);

  gimp_help_set_help_data (output_target_event_box,
			   _
			   ("The result of rescaling can be put in the current layer, in a new one "
                            "or in a new image"),
			   NULL);

  output_target_hbox = gtk_hbox_new (FALSE, 4);
  gtk_container_add (GTK_CONTAINER (output_target_event_box), output_target_hbox);
  gtk_widget_show (output_target_hbox);

  output_target_label = gtk_label_new(_("Output target:"));
  gtk_box_pack_start (GTK_BOX(output_target_hbox), output_target_label, FALSE, FALSE, 0);
  gtk_widget_show (output_target_label);

  output_target_combo_box =
    gimp_int_combo_box_new (_("selected layer"), OUTPUT_TARGET_SAME_LAYER,
			    _("new layer"), OUTPUT_TARGET_NEW_LAYER,
			    _("new image"), OUTPUT_TARGET_NEW_IMAGE,
			    NULL);
  gimp_int_combo_box_set_active (GIMP_INT_COMBO_BOX (output_target_combo_box),
				 state->output_target);

  gimp_int_combo_box_connect (GIMP_INT_COMBO_BOX (output_target_combo_box),
			      state->output_target,
			      G_CALLBACK (callback_output_target_changed),
			      (gpointer) & preview_data);

  gtk_box_pack_start (GTK_BOX (output_target_hbox), output_target_combo_box, FALSE, FALSE, 0);
  gtk_widget_show (output_target_combo_box);
  /**/

  resize_canvas_button =
    gtk_check_button_new_with_label (_("Resize image canvas"));

  gtk_box_pack_start (GTK_BOX (vbox), resize_canvas_button, FALSE, FALSE, 0);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (resize_canvas_button),
				state->resize_canvas);
  gtk_widget_show (resize_canvas_button);

  gimp_help_set_help_data (resize_canvas_button,
			   _("Resize and translate the image "
			     "canvas to fit the resized layer"), NULL);

  resize_aux_layers_button =
    gtk_check_button_new_with_label (_("Resize auxiliary layers"));

  gtk_box_pack_start (GTK_BOX (vbox), resize_aux_layers_button, FALSE, FALSE,
		      0);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (resize_aux_layers_button),
				state->resize_aux_layers);

  presdisc_status.ui_vals = (gpointer) ui_state;
  presdisc_status.button = (gpointer) resize_aux_layers_button;

  callback_resize_aux_layers_button_set_sensitive (NULL,
						   (gpointer)
						   (&presdisc_status));

  gtk_widget_show (resize_aux_layers_button);

  gimp_help_set_help_data (resize_aux_layers_button,
			   _
			   ("Resize the layers used as features or rigidity masks "
			    "along with the active layer"), NULL);

  out_seams_hbox = gtk_hbox_new (FALSE, 4);
  gtk_box_pack_start (GTK_BOX (vbox), out_seams_hbox, FALSE, FALSE, 0);
  gtk_widget_show (out_seams_hbox);

  out_seams_button = gtk_check_button_new_with_label (_("Output the seams"));

  gtk_box_pack_start (GTK_BOX (out_seams_hbox), out_seams_button, FALSE,
		      FALSE, 0);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (out_seams_button),
				state->output_seams);
  gtk_widget_show (out_seams_button);

  gimp_help_set_help_data (out_seams_button,
			   _("Creates an extra output layer with the seams, "
			     "for visual inspection of what the plugin did. "
			     "Use it together with \"Output on a new layer\", "
			     "and resize in one direction at a time.\n"
			     "Note that this option is ignored in interactive mode"), NULL);

  g_signal_connect (out_seams_button, "toggled",
		    G_CALLBACK (callback_out_seams_button),
		    (gpointer) &(state->output_seams));


  colour = g_new (GimpRGB, 1);

  gimp_rgba_set (colour, col_vals->r2, col_vals->g2, col_vals->b2, 1);

  out_seams_col_button2 =
    gimp_color_button_new (_("Last seams colour"), 14, 14, colour,
			   GIMP_COLOR_AREA_FLAT);
  gtk_box_pack_end (GTK_BOX (out_seams_hbox), out_seams_col_button2, FALSE,
		    FALSE, 0);
  gtk_widget_show (out_seams_col_button2);

  g_signal_connect (out_seams_col_button2, "color-changed",
		    G_CALLBACK (callback_out_seams_col_button2),
		    (gpointer) (col_vals));

  gimp_help_set_help_data (out_seams_col_button2,
			   _("Colour to use for the last seams"), NULL);

  gimp_rgba_set (colour, col_vals->r1, col_vals->g1, col_vals->b1, 1);

  out_seams_col_button1 =
    gimp_color_button_new (_("First seams colour"), 14, 14, colour,
			   GIMP_COLOR_AREA_FLAT);
  gtk_box_pack_end (GTK_BOX (out_seams_hbox), out_seams_col_button1, FALSE,
		    FALSE, 0);
  gtk_widget_show (out_seams_col_button1);

  g_signal_connect (out_seams_col_button1, "color-changed",
		    G_CALLBACK (callback_out_seams_col_button1),
		    (gpointer) (col_vals));

  gimp_help_set_help_data (out_seams_col_button1,
			   _("Colour to use for the first seams"), NULL);

  g_free(colour);

  scaleback_button =
    gtk_check_button_new_with_label (_("Scale back to the original size"));
  gtk_box_pack_start (GTK_BOX (vbox), scaleback_button, FALSE, FALSE, 0);

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (scaleback_button),
				state->scaleback);

  gtk_widget_show (scaleback_button);

  gimp_help_set_help_data (scaleback_button,
			   _
			   ("Select this if you want to transform back the "
			    "layer after LqR has been performed.\n"
			    "Note that this option is ignored in interactive mode"),
			   NULL);

  scaleback_mode_alignment = gtk_alignment_new(0, 0, 1, 1);
  gtk_alignment_set_padding (GTK_ALIGNMENT (scaleback_mode_alignment), 0, 0, BOX_INDENT, 0);
  gtk_box_pack_start (GTK_BOX (vbox), scaleback_mode_alignment, FALSE, FALSE, 0);
  gtk_widget_show (scaleback_mode_alignment);

  scaleback_mode_event_box = gtk_event_box_new ();
  gtk_container_add (GTK_CONTAINER (scaleback_mode_alignment), scaleback_mode_event_box);
  gtk_widget_show (scaleback_mode_event_box);

  gimp_help_set_help_data (scaleback_mode_event_box,
			   _
			   ("You can choose to rescale back to the original size with LqR or "
			    "standard scaling, or to use standard scaling to reach the previous width "
			    "or height while preserving the aspect ratio"),
			   NULL);

  scaleback_mode_hbox = gtk_hbox_new (FALSE, 4);
  gtk_container_add (GTK_CONTAINER (scaleback_mode_event_box), scaleback_mode_hbox);
  gtk_widget_show (scaleback_mode_hbox);

  scaleback_mode_label = gtk_label_new(_("Mode:"));
  gtk_box_pack_start (GTK_BOX(scaleback_mode_hbox), scaleback_mode_label, FALSE, FALSE, 0);
  gtk_widget_show (scaleback_mode_label);

  scaleback_mode_combo_box =
    gimp_int_combo_box_new (_("liquid rescale"), SCALEBACK_MODE_LQRBACK,
			    _("standard scaling"), SCALEBACK_MODE_STD,
			    _("width only (uniform scaling)"), SCALEBACK_MODE_STDW,
			    _("height only (uniform scaling)"), SCALEBACK_MODE_STDH,
			    NULL);
  gimp_int_combo_box_set_active (GIMP_INT_COMBO_BOX (scaleback_mode_combo_box),
				 state->scaleback_mode);

  gimp_int_combo_box_connect (GIMP_INT_COMBO_BOX (scaleback_mode_combo_box),
			      state->scaleback_mode,
			      G_CALLBACK (callback_scaleback_mode_changed),
			      (gpointer) & preview_data);

  gtk_box_pack_start (GTK_BOX (scaleback_mode_hbox), scaleback_mode_combo_box, FALSE, FALSE, 0);
  gtk_widget_show (scaleback_mode_combo_box);

  g_signal_connect (scaleback_button, "toggled",
		    G_CALLBACK (callback_scaleback_button),
		    (gpointer) (scaleback_mode_alignment));

  callback_scaleback_button (scaleback_button, (gpointer) scaleback_mode_alignment);



  /* Advanced settings page */

  advanced_page = advanced_page_new (image_ID, layer_ID);
  gtk_widget_show (advanced_page);
  notebook_data->advanced_page_ID =
    gtk_notebook_append_page_menu (GTK_NOTEBOOK (notebook), advanced_page,
				   notebook_data->label, NULL);

  /* Mask */

  if (has_mask == TRUE)
    {
      /* Mask page */

      label = gtk_label_new (_("Mask"));

      thispage = gtk_vbox_new (FALSE, 12);
      gtk_container_set_border_width (GTK_CONTAINER (thispage), 12);
      gtk_notebook_append_page_menu (GTK_NOTEBOOK (notebook), thispage, label,
				     NULL);
      gtk_widget_show (thispage);

      frame = gimp_frame_new (_("Select behaviour for the mask"));
      gtk_box_pack_start (GTK_BOX (thispage), frame, FALSE, FALSE, 0);
      gtk_widget_show (frame);

      mask_behavior_combo_box =
	gimp_int_combo_box_new (_("Apply"), GIMP_MASK_APPLY, _("Discard"),
				GIMP_MASK_DISCARD, NULL);
      gimp_int_combo_box_set_active (GIMP_INT_COMBO_BOX
				     (mask_behavior_combo_box),
				     state->mask_behavior);

      gtk_container_add (GTK_CONTAINER (frame), mask_behavior_combo_box);
      gtk_widget_show (mask_behavior_combo_box);
    }

  /*  Show the main containers  */

  gtk_widget_show (main_hbox);
  gtk_widget_show (dlg);
  gtk_main ();

  if ((dialog_response == GTK_RESPONSE_OK) || (dialog_response == RESPONSE_INTERACTIVE) || (dialog_response == RESPONSE_WORK_ON_AUX_LAYER))
    {
      /*  Save ui values  */
      ui_state->chain_active =
	gimp_chain_button_get_active (GIMP_COORDINATES_CHAINBUTTON
				      (coordinates));
      state->new_width =
	ROUND (alt_size_entry_get_refval (ALT_SIZE_ENTRY (coordinates), 0));
      state->new_height =
	ROUND (alt_size_entry_get_refval (ALT_SIZE_ENTRY (coordinates), 1));
      gimp_int_combo_box_get_active (GIMP_INT_COMBO_BOX (nrg_func_combo_box),
				     &(state->nrg_func));
      gimp_int_combo_box_get_active (GIMP_INT_COMBO_BOX (res_order_combo_box),
				     &(state->res_order));
      state->resize_canvas =
	gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON
				      (resize_canvas_button));
      state->resize_aux_layers =
	gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON
				      (resize_aux_layers_button));
      /* save mask behaviour */
      if (has_mask == TRUE)
	{
	  gimp_int_combo_box_get_active (GIMP_INT_COMBO_BOX
					 (mask_behavior_combo_box),
					 &(state->mask_behavior));
	}

      /* save all */
      memcpy (vals, state, sizeof (PlugInVals));
      memcpy (ui_vals, ui_state, sizeof (PlugInUIVals));
    }

  gtk_widget_destroy (dlg);

  g_object_unref (G_OBJECT (preview_data.pixbuf));
  g_free(state);
  g_free(ui_state);
  g_free(notebook_data);

  return dialog_response;
}


/***  Private functions  ***/

/* Callbacks */

static void
callback_dialog_response (GtkWidget * dialog, gint response_id, gpointer data)
{
  NotebookData *n_data = NOTEBOOK_DATA (data);
  switch (response_id)
    {
    case RESPONSE_WORK_ON_AUX_LAYER:
    case RESPONSE_INTERACTIVE:
    case RESPONSE_REFRESH:
    case GTK_RESPONSE_OK:
    case RESPONSE_FEAT_REFRESH:
    case RESPONSE_ADV_REFRESH:
    case RESPONSE_RESET:
      LAYER_CHECK_ACTION (n_data->layer_ID, gtk_dialog_response (GTK_DIALOG(dialog), RESPONSE_FATAL), );
      gtk_window_get_position(GTK_WINDOW(dialog), &(dialog_state->x), &(dialog_state->y));
      dialog_state->has_pos = TRUE;
      break;
    default:
      break;
    }
  switch (response_id)
    {
    case RESPONSE_REFRESH:
      refresh_advanced_page (n_data);
      refresh_features_page (n_data);
      break;
    case RESPONSE_FEAT_REFRESH:
      refresh_features_page (n_data);
      break;
    case RESPONSE_ADV_REFRESH:
      refresh_advanced_page (n_data);
      break;
    default:
      dialog_response = response_id;
      gtk_main_quit ();
      break;
    }
}

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

static void
callback_size_changed (GtkWidget * size_entry, gpointer data)
{
  gint new_width, new_height;
  PreviewData *p_data = PREVIEW_DATA (data);
  new_width =
    ROUND (alt_size_entry_get_refval (ALT_SIZE_ENTRY (size_entry), 0));
  new_height =
    ROUND (alt_size_entry_get_refval (ALT_SIZE_ENTRY (size_entry), 1));
  p_data->vals->new_width = new_width;
  p_data->vals->new_height = new_height;
  callback_set_disc_warning (NULL, data);
}

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
callback_output_target_changed (GtkWidget * output_target_combo, gpointer data)
{
  gint mode;
  PreviewData *p_data = PREVIEW_DATA (data);
  gimp_int_combo_box_get_active (GIMP_INT_COMBO_BOX (output_target_combo), &mode);
  p_data->vals->output_target = mode;
}

static void
callback_scaleback_button (GtkWidget * button, gpointer data)
{
  gboolean button_status =
    gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button));
  state->scaleback = button_status;
  if (button_status)
    {
      gtk_widget_show (GTK_WIDGET (data));
    }
  else
    {
      gtk_widget_hide (GTK_WIDGET (data));
    }
}


static void
callback_scaleback_mode_changed (GtkWidget * scaleback_mode_combo, gpointer data)
{
  gint mode;
  PreviewData *p_data = PREVIEW_DATA (data);
  gimp_int_combo_box_get_active (GIMP_INT_COMBO_BOX (scaleback_mode_combo), &mode);
  p_data->vals->scaleback_mode = mode;
}

static void
callback_lastvalues_button (GtkWidget * button, gpointer data)
{
  gint new_width, new_height;
  PreviewData *p_data = PREVIEW_DATA (data);
  new_width = p_data->ui_vals->last_used_width;
  new_height = p_data->ui_vals->last_used_height;

  alt_size_entry_set_refval (ALT_SIZE_ENTRY
			      (p_data->coordinates), 0, new_width);
  alt_size_entry_set_refval (ALT_SIZE_ENTRY
			      (p_data->coordinates), 1, new_height);
}

static void
callback_resetvalues_button (GtkWidget * button, gpointer data)
{
  gint new_width, new_height;
  PreviewData *p_data = PREVIEW_DATA (data);

  new_width = gimp_drawable_width (p_data->orig_layer_ID);
  new_height = gimp_drawable_height (p_data->orig_layer_ID);

  alt_size_entry_set_refval (ALT_SIZE_ENTRY (p_data->coordinates), 0,
			      new_width);
  alt_size_entry_set_refval (ALT_SIZE_ENTRY (p_data->coordinates), 1,
			      new_height);
}

static void
callback_interactive_button (GtkWidget * button, gpointer data)
{
  gtk_dialog_response (GTK_DIALOG (data), RESPONSE_INTERACTIVE);
}


static void
callback_out_seams_button (GtkWidget * button, gpointer data)
{
  *((gboolean*) data) =
    gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button));
}

static void
callback_out_seams_col_button1 (GtkWidget * button, gpointer data)
{
  GimpRGB *colour;
  PlugInColVals * col_data = (PlugInColVals *) data;

  colour = g_new (GimpRGB, 1);
  gimp_color_button_get_color (GIMP_COLOR_BUTTON
			       (button), colour);

  col_data->r1 = colour->r;
  col_data->g1 = colour->g;
  col_data->b1 = colour->b;

  g_free(colour);
}

static void
callback_out_seams_col_button2 (GtkWidget * button, gpointer data)
{
  GimpRGB *colour;
  PlugInColVals * col_data = (PlugInColVals *) data;

  colour = g_new (GimpRGB, 1);
  gimp_color_button_get_color (GIMP_COLOR_BUTTON
			       (button), colour);

  col_data->r2 = colour->r;
  col_data->g2 = colour->g;
  col_data->b2 = colour->b;

  g_free(colour);
}

static void
callback_resize_aux_layers_button_set_sensitive (GtkWidget * button,
						 gpointer data)
{
  PresDiscStatus *pd_status = PRESDISC_STATUS (data);
  PlugInUIVals *ui = PLUGIN_UI_VALS (pd_status->ui_vals);
  if ((ui->pres_status == TRUE) || (ui->disc_status == TRUE)
      || (ui->rigmask_status == TRUE))
    {
      gtk_widget_set_sensitive ((GtkWidget *) (pd_status->button), TRUE);
    }
  else
    {
      gtk_widget_set_sensitive ((GtkWidget *) (pd_status->button), FALSE);
    }
}

static void callback_expander_changed (GtkWidget * expander, gpointer data)
{
  gboolean * b_data = (gboolean*) data;
  *b_data = !gtk_expander_get_expanded(GTK_EXPANDER(expander));
}


/* Refresh */

static void
refresh_features_page (NotebookData * data)
{
  GtkWidget *new_page;
  gint current_page;

  current_page =
    gtk_notebook_get_current_page (GTK_NOTEBOOK (data->notebook));
  gtk_notebook_remove_page (GTK_NOTEBOOK (data->notebook),
			    data->features_page_ID);
  new_page = features_page_new (data->image_ID, data->layer_ID);
  gtk_widget_show (new_page);
  data->features_page_ID =
    gtk_notebook_prepend_page_menu (GTK_NOTEBOOK (data->notebook), new_page,
				    data->label, NULL);
  data->features_page = new_page;
  gtk_notebook_set_current_page (GTK_NOTEBOOK (data->notebook), current_page);
  callback_resize_aux_layers_button_set_sensitive (NULL,
						   (gpointer)
						   (&presdisc_status));
}

static void
refresh_advanced_page (NotebookData * data)
{
  GtkWidget *new_page;
  gint current_page;

  current_page =
    gtk_notebook_get_current_page (GTK_NOTEBOOK (data->notebook));
  gtk_notebook_remove_page (GTK_NOTEBOOK (data->notebook),
			    data->advanced_page_ID);
  new_page = advanced_page_new (data->image_ID, data->layer_ID);
  gtk_widget_show (new_page);
  data->advanced_page_ID =
    gtk_notebook_append_page_menu (GTK_NOTEBOOK (data->notebook), new_page,
				   data->label, NULL);
  data->advanced_page = new_page;
  gtk_notebook_set_current_page (GTK_NOTEBOOK (data->notebook), current_page);
  callback_resize_aux_layers_button_set_sensitive (NULL,
						   (gpointer)
						   (&presdisc_status));
}

/* Generate features page */

GtkWidget *
features_page_new (gint32 image_ID, gint32 layer_ID)
{
  gint num_extra_layers;
  GtkWidget *label;
  GtkWidget *thispage;
  gchar pres_inactive_tip_string[MAX_STRING_SIZE];
  gchar disc_inactive_tip_string[MAX_STRING_SIZE];
  gchar * disc_strength_tip_string;
  gchar disc_strength_tip_string_[MAX_STRING_SIZE];
  gchar * pres_strength_tip_string;
  gchar pres_strength_tip_string_[MAX_STRING_SIZE];
  NewLayerData *new_pres_layer_data;
  NewLayerData *new_disc_layer_data;
  GtkWidget *pres_frame_event_box1;
  GtkWidget *pres_frame_event_box2;
  GtkWidget *disc_frame_event_box1;
  GtkWidget *disc_frame_event_box2;
  GtkWidget *pres_combo_event_box;
  GtkWidget *disc_combo_event_box;
  gint32 old_layer_ID;
  GtkWidget *frame;
  GtkWidget *pres_vbox;
  GtkWidget *pres_vbox2;
  GtkWidget *hbox;
  GtkWidget *new_hbox;
  GtkWidget *new_icon;
  GtkWidget *new_label;
  GtkWidget *edit_hbox;
  GtkWidget *edit_icon;
  GtkWidget *edit_label;
  GtkWidget *pres_button;
  GtkWidget *pres_new_button;
  GtkWidget *pres_edit_button;
  GtkWidget *disc_vbox;
  GtkWidget *disc_vbox2;
  GtkWidget *disc_button;
  GtkWidget *disc_new_button;
  GtkWidget *disc_edit_button;
  GtkWidget *disc_warning_image;
  GtkWidget *guess_label;
  GtkWidget *guess_button_hor;
  GtkWidget *guess_button_ver;
  GtkWidget *table;
  gint row;
  GtkWidget *combo;
  GtkObject *adj;

  label = gtk_label_new (_("Feature masks"));
  notebook_data->label = label;

  new_pres_layer_data = g_new (NewLayerData, 1);
  new_disc_layer_data = g_new (NewLayerData, 1);

  new_pres_layer_data->preview_data = &preview_data;
  new_pres_layer_data->layer_ID = &(state->pres_layer_ID);
  new_pres_layer_data->status = &(ui_state->pres_status);
  /* The name of a newly created layer for preservation */
  /* (here "%s" represents the selected layer's name) */
  g_snprintf (new_pres_layer_data->name, LQR_MAX_NAME_LENGTH, _("%s pres mask"),
              gimp_drawable_get_name (preview_data.orig_layer_ID));
  gimp_rgb_set (&(new_pres_layer_data->colour), 0, 1, 0);
  new_pres_layer_data->layer_type = AUX_LAYER_PRES;

  new_disc_layer_data->preview_data = &preview_data;
  new_disc_layer_data->layer_ID = &(state->disc_layer_ID);
  new_disc_layer_data->status = &(ui_state->disc_status);
  /* The name of a newly created layer for discard */
  /* (here "%s" represents the selected layer's name) */
  g_snprintf (new_disc_layer_data->name, LQR_MAX_NAME_LENGTH, _("%s disc mask"),
	    gimp_drawable_get_name (preview_data.orig_layer_ID));
  gimp_rgb_set (&(new_disc_layer_data->colour), 1, 0, 0);
  new_disc_layer_data->layer_type = AUX_LAYER_DISC;

  num_extra_layers = count_extra_layers (image_ID);
  features_are_sensitive = (num_extra_layers > 0 ? TRUE : FALSE);

  if (!features_are_sensitive ||
      !gimp_drawable_is_valid(state->pres_layer_ID) ||
      !gimp_drawable_is_layer(state->pres_layer_ID) ||
      (state->pres_layer_ID == layer_ID))
    {
      ui_state->pres_status = FALSE;
      state->pres_layer_ID = 0;
      preview_data.pres_combo_awaked = FALSE;
    }
  if (!features_are_sensitive ||
      !gimp_drawable_is_valid(state->disc_layer_ID) ||
      !gimp_drawable_is_layer(state->disc_layer_ID) ||
      (state->disc_layer_ID == layer_ID))
    {
      ui_state->disc_status = FALSE;
      state->disc_layer_ID = 0;
      preview_data.disc_combo_awaked = FALSE;
    }

  thispage = gtk_vbox_new (FALSE, 12);
  gtk_container_set_border_width (GTK_CONTAINER (thispage), 12);
  notebook_data->features_page = thispage;


  /*  Feature preservation  */

  frame = gimp_frame_new (_("Feature preservation mask"));
  gtk_box_pack_start (GTK_BOX (thispage), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  g_snprintf (pres_inactive_tip_string, MAX_STRING_SIZE,
	    _("Extra layers are needed to activate feature preservation.\n"
	      "You can create one with the \"New\" button and paint on it, "
	      "then press the \"Refresh\" button.\n"
	      "Note that painting in black has no effect"));

  pres_vbox = gtk_vbox_new (FALSE, 4);
  gtk_container_add (GTK_CONTAINER (frame), pres_vbox);
  gtk_widget_show (pres_vbox);

  hbox = gtk_hbox_new (FALSE, 4);
  gtk_box_pack_start (GTK_BOX (pres_vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  pres_frame_event_box1 = gtk_event_box_new ();
  gtk_event_box_set_visible_window (GTK_EVENT_BOX (pres_frame_event_box1),
				    FALSE);
  gtk_box_pack_start (GTK_BOX (hbox), pres_frame_event_box1, FALSE, FALSE, 0);
  gtk_widget_show (pres_frame_event_box1);


  if (!features_are_sensitive)
    {
      gtk_event_box_set_above_child (GTK_EVENT_BOX (pres_frame_event_box1),
				     TRUE);
      gtk_widget_set_tooltip_text(pres_frame_event_box1, pres_inactive_tip_string);
    }


  pres_button = gtk_check_button_new_with_label (_("Preserve features"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (pres_button),
				ui_state->pres_status);

  gtk_widget_set_sensitive (pres_button, features_are_sensitive);


  gtk_container_add (GTK_CONTAINER (pres_frame_event_box1), pres_button);
  gtk_widget_show (pres_button);

  g_signal_connect (pres_button, "toggled",
		    G_CALLBACK
		    (callback_status_button),
		    (gpointer) (&ui_state->pres_status));

  gimp_help_set_help_data (pres_button,
			   _("Use an extra layer to preserve "
			     "selected areas from distortion"), NULL);

  pres_edit_button = gtk_button_new ();
  gtk_box_pack_end (GTK_BOX (hbox), pres_edit_button, FALSE, FALSE, 0);
  gtk_widget_show (pres_edit_button);

  edit_hbox = gtk_hbox_new (FALSE, 4);
  gtk_container_add (GTK_CONTAINER(pres_edit_button), edit_hbox);
  gtk_widget_show (edit_hbox);

  edit_icon = gtk_image_new_from_stock (GTK_STOCK_EDIT, GTK_ICON_SIZE_MENU);
  gtk_box_pack_start (GTK_BOX(edit_hbox), edit_icon, TRUE, TRUE, 0);
  gtk_widget_show (edit_icon);
  edit_label = gtk_label_new (_("Edit"));
  gtk_box_pack_end (GTK_BOX(edit_hbox), edit_label, TRUE, TRUE, 0);
  gtk_widget_show (edit_label);

  gimp_help_set_help_data (pres_edit_button,
			   _("Edit the currently selected preservation layer"),
			   NULL);

  pres_new_button = gtk_button_new ();
  gtk_box_pack_end (GTK_BOX (hbox), pres_new_button, FALSE, FALSE, 0);
  gtk_widget_show (pres_new_button);

  new_hbox = gtk_hbox_new (FALSE, 4);
  gtk_container_add (GTK_CONTAINER(pres_new_button), new_hbox);
  gtk_widget_show (new_hbox);

  new_icon = gtk_image_new_from_stock (GTK_STOCK_NEW, GTK_ICON_SIZE_MENU);
  gtk_box_pack_start (GTK_BOX(new_hbox), new_icon, TRUE, TRUE, 0);
  gtk_widget_show (new_icon);
  new_label = gtk_label_new (_("New"));
  gtk_box_pack_end (GTK_BOX(new_hbox), new_label, TRUE, TRUE, 0);
  gtk_widget_show (new_label);

  gimp_help_set_help_data (pres_new_button,
			   _("Creates a new transparent layer "
			     "ready to be used as a preservation mask"),
			   NULL);


  g_signal_connect (pres_new_button, "clicked",
		    G_CALLBACK
		    (callback_new_mask_button),
		    (gpointer) (new_pres_layer_data));

  g_signal_connect (pres_edit_button, "clicked",
		    G_CALLBACK
		    (callback_edit_mask_button),
		    (gpointer) (new_pres_layer_data));


  pres_frame_event_box2 = gtk_event_box_new ();
  gtk_event_box_set_visible_window (GTK_EVENT_BOX (pres_frame_event_box2),
				    FALSE);
  gtk_box_pack_start (GTK_BOX (pres_vbox), pres_frame_event_box2, FALSE,
		      FALSE, 0);
  gtk_widget_show (pres_frame_event_box2);


  if (!features_are_sensitive)
    {
      gtk_event_box_set_above_child (GTK_EVENT_BOX (pres_frame_event_box2),
				     TRUE);
      gtk_widget_set_tooltip_text(pres_frame_event_box2, pres_inactive_tip_string);
    }

  pres_vbox2 = gtk_vbox_new (FALSE, 4);
  gtk_container_add (GTK_CONTAINER (pres_frame_event_box2), pres_vbox2);
  gtk_widget_show (pres_vbox2);


  pres_combo_event_box = gtk_event_box_new ();
  gtk_box_pack_start (GTK_BOX (pres_vbox2), pres_combo_event_box, FALSE,
		      FALSE, 0);
  gtk_widget_show (pres_combo_event_box);

  if (features_are_sensitive)
    {
      gimp_help_set_help_data (pres_combo_event_box,
			       _("Layer to be used as a mask for "
				 "feature preservation.\n"
				 "Use the \"Refresh\" button to update the list"),
			       NULL);
    }

  table = gtk_table_new (1, 2, FALSE);
  gtk_container_set_border_width (GTK_CONTAINER (table), 4);
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);
  gtk_table_set_row_spacings (GTK_TABLE (table), 2);
  gtk_container_add (GTK_CONTAINER (pres_combo_event_box), table);
  gtk_widget_show (table);

  row = 0;

  combo =
    gimp_layer_combo_box_new (dialog_layer_constraint_func,
			      (gpointer) (&layer_ID));

  g_object_set (combo, "ellipsize", PANGO_ELLIPSIZE_START, NULL);

  old_layer_ID = state->pres_layer_ID;

  gimp_int_combo_box_connect (GIMP_INT_COMBO_BOX (combo),
			      layer_ID,
			      G_CALLBACK (callback_pres_combo_get_active),
			      (gpointer) (&preview_data));

  gimp_int_combo_box_set_active (GIMP_INT_COMBO_BOX (combo), old_layer_ID);

  label = gimp_table_attach_aligned (GTK_TABLE (table), 0, row++,
				     _("Layer:"), 0.0, 0.5, combo, 1, FALSE);

  gtk_widget_set_sensitive (label, ui_state->pres_status
			    && features_are_sensitive);

  gtk_widget_set_sensitive (combo, ui_state->pres_status
			    && features_are_sensitive);

  gtk_widget_set_sensitive (pres_edit_button, ui_state->pres_status
			    && features_are_sensitive);

  pres_toggle_data.combo = combo;
  pres_toggle_data.combo_label = label;
  pres_toggle_data.edit_button = pres_edit_button;
  preview_data.pres_combo = combo;

  gtk_widget_show (combo);

  table = gtk_table_new (1, 2, FALSE);
  gtk_container_set_border_width (GTK_CONTAINER (table), 4);
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);
  gtk_table_set_row_spacings (GTK_TABLE (table), 2);
  gtk_box_pack_start (GTK_BOX (pres_vbox2), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  if (features_are_sensitive)
    {
      g_snprintf (pres_strength_tip_string_, MAX_STRING_SIZE,
	        _("Overall coefficient for "
	          "feature preservation intensity"));
      pres_strength_tip_string = pres_strength_tip_string_;
    }
  else
    {
      pres_strength_tip_string = NULL;
    }

  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, row++,
			      _("Strength:"), SCALE_WIDTH, SPIN_BUTTON_WIDTH,
			      state->pres_coeff, 0, MAX_COEFF, 1, 10, 0,
			      TRUE, 0, 0,
			      pres_strength_tip_string,
			      NULL);

  g_signal_connect (adj, "value_changed",
		    G_CALLBACK (gimp_int_adjustment_update),
		    &state->pres_coeff);

  gtk_widget_set_sensitive (GIMP_SCALE_ENTRY_LABEL (adj),
			    (ui_state->pres_status
			     && features_are_sensitive));
  gtk_widget_set_sensitive (GIMP_SCALE_ENTRY_SCALE (adj),
			    (ui_state->pres_status
			     && features_are_sensitive));
  gtk_widget_set_sensitive (GIMP_SCALE_ENTRY_SPINBUTTON (adj),
			    (ui_state->pres_status
			     && features_are_sensitive));
  pres_toggle_data.scale = adj;

  pres_toggle_data.status = &(ui_state->pres_status);

  g_signal_connect (G_OBJECT (pres_button), "toggled",
		    G_CALLBACK (callback_combo_set_sensitive),
		    (gpointer) (&pres_toggle_data));

  g_signal_connect (G_OBJECT (pres_button), "toggled",
		    G_CALLBACK (callback_pres_combo_set_sensitive_preview),
		    (gpointer) (&preview_data));

  pres_toggle_data.guess_label = NULL;
  pres_toggle_data.guess_button_hor = NULL;
  pres_toggle_data.guess_button_ver = NULL;


  /*  Feature discard  */

  frame = gimp_frame_new (_("Feature discard mask"));
  gtk_box_pack_start (GTK_BOX (thispage), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  g_snprintf (disc_inactive_tip_string, MAX_STRING_SIZE,
	    _("Extra layers are needed to activate feature discard.\n"
	      "You can create one with the \"New\" button and paint on it, "
	      "then press the \"Refresh\" button.\n"
	      "Note that painting in black has no effect"));


  disc_vbox = gtk_vbox_new (FALSE, 4);
  gtk_container_add (GTK_CONTAINER (frame), disc_vbox);
  gtk_widget_show (disc_vbox);

  hbox = gtk_hbox_new (FALSE, 4);
  gtk_box_pack_start (GTK_BOX (disc_vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  disc_frame_event_box1 = gtk_event_box_new ();
  gtk_event_box_set_visible_window (GTK_EVENT_BOX (disc_frame_event_box1),
				    FALSE);
  gtk_box_pack_start (GTK_BOX (hbox), disc_frame_event_box1, FALSE, FALSE, 0);
  gtk_widget_show (disc_frame_event_box1);

  if (!features_are_sensitive)
    {
      gtk_event_box_set_above_child (GTK_EVENT_BOX (disc_frame_event_box1),
				     TRUE);
      gtk_widget_set_tooltip_text(disc_frame_event_box1, disc_inactive_tip_string);
    }

  disc_button = gtk_check_button_new_with_label (_("Discard features"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (disc_button),
				ui_state->disc_status);

  gtk_widget_set_sensitive (disc_button, features_are_sensitive);


  gtk_container_add (GTK_CONTAINER (disc_frame_event_box1), disc_button);
  gtk_widget_show (disc_button);

  g_signal_connect (disc_button, "toggled",
		    G_CALLBACK
		    (callback_status_button),
		    (gpointer) (&ui_state->disc_status));


  gimp_help_set_help_data (disc_button,
			   _("Use an extra layer to treat selected "
			     "areas as if they were meaningless "
			     "(useful to remove parts of the image "
			     "when shrinking)"), NULL);

  disc_warning_image = gtk_image_new_from_stock (GIMP_STOCK_WARNING,
						 GTK_ICON_SIZE_MENU);
  gtk_box_pack_start (GTK_BOX (hbox), disc_warning_image, FALSE, FALSE, 0);
  gimp_help_set_help_data (disc_warning_image,
			   _
			   ("Warning: the discard mask information will be ignored with the current settings.\n"
			    "(If you know what you're doing you can override this behaviour by unchecking the "
			    "corrensponding option in the \"Advanced\" tab)"),
			   NULL);

  preview_data.disc_warning_image = disc_warning_image;
  callback_set_disc_warning (NULL, (gpointer) & preview_data);

  disc_edit_button = gtk_button_new ();
  gtk_box_pack_end (GTK_BOX (hbox), disc_edit_button, FALSE, FALSE, 0);
  gtk_widget_show (disc_edit_button);

  edit_hbox = gtk_hbox_new (FALSE, 4);
  gtk_container_add (GTK_CONTAINER(disc_edit_button), edit_hbox);
  gtk_widget_show (edit_hbox);

  edit_icon = gtk_image_new_from_stock (GTK_STOCK_EDIT, GTK_ICON_SIZE_MENU);
  gtk_box_pack_start (GTK_BOX(edit_hbox), edit_icon, TRUE, TRUE, 0);
  gtk_widget_show (edit_icon);
  edit_label = gtk_label_new (_("Edit"));
  gtk_box_pack_end (GTK_BOX(edit_hbox), edit_label, TRUE, TRUE, 0);
  gtk_widget_show (edit_label);

  gimp_help_set_help_data (disc_edit_button,
			   _("Edit the currently selected discard layer"),
			   NULL);

  disc_new_button = gtk_button_new ();
  gtk_box_pack_end (GTK_BOX (hbox), disc_new_button, FALSE, FALSE, 0);
  gtk_widget_show (disc_new_button);

  new_hbox = gtk_hbox_new (FALSE, 4);
  gtk_container_add (GTK_CONTAINER(disc_new_button), new_hbox);
  gtk_widget_show (new_hbox);

  new_icon = gtk_image_new_from_stock (GTK_STOCK_NEW, GTK_ICON_SIZE_MENU);
  gtk_box_pack_start (GTK_BOX(new_hbox), new_icon, TRUE, TRUE, 0);
  gtk_widget_show (new_icon);
  new_label = gtk_label_new (_("New"));
  gtk_box_pack_end (GTK_BOX(new_hbox), new_label, TRUE, TRUE, 0);
  gtk_widget_show (new_label);

  gimp_help_set_help_data (disc_new_button,
			   _("Creates a new transparent layer "
			     "ready to be used as a discard mask"), NULL);


  g_signal_connect (disc_new_button, "clicked",
		    G_CALLBACK
		    (callback_new_mask_button),
		    (gpointer) (new_disc_layer_data));

  g_signal_connect (disc_edit_button, "clicked",
		    G_CALLBACK
		    (callback_edit_mask_button),
		    (gpointer) (new_disc_layer_data));


  disc_frame_event_box2 = gtk_event_box_new ();
  gtk_event_box_set_visible_window (GTK_EVENT_BOX (disc_frame_event_box2),
				    FALSE);
  gtk_box_pack_start (GTK_BOX (disc_vbox), disc_frame_event_box2, FALSE,
		      FALSE, 0);
  gtk_widget_show (disc_frame_event_box2);


  if (!features_are_sensitive)
    {
      gtk_event_box_set_above_child (GTK_EVENT_BOX (disc_frame_event_box2),
				     TRUE);
      gtk_widget_set_tooltip_text(disc_frame_event_box2, disc_inactive_tip_string);
    }

  disc_vbox2 = gtk_vbox_new (FALSE, 4);
  gtk_container_add (GTK_CONTAINER (disc_frame_event_box2), disc_vbox2);
  gtk_widget_show (disc_vbox2);


  disc_combo_event_box = gtk_event_box_new ();
  gtk_box_pack_start (GTK_BOX (disc_vbox2), disc_combo_event_box, FALSE,
		      FALSE, 0);
  gtk_widget_show (disc_combo_event_box);

  if (features_are_sensitive)
    {
      gimp_help_set_help_data (disc_combo_event_box,
			       _("Layer to be used as a mask "
				 "for feature discard.\n"
				 "Use the \"Refresh\" button "
				 "to update the list"), NULL);
    }

  table = gtk_table_new (1, 2, FALSE);
  gtk_container_set_border_width (GTK_CONTAINER (table), 4);
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);
  gtk_table_set_row_spacings (GTK_TABLE (table), 2);
  gtk_container_add (GTK_CONTAINER (disc_combo_event_box), table);
  gtk_widget_show (table);

  row = 0;

  combo =
    gimp_layer_combo_box_new (dialog_layer_constraint_func,
			      (gpointer) (&layer_ID));

  g_object_set (combo, "ellipsize", PANGO_ELLIPSIZE_START, NULL);

  old_layer_ID = state->disc_layer_ID;

  gimp_int_combo_box_connect (GIMP_INT_COMBO_BOX (combo),
			      layer_ID,
			      G_CALLBACK (callback_disc_combo_get_active),
			      (gpointer) (&preview_data));

  gimp_int_combo_box_set_active (GIMP_INT_COMBO_BOX (combo), old_layer_ID);

  label = gimp_table_attach_aligned (GTK_TABLE (table), 0, row++,
				     _("Layer:"), 0.0, 0.5, combo, 1, FALSE);


  gtk_widget_set_sensitive (combo, ui_state->disc_status
			    && features_are_sensitive);

  gtk_widget_set_sensitive (label, ui_state->disc_status
			    && features_are_sensitive);

  gtk_widget_set_sensitive (disc_edit_button, ui_state->disc_status
			    && features_are_sensitive);

  disc_toggle_data.combo = combo;
  disc_toggle_data.combo_label = label;
  disc_toggle_data.edit_button = disc_edit_button;
  preview_data.disc_combo = combo;

  gtk_widget_show (combo);

  table = gtk_table_new (1, 2, FALSE);
  gtk_container_set_border_width (GTK_CONTAINER (table), 4);
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);
  gtk_table_set_row_spacings (GTK_TABLE (table), 2);
  gtk_box_pack_start (GTK_BOX (disc_vbox2), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  if (features_are_sensitive)
    {
      g_snprintf (disc_strength_tip_string_, MAX_STRING_SIZE,
	        _("Overall coefficient for "
	          "feature discard intensity"));
      disc_strength_tip_string = disc_strength_tip_string_;
    }
  else
    {
      disc_strength_tip_string = NULL;
    }


  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, 0,
			      _("Strength:"), SCALE_WIDTH, SPIN_BUTTON_WIDTH,
			      state->disc_coeff, 0, MAX_COEFF, 1, 10, 0,
			      TRUE, 0, 0,
			      disc_strength_tip_string,
			      NULL);

  g_signal_connect (adj, "value_changed",
		    G_CALLBACK (gimp_int_adjustment_update),
		    (gpointer) & (state->disc_coeff));
  g_signal_connect (adj, "value_changed",
		    G_CALLBACK (callback_set_disc_warning),
		    (gpointer) & preview_data);


  gtk_widget_set_sensitive (GIMP_SCALE_ENTRY_LABEL (adj),
			    (ui_state->disc_status
			     && features_are_sensitive));
  gtk_widget_set_sensitive (GIMP_SCALE_ENTRY_SCALE (adj),
			    (ui_state->disc_status
			     && features_are_sensitive));
  gtk_widget_set_sensitive (GIMP_SCALE_ENTRY_SPINBUTTON (adj),
			    (ui_state->disc_status
			     && features_are_sensitive));

  disc_toggle_data.scale = adj;

  disc_toggle_data.status = &(ui_state->disc_status);

  g_signal_connect (G_OBJECT (disc_button), "toggled",
		    G_CALLBACK (callback_combo_set_sensitive),
		    (gpointer) (&disc_toggle_data));

  g_signal_connect (G_OBJECT (disc_button), "toggled",
		    G_CALLBACK (callback_disc_combo_set_sensitive_preview),
		    (gpointer) (&preview_data));

  g_signal_connect (pres_button, "toggled",
		    G_CALLBACK
		    (callback_resize_aux_layers_button_set_sensitive),
		    (gpointer) (&presdisc_status));
  g_signal_connect (disc_button, "toggled",
		    G_CALLBACK
		    (callback_resize_aux_layers_button_set_sensitive),
		    (gpointer) (&presdisc_status));

  g_signal_connect (disc_button, "toggled",
		    G_CALLBACK
		    (callback_set_disc_warning), (gpointer) (&preview_data));


  hbox = gtk_hbox_new (FALSE, 4);
  gtk_box_pack_start (GTK_BOX (disc_vbox2), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  // Auto-size buttons
  guess_label = gtk_label_new (_("Auto size:"));
  gtk_box_pack_start (GTK_BOX (hbox), guess_label, FALSE, FALSE, 0);
  gtk_widget_show (guess_label);

  disc_toggle_data.guess_label = guess_label;

  gtk_widget_set_sensitive (guess_label,
			    (ui_state->disc_status
			     && features_are_sensitive));

  // Width auto-size button
  guess_button_hor = gtk_button_new_with_label (_("Width"));
  gtk_box_pack_start (GTK_BOX (hbox), guess_button_hor, FALSE, FALSE, 0);
  gtk_widget_show (guess_button_hor);

  disc_toggle_data.guess_button_hor = guess_button_hor;

  gtk_widget_set_sensitive (guess_button_hor,
			    (ui_state->disc_status
			     && features_are_sensitive));

  if (features_are_sensitive)
    {
      gimp_help_set_help_data (guess_button_hor,
                               _
                               ("Try to set the final width as needed to remove the masked areas.\n"
                                "Only use with simple masks"), NULL);
    }

  g_signal_connect (guess_button_hor, "clicked",
		    G_CALLBACK (callback_guess_button_hor),
		    (gpointer) & preview_data);


  // Height auto-size button
  guess_button_ver = gtk_button_new_with_label (_("Height"));
  gtk_box_pack_start (GTK_BOX (hbox), guess_button_ver, FALSE, FALSE, 0);
  gtk_widget_show (guess_button_ver);

  disc_toggle_data.guess_button_ver = guess_button_ver;

  gtk_widget_set_sensitive (guess_button_ver,
			    (ui_state->disc_status
			     && features_are_sensitive));

  if (features_are_sensitive)
    {
      gimp_help_set_help_data (guess_button_ver,
                               _
                               ("Try to set the final height as needed to remove the masked areas.\n"
                                "Only use with simple masks"), NULL);
    }

  g_signal_connect (guess_button_ver, "clicked",
		    G_CALLBACK (callback_guess_button_ver),
		    (gpointer) & preview_data);

  return thispage;
}

/* Generate advanced options page */

GtkWidget *
advanced_page_new (gint32 image_ID, gint32 layer_ID)
{
  gint num_extra_layers;
  GtkWidget *label;
  GtkWidget *thispage;
  GtkWidget *scrollwindow;
  gchar rigmask_inactive_tip_string[MAX_STRING_SIZE];
  NewLayerData *new_rigmask_layer_data;
  GtkWidget *rigmask_frame_event_box1;
  GtkWidget *rigmask_frame_event_box2;
  GtkWidget *rigmask_combo_event_box;
  gint32 old_layer_ID;
  GtkWidget *seams_control_expander;
  GtkWidget *operations_expander;
  GtkWidget *rigmask_vbox;
  GtkWidget *rigmask_vbox2;
  GtkWidget *hbox;
  GtkWidget *new_hbox;
  GtkWidget *new_icon;
  GtkWidget *new_label;
  GtkWidget *edit_hbox;
  GtkWidget *edit_icon;
  GtkWidget *edit_label;
  GtkWidget *rigmask_button;
  GtkWidget *rigmask_new_button;
  GtkWidget *rigmask_edit_button;
  GtkWidget *operations_vbox;
  GtkWidget *no_disc_on_enlarge_button;
  GtkWidget *table;
  gint row;
  GtkWidget *combo;
  GtkObject *adj;

  GtkWidget *nrg_event_box;
  GtkWidget *res_order_event_box;

  label = gtk_label_new (_("Advanced"));
  notebook_data->label = label;

  new_rigmask_layer_data = g_new (NewLayerData, 1);

  new_rigmask_layer_data->preview_data = &preview_data;
  new_rigmask_layer_data->layer_ID = &(state->rigmask_layer_ID);
  new_rigmask_layer_data->status = &(ui_state->rigmask_status);
  /* The name of a newly created layer for rigidity mask */
  /* (here "%s" represents the selected layer's name) */
  g_snprintf (new_rigmask_layer_data->name, LQR_MAX_NAME_LENGTH,
	    _("%s rigidity mask"),
	    gimp_drawable_get_name (preview_data.orig_layer_ID));
  gimp_rgb_set (&(new_rigmask_layer_data->colour), 0, 0, 1);
  new_rigmask_layer_data->layer_type = AUX_LAYER_RIGMASK;

  num_extra_layers = count_extra_layers (image_ID);
  features_are_sensitive = (num_extra_layers > 0 ? TRUE : FALSE);
  preview_data.rigmask_combo_awaked = FALSE;

  if (!features_are_sensitive ||
      !gimp_drawable_is_valid(state->rigmask_layer_ID) ||
      !gimp_drawable_is_layer(state->rigmask_layer_ID) ||
      (state->rigmask_layer_ID == layer_ID))
    {
      ui_state->rigmask_status = FALSE;
      state->rigmask_layer_ID = 0;
    }

  thispage = gtk_vbox_new (FALSE, 12);
  gtk_container_set_border_width (GTK_CONTAINER (thispage), 12);
  gtk_widget_show (thispage);

  scrollwindow = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrollwindow), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (scrollwindow), thispage);

  notebook_data->advanced_page = scrollwindow;


  /*  Seams control  */

  /* Please keep the <b> and </b> tags in translations */
  seams_control_expander = gtk_expander_new (_("<b>Seams control</b>"));
  gtk_expander_set_use_markup (GTK_EXPANDER(seams_control_expander), TRUE);
  gtk_expander_set_expanded (GTK_EXPANDER(seams_control_expander), TRUE);
  g_signal_connect (seams_control_expander, "activate",
		    G_CALLBACK
		    (callback_expander_changed),
		    (gpointer) (&ui_state->seams_control_expanded));

  gtk_box_pack_start (GTK_BOX (thispage), seams_control_expander, FALSE, FALSE, 0);
  gtk_widget_show (seams_control_expander);

  g_snprintf (rigmask_inactive_tip_string, MAX_STRING_SIZE,
	    _("Extra layers are needed to be used as rigidity masks.\n"
	      "You can create one with the \"New\" button and paint on it, "
	      "then press the \"Refresh\" button.\n"
	      "Note that painting in black has no effect"));

  rigmask_vbox = gtk_vbox_new (FALSE, 4);
  gtk_container_add (GTK_CONTAINER (seams_control_expander), rigmask_vbox);
  gtk_widget_show (rigmask_vbox);

  table = gtk_table_new (3, 2, FALSE);
  gtk_container_set_border_width (GTK_CONTAINER (table), 4);
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);
  gtk_table_set_row_spacings (GTK_TABLE (table), 2);
  gtk_box_pack_start (GTK_BOX (rigmask_vbox), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  row = 0;

  /* Delta x */

  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, row++,
			      _("Max transversal step:"), SCALE_WIDTH,
			      SPIN_BUTTON_WIDTH, state->delta_x, 0,
			      MAX_DELTA_X, 1, 1, 0, TRUE, 0, 0,
			      _("Maximum displacement along a seam. "
				"Increasing this value allows to overcome "
				"the 45 degrees bound"), NULL);

  g_signal_connect (adj, "value_changed",
		    G_CALLBACK (gimp_int_adjustment_update), &state->delta_x);

  /* Rigidity */

  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, row++,
			      _("Overall rigidity:"), SCALE_WIDTH,
			      SPIN_BUTTON_WIDTH, state->rigidity, 0,
			      MAX_RIGIDITY, 0.2, 10, 2, TRUE, 0, 0,
			      _("Increasing this value results "
				"in straighter seams"), NULL);

  g_signal_connect (adj, "value_changed",
		    G_CALLBACK (gimp_float_adjustment_update),
		    &state->rigidity);


  hbox = gtk_hbox_new (FALSE, 4);
  gtk_box_pack_start (GTK_BOX (rigmask_vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  rigmask_frame_event_box1 = gtk_event_box_new ();
  gtk_event_box_set_visible_window (GTK_EVENT_BOX (rigmask_frame_event_box1),
				    FALSE);
  gtk_box_pack_start (GTK_BOX (hbox), rigmask_frame_event_box1, FALSE, FALSE,
		      0);
  gtk_widget_show (rigmask_frame_event_box1);


  if (!features_are_sensitive)
    {
      gtk_event_box_set_above_child (GTK_EVENT_BOX (rigmask_frame_event_box1),
				     TRUE);
      gtk_widget_set_tooltip_text(rigmask_frame_event_box1, rigmask_inactive_tip_string);
    }


  rigmask_button = gtk_check_button_new_with_label (_("Use a rigidity mask"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (rigmask_button),
				ui_state->rigmask_status);

  gtk_widget_set_sensitive (rigmask_button, features_are_sensitive);


  gtk_container_add (GTK_CONTAINER (rigmask_frame_event_box1),
		     rigmask_button);
  gtk_widget_show (rigmask_button);

  g_signal_connect (rigmask_button, "toggled",
		    G_CALLBACK
		    (callback_status_button),
		    (gpointer) (&ui_state->rigmask_status));

  g_signal_connect (rigmask_button, "toggled",
		    G_CALLBACK
		    (callback_resize_aux_layers_button_set_sensitive),
		    (gpointer) (&presdisc_status));

  callback_resize_aux_layers_button_set_sensitive (NULL,
						   (gpointer)
						   (&presdisc_status));

  gimp_help_set_help_data (rigmask_button,
			   _
			   ("Use an extra layer to mark areas where seams should be straighter"),
			   NULL);

  rigmask_edit_button = gtk_button_new ();
  gtk_box_pack_end (GTK_BOX (hbox), rigmask_edit_button, FALSE, FALSE, 0);
  gtk_widget_show (rigmask_edit_button);

  edit_hbox = gtk_hbox_new (FALSE, 4);
  gtk_container_add (GTK_CONTAINER(rigmask_edit_button), edit_hbox);
  gtk_widget_show (edit_hbox);

  edit_icon = gtk_image_new_from_stock (GTK_STOCK_EDIT, GTK_ICON_SIZE_MENU);
  gtk_box_pack_start (GTK_BOX(edit_hbox), edit_icon, TRUE, TRUE, 0);
  gtk_widget_show (edit_icon);
  edit_label = gtk_label_new (_("Edit"));
  gtk_box_pack_end (GTK_BOX(edit_hbox), edit_label, TRUE, TRUE, 0);
  gtk_widget_show (edit_label);

  gimp_help_set_help_data (rigmask_edit_button,
			   _("Edit the currently selected rigidity mask layer"),
			   NULL);

  rigmask_new_button = gtk_button_new ();
  gtk_box_pack_end (GTK_BOX (hbox), rigmask_new_button, FALSE, FALSE, 0);
  gtk_widget_show (rigmask_new_button);

  new_hbox = gtk_hbox_new (FALSE, 4);
  gtk_container_add (GTK_CONTAINER(rigmask_new_button), new_hbox);
  gtk_widget_show (new_hbox);

  new_icon = gtk_image_new_from_stock (GTK_STOCK_NEW, GTK_ICON_SIZE_MENU);
  gtk_box_pack_start (GTK_BOX(new_hbox), new_icon, TRUE, TRUE, 0);
  gtk_widget_show (new_icon);
  new_label = gtk_label_new (_("New"));
  gtk_box_pack_end (GTK_BOX(new_hbox), new_label, TRUE, TRUE, 0);
  gtk_widget_show (new_label);

  gimp_help_set_help_data (rigmask_new_button,
			   _("Creates a new transparent layer "
			     "ready to be used as a rigidity mask"), NULL);

  g_signal_connect (rigmask_new_button, "clicked",
		    G_CALLBACK
		    (callback_new_mask_button),
		    (gpointer) (new_rigmask_layer_data));

  g_signal_connect (rigmask_edit_button, "clicked",
		    G_CALLBACK
		    (callback_edit_mask_button),
		    (gpointer) (new_rigmask_layer_data));


  rigmask_frame_event_box2 = gtk_event_box_new ();
  gtk_event_box_set_visible_window (GTK_EVENT_BOX (rigmask_frame_event_box2),
				    FALSE);
  gtk_box_pack_start (GTK_BOX (rigmask_vbox), rigmask_frame_event_box2, FALSE,
		      FALSE, 0);
  gtk_widget_show (rigmask_frame_event_box2);


  if (!features_are_sensitive)
    {
      gtk_event_box_set_above_child (GTK_EVENT_BOX (rigmask_frame_event_box2),
				     TRUE);
      gtk_widget_set_tooltip_text(rigmask_frame_event_box2, rigmask_inactive_tip_string);
    }

  rigmask_vbox2 = gtk_vbox_new (FALSE, 4);
  gtk_container_add (GTK_CONTAINER (rigmask_frame_event_box2), rigmask_vbox2);
  gtk_widget_show (rigmask_vbox2);


  rigmask_combo_event_box = gtk_event_box_new ();
  gtk_box_pack_start (GTK_BOX (rigmask_vbox2), rigmask_combo_event_box, FALSE,
		      FALSE, 0);
  gtk_widget_show (rigmask_combo_event_box);

  if (features_are_sensitive)
    {
      gimp_help_set_help_data (rigmask_combo_event_box,
			       _("Layer to be used as a mask for "
				 "rigidity settings.\n"
				 "Use the \"Refresh\" button to update the list"),
			       NULL);
    }

  table = gtk_table_new (1, 2, FALSE);
  gtk_container_set_border_width (GTK_CONTAINER (table), 4);
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);
  gtk_table_set_row_spacings (GTK_TABLE (table), 2);
  gtk_container_add (GTK_CONTAINER (rigmask_combo_event_box), table);
  gtk_widget_show (table);

  row = 0;

  combo =
    gimp_layer_combo_box_new (dialog_layer_constraint_func,
			      (gpointer) (&layer_ID));

  g_object_set (combo, "ellipsize", PANGO_ELLIPSIZE_START, NULL);

  old_layer_ID = state->rigmask_layer_ID;

  gimp_int_combo_box_connect (GIMP_INT_COMBO_BOX (combo),
			      layer_ID,
			      G_CALLBACK (callback_rigmask_combo_get_active),
			      (gpointer) (&preview_data));

  gimp_int_combo_box_set_active (GIMP_INT_COMBO_BOX (combo), old_layer_ID);

  label = gimp_table_attach_aligned (GTK_TABLE (table), 0, row++,
				     _("Layer:"), 0.0, 0.5, combo, 1, FALSE);

  gtk_widget_set_sensitive (label, ui_state->rigmask_status
			    && features_are_sensitive);

  gtk_widget_set_sensitive (combo, ui_state->rigmask_status
			    && features_are_sensitive);

  gtk_widget_set_sensitive (rigmask_edit_button, ui_state->rigmask_status
			    && features_are_sensitive);

  rigmask_toggle_data.combo = combo;
  rigmask_toggle_data.combo_label = label;
  rigmask_toggle_data.edit_button = rigmask_edit_button;
  preview_data.rigmask_combo = combo;

  gtk_widget_show (combo);

  rigmask_toggle_data.status = &(ui_state->rigmask_status);

  rigmask_toggle_data.scale = NULL;
  rigmask_toggle_data.guess_label = NULL;
  rigmask_toggle_data.guess_button_hor = NULL;
  rigmask_toggle_data.guess_button_ver = NULL;

  g_signal_connect (G_OBJECT (rigmask_button), "toggled",
		    G_CALLBACK (callback_combo_set_sensitive),
		    (gpointer) (&rigmask_toggle_data));

  g_signal_connect (G_OBJECT (rigmask_button), "toggled",
		    G_CALLBACK (callback_rigmask_combo_set_sensitive_preview),
		    (gpointer) (&preview_data));

  /* Energy function */

  nrg_event_box = gtk_event_box_new ();
  gtk_box_pack_start (GTK_BOX (rigmask_vbox), nrg_event_box, FALSE, FALSE,
		      0);
  gtk_widget_show (nrg_event_box);

  gimp_help_set_help_data (nrg_event_box,
			   _
			   ("This affects the automatic feature recognition.\n"
			    "It's the filter which will be used to determine "
			    "the relevance of each pixel"),
			   NULL);

  hbox = gtk_hbox_new (FALSE, 4);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 4);
  gtk_container_add (GTK_CONTAINER (nrg_event_box), hbox);
  gtk_widget_show (hbox);

  label = gtk_label_new (_("Feature recog.:"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  nrg_func_combo_box =
    gimp_int_combo_box_new (_("Transversal grad. (bright.) "), LQR_EF_GRAD_XABS,
			    _("Grad. sum (bright.)"), LQR_EF_GRAD_SUMABS,
			    _("Grad. norm (bright.)"), LQR_EF_GRAD_NORM,
			    _("Transversal grad. (luma) "), LQR_EF_LUMA_GRAD_XABS,
                            _("Grad. sum (luma)"), LQR_EF_LUMA_GRAD_SUMABS,
                            _("Grad. norm (luma)"), LQR_EF_LUMA_GRAD_NORM,
			    /* Null can be translated as Zero */
			    _("Null"), LQR_EF_NULL, NULL);
  gimp_int_combo_box_set_active (GIMP_INT_COMBO_BOX (nrg_func_combo_box),
				 state->nrg_func);

  gtk_box_pack_start (GTK_BOX (hbox), nrg_func_combo_box, TRUE, TRUE, 0);
  gtk_widget_show (nrg_func_combo_box);


  /* Operations control */

  /* Please keep the <b> and </b> tags in translations */
  operations_expander = gtk_expander_new (_("<b>Operations control</b>"));
  gtk_expander_set_use_markup (GTK_EXPANDER(operations_expander), TRUE);
  gtk_expander_set_expanded (GTK_EXPANDER(operations_expander), TRUE);
  g_signal_connect (operations_expander, "activate",
		    G_CALLBACK
		    (callback_expander_changed),
		    (gpointer) (&ui_state->operations_expanded));

  gtk_box_pack_start (GTK_BOX (thispage), operations_expander, FALSE, FALSE, 0);
  gtk_widget_show (operations_expander);

  operations_vbox = gtk_vbox_new (FALSE, 4);
  gtk_container_add (GTK_CONTAINER(operations_expander), operations_vbox);
  gtk_widget_show (operations_vbox);

  /* Enlargement step */

  table = gtk_table_new (3, 1, FALSE);
  gtk_container_set_border_width (GTK_CONTAINER (table), 4);
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);
  gtk_table_set_row_spacings (GTK_TABLE (table), 2);
  gtk_box_pack_start (GTK_BOX (operations_vbox), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  row = 0;

  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, row++,
			      _("Max enlargement per step:"), SCALE_WIDTH,
			      SPIN_BUTTON_WIDTH, state->enl_step, 100.1,
			      200, 1, 10, 1, TRUE, 0, 0,
			      _("When enlarging beyond the value set here "
				"the rescaling will be performed in multiple steps."), NULL);

  g_signal_connect (adj, "value_changed",
		    G_CALLBACK (gimp_float_adjustment_update),
		    &state->enl_step);

  /* Resize order */

  res_order_event_box = gtk_event_box_new ();
  gtk_box_pack_start (GTK_BOX (operations_vbox), res_order_event_box, FALSE, FALSE,
		      0);
  gtk_widget_show (res_order_event_box);

  gimp_help_set_help_data (res_order_event_box,
			   _("This controls the order of operations "
			     "if rescaling in both directions"), NULL);

  hbox = gtk_hbox_new (FALSE, 4);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 4);
  gtk_container_add (GTK_CONTAINER (res_order_event_box), hbox);
  gtk_widget_show (hbox);

  label = gtk_label_new (_("Rescale order:"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  res_order_combo_box =
    gimp_int_combo_box_new (_("Horizontal first"), LQR_RES_ORDER_HOR,
			    _("Vertical first"), LQR_RES_ORDER_VERT, NULL);
  gimp_int_combo_box_connect (GIMP_INT_COMBO_BOX (res_order_combo_box),
			      state->res_order,
			      G_CALLBACK (callback_res_order_changed),
			      (gpointer) & preview_data);

  gtk_box_pack_start (GTK_BOX (hbox), res_order_combo_box, TRUE, TRUE, 0);
  gtk_widget_show (res_order_combo_box);

  /* No discard when enlarging ? */

  no_disc_on_enlarge_button =
    gtk_check_button_new_with_label (_("Ignore discard mask when enlarging"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (no_disc_on_enlarge_button),
				state->no_disc_on_enlarge);

  gimp_help_set_help_data (no_disc_on_enlarge_button,
			   _
			   ("This will have the same effect as setting the strenght "
			    "to 0 in the discard mask when the first rescale step is "
			    "an image enlargment (which normally is the best choice).\n"
			    "Note that this option is ignored in interactive mode"),
			   NULL);

  gtk_box_pack_start (GTK_BOX (operations_vbox), no_disc_on_enlarge_button, FALSE,
		      FALSE, 0);
  gtk_widget_show (no_disc_on_enlarge_button);

  g_signal_connect (no_disc_on_enlarge_button, "toggled",
		    G_CALLBACK
		    (callback_status_button),
		    (gpointer) (&state->no_disc_on_enlarge));

  g_signal_connect (no_disc_on_enlarge_button, "toggled",
		    G_CALLBACK
		    (callback_set_disc_warning), (gpointer) (&preview_data));

  callback_set_disc_warning (no_disc_on_enlarge_button,
			     (gpointer) & preview_data);

  return scrollwindow;
}
