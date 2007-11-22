/* GIMP LiquidRescaling Plug-in
 * Copyright (C) 2007 Carlo Baldassi (the "Author") <carlobaldassi@yahoo.it>.
 * (implementation based on the GIMP Plug-in Template by Michael Natterer)
 * All Rights Reserved.
 *
 * This plugin implements the algorithm described in the paper
 * "Seam Carving for Content-Aware Image Resizing"
 * by Shai Avidan and Ariel Shamir
 * which can be found at http://www.faculty.idc.ac.il/arik/imret.pdf
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 dated June, 1991.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 */

#include "config.h"

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include <math.h>
#include <string.h>

#include "plugin-intl.h"

#include "lqr.h"
#include "lqr_gradient.h"

#include "main.h"
#include "interface.h"



/*  Constants  */

#define SCALE_WIDTH         80
#define SPIN_BUTTON_WIDTH   75
#define MAX_COEFF	  3000
#define MAX_RIGIDITY      1000
#define MAX_DELTA_X         10
#define PREVIEW_MAX_WIDTH  300
#define PREVIEW_MAX_HEIGHT 200
#define MAX_STRING_SIZE   2048

/*  Local function prototypes  */

static gint count_extra_layers (gint32 image_ID, GimpDrawable * drawable);

static gboolean dialog_layer_constraint_func (gint32 image_id,
                                              gint32 layer_id, gpointer data);

static void callback_pres_combo_get_active (GtkWidget * combo, gpointer data);
static void callback_disc_combo_get_active (GtkWidget * combo, gpointer data);
static void callback_combo_set_sensitive (GtkWidget * button, gpointer data);
static void callback_status_button (GtkWidget * button, gpointer data);
static void callback_new_mask_button (GtkWidget * button, gpointer data);
static void callback_guess_button (GtkWidget * button, gpointer data);
static void callback_guess_direction (GtkWidget * combo, gpointer data);
static int guess_new_width (GtkWidget * button, gpointer data);
static int guess_new_height (GtkWidget * button, gpointer data);
static void callback_out_seams_button (GtkWidget * button, gpointer data);
static void refresh_features_page (NotebookData * data);
static void callback_pres_combo_set_sensitive_preview (GtkWidget * button,
                                                       gpointer data);
static void callback_disc_combo_set_sensitive_preview (GtkWidget * button,
                                                       gpointer data);
static void callback_resize_aux_layers_button_set_sensitive (GtkWidget *
                                                             button,
                                                             gpointer data);

static void callback_dialog_response (GtkWidget * dialog, gint response_id,
                                      gpointer data);

static void preview_init_mem (PreviewData * preview_data);
static guchar *preview_build_buffer (gint32 layer_ID);
#if 0
static guchar * preview_build_buffer_new (gint32 layer_ID);
#endif
static void preview_build_pixbuf (PreviewData * preview_data);

static gboolean
preview_expose_event_callback (GtkWidget * preview_area,
                               GdkEventExpose * event, gpointer data);

GtkWidget *features_page_new (gint32 image_ID, GimpDrawable * drawable);


/*  Local variables  */

gint dialog_response = GTK_RESPONSE_CANCEL;

gint context_calls = 0;

PlugInUIVals *ui_state;
PlugInVals *state;
NotebookData *notebook_data;
NewLayerData *new_pres_layer_data;
NewLayerData *new_disc_layer_data;
gboolean features_are_sensitive;
PreviewData preview_data;
ResponseData response_data;
PresDiscStatus presdisc_status;
ToggleData pres_toggle_data;
ToggleData disc_toggle_data;
gboolean pres_combo_awaked = FALSE;
gboolean disc_combo_awaked = FALSE;

GtkWidget *dlg;
GtkTooltips *dlg_tips;

/*  Public functions  */

