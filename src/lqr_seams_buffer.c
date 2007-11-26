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

#include <stdio.h>

#include <libgimp/gimp.h>

#include "lqr.h"
#include "lqr_gradient.h"
#include "lqr_cursor.h"
#include "lqr_raster.h"
#include "lqr_seams_buffer_list.h"
#include "lqr_seams_buffer.h"

#ifdef __LQR_DEBUG__
#include <assert.h>
#endif // __LQR_DEBUG__


/**** SEAMS BUFFER FUNCTIONS ****/

LqrSeamsBuffer*
lqr_seams_buffer_new (guchar * buffer, gint width, gint height)
{
  LqrSeamsBuffer * seams_buffer;

  TRY_N_N (seams_buffer = g_try_new(LqrSeamsBuffer, 1));
  seams_buffer->buffer = buffer;
  seams_buffer->width = width;
  seams_buffer->height = height;
  return seams_buffer;
}

void
lqr_seams_buffer_destroy (LqrSeamsBuffer * seams_buffer)
{
  g_free (seams_buffer->buffer);
  g_free (seams_buffer);
}

/* flush the visibility level of the image
 * uses original size
 * uninitialized points are plotted transparent */
gboolean
lqr_seams_buffer_flush_vs (LqrRaster * r)
{
  guchar * buffer;
  LqrSeamsBuffer * seams_buffer;
  gint w, h, w1, x, y, z0, k, vs;
  gint bpp;
  gdouble value, rd, gr, bl, al;

  /* save current size */
  w1 = r->w;

  /* temporarily set the size to the original */
  lqr_raster_set_width (r, r->w_start);

  w = lqr_raster_get_width (r);
  h = lqr_raster_get_height (r);

  bpp = 4;

  TRY_N_F (buffer = g_try_new (guchar, w * h * bpp));

  lqr_cursor_reset (r->c);
  for (y = 0; y < r->h; y++)
    {
      for (x = 0; x < r->w; x++)
        {
          vs = r->vs[r->c->now];
	  if (!r->transposed)
	    {
	      z0 = y * r->w + x;
	    }
	  else
	    {
	      z0 = x * r->h + y;
	    }
          if (vs == 0)
            {
              for (k = 0; k < bpp; k++)
                {
                  buffer[z0 * bpp + k] = 0;
                }
            }
          else
            {
              value =
                (double) (r->max_level -
                          (vs - r->w0 + r->w_start)) / r->max_level;
              rd =
                value * r->seam_color_start.r + (1 -
                                                 value) * r->seam_color_end.r;
              gr =
                value * r->seam_color_start.g + (1 -
                                                 value) * r->seam_color_end.g;
              bl =
                value * r->seam_color_start.b + (1 -
                                                 value) * r->seam_color_end.b;
              al = 0.5 * (1 + value);
              buffer[z0 * bpp] = 255 * rd;
              buffer[z0 * bpp + 1] = 255 * gr;
              buffer[z0 * bpp + 2] = 255 * bl;
              buffer[z0 * bpp + 3] = 255 * al;
            }
          lqr_cursor_next (r->c);
        }
    }

  /* recover size */
  lqr_raster_set_width (r, w1);

  TRY_N_F (seams_buffer = lqr_seams_buffer_new(buffer, w, h));

  TRY_N_F (r->flushed_vs = lqr_seams_buffer_list_append(r->flushed_vs, seams_buffer));

  return TRUE;
}


