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

#ifndef __RENDER_H__
#define __RENDER_H__

typedef struct
{
  LqrCarver * carver;
  gint32 layer_ID;
  gboolean alpha_lock;
  gboolean alpha_lock_pres;
  gboolean alpha_lock_disc;
  gboolean alpha_lock_rigmask;
  gint ref_w;
  gint ref_h;
  gint orientation;
  gint depth;
  gfloat enl_step;
} CarverData;

#define CARVER_DATA(data) ((CarverData*)data)


/* progress functions wrappers */
gboolean my_progress_end (const gchar * message);

/* Functions  */

CarverData *
render_init_carver (gint32 image_ID,
        GimpDrawable * drawable,
        PlugInVals * vals,
        PlugInDrawableVals * drawable_vals,
        gboolean interactive);

gboolean
render_noninteractive (gint32 image_ID,
        GimpDrawable * drawable,
        PlugInVals * vals,
        PlugInDrawableVals * drawable_vals,
        PlugInColVals * col_vals,
        CarverData * carver_data);

gboolean
render_interactive (gint32 image_ID,
        GimpDrawable * drawable,
        PlugInVals * vals,
        PlugInDrawableVals * drawable_vals,
        CarverData * carver_data);

#endif /* __RENDER_H__ */