gboolean
dialog (gint32 image_ID,
        GimpDrawable * drawable,
        PlugInVals * vals,
        PlugInImageVals * image_vals,
        PlugInDrawableVals * drawable_vals, PlugInUIVals * ui_vals,
        PlugInColVals * col_vals)
{

  gint32 layer_ID;
  GimpRGB saved_color;
  gint num_extra_layers;
  GtkWidget *gradient_event_box;
  GtkWidget *main_hbox;
  GtkWidget *vbox;
  GtkWidget *vbox2;
  GtkWidget *hbox;
  GtkWidget *frame;
  GtkWidget *notebook;
  gfloat wfactor, hfactor;
  GtkWidget *preview_area;
  GtkWidget *table;
  GtkWidget *coordinates;
  GtkWidget *mode_event_box;
  GtkWidget *oper_mode_combo_box;
  GtkWidget *features_page;
  GtkWidget *thispage;
  GtkWidget *label;
  GtkWidget *grad_func_combo_box;
  GtkWidget *res_order_event_box;
  GtkWidget *res_order_combo_box;
  GtkWidget *new_layer_button;
  GtkWidget *resize_canvas_button;
  GtkWidget *resize_aux_layers_button;
  GtkWidget *out_seams_hbox;
  GtkWidget *out_seams_button;
  GimpRGB *color_start;
  GimpRGB *color_end;
  GtkWidget *out_seams_col_button1;
  GtkWidget *out_seams_col_button2;
  GtkWidget *mask_behavior_combo_box = NULL;
  gboolean has_mask = FALSE;
  GtkObject *adj;
  gboolean run = FALSE;
  GimpUnit unit;
  gdouble xres, yres;


  gimp_ui_init (PLUGIN_NAME, TRUE);

  gimp_context_get_foreground (&saved_color);

  state = g_new (PlugInVals, 1);
  memcpy (state, vals, sizeof (PlugInVals));

  ui_state = g_new (PlugInUIVals, 1);
  memcpy (ui_state, ui_vals, sizeof (PlugInUIVals));

  notebook_data = g_new (NotebookData, 1);
  response_data.notebook_data = notebook_data;
  response_data.vals = vals;
  response_data.ui_vals = ui_vals;


  pres_toggle_data.ui_toggled = &(ui_state->pres_status);
  disc_toggle_data.ui_toggled = &(ui_state->disc_status);
  if (ui_state->pres_status == TRUE)
    {
      pres_combo_awaked = TRUE;
    }
  if (ui_state->disc_status == TRUE)
    {
      disc_combo_awaked = TRUE;
    }
  
  layer_ID = drawable->drawable_id;

  state->new_width = gimp_drawable_width (layer_ID);
  state->new_height = gimp_drawable_height (layer_ID);


  g_assert (gimp_drawable_is_layer (layer_ID) == TRUE);

  drawable_vals->layer_ID = layer_ID;
  preview_data.orig_layer_ID = layer_ID;

  if (gimp_layer_get_mask (layer_ID) != -1)
    {
      has_mask = TRUE;
    }

  num_extra_layers = count_extra_layers (image_ID, drawable);
  features_are_sensitive = (num_extra_layers > 0 ? TRUE : FALSE);
  if (!features_are_sensitive)
    {
      ui_state->pres_status = FALSE;
      ui_state->disc_status = FALSE;
      pres_combo_awaked = FALSE;
      disc_combo_awaked = FALSE;
    }


  dlg = gimp_dialog_new (_("GIMP LiquidRescale Plug-In"), PLUGIN_NAME,
                         NULL, 0,
                         gimp_standard_help_func, "plug-in-lqr",
                         GTK_STOCK_REFRESH, RESPONSE_REFRESH,
                         GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                         GTK_STOCK_OK, GTK_RESPONSE_OK, NULL);

  gtk_window_set_resizable (GTK_WINDOW (dlg), FALSE);

  g_signal_connect (dlg, "response", G_CALLBACK (callback_dialog_response),
                    (gpointer) (&response_data));

  dlg_tips = gtk_tooltips_new ();

  main_hbox = gtk_hbox_new (FALSE, 12);
  gtk_container_set_border_width (GTK_CONTAINER (main_hbox), 12);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dlg)->vbox), main_hbox);

  vbox = gtk_vbox_new (FALSE, 12);
  gtk_box_pack_start (GTK_BOX (main_hbox), vbox, TRUE, TRUE, 0);
  gtk_widget_show (vbox);

  frame = gimp_frame_new (_("Selected layer"));
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  /* Preview */

  preview_data.image_ID = image_ID;
  preview_data.vals = state;
  preview_data.ui_vals = ui_state;
  wfactor = (gfloat) gimp_drawable_width(layer_ID) / PREVIEW_MAX_WIDTH;
  hfactor = (gfloat) gimp_drawable_height(layer_ID) / PREVIEW_MAX_HEIGHT;
  preview_data.factor = MAX (wfactor, hfactor);
  preview_data.factor = MAX (preview_data.factor, 1);


  preview_data.old_width = gimp_drawable_width(layer_ID);
  preview_data.old_height = gimp_drawable_height(layer_ID);
  gimp_drawable_offsets (layer_ID, &(preview_data.x_off),
                         &(preview_data.y_off));
  preview_data.width =
    gimp_drawable_width (preview_data.orig_layer_ID) / preview_data.factor;
  preview_data.height =
    gimp_drawable_height (preview_data.orig_layer_ID) / preview_data.factor;

  
  gimp_image_undo_freeze (image_ID);
  preview_data.layer_ID = gimp_layer_copy (layer_ID);
  gimp_image_add_layer (image_ID, preview_data.layer_ID, 1);

  gimp_layer_scale (preview_data.layer_ID, preview_data.width,
                    preview_data.height, TRUE);
  gimp_layer_add_alpha (preview_data.layer_ID);
  preview_data.drawable = gimp_drawable_get (preview_data.layer_ID);
  
  preview_data.toggle = TRUE;

  preview_init_mem (&preview_data);
  g_free (preview_data.buffer);
  preview_data.buffer = preview_build_buffer (preview_data.layer_ID);
  //preview_data.buffer = preview_build_buffer_new (preview_data.orig_layer_ID);

  gimp_image_remove_layer (image_ID, preview_data.layer_ID);
  gimp_image_undo_thaw (image_ID);

  preview_build_pixbuf (&preview_data);

  preview_area = gtk_drawing_area_new ();
  preview_data.area = preview_area;
  gtk_drawing_area_size (GTK_DRAWING_AREA (preview_area), PREVIEW_MAX_WIDTH,
                         PREVIEW_MAX_HEIGHT);

  g_signal_connect (G_OBJECT (preview_area), "expose_event",
                    G_CALLBACK (preview_expose_event_callback),
                    (gpointer) (&preview_data));

  gtk_container_add (GTK_CONTAINER (frame), preview_area);

  gtk_widget_show (preview_area);


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
    gimp_coordinates_new (unit, "%p", TRUE, TRUE, SPIN_BUTTON_WIDTH,
                          GIMP_SIZE_ENTRY_UPDATE_SIZE, ui_state->chain_active,
                          TRUE, _("Width:"), state->new_width, xres, 2,
                          state->new_width * 2 - 1, 0, state->new_width,
                          _("Height:"), state->new_height, yres, 2,
                          state->new_height * 2 - 1, 0, state->new_height);

  gtk_box_pack_start (GTK_BOX (hbox), coordinates, FALSE, FALSE, 0);
  gtk_widget_show (coordinates);

  preview_data.coordinates = (gpointer) coordinates;

  /* Operational mode combo box */

  mode_event_box = gtk_event_box_new ();
  gtk_box_pack_start (GTK_BOX (vbox2), mode_event_box, FALSE, FALSE, 0);
  gtk_widget_show (mode_event_box);

  gimp_help_set_help_data (mode_event_box,
                           _("Choose mode of operation"),
                           NULL);

  hbox = gtk_hbox_new (FALSE, 4);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 4);
  gtk_container_add (GTK_CONTAINER (mode_event_box), hbox);
  gtk_widget_show (hbox);

  label = gtk_label_new (_("Mode:"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  oper_mode_combo_box =
    gimp_int_combo_box_new (_("LqR only"), LQR_MODE_NORMAL,
                            _("LqR + LqR back to the original size"), LQR_MODE_LQRBACK,
                            _("LqR + scale back to the original size"), LQR_MODE_SCALEBACK, NULL);
  gimp_int_combo_box_set_active (GIMP_INT_COMBO_BOX (oper_mode_combo_box),
                                 state->oper_mode);

  gtk_box_pack_start (GTK_BOX (hbox), oper_mode_combo_box, TRUE, TRUE, 0);
  gtk_widget_show (oper_mode_combo_box);

  /* Notebook */

  notebook = gtk_notebook_new ();
  gtk_box_pack_start (GTK_BOX (main_hbox), notebook, TRUE, TRUE, 5);
  gtk_widget_show (notebook);
  notebook_data->notebook = notebook;
  notebook_data->image_ID = image_ID;
  notebook_data->drawable = drawable;

  /* Fature masks page */

  features_page = features_page_new (image_ID, drawable);
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

  new_layer_button =
    gtk_check_button_new_with_label (_("Output on a new layer"));

  gtk_box_pack_start (GTK_BOX (vbox), new_layer_button, FALSE, FALSE, 0);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (new_layer_button),
                                state->new_layer);
  gtk_widget_show (new_layer_button);

  gimp_help_set_help_data (new_layer_button,
                           _("Outputs the resulting image "
                             "on a new layer"), NULL);

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
    gtk_check_button_new_with_label (_("Resize preserve/discard layers"));

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
                           _("Resize the layers used as masks "
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
                             "and resize in one direction at a time"), NULL);

  color_start = g_new (GimpRGB, 1);
  color_end = g_new (GimpRGB, 1);

  gimp_rgba_set (color_start, col_vals->r1, col_vals->g1, col_vals->b1, 1);
  gimp_rgba_set (color_end, col_vals->r2, col_vals->g2, col_vals->b2, 1);

  out_seams_col_button2 =
    gimp_color_button_new ("Seam-color-end", 14, 14, color_end,
                           GIMP_COLOR_AREA_FLAT);
  gtk_box_pack_end (GTK_BOX (out_seams_hbox), out_seams_col_button2, FALSE,
                    FALSE, 0);
  //gtk_widget_show (out_seams_col_button2);

  g_signal_connect(out_seams_button, "toggled", G_CALLBACK(callback_out_seams_button), (gpointer) (out_seams_col_button2));

  callback_out_seams_button(out_seams_button, (gpointer) out_seams_col_button2);

  gimp_help_set_help_data (out_seams_col_button2,
                           _("Color to use for the last seams"), NULL);

  out_seams_col_button1 =
    gimp_color_button_new ("Seam-color-start", 14, 14, color_start,
                           GIMP_COLOR_AREA_FLAT);
  gtk_box_pack_end (GTK_BOX (out_seams_hbox), out_seams_col_button1, FALSE,
                    FALSE, 0);
  gtk_widget_show (out_seams_col_button1);

  g_signal_connect(out_seams_button, "toggled", G_CALLBACK(callback_out_seams_button), (gpointer) (out_seams_col_button1));

  callback_out_seams_button(out_seams_button, (gpointer) out_seams_col_button1);

  gimp_help_set_help_data (out_seams_col_button1,
                           _("Color to use for the first seams"), NULL);


  /* Advanced settings page */

  label = gtk_label_new (_("Advanced"));

  thispage = gtk_vbox_new (FALSE, 12);
  gtk_container_set_border_width (GTK_CONTAINER (thispage), 12);
  gtk_notebook_append_page_menu (GTK_NOTEBOOK (notebook), thispage, label,
                                 NULL);
  gtk_widget_show (thispage);

  /* Rigidity */

  table = gtk_table_new (2, 2, FALSE);
  gtk_container_set_border_width (GTK_CONTAINER (table), 4);
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);
  gtk_table_set_row_spacings (GTK_TABLE (table), 2);
  gtk_box_pack_start (GTK_BOX (thispage), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, 0,
                              _("Seams rigidity:"), SCALE_WIDTH,
                              SPIN_BUTTON_WIDTH, state->rigidity, 0,
                              MAX_RIGIDITY, 1, 10, 0, TRUE, 0, 0,
                              _("Increasing this value results "
                                "in straighter seams"), NULL);
  g_signal_connect (adj, "value_changed",
                    G_CALLBACK (gimp_int_adjustment_update),
                    &state->rigidity);

  /* Delta x */

  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, 1,
                              _("Seams max step:"), SCALE_WIDTH,
                              SPIN_BUTTON_WIDTH, state->delta_x, 0,
                              MAX_DELTA_X, 1, 1, 0, TRUE, 0, 0,
                              _("Maximum displacement along a seam. "
				"Increasing this value allows to overcome "
				"the 45 degrees bound"), NULL);

  g_signal_connect (adj, "value_changed",
                    G_CALLBACK (gimp_int_adjustment_update),
                    &state->delta_x);

  /* Gradient */

  gradient_event_box = gtk_event_box_new ();
  gtk_box_pack_start (GTK_BOX (thispage), gradient_event_box, FALSE, FALSE,
                      0);
  gtk_widget_show (gradient_event_box);

  gimp_help_set_help_data (gradient_event_box,
                           _
                           ("This affects the automatic feature recognition.\n"
                            "It's the function which will be applied to "
                            "the components of the gradient on each pixel"),
                           NULL);

  hbox = gtk_hbox_new (FALSE, 4);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 4);
  gtk_container_add (GTK_CONTAINER (gradient_event_box), hbox);
  gtk_widget_show (hbox);

  label = gtk_label_new (_("Gradient function:"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  grad_func_combo_box =
    gimp_int_combo_box_new (_("Transversal absolute value"), LQR_GF_XABS,
                            _("Sum of absolute values"), LQR_GF_SUMABS,
                            _("Norm"), LQR_GF_NORM,
                            /* Null can be translated as Zero */
                            _("Null"), LQR_GF_NULL, NULL);
  gimp_int_combo_box_set_active (GIMP_INT_COMBO_BOX (grad_func_combo_box),
                                 state->grad_func);

  gtk_box_pack_start (GTK_BOX (hbox), grad_func_combo_box, TRUE, TRUE, 0);
  gtk_widget_show (grad_func_combo_box);

  /* Resize order */

  res_order_event_box = gtk_event_box_new ();
  gtk_box_pack_start (GTK_BOX (thispage), res_order_event_box, FALSE, FALSE,
                      0);
  gtk_widget_show (res_order_event_box);

  gimp_help_set_help_data (res_order_event_box,
                           _
                           ("This controls the order of operations "
                            "if rescaling in both directions"),
                           NULL);

  hbox = gtk_hbox_new (FALSE, 4);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 4);
  gtk_container_add (GTK_CONTAINER (res_order_event_box), hbox);
  gtk_widget_show (hbox);

  label = gtk_label_new (_("Rescale order:"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  res_order_combo_box =
    gimp_int_combo_box_new (_("Horizontal first"), LQR_RES_ORDER_HOR,
                            _("Vertical first"), LQR_RES_ORDER_VERT,
                            NULL);
  gimp_int_combo_box_set_active (GIMP_INT_COMBO_BOX (res_order_combo_box),
                                 state->res_order);

  gtk_box_pack_start (GTK_BOX (hbox), res_order_combo_box, TRUE, TRUE, 0);
  gtk_widget_show (res_order_combo_box);




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

      frame = gimp_frame_new (_("Select behavior for the mask"));
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

  run = (dialog_response == GTK_RESPONSE_OK);


  if (run)
    {
      /*  Save ui values  */
      ui_state->chain_active =
        gimp_chain_button_get_active (GIMP_COORDINATES_CHAINBUTTON
                                      (coordinates));
      gimp_int_combo_box_get_active (GIMP_INT_COMBO_BOX(disc_toggle_data.guess_dir_combo), &(ui_state->guess_direction));
      state->new_width =
        (gint) gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (coordinates), 0);
      state->new_height =
        (gint) gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (coordinates), 1);
      gimp_int_combo_box_get_active (GIMP_INT_COMBO_BOX (oper_mode_combo_box),
                                     &(state->oper_mode));
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

      /* save vsmap colors */
      if (state->output_seams)
        {
	  gimp_color_button_get_color (GIMP_COLOR_BUTTON (out_seams_col_button1),
			               color_start);
	  gimp_color_button_get_color (GIMP_COLOR_BUTTON (out_seams_col_button2),
				       color_end);

	  col_vals->r1 = color_start->r;
	  col_vals->g1 = color_start->g;
	  col_vals->b1 = color_start->b;

	  col_vals->r2 = color_end->r;
	  col_vals->g2 = color_end->g;
	  col_vals->b2 = color_end->b;
	}

      /* save mask behavior */
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

  if (context_calls > 0)
    {
      gimp_context_set_foreground (&saved_color);
    }

  gimp_drawable_detach(preview_data.drawable);

  return run;
}


/*  Private functions  */

static gint
count_extra_layers (gint32 image_ID, GimpDrawable * drawable)
{
  gint32 *layer_array;
  gint num_layers;

  layer_array = gimp_image_get_layers (image_ID, &num_layers);
  return num_layers - 1;
}

static gboolean
dialog_layer_constraint_func (gint32 image_id, gint32 layer_id, gpointer data)
{
  if (image_id !=
      gimp_drawable_get_image (((GimpDrawable *) data)->drawable_id))
    {
      return FALSE;
    }
  if (layer_id == ((GimpDrawable *) data)->drawable_id)
    {
      return FALSE;
    }
  return TRUE;
}

/* Callbacks */

static void
callback_dialog_response (GtkWidget * dialog, gint response_id, gpointer data)
{
  switch (response_id)
    {
    case RESPONSE_REFRESH:
      refresh_features_page (RESPONSE_DATA (data)->notebook_data);
      break;
    default:
      dialog_response = response_id;
      gtk_main_quit ();
      break;
    }
}


static void
callback_pres_combo_get_active (GtkWidget * combo, gpointer data)
{
  gint32 pres_layer_ID;
  gint x_off, y_off;
  gimp_int_combo_box_get_active (GIMP_INT_COMBO_BOX (combo),
                                 &(PREVIEW_DATA (data)->vals->pres_layer_ID));
  if (PREVIEW_DATA (data)->ui_vals->pres_status == TRUE)
    {
      gimp_image_undo_freeze (PREVIEW_DATA (data)->image_ID);
      pres_layer_ID =
        gimp_layer_copy (PREVIEW_DATA (data)->vals->pres_layer_ID);
      gimp_image_add_layer (PREVIEW_DATA (data)->image_ID, pres_layer_ID, -1);
      gimp_drawable_offsets (pres_layer_ID, &x_off, &y_off);
      gimp_layer_resize (pres_layer_ID, PREVIEW_DATA (data)->old_width,
                         PREVIEW_DATA (data)->old_height,
                         x_off - PREVIEW_DATA (data)->x_off,
                         y_off - PREVIEW_DATA (data)->y_off);
      gimp_layer_scale (pres_layer_ID, PREVIEW_DATA (data)->width,
                        PREVIEW_DATA (data)->height, TRUE);
      gimp_layer_add_alpha (pres_layer_ID);
      g_free (PREVIEW_DATA (data)->pres_buffer);
      PREVIEW_DATA (data)->pres_buffer = preview_build_buffer (pres_layer_ID);
      //PREVIEW_DATA (data)->pres_buffer = preview_build_buffer_new (PREVIEW_DATA(data)->vals->pres_layer_ID);
      gimp_image_remove_layer (PREVIEW_DATA (data)->image_ID, pres_layer_ID);
      gimp_image_undo_thaw (PREVIEW_DATA (data)->image_ID);

    }
  preview_build_pixbuf (PREVIEW_DATA (data));
  gtk_widget_queue_draw (PREVIEW_DATA (data)->area);
}

static void
callback_disc_combo_get_active (GtkWidget * combo, gpointer data)
{
  gint32 disc_layer_ID;
  gint x_off, y_off;
  gimp_int_combo_box_get_active (GIMP_INT_COMBO_BOX (combo),
                                 &(PREVIEW_DATA (data)->vals->disc_layer_ID));
  if (PREVIEW_DATA (data)->ui_vals->disc_status == TRUE)
    {
      gimp_image_undo_freeze (PREVIEW_DATA (data)->image_ID);
      disc_layer_ID =
        gimp_layer_copy (PREVIEW_DATA (data)->vals->disc_layer_ID);
      gimp_image_add_layer (PREVIEW_DATA (data)->image_ID, disc_layer_ID, -1);
      gimp_drawable_offsets (disc_layer_ID, &x_off, &y_off);
      gimp_layer_resize (disc_layer_ID, PREVIEW_DATA (data)->old_width,
                         PREVIEW_DATA (data)->old_height,
                         x_off - PREVIEW_DATA (data)->x_off,
                         y_off - PREVIEW_DATA (data)->y_off);
      gimp_layer_scale (disc_layer_ID, PREVIEW_DATA (data)->width,
                        PREVIEW_DATA (data)->height, TRUE);
      gimp_layer_add_alpha (disc_layer_ID);
      g_free (PREVIEW_DATA (data)->disc_buffer);
      PREVIEW_DATA (data)->disc_buffer = preview_build_buffer (disc_layer_ID);
      gimp_image_remove_layer (PREVIEW_DATA (data)->image_ID, disc_layer_ID);
      gimp_image_undo_thaw (PREVIEW_DATA (data)->image_ID);
    }
  preview_build_pixbuf (PREVIEW_DATA (data));
  gtk_widget_queue_draw (PREVIEW_DATA (data)->area);
}

static void
callback_combo_set_sensitive (GtkWidget * button, gpointer data)
{
  gboolean button_status =
    gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button));
  gtk_widget_set_sensitive ((GtkWidget *) (TOGGLE_DATA (data)->combo),
                            button_status);
  gtk_widget_set_sensitive ((GtkWidget *) (TOGGLE_DATA (data)->combo_label),
                            button_status);
  gtk_widget_set_sensitive (GIMP_SCALE_ENTRY_LABEL
                            (TOGGLE_DATA (data)->scale), button_status);
  gtk_widget_set_sensitive (GIMP_SCALE_ENTRY_SCALE
                            (TOGGLE_DATA (data)->scale), button_status);
  gtk_widget_set_sensitive (GIMP_SCALE_ENTRY_SPINBUTTON
                            (TOGGLE_DATA (data)->scale), button_status);
  if (TOGGLE_DATA(data)->guess_button)
    {
      gtk_widget_set_sensitive (TOGGLE_DATA (data)->guess_button, button_status);
      gtk_widget_set_sensitive (TOGGLE_DATA (data)->guess_dir_combo, button_status);
    }
  *((gboolean *) (TOGGLE_DATA (data)->status)) = button_status;
}

