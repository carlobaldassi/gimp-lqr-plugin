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
#include <string.h>

#include <libgimp/gimp.h>

#include "config.h"
#include "plugin-intl.h"

#include "lqr.h"
#include "lqr_gradient.h"
#include "lqr_carver.h"
#include "lqr_carver_external.h"

#ifdef __LQR_DEBUG__
#include <assert.h>
#endif // __LQR_DEBUG__


/**** EXTERNAL FUNCTIONS ****/

gboolean
lqr_carver_external_readimage (LqrCarver * r, GimpDrawable * drawable)
{
  gint y, bpp;
  gint x1, y1, x2, y2;
  GimpPixelRgn rgn_in;
  guchar *inrow;
  gint update_step;

  update_step = MAX ((r->h - 1) / 20, 1);
  gimp_progress_init (_("Parsing layer..."));

  //gimp_drawable_mask_bounds (drawable->drawable_id, &x1, &y1, &x2, &y2);
  x1 = y1 = 0;
  x2 = gimp_drawable_width (drawable->drawable_id);
  y2 = gimp_drawable_height (drawable->drawable_id);

  bpp = gimp_drawable_bpp (drawable->drawable_id);

  gimp_pixel_rgn_init (&rgn_in,
                       drawable, x1, y1, x2 - x1, y2 - y1, FALSE, FALSE);

  gimp_drawable_offsets (drawable->drawable_id, &r->x_off, &r->y_off);

#ifdef __LQR_DEBUG__
  assert (x2 - x1 == r->w);
  assert (y2 - y1 == r->h);
#endif // __LQR_DEBUG__

  TRY_N_F (inrow = g_try_new (guchar, bpp * r->w));

  for (y = 0; y < r->h; y++)
    {
      gimp_pixel_rgn_get_row (&rgn_in, inrow, x1, y, r->w);

      memcpy(r->rgb + y * r->w * bpp, inrow, r->w * bpp * sizeof(guchar));

      if (y % update_step == 0)
        {
          gimp_progress_update ((gdouble) y / (r->h - 1));
        }

    }

  g_free (inrow);

  return TRUE;
}

gboolean
lqr_carver_external_readbias (LqrCarver * r, gint32 layer_ID, gint bias_factor)
{
  gint x, y, k, bpp, c_bpp;
  gboolean has_alpha;
  gint x1, y1, x2, y2;
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
  x1 = y1 = 0;
  x2 = gimp_drawable_width (layer_ID);
  y2 = gimp_drawable_height (layer_ID);

  bpp = gimp_drawable_bpp (layer_ID);
  has_alpha = gimp_drawable_has_alpha (layer_ID);
  c_bpp = bpp - (has_alpha ? 1 : 0);

  gimp_pixel_rgn_init (&rgn_in,
                       gimp_drawable_get (layer_ID), x1, y1, x2 - x1, y2 - y1,
                       FALSE, FALSE);

  x_off -= r->x_off;
  y_off -= r->y_off;

  lw = (MIN (r->w, x2 + x_off) - MAX (0, x1 + x_off));
  lh = (MIN (r->h, y2 + y_off) - MAX (0, y1 + y_off));


  TRY_N_F( inrow = g_try_new (guchar, bpp * lw));

  for (y = MAX (0, y1 + y_off); y < MIN (r->h, y2 + y_off); y++)
    {
      gimp_pixel_rgn_get_row (&rgn_in, inrow, MAX (x1, -x_off),
                              y - y1 - y_off, lw);

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

          r->bias[y * r->w + (x + MAX (0, x1 + x_off))] += bias;

        }

    }

  g_free (inrow);

  return TRUE;
}

