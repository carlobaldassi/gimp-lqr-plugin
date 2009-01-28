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

#ifndef __MAIN_H__
#define __MAIN_H__

#include "main_common.h"

/* Additional responses for dialog */

#define RESPONSE_REFRESH (1)
#define RESPONSE_FEAT_REFRESH (2)
#define RESPONSE_ADV_REFRESH (3)
#define RESPONSE_RESET (4)
#define RESPONSE_INTERACTIVE (5)
#define RESPONSE_NONINTERACTIVE (6)
#define RESPONSE_FATAL (7)


/* Structs for parameters */

typedef struct
{
  gint32 image_ID;
} PlugInImageVals;

typedef struct
{
  gint32 layer_ID;
} PlugInDrawableVals;

typedef struct
{
  gboolean chain_active;
  gboolean pres_status;
  gboolean disc_status;
  gboolean rigmask_status;
  gint last_used_width;
  gint last_used_height;
  gint32 last_layer_ID;
  gboolean seams_control_expanded;
  gboolean operations_expanded;
  gboolean dlg_has_pos;
  gint dlg_x;
  gint dlg_y;
} PlugInUIVals;

#define PLUGIN_UI_VALS(data) ((PlugInUIVals*)data)

typedef struct
{
  gdouble r1;
  gdouble g1;
  gdouble b1;
  gdouble r2;
  gdouble g2;
  gdouble b2;
} PlugInColVals;

typedef struct
{
  gboolean has_pos;
  gint x;
  gint y;
} PlugInDialogVals;

#define PLUGIN_DIALOG_VALS(data) ((PlugInDialogVals*)data)


/* Scaleback modes */

enum _ScalebackMode
{
  SCALEBACK_MODE_LQRBACK,
  SCALEBACK_MODE_STD,
  SCALEBACK_MODE_STDW,
  SCALEBACK_MODE_STDH
};

typedef enum _ScalebackMode ScalebackMode;


/*  Default values  */

extern const PlugInVals default_vals;
extern const PlugInImageVals default_image_vals;
extern const PlugInDrawableVals default_drawable_vals;
extern const PlugInUIVals default_ui_vals;
extern const PlugInColVals default_col_vals;


#endif /* __MAIN_H__ */