static void
callback_status_button (GtkWidget * button, gpointer data)
{
  gboolean button_status =
    gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button));
  *((gboolean *) (data)) = button_status;
}

static void
callback_new_mask_button (GtkWidget * button, gpointer data)
{
  gint32 layer_ID;
  gimp_image_undo_group_start (preview_data.image_ID);
  layer_ID =
    gimp_layer_new (preview_data.image_ID, NEW_LAYER_DATA (data)->name,
                    preview_data.old_width, preview_data.old_height,
                    GIMP_RGBA_IMAGE, 50, GIMP_NORMAL_MODE);
  gimp_image_add_layer (preview_data.image_ID, layer_ID, -1);
  gimp_drawable_fill (layer_ID, GIMP_TRANSPARENT_FILL);
  gimp_layer_translate (layer_ID, preview_data.x_off, preview_data.y_off);
  gimp_image_undo_group_end (preview_data.image_ID);
  *(NEW_LAYER_DATA (data)->layer_ID) = layer_ID;
  *(NEW_LAYER_DATA (data)->status) = TRUE;
  context_calls++;
  gimp_context_set_foreground (&(NEW_LAYER_DATA (data)->color));

  gtk_dialog_response (GTK_DIALOG (dlg), RESPONSE_REFRESH);
}

