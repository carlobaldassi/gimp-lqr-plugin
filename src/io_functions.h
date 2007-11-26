/* GIMP LiquidRescaling Plug-in
 * Copyright (C) 2007 Carlo Baldassi (the "Author") <carlobaldassi@yahoo.it>.
 * (implementation based on the GIMP Plug-in Template by Michael Natterer)
 * All Rights Reserved.
 *
 * This plugin implements the algorithm described in the paper
 * "Seam Carving for Content-Aware Image Resizing"
 * by Shai Avidan and Ariel Shamir
 * which can be found at http://www.faculty.idc.ac.il/arik/imret.pdf
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 dated June, 1991.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 */

#ifndef __IO_FUNCTIONS__
#define __IO_FUNCTIONS__

#ifndef __LQR_H__
#error "lqr.h must be included prior to io_functions.h"
#endif /* __LQR_H__ */

#ifndef __LQR_RASTER_H__
#error "lqr_raster.h must be included prior to io_functions.h"
#endif /* __LQR_H__ */

/* INPUT/OUTPUT FUNCTIONS */

guchar * rgb_buffer_from_layer (gint32 layer_ID);
gboolean update_bias (LqrRaster *r, gint32 layer_ID, gint bias_factor, gint base_x_off, gint base_y_off);
gboolean write_raster_to_layer (LqrRaster * r, GimpDrawable * drawable);
void write_seams_buffer_to_layer (LqrSeamsBuffer * seams_buffer, gint32 image_ID, gchar *name, gint x_off, gint y_off);
void write_all_seams_buffers (LqrSeamsBufferList * list, gint32 image_ID, gchar *name, gint x_off, gint y_off);
/* unimplemented */
gboolean lqr_external_write_energy (LqrRaster * r /*, pngwriter& output */ );     /* output the energy */

#endif // __IO_FUNCTIONS__

