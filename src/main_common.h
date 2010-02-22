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

#ifndef __MAIN_COMMON_H__
#define __MAIN_COMMON_H__

/*  Constants  */

#define PLUG_IN_NAME   "plug-in-lqr"

#define DATA_KEY_VALS    "plug_in_lqr"
#define DATA_KEY_UI_VALS "plug_in_lqr_ui"
#define DATA_KEY_COL_VALS "plug_in_lqr_col"
#define PARASITE_KEY     "plug_in_lqr_options"

#define VALS_MAX_NAME_LENGTH (1024)
#define MAX_STRING_SIZE   (2048)

typedef struct
{
  gint new_width;
  gint new_height;
  gint32 pres_layer_ID;
  gint pres_coeff;
  gint32 disc_layer_ID;
  gint disc_coeff;
  gfloat rigidity;
  gint32 rigmask_layer_ID;
  gint delta_x;
  gfloat enl_step;
  gboolean resize_aux_layers;
  gboolean resize_canvas;
  gboolean new_layer;
  gboolean output_seams;
  gint nrg_func;
  gint res_order;
  gint mask_behavior;
  gboolean scaleback;
  gint scaleback_mode;
  gboolean no_disc_on_enlarge;
  gchar pres_layer_name[VALS_MAX_NAME_LENGTH];
  gchar disc_layer_name[VALS_MAX_NAME_LENGTH];
  gchar rigmask_layer_name[VALS_MAX_NAME_LENGTH];
  gchar selected_layer_name[VALS_MAX_NAME_LENGTH];
} PlugInVals;

#endif /* __MAIN_COMMON_H__ */