static void callback_guess_button (GtkWidget * button, gpointer data)
{
  gint new_width, new_height;
  new_width = PREVIEW_DATA(data)->old_width;
  new_height = PREVIEW_DATA(data)->old_height;
  switch (PREVIEW_DATA(data)->guess_direction)
    {
      case 0 : new_width = guess_new_width (button, data);
	       break;
      case 1 : new_height = guess_new_height (button, data);
	       break;
      default : g_message("You just found a bug");
    }

  gimp_size_entry_set_refval(GIMP_SIZE_ENTRY(PREVIEW_DATA(data)->coordinates),
     0, new_width);
  gimp_size_entry_set_refval(GIMP_SIZE_ENTRY(PREVIEW_DATA(data)->coordinates),
     1, new_height);
}

static void callback_guess_direction (GtkWidget * combo, gpointer data)
{
  gimp_int_combo_box_get_active(GIMP_INT_COMBO_BOX(combo), &(PREVIEW_DATA(data)->guess_direction));
  ui_state->guess_direction = PREVIEW_DATA(data)->guess_direction;
}

static gint guess_new_width (GtkWidget * button, gpointer data)
{
  gint32 disc_layer_ID;
  GimpDrawable *drawable;
  gint x, y, k;
  gint width, height;
  gint lw, lh;
  gint x_off, y_off;
  gint bpp, c_bpp;
  GimpPixelRgn rgn_in;
  guchar *inrow;
  gboolean has_alpha;
  gdouble sum;
  gint mask_size;
  gint max_mask_size = 0;
  gint new_width;

  disc_layer_ID = PREVIEW_DATA(data)->vals->disc_layer_ID;
  if (!gimp_drawable_is_valid(disc_layer_ID)) {
      g_message (_("Error: it seems that the selected layer "
                   "is no longer valid"));
      return PREVIEW_DATA(data)->old_width; /* Should refresh here */
  }

  width = gimp_drawable_width(disc_layer_ID);
  height = gimp_drawable_height(disc_layer_ID);
  has_alpha = gimp_drawable_has_alpha (disc_layer_ID);
  bpp = gimp_drawable_bpp (disc_layer_ID);
  c_bpp = bpp - (has_alpha ? 1 : 0);

  drawable = gimp_drawable_get(disc_layer_ID);
  gimp_pixel_rgn_init (&rgn_in, drawable, 0, 0, width, height, FALSE, FALSE);


  gimp_drawable_offsets (disc_layer_ID, &x_off, &y_off);

  x_off -= PREVIEW_DATA(data)->x_off;
  y_off -= PREVIEW_DATA(data)->y_off;

  lw = (MIN (PREVIEW_DATA(data)->old_width, width + x_off) - MAX (0, x_off));
  lh = (MIN (PREVIEW_DATA(data)->old_height, height + y_off) - MAX (0, y_off));

  inrow = g_try_new (guchar, bpp * lw);

  for (y = MAX (0, y_off); y < MIN (PREVIEW_DATA(data)->old_height, height + y_off); y++)
    {
      gimp_pixel_rgn_get_row (&rgn_in, inrow, MAX (0, -x_off),
                              y - y_off, lw);

      mask_size = 0;
      for (x = 0; x < lw; x++)
        {
          sum = 0;
          for (k = 0; k < c_bpp; k++)
            {
              sum += inrow[bpp * x + k];
            }

          sum /= (255 * c_bpp);
          if (has_alpha)
            {
              sum *= (gdouble) inrow[bpp * (x + 1) - 1] / 255;
            }

	  if (sum >= (0.5 / c_bpp))
	    {
	      mask_size++;
	    }
        }
      if (mask_size > max_mask_size)
        {
	  max_mask_size = mask_size;
	}

    }

  new_width = PREVIEW_DATA(data)->old_width - max_mask_size;

  g_free (inrow);
  gimp_drawable_detach(drawable);

  return new_width;

}

