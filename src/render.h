/* GIMP LiquidRescale Plug-in
 * Copyright (C) 2007-2009 Carlo Baldassi (the "Author") <carlobaldassi@gmail.com>.
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

#ifndef __RENDER_H__
#define __RENDER_H__

typedef struct
{
  LqrCarver * carver;
  gint32 layer_ID;
  GimpImageBaseType base_type;
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


/* Functions  */

CarverData *
render_init_carver (gint32 image_ID,
        PlugInVals * vals,
        PlugInDrawableVals * drawable_vals,
        gboolean interactive);

gboolean
render_noninteractive (gint32 image_ID,
        PlugInVals * vals,
        PlugInDrawableVals * drawable_vals,
        PlugInColVals * col_vals,
        CarverData * carver_data);

gboolean
render_interactive (gint32 image_ID,
        PlugInVals * vals,
        PlugInDrawableVals * drawable_vals,
        CarverData * carver_data);

gboolean
render_flatten (gint32 image_ID,
        PlugInVals * vals,
        PlugInDrawableVals * drawable_vals,
        CarverData * carver_data);

gboolean
render_dump_vmap (gint32 image_ID,
        PlugInVals * vals,
        PlugInDrawableVals * drawable_vals,
        PlugInColVals * col_vals,
        CarverData * carver_data,
        gint32 * vmap_layer_ID_p);

#endif /* __RENDER_H__ */