/* flush the LqrCarver to a layer */
gboolean
lqr_carver_external_writeimage (LqrCarver * r, GimpDrawable * drawable)
{
  gint y;
  gint x1, y1, x2, y2;
  GimpPixelRgn rgn_out;
  guchar *outrow;
  gint update_step;

  update_step = MAX ((r->h - 1) / 20, 1);
  gimp_progress_init (_("Applying changes..."));

  x1 = y1 = 0;
  x2 = gimp_drawable_width (drawable->drawable_id);
  y2 = gimp_drawable_height (drawable->drawable_id);

  gimp_pixel_rgn_init (&rgn_out,
                       drawable, x1, y1, x2 - x1, y2 - y1, TRUE, TRUE);



  if (!r->transposed)
    {
      TRY_N_F (outrow = g_try_new (guchar, r->bpp * (x2 - x1)));
    }
  else
    {
      TRY_N_F (outrow = g_try_new (guchar, r->bpp * (y2 - y1)));
    }

  for (y = 0; y < r->h; y++)
    {
      memcpy(outrow, r->rgb + y * r->w_start * r->bpp, r->w * r->bpp * sizeof(guchar));

      if (!r->transposed)
        {
          gimp_pixel_rgn_set_row (&rgn_out, outrow, x1, y + y1, x2 - x1);
        }
      else
        {
          gimp_pixel_rgn_set_col (&rgn_out, outrow, y + x1, y1, y2 - y1);
        }

      if (y % update_step == 0)
        {
          gimp_progress_update ((gdouble) y / (r->h - 1));
        }

    }

  g_free (outrow);

  gimp_drawable_flush (drawable);
  gimp_drawable_merge_shadow (drawable->drawable_id, TRUE);
  gimp_drawable_update (drawable->drawable_id, x1, y1, x2 - x1, y2 - y1);

  return TRUE;
}


#if 0

/* plot the visibility level of the image
 * uses original size
 * uninitialized points are plotted transparent */
gboolean
lqr_external_write_vs (LqrCarver * r)
{
  gchar name[LQR_MAX_NAME_LENGTH];
  gint w, h, w1, x, y, k, vs;
  gint bpp;
  gint32 seam_layer_ID;
  GimpPixelRgn rgn_out;
  guchar *outrow;
  GimpDrawable *drawable;
  gdouble value, rd, gr, bl, al;
  gint update_step;

  update_step = MAX ((r->h - 1) / 20, 1);
  gimp_progress_init (_("Drawing seam map..."));

  /* The name of the layer with the seams map */
  /* (here "%s" represents the selected layer's name) */
  snprintf (name, LQR_MAX_NAME_LENGTH, _("%s seam map"), r->name);

  /* save current size */
  w1 = r->w;

  /* temporarily set the size to the original */
  lqr_raster_set_width (r, r->w_start);

  if (!r->transposed)
    {
      w = r->w;
      h = r->h;
    }
  else
    {
      w = r->h;
      h = r->w;
    }

  seam_layer_ID =
    gimp_layer_new (r->image_ID, name, w, h, GIMP_RGBA_IMAGE, 100,
                    GIMP_NORMAL_MODE);
  gimp_drawable_fill (seam_layer_ID, GIMP_TRANSPARENT_FILL);
  gimp_image_add_layer (r->image_ID, seam_layer_ID, -1);
  drawable = gimp_drawable_get (seam_layer_ID);

  bpp = 4;

  gimp_pixel_rgn_init (&rgn_out, drawable, 0, 0, w, h, TRUE, TRUE);
  TRY_N_F (outrow = g_try_new (guchar, bpp * r->w));

  lqr_cursor_reset (r->c);
  for (y = 0; y < r->h; y++)
    {
      for (x = 0; x < r->w; x++)
        {
          vs = r->vs[r->c->now];
          if (vs == 0)
            {

              for (k = 0; k < bpp; k++)
                {
                  outrow[bpp * x + k] = 0;
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
              outrow[bpp * x] = 255 * rd;
              outrow[bpp * x + 1] = 255 * gr;
              outrow[bpp * x + 2] = 255 * bl;
              outrow[bpp * x + 3] = 255 * al;
            }
          lqr_cursor_next (r->c);
        }
      if (!r->transposed)
        {
          gimp_pixel_rgn_set_row (&rgn_out, outrow, 0, y, r->w);
        }
      else
        {
          gimp_pixel_rgn_set_col (&rgn_out, outrow, y, 0, r->w);
        }

      if (y % update_step == 0)
        {
          gimp_progress_update ((gdouble) y / (r->h - 1));
        }
    }

  g_free (outrow);

  gimp_drawable_flush (drawable);
  gimp_drawable_merge_shadow (seam_layer_ID, TRUE);
  gimp_drawable_update (seam_layer_ID, 0, 0, w, h);
  gimp_drawable_set_visible (seam_layer_ID, FALSE);

  /* recover size */
  lqr_raster_set_width (r, w1);

  return TRUE;
}

/* plot the energy (at current size / visibility) to a file
 * (greyscale) */
gboolean
lqr_raster_write_energy (LqrRaster * r /*, pngwriter& output */ )
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
#endif