static gint guess_new_height (GtkWidget * button, gpointer data)
{
  gint32 disc_layer_ID;
  GimpDrawable *drawable;
  gint x, y, k;
  gint width, height;
  gint lw, lh;
  gint x_off, y_off;
  gint bpp, c_bpp;
  GimpPixelRgn rgn_in;
  guchar *incol;
  gboolean has_alpha;
  gdouble sum;
  gint mask_size;
  gint max_mask_size = 0;
  gint new_height;

  disc_layer_ID = PREVIEW_DATA(data)->vals->disc_layer_ID;
  if (!gimp_drawable_is_valid(disc_layer_ID)) {
      g_message (_("Error: it seems that the selected layer "
                   "is no longer valid"));
      return PREVIEW_DATA(data)->old_height; /* Should refresh here */
  }

  width = gimp_drawable_width(disc_layer_ID);
  height = gimp_drawable_height(disc_layer_ID);
  has_alpha = gimp_drawable_has_alpha (disc_layer_ID);
  bpp = gimp_drawable_bpp (disc_layer_ID);
  c_bpp = bpp - (has_alpha ? 1 : 0);

  drawable = gimp_drawable_get(disc_layer_ID);
  gimp_pixel_rgn_init (&rgn_in, drawable, 0, 0, width, height, FALSE, FALSE);


  gimp_drawable_offsets (disc_layer_ID, &x_off, &y_off);

  x_off -= PREVIEW_DATA(data)->x_off;
  y_off -= PREVIEW_DATA(data)->y_off;

  lw = (MIN (PREVIEW_DATA(data)->old_width, width + x_off) - MAX (0, x_off));
  lh = (MIN (PREVIEW_DATA(data)->old_height, height + y_off) - MAX (0, y_off));

  incol = g_try_new (guchar, bpp * lh);


  for (x = MAX (0, x_off); x < MIN (PREVIEW_DATA(data)->old_width, width + x_off); x++)
    {
      gimp_pixel_rgn_get_col (&rgn_in, incol, x - x_off, MAX (0, -y_off),
                              lh);

      mask_size = 0;
      for (y = 0; y < lh; y++)
        {
          sum = 0;
          for (k = 0; k < c_bpp; k++)
            {
              sum += incol[bpp * y + k];
            }

          sum /= (255 * c_bpp);
          if (has_alpha)
            {
              sum *= (gdouble) incol[bpp * (y + 1) - 1] / 255;
            }

	  if (sum >= (0.5 / c_bpp))
	    {
	      mask_size++;
	    }
        }
      if (mask_size > max_mask_size)
        {
	  max_mask_size = mask_size;
	}

    }

  new_height = PREVIEW_DATA(data)->old_height - max_mask_size;

  g_free (incol);
  gimp_drawable_detach(drawable);

  return new_height;
}

static void
callback_out_seams_button (GtkWidget * button, gpointer data)
{
  gboolean button_status =
    gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button));
  if (button_status) {
	  gtk_widget_show(GTK_WIDGET(data));
  } else {
	  gtk_widget_hide(GTK_WIDGET(data));
  }
}



static void
callback_pres_combo_set_sensitive_preview (GtkWidget * button, gpointer data)
{
  if (pres_combo_awaked == FALSE)
    {
      g_signal_emit_by_name (G_OBJECT (PREVIEW_DATA (data)->pres_combo),
                             "changed");
      pres_combo_awaked = TRUE;
    }
  preview_build_pixbuf (PREVIEW_DATA (data));
  gtk_widget_queue_draw (PREVIEW_DATA (data)->area);
}

static void
callback_disc_combo_set_sensitive_preview (GtkWidget * button, gpointer data)
{
  if (disc_combo_awaked == FALSE)
    {
      g_signal_emit_by_name (G_OBJECT (PREVIEW_DATA (data)->disc_combo),
                             "changed");
      disc_combo_awaked = TRUE;
    }
  preview_build_pixbuf (PREVIEW_DATA (data));
  gtk_widget_queue_draw (PREVIEW_DATA (data)->area);
}

static void
callback_resize_aux_layers_button_set_sensitive (GtkWidget * button,
                                                 gpointer data)
{
  if ((PLUGIN_UI_VALS (PRESDISC_STATUS (data)->ui_vals)->pres_status == TRUE)
      || (PLUGIN_UI_VALS (PRESDISC_STATUS (data)->ui_vals)->disc_status ==
          TRUE))
    {
      gtk_widget_set_sensitive ((GtkWidget *) (PRESDISC_STATUS (data)->
                                               button), TRUE);
    }
  else
    {
      gtk_widget_set_sensitive ((GtkWidget *) (PRESDISC_STATUS (data)->
                                               button), FALSE);
    }
}

/* Refresh */

