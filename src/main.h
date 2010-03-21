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

#ifndef __MAIN_H__
#define __MAIN_H__

#include "main_common.h"

/* Additional responses for dialog */

#define RESPONSE_REFRESH (1)
#define RESPONSE_FEAT_REFRESH (2)
#define RESPONSE_ADV_REFRESH (3)
#define RESPONSE_RESET (4)
#define RESPONSE_WORK_ON_AUX_LAYER (5)
#define RESPONSE_INTERACTIVE (6)
#define RESPONSE_NONINTERACTIVE (7)
#define RESPONSE_FATAL (8)

typedef enum
{
  AUX_LAYER_PRES,
  AUX_LAYER_DISC,
  AUX_LAYER_RIGMASK,
} AuxLayerType;

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
  gint32 layer_on_edit_ID;
  AuxLayerType layer_on_edit_type;
  gboolean layer_on_edit_is_new;
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


/* Output target */

enum _OutputTarget
{
  OUTPUT_TARGET_SAME_LAYER,
  OUTPUT_TARGET_NEW_LAYER,
  OUTPUT_TARGET_NEW_IMAGE
};

typedef enum _OutputTarget OutputTarget;


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

/* Convenience macros for checking */
;

#define IMAGE_CHECK_ACTION(image_ID, action, ret_val) G_STMT_START { \
  if (!gimp_image_is_valid (image_ID)) \
    { \
      g_message (_("Error: invalid image")); \
      action; \
      return ret_val; \
    } \
  } G_STMT_END

#define IMAGE_CHECK(image_ID, ret_val) IMAGE_CHECK_ACTION (image_ID, , ret_val)

#define LAYER_CHECK_ACTION(layer_ID, action, ret_val) G_STMT_START { \
  if (!gimp_drawable_is_valid (layer_ID)) \
    { \
      g_message (_("Error: invalid layer")); \
      action; \
      return ret_val; \
    } \
  } G_STMT_END

#define LAYER_CHECK(layer_ID, ret_val) LAYER_CHECK_ACTION (layer_ID, , ret_val)

#define LAYER_CHECK0(layer_ID, ret_val) if (layer_ID) LAYER_CHECK (layer_ID, ret_val)

#define AUX_LAYER_STATUS(layer_ID, status) G_STMT_START { \
  if ((layer_ID == -1) || (status == FALSE)) \
    { \
      layer_ID = 0; \
    } \
  } G_STMT_END

#endif /* __MAIN_H__ */
