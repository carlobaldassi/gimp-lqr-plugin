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
#include "config.h"
#include "plugin-intl.h"

#include "lqr.h"
#include "lqr_gradient.h"
#include "lqr_cursor.h"
#include "lqr_raster.h"
#include "lqr_seams_buffer_list.h"
#include "lqr_seams_buffer.h"

#include "io_functions.h"

guchar * 
rgb_buffer_from_layer (gint32 layer_ID)
{
  gint x, y, k, bpp;
  gint w, h;
  gint x_off, y_off;
  GimpDrawable * drawable;
  GimpPixelRgn rgn_in;
  guchar *inrow;
  guchar * buffer;
  gint update_step;

  gimp_progress_init (_("Parsing layer..."));

  w = gimp_drawable_width (layer_ID);
  h = gimp_drawable_height (layer_ID);

  bpp = gimp_drawable_bpp (layer_ID);

  TRY_N_N (buffer = g_try_new (guchar , bpp * w * h));

  drawable = gimp_drawable_get(layer_ID);

  gimp_pixel_rgn_init (&rgn_in,
                       drawable, 0, 0, w, h, FALSE, FALSE);

  gimp_drawable_offsets (layer_ID, &x_off, &y_off);

  TRY_N_N (inrow = g_try_new (guchar, bpp * w));

  for (y = 0; y < h; y++)
    {
      gimp_pixel_rgn_get_row (&rgn_in, inrow, 0, y, w);

      for (x = 0; x < w; x++)
        {
          for (k = 0; k < bpp; k++)
            {
              buffer[(y * w + x) * bpp + k] = inrow[x * bpp + k];
            }
        }

      update_step = MAX ((h - 1) / 20, 1);
      if (y % update_step == 0)
        {
          gimp_progress_update ((gdouble) y / (h - 1));
        }

    }

  g_free (inrow);

  gimp_drawable_detach(drawable);

  return buffer;
}

gboolean
update_bias (LqrRaster *r, gint32 layer_ID, gint bias_factor, gint base_x_off, gint base_y_off)
{
  gint x, y, k, bpp, c_bpp;
  gboolean has_alpha;
  gint w, h;
  gint x_off, y_off;
  gint lw, lh;
  gint sum;
  gdouble bias;
  GimpPixelRgn rgn_in;
  guchar *inrow;

  if ((layer_ID == 0) || (bias_factor == 0))
    {
      return TRUE;
    }

  gimp_drawable_offsets (layer_ID, &x_off, &y_off);

  //gimp_drawable_mask_bounds (layer_ID, &x1, &y1, &x2, &y2);
  w = gimp_drawable_width (layer_ID);
  h = gimp_drawable_height (layer_ID);

  bpp = gimp_drawable_bpp (layer_ID);
  has_alpha = gimp_drawable_has_alpha (layer_ID);
  c_bpp = bpp - (has_alpha ? 1 : 0);

  gimp_pixel_rgn_init (&rgn_in,
                       gimp_drawable_get (layer_ID), 0, 0, w, h,
                       FALSE, FALSE);

  x_off -= base_x_off;
  y_off -= base_y_off;

  lw = (MIN (r->w, w + x_off) - MAX (0, x_off));
  lh = (MIN (r->h, h + y_off) - MAX (0, y_off));


  TRY_N_F (inrow = g_try_new (guchar, bpp * lw));

  for (y = MAX (0, y_off); y < MIN (r->h, h + y_off); y++)
    {
      gimp_pixel_rgn_get_row (&rgn_in, inrow, MAX (0, -x_off),
                              y - y_off, lw);

      for (x = 0; x < lw; x++)
        {
          sum = 0;
          for (k = 0; k < c_bpp; k++)
            {
              sum += inrow[bpp * x + k];
            }

          bias = (double) bias_factor *sum / (2 * 255 * c_bpp);
          if (has_alpha)
            {
              bias *= (double) inrow[bpp * (x + 1) - 1] / 255;
            }

          r->bias[y * r->w + (x + MAX (0, x_off))] += bias;

        }

    }

  g_free (inrow);

  return TRUE;
}

