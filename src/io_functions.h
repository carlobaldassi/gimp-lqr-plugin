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

#ifndef __IO_FUNCTIONS__
#define __IO_FUNCTIONS__

#ifndef __LQR_H__
#error "lqr/lqr.h must be included prior to io_functions.h"
#endif /* __LQR_H__ */

struct _VMapFuncArg;

typedef struct _VMapFuncArg VMapFuncArg;

struct _VMapFuncArg
{
  gint32 image_ID;
  gchar *name;
  gint x_off;
  gint y_off;
  GimpRGB colour_start;
  GimpRGB colour_end;
};

#define VMAP_FUNC_ARG(data) ((VMapFuncArg*)(data))

/* INPUT/OUTPUT FUNCTIONS */

guchar *rgb_buffer_from_layer (gint32 layer_ID);
LqrRetVal update_bias (LqrCarver * r, gint32 layer_ID, gint bias_factor,
                       gint base_x_off, gint base_y_off);
LqrRetVal set_rigmask (LqrCarver * r, gint32 layer_ID, gint base_x_off, gint base_y_off);
LqrRetVal write_carver_to_layer (LqrCarver * r, gint32 layer_ID);
LqrRetVal write_vmap_to_layer (LqrVMap * vmap, gpointer data);
LqrRetVal write_all_vmaps (LqrVMapList * list, gint32 image_ID,
                           gchar * orig_name, gint x_off, gint y_off,
                           GimpRGB col_start, GimpRGB col_end);
/* unimplemented */
LqrRetVal lqr_external_write_energy (LqrCarver * r /*, pngwriter& output */ );  /* output the energy */

#endif /* __IO_FUNCTIONS__ */
