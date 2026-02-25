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

#ifndef __INTERFACE_H__
#define __INTERFACE_H__

#include <gtk/gtk.h>
#include <libgimp/gimp.h>
#include "main.h"
#include "notebook.h"
#include "preview.h"
#include "layers_combo.h"

/* Constants */
#define SCALE_WIDTH         (80)
#define SPIN_BUTTON_WIDTH   (75)
#define BOX_INDENT          (22)
#define MAX_COEFF          (3000)
#define MAX_RIGIDITY       (1000)
#define MAX_DELTA_X        (10)
#define MAX_STRING_SIZE    (2048)

/* Data structs for callbacks */
typedef struct {
    gpointer ui_vals;
    gpointer button;
} PresDiscStatus;

#define PRESDISC_STATUS(data) ((PresDiscStatus*)data)




/* Function declarations */
/* Public functions */
gint dialog(
        GimpImage *image,
        GimpDrawable **drawables,
        PlugInImageVals *image_vals,
        PlugInDrawableVals *drawable_vals,
        PlugInVals *vals,
        PlugInUIVals *ui_vals,
        PlugInColVals *col_vals,
        PlugInDialogVals *dialog_vals);

#endif /* __INTERFACE_H__ */