gboolean
write_raster_to_layer (LqrRaster * r, GimpDrawable * drawable)
{
  gint32 layer_ID;
  gint x, y, k;
  gint w, h;
  GimpPixelRgn rgn_out;
  guchar *outrow;
  gint update_step;

  gimp_progress_init (_("Applying changes..."));
  update_step = MAX ((r->h - 1) / 20, 1);

  layer_ID = drawable->drawable_id;

  //gimp_drawable_mask_bounds (layer_ID, &x1, &y1, &x2, &y2);
  w = gimp_drawable_width (layer_ID);
  h = gimp_drawable_height (layer_ID);

  gimp_pixel_rgn_init (&rgn_out,
                       drawable, 0, 0, w, h, TRUE, TRUE);



  if (!r->transposed)
    {
      TRY_N_F (outrow = g_try_new (guchar, r->bpp * w));
    }
  else
    {
      TRY_N_F (outrow = g_try_new (guchar, r->bpp * h));
    }

  lqr_cursor_reset (r->c);

  for (y = 0; y < r->h; y++)
    {
      for (x = 0; x < r->w; x++)
        {
          for (k = 0; k < r->bpp; k++)
            {
              //outrow[r->bpp * x + k] = r->c->now->rgb[k];
              outrow[r->bpp * x + k] = r->rgb[r->c->now * r->bpp + k];
            }
          lqr_cursor_next (r->c);
        }
      if (!r->transposed)
        {
          gimp_pixel_rgn_set_row (&rgn_out, outrow, 0, y, w);
        }
      else
        {
          gimp_pixel_rgn_set_col (&rgn_out, outrow, y, 0, h);
        }

      if (y % update_step == 0)
        {
          gimp_progress_update ((gdouble) y / (r->h - 1));
        }

    }

  g_free (outrow);

  gimp_drawable_flush (drawable);
  gimp_drawable_merge_shadow (layer_ID, TRUE);
  gimp_drawable_update (layer_ID, 0, 0, w, h);

  return TRUE;
}

void
write_seams_buffer_to_layer (LqrSeamsBuffer * seams_buffer, gint32 image_ID, gchar * name, gint x_off, gint y_off)
{
  gint w, h, y;
  gint bpp;
  gint32 seam_layer_ID;
  GimpPixelRgn rgn_out;
  guchar *outrow;
  GimpDrawable *drawable;
  gint update_step;

  w = seams_buffer->width;
  h = seams_buffer->height;

  gimp_progress_init (_("Drawing seam map..."));
  update_step = MAX ((h - 1) / 20, 1);

  seam_layer_ID =
    gimp_layer_new (image_ID, name, w, h, GIMP_RGBA_IMAGE, 100,
                    GIMP_NORMAL_MODE);
  gimp_drawable_fill (seam_layer_ID, GIMP_TRANSPARENT_FILL);
  gimp_image_add_layer (image_ID, seam_layer_ID, -1);
  gimp_layer_translate (seam_layer_ID, x_off, y_off);
  drawable = gimp_drawable_get (seam_layer_ID);

  bpp = 4;

  gimp_pixel_rgn_init (&rgn_out, drawable, 0, 0, w, h, TRUE, TRUE);
  //TRY_N_F (outrow = g_try_new (guchar, bpp * w));

  for (y = 0; y < h; y++)
    {
      outrow = seams_buffer->buffer + y * w * bpp;
      gimp_pixel_rgn_set_row (&rgn_out, outrow, 0, y, w);
      if (y % update_step == 0)
        {
          gimp_progress_update ((gdouble) y / (h - 1));
        }
    }

  //g_free (outrow);

  gimp_drawable_flush (drawable);
  gimp_drawable_merge_shadow (seam_layer_ID, TRUE);
  gimp_drawable_update (seam_layer_ID, 0, 0, w, h);
  gimp_drawable_set_visible (seam_layer_ID, FALSE);
}

void
write_all_seams_buffers (LqrSeamsBufferList * list, gint32 image_ID, gchar * orig_name, gint x_off, gint y_off)
{
  LqrSeamsBufferList * now = list;
  gchar name[LQR_MAX_NAME_LENGTH];
  while (now != NULL)
    {
      /* The name of the layer with the seams map */
      /* (here "%s" represents the selected layer's name) */
      snprintf (name, LQR_MAX_NAME_LENGTH, _("%s seam map"), orig_name);
      write_seams_buffer_to_layer (now->current, image_ID, name, x_off, y_off);
      now = now->next;
    }
}


/* plot the energy (at current size / visibility) to a file
 * (greyscale) */
gboolean
lqr_external_write_energy (LqrRaster * r /*, pngwriter& output */ )
{
  int x, y;
  double e;

  if (!r->transposed)
    {
      /* external_resize(r->w, r->h); */
    }
  else
    {
      /* external_resize(r->h, r->w); */
    }

  lqr_cursor_reset (r->c);
  for (y = 1; y <= r->h; y++)
    {
      for (x = 1; x <= r->w; x++)
        {
          e = r->en[r->c->now];
          if (!r->transposed)
            {
              /* external_write(x, y, e, e, e); */
            }
          else
            {
              /* external_write(y, x, e, e, e); */
            }
          lqr_cursor_next (r->c);
        }
    }

  return TRUE;
}