static void
refresh_features_page (NotebookData * data)
{
  GtkWidget *new_page;

  gtk_notebook_remove_page (GTK_NOTEBOOK (data->notebook),
                            data->features_page_ID);
  new_page = features_page_new (data->image_ID, data->drawable);
  gtk_widget_show (new_page);
  data->features_page_ID =
    gtk_notebook_prepend_page_menu (GTK_NOTEBOOK (data->notebook), new_page,
                                    data->label, NULL);
  data->features_page = new_page;
  gtk_notebook_set_current_page (GTK_NOTEBOOK (data->notebook),
                                 data->features_page_ID);
  callback_resize_aux_layers_button_set_sensitive (NULL,
                                                   (gpointer)
                                                   (&presdisc_status));
}

/* Preview construction */

static void
preview_init_mem (PreviewData * preview_data)
{
  preview_data->buffer = NULL;
  preview_data->pres_buffer = NULL;
  preview_data->disc_buffer = NULL;
  preview_data->pixbuf = NULL;
}


#if 0
static guchar *
preview_build_buffer_new (gint32 layer_ID)
{

  gint x, y, k;
  gint width, height, bpp;
  gint x1, y1;
  gint z0, z1;
  gint x_off, y_off;
  gint lw, lh;
  gint sq_size;
  gint sum[4];
  GimpPixelRgn rgn_in;
  guchar *inrect, *aux;
  GimpDrawable *drawable;
  guchar *buffer;

  printf("build buffer\n"); fflush(stdout);

  drawable = gimp_drawable_get (layer_ID);

  gimp_drawable_offsets (layer_ID, &x_off, &y_off);

  width = gimp_drawable_width(layer_ID);
  height = gimp_drawable_height(layer_ID);
  bpp = gimp_drawable_bpp(layer_ID);
  //isgray = gimp_drawable_is_gray (layer_ID);

  gimp_pixel_rgn_init (&rgn_in,
                       drawable, 0, 0, width, height,
                       FALSE, FALSE);

  x_off -= preview_data.x_off;
  y_off -= preview_data.y_off;

  lw = (MIN (preview_data.old_width, width + x_off) - MAX (0, x_off));
  lh = (MIN (preview_data.old_height, height + y_off) - MAX (0, y_off));

  sq_size = (gint) (1.0 * preview_data.factor);

  inrect = g_new0 (guchar, bpp * lw * sq_size);

  buffer = g_new0 (guchar, 4 * preview_data.width * preview_data.height);

  printf("w,h=%i,%i xo,yo=%i,%i lw,lh=%i,%i ss=%i pdw,pdh=%i,%i pdxo,pdyo=%i,%i\n", width, height, x_off, y_off, lw, lh, sq_size, preview_data.width, preview_data.height, preview_data.x_off, preview_data.y_off); fflush(stdout);

  for (y = 0; y < preview_data.old_height; y++)
  {
	  aux = inrect;
	  if (y < y_off / sq_size) {
		  continue;
		  printf("  0 : y=%i cont\n", y); fflush(stdout);
	  } else if (y == y_off / sq_size) {
		  if (y < (((height + y_off) / sq_size) - 1)) {
			  printf(" 1,0 : y=%i\n", y); fflush(stdout);
			  aux = inrect + (y_off % sq_size) * lw;
			  gimp_pixel_rgn_get_rect(&rgn_in, aux, MAX(0, -x_off),
					 y_off, lw, sq_size - y_off % sq_size);
		  } else if (y == (((height + y_off) / sq_size) - 1)) {
			  printf(" 1,1 : y=%i\n", y); fflush(stdout);
			  aux = inrect + (y_off % sq_size) * lw;
			  gimp_pixel_rgn_get_rect(&rgn_in, aux, MAX(0, -x_off),
					 y_off, lw, height);
		  } else {
			  printf(" 1,x : y=%i\n", y); fflush(stdout);
			  g_assert(0);
		  }
	  } else if (y < (((height + y_off) / sq_size) - 1)) {
		  printf(" 2 : y=%i\n", y); fflush(stdout);
		  gimp_pixel_rgn_get_rect(&rgn_in, aux, MAX(0, -x_off),
				 y * sq_size, lw, sq_size);
	  } else if (y == (((height + y_off) / sq_size) - 1)) {
		  printf(" 3 : y=%i\n", y); fflush(stdout);
		  gimp_pixel_rgn_get_rect(&rgn_in, aux, MAX(0, -x_off),
				 y * sq_size, lw, height + y_off - y * sq_size);
	  } else {
		  printf(" 4 : y=%i cont\n", y); fflush(stdout);
		  continue;
	  }
	  for (x = 0; x < preview_data.old_width; x++)
	  {
		  z0 = y * preview_data.width + x;
		  for (k = 0; k < bpp; k++) {
			  sum[k] = 0;
		  }
		  for (y1 = 0; y1 < sq_size; y1++) {
			  for (x1 = 0; x1 < sq_size; x1++) {
				  z1 = (y1 * lw) + x * sq_size + x1 - MAX(0,-x_off);
				  if (x * sq_size + x1 < x_off) {
					  /* */
				  } else if (x * sq_size + x1 < width + x_off - 1) {
					  if (y * sq_size + y1 < y_off) {
						  /* */
					  } else if (y * sq_size + y1 < height + y_off - 1) {
						  for (k = 0; k < bpp; k++) {
							  sum[k] += inrect[z1 * bpp + k];
						  }
					  } else {
						  /* */
					  }
				  } else {
					  /* */
				  }
			  }
		  }
		  for (k = 0; k < bpp; k++) {
			  buffer[z0 * 4 + k] = sum[k] / (sq_size * sq_size);
		  }
	  }
  }
  if (bpp < 4) {
	  /* TODO */
	  g_assert(bpp == 4);
  }

  g_free(inrect);
  gimp_drawable_detach(drawable);
  printf("built\n"); fflush(stdout);

  return buffer;
}
#endif //0

static guchar *
preview_build_buffer (gint32 layer_ID)
{

  gint x, y, k;
  gint width, height;
  GimpPixelRgn rgn_in;
  guchar *inrow;
  GimpDrawable *drawable;
  gboolean isgray;
  guchar *buffer;

  drawable = gimp_drawable_get (layer_ID);
  width = gimp_drawable_width(layer_ID);
  height = gimp_drawable_height(layer_ID);
  isgray = gimp_drawable_is_gray (layer_ID);

  gimp_pixel_rgn_init (&rgn_in, drawable, 0, 0, width, height, FALSE, FALSE);

  inrow = g_new (guchar, drawable->bpp * width);
  buffer = g_new (guchar, 4 * width * height);

  for (y = 0; y < height; y++)
    {
      gimp_pixel_rgn_get_row (&rgn_in, inrow, 0, y, width);

      for (x = 0; x < width; x++)
        {
          for (k = 0; k < 3; k++)
            {
              if (isgray)
                {
                  buffer[(y * width + x) * 4 + k] = inrow[2 * x];
                }
              else
                {
                  buffer[(y * width + x) * 4 + k] = inrow[4 * x + k];
                }
            }
          if (isgray)
            {
              buffer[(y * width + x) * 4 + 3] = inrow[2 * x + 1];
            }
          else
            {
              buffer[(y * width + x) * 4 + 3] = inrow[4 * x + 3];
            }
        }

    }

  g_free (inrow);
  gimp_drawable_detach(drawable);
  return buffer;
}


static void
preview_free_pixbuf (guchar * buffer, gpointer data)
{
  g_free (buffer);
}


