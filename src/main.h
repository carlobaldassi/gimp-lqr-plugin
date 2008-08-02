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

#define RESPONSE_REFRESH (1)
#define RESPONSE_FEAT_REFRESH (2)
#define RESPONSE_ADV_REFRESH (3)
#define RESPONSE_RESET (4)

/**** OPERATIONAL_MODES ****/
enum _OperMode
{
  OPER_MODE_NORMAL,
  OPER_MODE_LQRBACK,
  OPER_MODE_SCALEBACK
};

typedef enum _OperMode OperMode;

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
  gint guess_direction;
  gint last_used_width;
  gint last_used_height;
  gint32 last_layer_ID;
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
  gint32 image_ID;
  gint32 orig_layer_ID;
  gint32 layer_ID;
  GimpDrawable *drawable;
  GimpImageType type;
  gint width;
  gint height;
  gint old_width;
  gint old_height;
  gint x_off;
  gint y_off;
  gfloat factor;
  guchar *buffer;
  guchar *pres_buffer;
  guchar *disc_buffer;
  guchar *rigmask_buffer;
  guchar *preview_buffer;
  gboolean toggle;
  PlugInVals *vals;
  PlugInUIVals *ui_vals;
  GdkPixbuf *pixbuf;
  GtkWidget *area;
  GtkWidget *pres_combo;
  GtkWidget *disc_combo;
  GtkWidget *rigmask_combo;
  GtkWidget *coordinates;
  GtkWidget *disc_warning_image;
  gint guess_direction;
} PreviewData;

#define PREVIEW_DATA(data) ((PreviewData*)data)

typedef struct
{
  gpointer ui_vals;
  gpointer button;
} PresDiscStatus;

#define PRESDISC_STATUS(data) ((PresDiscStatus*)data)

typedef struct
{
  gpointer ui_toggled;
  gpointer combo;
  gpointer combo_label;
  gpointer scale;
  gpointer status;
  gpointer guess_button;
  gpointer guess_dir_combo;
} ToggleData;

#define TOGGLE_DATA(data) ((ToggleData*)data)

typedef struct
{
  GtkWidget *notebook;
  GtkWidget *features_page;
  GtkWidget *advanced_page;
  gint features_page_ID;
  gint advanced_page_ID;
  GtkWidget *label;
  gint32 image_ID;
  GimpDrawable *drawable;
} NotebookData;

#define NOTEBOOK_DATA(data) ((NotebookData*)data)

typedef struct
{
  NotebookData *notebook_data;
  PlugInVals *vals;
  PlugInUIVals *ui_vals;
} ResponseData;

#define RESPONSE_DATA(data) ((ResponseData*)data)

typedef struct
{
  gint32 *layer_ID;
  gboolean *status;
  gchar name[LQR_MAX_NAME_LENGTH];
  GimpRGB colour;
  gboolean presdisc;
} NewLayerData;

#define NEW_LAYER_DATA(data) ((NewLayerData*)(data))

typedef struct
{
  gint32 *layer_ID;
  gint32 *disc_layer_ID;
  gpointer *coordinates;
} GuessData;

#define GUESS_DATA(data) ((GuessData*)(data))



/*  Default values  */

extern const PlugInVals default_vals;
extern const PlugInImageVals default_image_vals;
extern const PlugInDrawableVals default_drawable_vals;
extern const PlugInUIVals default_ui_vals;
extern const PlugInColVals default_col_vals;


#endif /* __MAIN_H__ */
