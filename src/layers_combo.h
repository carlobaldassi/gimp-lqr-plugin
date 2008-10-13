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


#ifndef __COMBO_H__
#define __COMBO_H__

typedef enum
{
  GUESS_DIR_HOR,
  GUESS_DIR_VERT
} GuessDir;

/* Data structs for callbacks */

typedef struct
{
  gboolean *ui_toggled;
  GtkWidget *combo;
  GtkWidget *combo_label;
  GtkObject *scale;
  gboolean *status;
  GtkWidget *guess_button;
  GtkWidget *guess_dir_combo;
} ToggleData;

#define TOGGLE_DATA(data) ((ToggleData*)data)

typedef struct
{
  gint32 *layer_ID;
  gboolean *status;
  gchar name[LQR_MAX_NAME_LENGTH];
  GimpRGB colour;
  gboolean presdisc;
  gboolean *info_show;
  PreviewData *preview_data;
} NewLayerData;

#define NEW_LAYER_DATA(data) ((NewLayerData*)(data))


/* Functions */

gint count_extra_layers (gint32 image_ID, GimpDrawable * drawable);
gboolean dialog_layer_constraint_func (gint32 image_id,
					      gint32 layer_id, gpointer data);

void combo_get_active (GtkWidget * combo, PreviewData * data,
			      gint32 * layer_ID_add, gboolean status,
			      guchar ** buffer_add);
void callback_pres_combo_get_active (GtkWidget * combo, gpointer data);
void callback_disc_combo_get_active (GtkWidget * combo, gpointer data);
void callback_rigmask_combo_get_active (GtkWidget * combo,
 				       gpointer data);

void callback_combo_set_sensitive (GtkWidget * button, gpointer data);
void callback_status_button (GtkWidget * button, gpointer data);
void callback_new_mask_button (GtkWidget * button, gpointer data);

void callback_guess_button (GtkWidget * button, gpointer data);
void callback_guess_direction (GtkWidget * combo, gpointer data);
gint guess_new_size (GtkWidget * button, PreviewData * data, GuessDir direction);
int guess_new_width (GtkWidget * button, gpointer data);
int guess_new_height (GtkWidget * button, gpointer data);

#endif /* __COMBO_H__ */