static void
preview_build_pixbuf (PreviewData * preview_data)
{
  gint bpp;
  gint x, y, k;
  gint index, index1;
  gdouble tfactor_orig, tfactor_pres, tfactor_disc, tfactor;
  gdouble value;

  if (preview_data->pixbuf != NULL)
    {
      g_object_unref (G_OBJECT (preview_data->pixbuf));
    }

  preview_data->preview_buffer =
    g_new (guchar, preview_data->width * preview_data->height * 4);

  bpp = 4;

  for (y = 0; y < preview_data->height; y++)
    {
      for (x = 0; x < preview_data->width; x++)
        {
          index1 = (y * preview_data->width + x);
          tfactor_orig = 0;
          tfactor_pres = 1;
          tfactor_disc = 1;
          tfactor_orig =
            (gdouble) (255 -
                       preview_data->buffer[index1 * bpp + bpp - 1]) / 255;
          if ((preview_data->pres_buffer != NULL)
              && (preview_data->ui_vals->pres_status == TRUE))
            {
              tfactor_pres =
                (gdouble) (255 -
                           preview_data->pres_buffer[index1 * bpp + bpp -
                                                     1] / 2) / 255;
            }
          if ((preview_data->disc_buffer != NULL)
              && (preview_data->ui_vals->disc_status == TRUE))
            {
              tfactor_disc =
                (gdouble) (255 -
                           0.5 * preview_data->disc_buffer[(index1 + 1) *
                                                           bpp - 1]) / 255;
            }
          tfactor = (1 - tfactor_orig) * tfactor_pres * tfactor_disc;
          for (k = 0; k < bpp; k++)
            {
              index = index1 * bpp + k;
              value = (tfactor * preview_data->buffer[index]);
              if (tfactor_pres < 1)
                {
                  value +=
                    (guchar) (tfactor_disc * (1 - tfactor_pres) *
                              preview_data->pres_buffer[index]);
                }
              if (tfactor_disc < 1)
                {
                  value +=
                    (guchar) ((1 -
                               tfactor_disc) *
                              preview_data->disc_buffer[index]);
                }
              if (value > 255)
                {
                  value = 255;
                }
              preview_data->preview_buffer[index] = (guchar) value;
            }
        }
    }
  preview_data->pixbuf =
    gdk_pixbuf_new_from_data (preview_data->preview_buffer,
                              GDK_COLORSPACE_RGB, TRUE, 8,
                              preview_data->width, preview_data->height,
                              bpp * preview_data->width * sizeof (guchar),
                              preview_free_pixbuf, NULL);

}

static gboolean
preview_expose_event_callback (GtkWidget * preview_area,
                               GdkEventExpose * event, gpointer data)
{

  gdk_draw_pixbuf (PREVIEW_DATA (data)->area->window, NULL,
                   PREVIEW_DATA (data)->pixbuf, 0, 0,
                   (PREVIEW_MAX_WIDTH - PREVIEW_DATA (data)->width) / 2,
                   (PREVIEW_MAX_HEIGHT - PREVIEW_DATA (data)->height) / 2,
                   -1, -1, GDK_RGB_DITHER_NORMAL, 0, 0);

  return TRUE;
}

/* Generate features page */

