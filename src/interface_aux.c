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

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include <math.h>
#include <string.h>

#include <lqr.h>

#include "plugin-intl.h"
#include "main_common.h"
#include "main.h"
#include "interface_aux.h"
#include "preview.h"
#include "layers_combo.h"

extern GimpRGB default_pres_col;
extern GimpRGB default_disc_col;
extern GimpRGB default_rigmask_col;
extern GimpRGB default_gray_col;

/***  Local functions declariations  ***/

/* Callbacks */
static void callback_dialog_aux_response (GtkWidget * dialog, gint response_id,
				      gpointer data);
//static void callback_noninter_button (GtkWidget * button, gpointer data);

/***  Local variables  ***/

gint dialog_aux_response = GTK_RESPONSE_OK;

PlugInUIVals *ui_state;
PlugInVals *state;
PlugInDialogVals *dialog_state;

GtkWidget *dlg;

/***  Public functions  ***/

gint
dialog_aux (PlugInImageVals * image_vals,
	PlugInDrawableVals * drawable_vals,
	PlugInVals * vals,
	PlugInUIVals * ui_vals,
        PlugInColVals * col_vals, PlugInDialogVals * dialog_vals)
{
  gint32 image_ID;
  gint32 layer_ID;
  GimpRGB fg_colour;
  GimpRGB saved_colour;
  GtkWidget *main_hbox;
  GtkWidget *info_icon;
  GtkWidget *info_label;
  InterfaceAuxData *ia_data;

  image_ID = image_vals->image_ID;
  layer_ID = drawable_vals->layer_ID;

  state = g_new (PlugInVals, 1);
  memcpy (state, vals, sizeof (PlugInVals));

  ui_state = g_new (PlugInUIVals, 1);
  memcpy (ui_state, ui_vals, sizeof (PlugInUIVals));

  dialog_state = dialog_vals;

  g_assert (gimp_drawable_is_layer (layer_ID) == TRUE);
  g_assert (gimp_drawable_is_layer (ui_state->layer_on_edit_ID) == TRUE);

  ia_data = g_new(InterfaceAuxData, 1);
  ia_data->image_ID = image_ID;
  ia_data->layer_ID = layer_ID;

  gimp_image_undo_group_start (image_ID);
  gimp_image_set_active_layer(image_ID, ui_state->layer_on_edit_ID);
  gimp_layer_set_opacity(ui_state->layer_on_edit_ID, 50);
  gimp_image_undo_group_end (image_ID);

  gimp_displays_flush();

  fg_colour = *(colour_from_type(image_ID, ui_state->layer_on_edit_type));
  gimp_context_get_foreground (&saved_colour);
  gimp_context_set_foreground (&fg_colour);

  dlg = gtk_dialog_new_with_buttons (_("GIMP LqR Plug-In - Mask editor mode"),
			 NULL, 0,
			 GTK_STOCK_OK, GTK_RESPONSE_OK, NULL);

  gtk_window_set_resizable (GTK_WINDOW (dlg), FALSE);

  gtk_window_set_keep_above(GTK_WINDOW (dlg), TRUE);

  if (dialog_state->has_pos)
    {
      //printf("move window, x,y=%i,%i\n", dialog_state->x, dialog_state->y); fflush(stdout);
      gtk_window_move (GTK_WINDOW(dlg), dialog_state->x, dialog_state->y);
      dialog_state->has_pos = FALSE;
    }

  g_signal_connect (dlg, "response", G_CALLBACK (callback_dialog_aux_response),
		    (gpointer) ia_data);

  main_hbox = gtk_hbox_new (FALSE, 12);
  gtk_container_set_border_width (GTK_CONTAINER (main_hbox), 12);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dlg)->vbox), main_hbox);

  info_icon = gtk_image_new_from_stock(GIMP_STOCK_INFO, GTK_ICON_SIZE_DIALOG);
  gtk_box_pack_start (GTK_BOX (main_hbox), info_icon, TRUE, TRUE, 0);
  gtk_widget_show (info_icon);

  switch (ui_state->layer_on_edit_type)
    {
      case AUX_LAYER_PRES:
        /* Note for translators: remember to split the line with \n so that the window dialog won't be too large */
        info_label = gtk_label_new(_("Paint the preservation mask\non the current layer, then press OK"));
        break;
      case AUX_LAYER_DISC:
        /* Note for translators: remember to split the line with \n so that the window dialog won't be too large */
        info_label = gtk_label_new(_("Paint the discard mask\non the current layer, then press OK"));
        break;
      case AUX_LAYER_RIGMASK:
        /* Note for translators: remember to split the line with \n so that the window dialog won't be too large */
        info_label = gtk_label_new(_("Paint the rigidity mask\non the current layer, then press OK"));
        break;
      default:
        g_message("You just found a bug");
        g_assert(0);
    }
  gtk_box_pack_start (GTK_BOX (main_hbox), info_label, TRUE, TRUE, 0);
  gtk_widget_show (info_label);

  /*  Show the main containers  */

  gtk_widget_show (main_hbox);
  gtk_widget_show (dlg);
  gtk_main ();

  gtk_widget_destroy (dlg);

  gimp_displays_flush();

  gimp_context_set_foreground (&saved_colour);

  g_free(state);
  g_free(ui_state);

  return dialog_aux_response;
}


/***  Private functions  ***/

/* Callbacks */

static void
callback_dialog_aux_response (GtkWidget * dialog, gint response_id, gpointer data)
{
  InterfaceAuxData * ia_data = INTERFACE_AUX_DATA(data);
  switch (response_id)
    {
      case GTK_RESPONSE_OK:
        gimp_image_set_active_layer(ia_data->image_ID, ia_data->layer_ID);
        gtk_window_get_position(GTK_WINDOW(dialog), &(dialog_state->x), &(dialog_state->y));
        dialog_state->has_pos = TRUE;
      default:
        dialog_aux_response = response_id;
        gtk_main_quit ();
        break;
    }
  g_free(ia_data);
}

/* Aux functions */

GimpRGB * colour_from_type (gint32 image_ID, AuxLayerType layer_type)
{
  switch (gimp_image_base_type (image_ID))
    {
      case GIMP_RGB:
        switch (layer_type)
          {
            case AUX_LAYER_PRES:
              return &default_pres_col;
            case AUX_LAYER_DISC:
              return &default_disc_col;
            case AUX_LAYER_RIGMASK:
              return &default_rigmask_col;
            default:
              g_message("You just found a bug");
              g_assert(0);
          }
        break;
      case GIMP_GRAY:
        return &default_gray_col;
      default:
        g_message("You just found a bug");
        g_assert(0);
    }
  g_message("You just found a bug");
  g_assert(0);
  return NULL;
}