GtkWidget *
features_page_new (gint32 image_ID, GimpDrawable * drawable)
{
  gint32 layer_ID;
  gint num_extra_layers;
  GtkWidget *label;
  GtkWidget *thispage;
  gchar pres_inactive_tip_string[MAX_STRING_SIZE];
  gchar disc_inactive_tip_string[MAX_STRING_SIZE];
  GtkWidget *pres_frame_event_box1;
  GtkWidget *pres_frame_event_box2;
  GtkWidget *disc_frame_event_box1;
  GtkWidget *disc_frame_event_box2;
  GtkWidget *pres_combo_event_box;
  GtkWidget *disc_combo_event_box;
  GtkTooltips *pres_frame_tips;
  GtkTooltips *disc_frame_tips;
  gint32 old_layer_ID;
  GtkWidget *frame;
  GtkWidget *pres_vbox;
  GtkWidget *pres_vbox2;
  GtkWidget *hbox;
  GtkWidget *pres_button;
  GtkWidget *pres_new_button;
  GtkWidget *disc_vbox;
  GtkWidget *disc_vbox2;
  GtkWidget *disc_button;
  GtkWidget *disc_new_button;
  GtkWidget *guess_button;
  GtkWidget *guess_dir_combo;
  GtkWidget *table;
  gint row;
  GtkWidget *combo;
  GtkObject *adj;

  layer_ID = drawable->drawable_id;

  label = gtk_label_new (_("Feature masks"));
  notebook_data->label = label;

  new_pres_layer_data = g_new (NewLayerData, 1);
  new_disc_layer_data = g_new (NewLayerData, 1);

  new_pres_layer_data->layer_ID = &(state->pres_layer_ID);
  new_pres_layer_data->status = &(ui_state->pres_status);
  /* The name of a newly created layer for preservation */
  /* (here "%s" represents the selected layer's name) */
  snprintf (new_pres_layer_data->name, LQR_MAX_NAME_LENGTH, _("%s pres mask"),
            gimp_drawable_get_name (preview_data.orig_layer_ID));
  //gimp_rgb_set (&(new_pres_layer_data->color), 1, 1, 0);
  gimp_rgb_set (&(new_pres_layer_data->color), 0, 1, 0);

  new_disc_layer_data->layer_ID = &(state->disc_layer_ID);
  new_disc_layer_data->status = &(ui_state->disc_status);
  /* The name of a newly created layer for discard */
  /* (here "%s" represents the selected layer's name) */
  snprintf (new_disc_layer_data->name, LQR_MAX_NAME_LENGTH, _("%s disc mask"),
            gimp_drawable_get_name (preview_data.orig_layer_ID));
  //gimp_rgb_set (&(new_disc_layer_data->color), 0.5, 0.5, 1);
  gimp_rgb_set (&(new_disc_layer_data->color), 1, 0, 0);

  num_extra_layers = count_extra_layers (image_ID, drawable);
  features_are_sensitive = (num_extra_layers > 0 ? TRUE : FALSE);
  if (!features_are_sensitive)
    {
      ui_state->pres_status = FALSE;
      ui_state->disc_status = FALSE;
      pres_combo_awaked = FALSE;
      disc_combo_awaked = FALSE;
    }

  thispage = gtk_vbox_new (FALSE, 12);
  gtk_container_set_border_width (GTK_CONTAINER (thispage), 12);
  notebook_data->features_page = thispage;


  /*  Feature preservation  */

  frame = gimp_frame_new (_("Feature preservation mask"));
  gtk_box_pack_start (GTK_BOX (thispage), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  snprintf (pres_inactive_tip_string, MAX_STRING_SIZE,
            _("Extra layers are needed to activate feature preservation.\n"
              "You can create one with the \"New\" button and paint on it, "
              "then press the \"Refresh\" button.\n"
              "Note that painting in black has no effect"));

  pres_frame_tips = gtk_tooltips_new ();


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
      gtk_tooltips_set_tip (GTK_TOOLTIPS (pres_frame_tips),
                            pres_frame_event_box1,
                            pres_inactive_tip_string, NULL);
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

  pres_new_button = gtk_button_new_with_label (_("New"));
  gtk_box_pack_end (GTK_BOX (hbox), pres_new_button, FALSE, FALSE, 0);
  gtk_widget_show (pres_new_button);

  gimp_help_set_help_data (pres_new_button,
                           _("Creates a new transparent layer "
                             "ready to be used as a preservation mask"),
                           NULL);

  g_signal_connect (pres_new_button, "clicked",
                    G_CALLBACK
                    (callback_new_mask_button),
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
      gtk_tooltips_set_tip (GTK_TOOLTIPS (pres_frame_tips),
                            pres_frame_event_box2,
                            pres_inactive_tip_string, NULL);
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
                              (gpointer *) drawable);

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
  pres_toggle_data.combo = (gpointer) combo;
  pres_toggle_data.combo_label = (gpointer) label;
  preview_data.pres_combo = combo;

  gtk_widget_show (combo);

  table = gtk_table_new (1, 2, FALSE);
  gtk_container_set_border_width (GTK_CONTAINER (table), 4);
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);
  gtk_table_set_row_spacings (GTK_TABLE (table), 2);
  gtk_box_pack_start (GTK_BOX (pres_vbox2), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, row++,
                              _("Strength:"), SCALE_WIDTH, SPIN_BUTTON_WIDTH,
                              state->pres_coeff, 0, MAX_COEFF, 1, 10, 0,
                              TRUE, 0, 0,
                              _
                              ("Overall coefficient for feature preservation intensity"),
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
  pres_toggle_data.scale = (gpointer) adj;

  pres_toggle_data.status = &(ui_state->pres_status);

  g_signal_connect (pres_button, "toggled",
                    G_CALLBACK (callback_combo_set_sensitive),
                    (gpointer) (&pres_toggle_data));

  g_signal_connect (G_OBJECT (pres_button), "toggled",
                    G_CALLBACK (callback_pres_combo_set_sensitive_preview),
                    (gpointer) (&preview_data));

  pres_toggle_data.guess_button = NULL;
  pres_toggle_data.guess_dir_combo = NULL;


  /*  Feature discard  */

  frame = gimp_frame_new (_("Feature discard mask"));
  gtk_box_pack_start (GTK_BOX (thispage), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  snprintf (disc_inactive_tip_string, MAX_STRING_SIZE,
            _("Extra layers are needed to activate feature discard.\n"
              "You can create one with the \"New\" button and paint on it, "
              "then press the \"Refresh\" button.\n"
              "Note that painting in black has no effect"));

  disc_frame_tips = gtk_tooltips_new ();


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
      gtk_tooltips_set_tip (GTK_TOOLTIPS (disc_frame_tips),
                            disc_frame_event_box1,
                            disc_inactive_tip_string, NULL);
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

  disc_new_button = gtk_button_new_with_label (_("New"));
  gtk_box_pack_end (GTK_BOX (hbox), disc_new_button, FALSE, FALSE, 0);
  gtk_widget_show (disc_new_button);

  gimp_help_set_help_data (disc_new_button,
                           _("Creates a new transparent layer "
                             "ready to be used as a discard mask"), NULL);

  g_signal_connect (disc_new_button, "clicked",
                    G_CALLBACK
                    (callback_new_mask_button),
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
      gtk_tooltips_set_tip (GTK_TOOLTIPS (disc_frame_tips),
                            disc_frame_event_box2,
                            disc_inactive_tip_string, NULL);
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
                              (gpointer *) drawable);

  g_object_set (combo, "ellipsize", PANGO_ELLIPSIZE_START, NULL);

  old_layer_ID = state->disc_layer_ID;

  gimp_int_combo_box_connect (GIMP_INT_COMBO_BOX (combo),
                              layer_ID,
                              G_CALLBACK (callback_disc_combo_get_active),
                              (gpointer) (&preview_data));

  /* if (gimp_drawable_is_valid(old_layer_ID)) { */
  gimp_int_combo_box_set_active (GIMP_INT_COMBO_BOX (combo), old_layer_ID);
  /* } */

  gtk_widget_set_sensitive (combo, ui_state->disc_status
                            && features_are_sensitive);
  label = gimp_table_attach_aligned (GTK_TABLE (table), 0, row++,
                                     _("Layer:"), 0.0, 0.5, combo, 1, FALSE);

  disc_toggle_data.combo = (gpointer) combo;
  disc_toggle_data.combo_label = (gpointer) label;
  preview_data.disc_combo = combo;


  gtk_widget_set_sensitive (label, ui_state->disc_status
                            && features_are_sensitive);

  table = gtk_table_new (1, 2, FALSE);
  gtk_container_set_border_width (GTK_CONTAINER (table), 4);
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);
  gtk_table_set_row_spacings (GTK_TABLE (table), 2);
  gtk_box_pack_start (GTK_BOX (disc_vbox2), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, 0,
                              _("Strength:"), SCALE_WIDTH, SPIN_BUTTON_WIDTH,
                              state->disc_coeff, 0, MAX_COEFF, 1, 10, 0,
                              TRUE, 0, 0,
                              _
                              ("Overall coefficient for feature discard intensity"),
                              NULL);
  g_signal_connect (adj, "value_changed",
                    G_CALLBACK (gimp_int_adjustment_update),
                    &state->disc_coeff);


  gtk_widget_set_sensitive (GIMP_SCALE_ENTRY_LABEL (adj),
                            (ui_state->disc_status
                             && features_are_sensitive));
  gtk_widget_set_sensitive (GIMP_SCALE_ENTRY_SCALE (adj),
                            (ui_state->disc_status
                             && features_are_sensitive));
  gtk_widget_set_sensitive (GIMP_SCALE_ENTRY_SPINBUTTON (adj),
                            (ui_state->disc_status
                             && features_are_sensitive));

  disc_toggle_data.scale = (gpointer) adj;

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

  hbox = gtk_hbox_new (FALSE, 4);
  gtk_box_pack_start (GTK_BOX (disc_vbox2), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  guess_button = gtk_button_new_with_label(_("Auto size"));
  gtk_box_pack_start (GTK_BOX (hbox), guess_button, FALSE, FALSE, 0);
  gtk_widget_show(guess_button);

  disc_toggle_data.guess_button = (gpointer) guess_button;
  
  gtk_widget_set_sensitive (guess_button,
                            (ui_state->disc_status
                             && features_are_sensitive));

  gimp_help_set_help_data (guess_button, _("Try to set the final size as needed to remove the masked areas.\n"
			                   "Only use with simple masks"), NULL);

  guess_dir_combo = gimp_int_combo_box_new (_("horizontal"), 0, _("vertical"), 1, NULL);
  gtk_box_pack_end (GTK_BOX (hbox), guess_dir_combo, TRUE, TRUE, 0);
  gtk_widget_show(guess_dir_combo);

  gimp_int_combo_box_set_active(GIMP_INT_COMBO_BOX(guess_dir_combo), ui_state->guess_direction);

  disc_toggle_data.guess_dir_combo = (gpointer) guess_dir_combo;
  preview_data.guess_direction = ui_state->guess_direction;

  gimp_help_set_help_data (guess_dir_combo, _("Resizing direction for auto size"), NULL);

  gtk_widget_set_sensitive (guess_dir_combo, (ui_state->disc_status && features_are_sensitive));

  g_signal_connect (guess_dir_combo, "changed",
      G_CALLBACK(callback_guess_direction), (gpointer) &preview_data);

  g_signal_connect (guess_button, "clicked",
      G_CALLBACK(callback_guess_button), (gpointer) &preview_data);




  return thispage;


}

