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
#include "lqr_cursor.h"
#include "lqr_raster.h"
#include "lqr_external.h"

#ifdef __LQR_DEBUG__
#include <assert.h>
#endif // __LQR_DEBUG__

/**** LQR_RASTER CLASS FUNCTIONS ****/

/*** constructor & destructor ***/

/* constructor */
LqrRaster *
lqr_raster_new (gint32 image_ID, GimpDrawable * drawable, gchar * name,
                gint32 pres_layer_ID, gint pres_coeff,
                gint32 disc_layer_ID, gint disc_coeff,
                LqrGradFunc gf_ind, gint rigidity,
                gboolean resize_aux_layers,
                gboolean fast_update,
                gboolean output_seams,
                GimpRGB seam_color_start, GimpRGB seam_color_end)
{
  LqrRaster *r;
  gint dx;
  gint y;

  TRY_N_N (r = g_try_new (LqrRaster, 1));

  r->image_ID = image_ID;
  strncpy (r->name, name, LQR_MAX_NAME_LENGTH);

  r->level = 1;
  r->max_level = 1;
  r->transposed = 0;
  r->aux = 0;
  r->rigidity = rigidity;
  r->fast_update = fast_update;
  r->resize_aux_layers = resize_aux_layers;
  r->output_seams = output_seams;
  r->seam_color_start = seam_color_start;
  r->seam_color_end = seam_color_end;
  r->pres_raster = NULL;
  r->disc_raster = NULL;

  r->h = gimp_drawable_height (drawable->drawable_id);
  r->w = gimp_drawable_width (drawable->drawable_id);
  r->bpp = gimp_drawable_bpp (drawable->drawable_id);

  r->w0 = r->w;
  r->h0 = r->h;
  r->w_start = r->w;
  r->h_start = r->h;

  /* allocate memory for internal structures */
  TRY_N_N (r->rgb = g_try_new (guchar, r->w * r->h * r->bpp));
  TRY_N_N (r->vs = g_try_new0 (gint, r->w * r->h));
  TRY_N_N (r->en = g_try_new (gdouble, r->w * r->h));
  TRY_N_N (r->bias = g_try_new0 (gdouble, r->w * r->h));
  TRY_N_N (r->m = g_try_new (gdouble, r->w * r->h));

  TRY_N_N (r->_raw = g_try_new (gint, r->h_start * r->w_start));
  TRY_N_N (r->raw = g_try_new (gint*, r->h_start));

  r->raw[0] = r->_raw;
  for (y = 1; y < r->h; y++) {
	  r->raw[y] = r->raw[y - 1] + r->w_start;
  }

  TRY_N_N (r->vpath = g_try_new (gint, r->h));
  TRY_N_N (r->vpath_x = g_try_new (gint, r->h));

  /* initialize cursors */

  TRY_N_N (r->c = lqr_cursor_create (r, r->vs));

  /* read input layer */
  TRY_F_N (lqr_external_readimage (r, drawable));

  /* read bias layers */
  TRY_F_N (lqr_external_readbias (r, pres_layer_ID, pres_coeff));
  TRY_F_N (lqr_external_readbias (r, disc_layer_ID, -disc_coeff));

  /* rasterize bias layers */
  if (resize_aux_layers == TRUE)
    {
      if (pres_layer_ID != 0)
        {
          TRY_N_N (r->pres_raster =
                   lqr_raster_aux_new (image_ID,
                                       gimp_drawable_get
                                       (pres_layer_ID), ""));
        }
      if (disc_layer_ID != 0)
        {
          TRY_N_N (r->disc_raster =
                   lqr_raster_aux_new (image_ID,
                                       gimp_drawable_get
                                       (disc_layer_ID), ""));
        }
    }

  /* set gradient function */
  lqr_raster_set_gf (r, gf_ind);

  /* set rigidity map */
  r->delta_x = 1;               /* currently only 0 or 1 are meaningful values */

  r->rigidity_map = g_try_new0 (gdouble, 2 * r->delta_x + 1);
  r->rigidity_map += r->delta_x;
  for (dx = -r->delta_x; dx <= r->delta_x; dx++)
    {
      r->rigidity_map[dx] = (gdouble) r->rigidity * dx * dx / r->h;
    }

  return r;
}

/* aux constructor (for preserve/discard layers) */
LqrRaster *
lqr_raster_aux_new (gint32 image_ID, GimpDrawable * drawable, gchar * name)
{
  LqrRaster *r;

  TRY_N_N (r = g_try_new (LqrRaster, 1));

  r->image_ID = image_ID;
  strncpy (r->name, name, LQR_MAX_NAME_LENGTH);

  r->level = 1;
  r->max_level = 1;
  r->transposed = 0;
  r->aux = 1;
  r->rigidity = 0;
  r->resize_aux_layers = FALSE;
  r->output_seams = FALSE;
  r->pres_raster = NULL;
  r->disc_raster = NULL;

  r->en = NULL;
  r->bias = NULL;
  r->m = NULL;
  r->_raw = NULL;
  r->raw = NULL;
  r->vpath = NULL;
  r->vpath_x = NULL;
  r->rigidity_map = NULL;

  r->h = gimp_drawable_height (drawable->drawable_id);
  r->w = gimp_drawable_width (drawable->drawable_id);
  r->bpp = gimp_drawable_bpp (drawable->drawable_id);

  r->w0 = r->w;
  r->h0 = r->h;
  r->w_start = r->w;
  r->h_start = r->h;

  /* allocate memory for internal structures */
  TRY_N_N (r->rgb = g_try_new (guchar, r->w * r->h * r->bpp));
  TRY_N_N (r->vs = g_try_new (gint, r->w * r->h));

  /* initialize cursors */

  TRY_N_N (r->c = lqr_cursor_create (r, r->vs));

  /* read input layer */
  TRY_F_N (lqr_external_readimage (r, drawable));

  return r;
}

/* destructor */
void
lqr_raster_destroy (LqrRaster * r)
{
  g_free (r->rgb);
  g_free (r->vs);
  g_free (r->en);
  g_free (r->bias);
  g_free (r->m);

  lqr_cursor_destroy (r->c);
  g_free (r->vpath);
  g_free (r->vpath_x);
  if (r->pres_raster != NULL)
    {
      lqr_raster_destroy (r->pres_raster);
    }
  if (r->disc_raster != NULL)
    {
      lqr_raster_destroy (r->disc_raster);
    }
  if (r->rigidity_map != NULL)
    {
      r->rigidity_map -= r->delta_x;
      g_free (r->rigidity_map);
    }
  g_free (r->_raw);
  g_free (r->raw);
  g_free (r);
}


/*** gradient related functions ***/

/* set the gradient function for energy computation 
 * choosing among those those listed in enum LqrGradFunc
 * the arguments are the x, y components of the gradient */
void
lqr_raster_set_gf (LqrRaster * r, LqrGradFunc gf_ind)
{
  switch (gf_ind)
    {
    case LQR_GF_NORM:
      r->gf = &norm;
      break;
    case LQR_GF_NORM_BIAS:
      r->gf = &norm_bias;
      break;
    case LQR_GF_SUMABS:
      r->gf = &sumabs;
      break;
    case LQR_GF_XABS:
      r->gf = &xabs;
      break;
    case LQR_GF_YABS:
      r->gf = &yabs;
      break;
    case LQR_GF_NULL:
      r->gf = &zero;
      break;
#ifdef __LQR_DEBUG__
    default:
      assert (0);
#endif // __LQR_DEBUG__
    }
}


inline gdouble
lqr_raster_read (LqrRaster * r, gint x, gint y)
{
  gdouble sum = 0;
  gint k;
  gint now = r->raw[y][x];
  for (k = 0; k < r->bpp; k++)
    {
      sum += r->rgb[now * r->bpp + k];
    }
  return sum / (255 * r->bpp);
}

void
lqr_raster_carve (LqrRaster * r)
{
  gint x, y;

  for (y = 0; y < r->h_start; y++)
    {
#ifdef __LQR_DEBUG__
      assert (r->vs[r->raw[y][r->vpath_x[y]]] != 0);
      for (x = 0; x < r->vpath_x[y]; x++)
        {
          assert (r->vs[r->raw[y][x]] == 0);
        }
#endif // __LQR_DEBUG__
      for (x = r->vpath_x[y]; x < r->w; x++)
        {
          r->raw[y][x] = r->raw[y][x + 1];
#ifdef __LQR_DEBUG__
          assert (r->vs[r->raw[y][x]] == 0);
#endif // __LQR_DEBUG__
        }
    }
}



/*** compute maps (energy, minpath & visibility ***/

/* build multisize image up to given depth
 * it is progressive (can be called multilple times) */
gboolean
lqr_raster_build_maps (LqrRaster * r, gint depth)
{
#ifdef __LQR_DEBUG__
  assert (depth <= r->w_start);
  assert (depth >= 1);
#endif // __LQR_DEBUG__

  /* only go deeper if needed */
  if (depth > r->max_level)
    {
      /* set to minimum width */
      lqr_raster_set_width (r, r->w_start - r->max_level + 1);

      /* compute energy & minpath maps */
      lqr_raster_build_emap (r);
      lqr_raster_build_mmap (r);

      /* compute visibility */
      TRY_F_F (lqr_raster_build_vsmap (r, depth));
    }
  return TRUE;
}

/* compute energy map */
void
lqr_raster_build_emap (LqrRaster * r)
{
  gint x, y;

  for (y = 0; y < r->h; y++)
    {
      for (x = 0; x < r->w; x++)
        {
          lqr_raster_compute_e (r, x, y);
        }
    }
}

/* compute auxiliary minpath map
 * defined as
 * y = 1 : m(x,y) = e(x,y)
 * y > 1 : m(x,y) = min{ m(x-1,y-1), m(x,y-1), m(x+1,y-1) } + e(x,y) */
void
lqr_raster_build_mmap (LqrRaster * r)
{
  gint x, y;
  gint data;
  gint data_down;
  gint x1_min, x1_max, x1;
  double m;

  /* span first row */
  for (x = 0; x < r->w; x++)
    {
      data = r->raw[0][x];
#ifdef __LQR_DEBUG__
      assert (r->vs[data] == 0);
#endif //__LQR_DEBUG__
      r->m[data] = r->en[data];
    }

  /* span all other rows */
  for (y = 1; y < r->h; y++)
    {
      for (x = 0; x < r->w; x++)
        {
          data = r->raw[y][x];
#ifdef __LQR_DEBUG__
          assert (r->vs[data] == 0);
#endif //__LQR_DEBUG__
	  x1_min = MAX(-x, -r->delta_x);
	  x1_max = MIN(r->w - 1 - x, r->delta_x);
	  data_down = r->raw[y - 1][x + x1_min];
          m = r->m[data_down];
          for (x1 = x1_min + 1; x1 <= x1_max; x1++)
            {
	      data_down = r->raw[y - 1][x + x1];
              /* find the min among the neighbors
               * in the last row */
	      if (r->rigidity) {
		      m = MIN(m, r->m[data_down] + r->rigidity_map[x1]);
	      } else {
		      m = MIN(m, r->m[data_down]);
	      }
            }

          /* set current m */
          r->m[data] = r->en[data] + m;
        }
    }
}

/* compute (vertical) visibility map up to given depth
 * (it also calls inflate() to add image enlargment information) */
gboolean
lqr_raster_build_vsmap (LqrRaster * r, gint depth)
{
  gint l;
  gint update_step;

#ifdef __LQR_VERBOSE__
  printf("[ building visibility map ]\n"); fflush(stdout);
#endif /* __LQR_VERBOSE__ */


#ifdef __LQR_DEBUG__
  assert (depth <= r->w_start + 1);
  assert (depth >= 1);
#endif // __LQR_DEBUG__

  /* default behavior : compute all possible levels
   * (complete map) */
  if (depth == 0)
    {
      depth = r->w_start + 1;
    }

  /* here we assume that
   * lqr_raster_set_width(w_start - max_level + 1);
   * has been given */

  /* reset visibility map and level (WHY????) */
  /*
  if (r->max_level == 1)
    {
      for (z = 0; z < r->w0 * r->h0; z++)
        {
          r->vs[z] = 0;
        }
    }
    */

  /* update step for progress */
  update_step = MAX ((depth - r->max_level) / 50, 1);

  /* cycle over levels */
  for (l = r->max_level; l < depth; l++)
    {

      if ((l - r->max_level) % update_step == 0)
        {
          gimp_progress_update ((gdouble) (l - r->max_level) /
                                (gdouble) (depth - r->max_level));
        }

      /* compute vertical seam */
      lqr_raster_build_vpath (r);

      /* update visibility map
       * (assign level to the seam) */
      lqr_raster_update_vsmap (r, l + r->max_level - 1);

      /* increase (in)visibility level
       * (make the last seam invisible) */
      r->level++;
      r->w--;

      /* update raw data */
      lqr_raster_carve (r);

      if (r->w > 1)
        {
          /* update the energy */
          //lqr_raster_build_emap (r);
          lqr_raster_update_emap (r);

          /* recalculate the minpath map */
          lqr_raster_update_mmap (r);
          //lqr_raster_build_mmap (r);
        }
      else
        {
          /* complete the map (last seam) */
          lqr_raster_finish_vsmap (r);
        }
    }

  /* reset width to the maximum */
  lqr_raster_set_width (r, r->w0);

  /* copy visibility maps on auxiliary layers */
  if (r->resize_aux_layers)
    {
      if (r->pres_raster != NULL)
        {
          lqr_raster_copy_vsmap (r, r->pres_raster);
        }
      if (r->disc_raster != NULL)
        {
          lqr_raster_copy_vsmap (r, r->disc_raster);
        }
    }

  /* insert seams for image enlargement */
  TRY_F_F (lqr_raster_inflate (r, depth - 1));

  /* set new max_level */
  r->max_level = depth;

  /* reset image size */
  lqr_raster_set_width (r, r->w_start);

  /* repeat the above steps for auxiliary layers */
  if (r->resize_aux_layers)
    {
      if (r->pres_raster != NULL)
        {
          TRY_F_F (lqr_raster_inflate (r->pres_raster, depth - 1));
          r->pres_raster->max_level = depth;
          lqr_raster_set_width (r->pres_raster, r->pres_raster->w_start);
        }
      if (r->disc_raster != NULL)
        {
          TRY_F_F (lqr_raster_inflate (r->disc_raster, depth - 1));
          r->disc_raster->max_level = depth;
          lqr_raster_set_width (r->disc_raster, r->disc_raster->w_start);
        }
    }

#ifdef __LQR_VERBOSE__
  printf("[ visibility map OK ]\n"); fflush(stdout);
#endif /* __LQR_VERBOSE__ */

  return TRUE;
}

/* enlarge the image by seam insertion
 * visibility map is updated and the resulting multisize image
 * is complete in both directions */
gboolean
lqr_raster_inflate (LqrRaster * r, gint l)
{
  gint w1, z0, vs, k;
  gint x, y;
  gint c_left;
  guchar *new_rgb;
  gint *new_vs;
  gdouble *new_bias = NULL;

#ifdef __LQR_VERBOSE__
  printf("  [ inflating (aux=%i) ]\n", r->aux); fflush(stdout);
#endif /* __LQR_VERBOSE__ */

#ifdef __LQR_DEBUG__
  assert (l + 1 > r->max_level);        /* otherwise is useless */
#endif // __LQR_DEBUG__

  /* scale to current maximum size
   * (this is the original size the first time) */
  lqr_raster_set_width (r, r->w0);

  /* final width */
  w1 = r->w0 + l - r->max_level + 1;

  /* allocate room for new maps */
  TRY_N_F (new_rgb = g_try_new0 (guchar, w1 * r->h0 * r->bpp));
  TRY_N_F (new_vs = g_try_new0 (gint, w1 * r->h0));
  if (r->aux == 0)
    {
      TRY_N_F (new_bias = g_try_new0 (gdouble, w1 * r->h0));
    }

  /* span the image with a cursor
   * and build the new image */
  lqr_cursor_reset (r->c);
  x = 0;
  y = 0;
  for (z0 = 0; z0 < w1 * r->h0; z0++, lqr_cursor_next (r->c))
    {
      /* read visibility */
      vs = r->vs[r->c->now];
      if ((vs != 0) && (vs <= l + r->max_level - 1)
          && (vs >= 2 * r->max_level - 1))
        {
          /* the point belongs to a previously computed seam
           * and was not inserted during a previous
           * inflate() call : insert another seam */
		
          //newmap[z0] = *(r->c->now);
	  
          /* the new pixel value is equal to the average of its
           * left and right neighbors */
          if (r->c->x > 0)
            {
	      c_left = lqr_cursor_left(r->c);
	    }
	  else
	    {
	      c_left = r->c->now;
	    }
          for (k = 0; k < r->bpp; k++)
            {
              new_rgb[z0 * r->bpp + k] =
                (r->rgb[c_left * r->bpp + k] + r->rgb[r->c->now * r->bpp + k]) / 2;
            }
	  if (r->aux == 0)
	    {
	      new_bias[z0] = (r->bias[c_left] + r->bias[r->c->now]) / 2;
	    }
          /* the first time inflate() is called
           * the new visibility should be -vs + 1 but we shift it
           * so that the final minimum visibiliy will be 1 again
           * and so that vs=0 still means "uninitialized"
           * subsequent inflations have to account for that */
          new_vs[z0] = l - vs + r->max_level;
          z0++;
        }
      for (k = 0; k < r->bpp; k++)
	{
      	  new_rgb[z0 * r->bpp + k] = r->rgb[r->c->now * r->bpp + k];
	}
      if (r->aux == 0)
        {
          new_bias[z0] = r->bias[r->c->now];
	}
      if (vs != 0)
        {
          /* visibility has to be shifted up */
          new_vs[z0] = vs + l - r->max_level + 1;
        }
      else if (r->raw != NULL)
        {
#ifdef __LQR_DEBUG__
          assert (y < r->h_start);
          assert (x < r->w_start - l);
#endif // __LQR_DEBUG__
          r->raw[y][x] = z0;
          x++;
          if (x >= r->w_start - l)
            {
              x = 0;
              y++;
            }
        }
    }

#ifdef __LQR_DEBUG__
  if (r->raw != NULL)
    {
      assert (x == 0);
      if (w1 != 2 * r->w_start - 1)
        {
          assert ((y == r->h_start) || (printf("y=%i hst=%i w1=%i\n", y, r->h_start, w1) && fflush(stdout) && 0) );
	}
    }
#endif // __LQR_DEBUG__

  /* substitute maps */
  g_free (r->rgb);
  g_free (r->vs);
  g_free (r->en);
  g_free (r->bias);
  g_free (r->m);

  r->rgb = new_rgb;
  r->vs = new_vs;
  if (r->aux == 0)
    {
      r->bias = new_bias;
      TRY_N_F (r->en = g_try_new0 (gdouble, w1 * r->h0));
      TRY_N_F (r->m = g_try_new0 (gdouble, w1 * r->h0));
    }

  /* set new widths & levels (w_start is kept for reference) */
  r->level = l + 1;
  r->w0 = w1;
  r->w = r->w_start;

  /* reset seam path and cursors */
  lqr_cursor_destroy (r->c);
  r->c = lqr_cursor_create (r, r->vs);

#ifdef __LQR_VERBOSE__
  printf("  [ inflating OK ]\n"); fflush(stdout);
#endif /* __LQR_VERBOSE__ */

  return TRUE;
}

/*** internal functions for maps computations ***/

/* compute energy at x, y */
void
lqr_raster_compute_e (LqrRaster * r, gint x, gint y)
{
  gdouble gx, gy;
  gint data;

  if (y == 0)
    {
      gy = lqr_raster_read (r, x, y + 1) - lqr_raster_read (r, x, y);
    }
  else if (y < r->h - 1)
    {
      gy =
        (lqr_raster_read (r, x, y + 1) - lqr_raster_read (r, x, y - 1)) / 2;
    }
  else
    {
      gy = lqr_raster_read (r, x, y) - lqr_raster_read (r, x, y - 1);
    }

  if (x == 0)
    {
      gx = lqr_raster_read (r, x + 1, y) - lqr_raster_read (r, x, y);
    }
  else if (x < r->w - 1)
    {
      gx =
        (lqr_raster_read (r, x + 1, y) - lqr_raster_read (r, x - 1, y)) / 2;
    }
  else
    {
      gx = lqr_raster_read (r, x, y) - lqr_raster_read (r, x - 1, y);
    }
  data = r->raw[y][x];
  r->en[data] = (*(r->gf)) (gx, gy) + r->bias[data] / r->w_start;
}

/* update energy map after seam removal
 * (the only affected energies are to the
 * left and right of the removed seam) */
void
lqr_raster_update_emap (LqrRaster * r)
{
  gint x, y;
  gint dx;

  for (y = 0; y < r->h; y++)
    {
      x = r->vpath_x[y];
      for (dx = -1; dx < 1; dx++)
        {
          if ((x + dx < 0) || (x + dx >= r->w))
            {
              continue;
            }
          lqr_raster_compute_e (r, x + dx, y);
        }
    }
}


void
lqr_raster_update_mmap (LqrRaster * r)  /* BUGGGGGY if r->delta_x > 1 */
{
  gint x, y;
  gint x_min, x_max;
  gint x1;
  gint x1_min, x1_max;
  gint data, data_down;
  gdouble m;
  gint stop;

  /* span first row */
  x_min = MAX (r->vpath_x[0] - r->delta_x, 0);
  x_max = MIN (r->vpath_x[0] + r->delta_x - 1, r->w - 1);

  for (x = x_min; x <= x_max; x++)
    {
      r->m[r->raw[0][x]] = r->en[r->raw[0][x]];
    }

  for (y = 1; y < r->h; y++)
    {
      x_min = MIN (x_min, r->vpath_x[y]);
      x_max = MAX (x_max, r->vpath_x[y] - 1);

      x_min = MAX (x_min - r->delta_x, 0);
      x_max = MIN (x_max + r->delta_x, r->w - 1);

      stop = 0;
      for (x = x_min; x <= x_max; x++)
        {
          data = r->raw[y][x];

	  x1_min = MAX(-x, -r->delta_x);
	  x1_max = MIN(r->w - 1 - x, r->delta_x);
          data_down = r->raw[y - 1][x + x1_min];
          m = r->m[data_down];
          for (x1 = x1_min + 1; x1 <= x1_max; x1++) 
            {
              data_down = r->raw[y - 1][x + x1];
              /* find the min among the neighbors
               * in the last row */
	      if (r->rigidity) {
		      m = MIN(m, r->m[data_down] + r->rigidity_map[x1]);
	      } else {
		      m = MIN(m, r->m[data_down]);
	      }
            }

	  if (r->fast_update == 1) {
		  if ((x == x_min) && (x < r->vpath_x[y]) && (r->m[data] == r->en[data] + m))
		    {
		      x_min++;
		    }
		  if ((x >= r->vpath_x[y]) && (r->m[data] == r->en[data] + m))
		    {
		      stop = 1;
		      x_max = x;
		    }
	  }


          /* set current m */
          r->m[data] = r->en[data] + m;

#ifdef __LQR_DEBUG__
          assert (m < 1 << 29);
#endif // __LQR_DEBUG__

	  if (stop)
	    {
	      break;
	    }
        }

    }
}


/* compute seam path from minpath map */
void
lqr_raster_build_vpath (LqrRaster * r)
{
  gint x, y, z0;
  gdouble m, m1;
  gint last = -1;
  gint last_x = 0;
  gint x_min, x_max;

  /* we start at last row */
  y = r->h - 1;

  /* span the last row for the minimum mmap value */
  m = (1 << 29);
  for (x = 0, z0 = y * r->w_start; x < r->w; x++, z0++)
    {
#ifdef __LQR_DEBUG__
      assert (r->vs[r->raw[y][x]] == 0);
#endif /* __LQR_DEBUG__ */

      m1 = r->m[r->raw[y][x]];
      if (m1 < m)
        {
          last = r->raw[y][x];
          last_x = x;
          m = m1;
        }
    }

#ifdef __LQR_DEBUG__
  assert(last >= 0);
#endif /* __LQR_DEBUG__ */

  /* we backtrack the seam following the min mmap */
  for (y = r->h0 - 1; y >= 0; y--)
    {
#ifdef __LQR_DEBUG__
      assert (r->vs[last] == 0);
      assert (last_x < r->w);
#endif // __LQR_DEBUG__

      r->vpath[y] = last;
      r->vpath_x[y] = last_x;
      if (y > 0) {
	      m = (1<<29);
	      x_min = MAX(0, last_x - r->delta_x);
	      x_max = MIN(r->w - 1, last_x + r->delta_x);
	      for (x = x_min; x <= x_max; x++) {
		      m1 = r->m[r->raw[y - 1][x]];
		      if (m1 < m) {
			      last = r->raw[y - 1][x];
			      last_x = x;
			      m = m1;
		      }
	      }
      }
    }
}

/* update visibility map after seam computation */
void
lqr_raster_update_vsmap (LqrRaster * r, gint l)
{
  gint y;
  for (y = 0; y < r->h; y++)
    {
#ifdef __LQR_DEBUG__
      assert (r->vs[r->vpath[y]] == 0);
      assert (r->vpath[y] == r->raw[y][r->vpath_x[y]]);
#endif // __LQR_DEBUG__
      r->vs[r->vpath[y]] = l;
    }
}

/* complete visibility map (last seam) */
/* set the last column of pixels to vis. level w0 */
void
lqr_raster_finish_vsmap (LqrRaster * r)
{
  gint y;

#ifdef __LQR_DEBUG__
  assert (r->w == 1);
#endif // __LQR_DEBUG__
  lqr_cursor_reset (r->c);
  for (y = 1; y <= r->h; y++, lqr_cursor_next (r->c))
    {
#ifdef __LQR_DEBUG__
      assert (r->vs[r->c->now] == 0);
#endif // __LQR_DEBUG__
      r->vs[r->c->now] = r->w0;
    }
}

void
lqr_raster_copy_vsmap (LqrRaster * r, LqrRaster * dest)
{
  gint x, y;
#ifdef __LQR_DEBUG__
  assert (r->w0 == dest->w0);
  assert (r->h0 == dest->h0);
#endif // __LQR_DEBUG__
  for (y = 0; y < r->h0; y++)
    {
      for (x = 0; x < r->w0; x++)
        {
          dest->vs[y * r->w0 + x] = r->vs[y * r->w0 + x];
        }
    }
}


/*** image manipulations ***/

/* set width of the multisize image
 * (maps have to be computed already) */
void
lqr_raster_set_width (LqrRaster * r, gint w1)
{
#ifdef __LQR_DEBUG__
  assert (w1 <= r->w0);
  assert (w1 >= r->w_start - r->max_level + 1);
#endif // __LQR_DEBUG__
  r->w = w1;
  r->level = r->w0 - w1 + 1;
}



/* flatten the image to its current state
 * (all maps are reset, invisible points are lost) */
gboolean
lqr_raster_flatten (LqrRaster * r)
{
  guchar *new_rgb;
  gdouble *new_bias = NULL;
  gint x, y, k;
  gint z0;

#ifdef __LQR_VERBOSE__
  printf("    [ flattening (aux=%i) ]\n", r->aux); fflush(stdout);
#endif /* __LQR_VERBOSE__ */

  /* free non needed maps first */
  g_free (r->en);
  g_free (r->m);

  /* allocate room for new map */
  TRY_N_F (new_rgb = g_try_new0 (guchar, r->w * r->h * r->bpp));
  if (r->aux == 0)
    {
       TRY_N_F (new_bias = g_try_new0 (gdouble, r->w * r->h));

       g_free (r->_raw);
       g_free (r->raw);
       TRY_N_F (r->_raw = g_try_new (gint, r->w * r->h));
       TRY_N_F (r->raw = g_try_new (gint*, r->h));
    }


  /* span the image with the cursor and copy
   * it in the new array  */
  lqr_cursor_reset (r->c);
  for (y = 0; y < r->h; y++)
    {
      if (r->aux == 0) {
        r->raw[y] = r->_raw + y * r->w;
      }
      for (x = 0; x < r->w; x++)
        {
          z0 = y * r->w + x;
	  for (k = 0; k < r->bpp; k++)
            {
              new_rgb[z0 * r->bpp + k] = r->rgb[r->c->now * r->bpp + k];
	    }
	  if (r->aux == 0)
	    {
	      new_bias[z0] = r->bias[r->c->now];
              r->raw[y][x] = z0;
	    }
          lqr_cursor_next (r->c);
        }
    }

  /* substitute the old maps */
  g_free (r->rgb);
  r->rgb = new_rgb;
  if (r->aux == 0)
    {
      g_free (r->bias);
      r->bias = new_bias;
    }

  /* init the other maps */
  g_free (r->vs);
  TRY_N_F (r->vs = g_try_new0 (gint, r->w * r->h));
  if (r->aux == 0)
    {
      TRY_N_F (r->en = g_try_new0 (gdouble, r->w * r->h));
      TRY_N_F (r->m = g_try_new0 (gdouble, r->w * r->h));
    }

  /* reset widths, heights & levels */
  r->w0 = r->w;
  r->h0 = r->h;
  r->w_start = r->w;
  r->h_start = r->h;
  r->level = 1;
  r->max_level = 1;

  /* reset seam path and cursors */
  lqr_cursor_destroy (r->c);
  r->c = lqr_cursor_create (r, r->vs);

#ifdef __LQR_VERBOSE__
  printf("    [ flattening OK ]\n"); fflush(stdout);
#endif /* __LQR_VERBOSE__ */

  return TRUE;
}

/* transpose the image, in its current state
 * (all maps and invisible points are lost) */
gboolean
lqr_raster_transpose (LqrRaster * r)
{
  gint x, y, k;
  gint z0, z1;
  gint d;
  guchar *new_rgb;
  gdouble *new_bias = NULL;

#ifdef __LQR_VERBOSE__
  printf("[ transposing (aux=%i) ]\n", r->aux); fflush(stdout);
#endif /* __LQR_VERBOSE__ */

  if (r->level > 1)
    {
      TRY_F_F (lqr_raster_flatten (r));
    }

  /* free non needed maps first */
  g_free (r->vs);
  g_free (r->en);
  g_free (r->m);

  /* allocate room for the new maps */
  TRY_N_F (new_rgb = g_try_new0 (guchar, r->w0 * r->h0 * r->bpp));
  if (r->aux == 0)
    {
      TRY_N_F (new_bias = g_try_new0 (gdouble, r->w0 * r->h0));

      g_free (r->_raw);
      g_free (r->raw);
      TRY_N_F (r->_raw = g_try_new0 (gint, r->h0 * r->w0));
      TRY_N_F (r->raw = g_try_new0 (gint*, r->w0));
    }

  /* compute trasposed maps */
  for (x = 0; x < r->w; x++)
    {
      if (r->aux == 0)
        {
          r->raw[x] = r->_raw + x * r->h0;
	}
      for (y = 0; y < r->h; y++)
        {
          z0 = y * r->w0 + x;
          z1 = x * r->h0 + y;
	  for (k = 0; k < r->bpp; k++)
	    {
              new_rgb[z1 * r->bpp + k] = r->rgb[z0 * r->bpp + k];
	    }
	  if (r->aux == 0)
	    {
	      new_bias[z1] = r->bias[z0];
              r->raw[x][y] = z1;
	    }
        }
    }

  /* substitute the map */
  g_free (r->rgb);
  r->rgb = new_rgb;
  if (r->aux == 0)
    {
      g_free (r->bias);
      r->bias = new_bias;
    }

  /* init the other maps */
  TRY_N_F (r->vs = g_try_new0 (gint, r->w0 * r->h0));
  if (r->aux == 0)
    {
      TRY_N_F (r->en = g_try_new0 (gdouble, r->w0 * r->h0));
      TRY_N_F (r->m = g_try_new0 (gdouble, r->w0 * r->h0));
    }

  /* switch widths & heights */
  d = r->w0;
  r->w0 = r->h0;
  r->h0 = d;
  r->w = r->w0;
  r->h = r->h0;

  /* reset w_start, h_start & levels */
  r->w_start = r->w0;
  r->h_start = r->h0;
  r->level = 1;
  r->max_level = 1;

  /* reset seam path and cursors */
  if (r->aux == 0)
    {
      g_free (r->vpath);
      TRY_N_F (r->vpath = g_try_new (gint, r->h));
      g_free (r->vpath_x);
      TRY_N_F (r->vpath_x = g_try_new (gint, r->h));
    }
  lqr_cursor_destroy (r->c);
  r->c = lqr_cursor_create (r, r->vs);

  /* rescale rigidity */

  if (r->aux == 0) {
	  for (x = -r->delta_x; x <= r->delta_x; x++)
	    {
	      r->rigidity_map[x] = (gdouble) r->rigidity * r->w0 / r->h0;
	    }
  }

  /* set transposed flag */
  r->transposed = (r->transposed ? 0 : 1);

  /* call transpose on auxiliary layers */
  if (r->resize_aux_layers == TRUE)
    {
      if (r->pres_raster != NULL)
        {
          TRY_F_F (lqr_raster_transpose (r->pres_raster));
        }
      if (r->disc_raster != NULL)
        {
          TRY_F_F (lqr_raster_transpose (r->disc_raster));
        }
    }

#ifdef __LQR_VERBOSE__
  printf("[ transpose OK ]\n"); fflush(stdout);
#endif /* __LQR_VERBOSE__ */

  return TRUE;
}


/* liquid resize : this is the main method
 * it automatically determines the depth of the map
 * according to the desired size */
gboolean
lqr_raster_resize (LqrRaster * r, gint w1, gint h1)
{
  /* resize width */
  gint delta, gamma;
  if (!r->transposed)
    {
      delta = w1 - r->w_start;
      gamma = w1 - r->w;
    }
  else
    {
      delta = w1 - r->h_start;
      gamma = w1 - r->h;
    }
  delta = delta > 0 ? delta : -delta;
  if (gamma)
    {
      if (r->transposed)
        {
          TRY_F_F (lqr_raster_transpose (r));
        }
      gimp_progress_init (_("Resizing width..."));
      TRY_F_F (lqr_raster_build_maps (r, delta + 1));
      lqr_raster_set_width (r, w1);
      if (r->resize_aux_layers == TRUE)
        {
          if (r->pres_raster != NULL)
            {
              lqr_raster_set_width (r->pres_raster, w1);
            }
          if (r->disc_raster != NULL)
            {
              lqr_raster_set_width (r->disc_raster, w1);
            }
        }
      if (r->output_seams)
        {
          TRY_F_F (lqr_external_write_vs (r));
        }
    }

  /* resize height */
  if (!r->transposed)
    {
      delta = h1 - r->h_start;
      gamma = h1 - r->h;
    }
  else
    {
      delta = h1 - r->w_start;
      gamma = h1 - r->w;
    }
  delta = delta > 0 ? delta : -delta;
  if (gamma)
    {
      if (!r->transposed)
        {
          TRY_F_F (lqr_raster_transpose (r));
        }
      gimp_progress_init (_("Resizing height..."));
      TRY_F_F (lqr_raster_build_maps (r, delta + 1));
      lqr_raster_set_width (r, h1);
      if (r->resize_aux_layers == TRUE)
        {
          if (r->pres_raster != NULL)
            {
              lqr_raster_set_width (r->pres_raster, h1);
            }
          if (r->disc_raster != NULL)
            {
              lqr_raster_set_width (r->disc_raster, h1);
            }
        }
      if (r->output_seams)
        {
          TRY_F_F (lqr_external_write_vs (r));
        }
    }

  return TRUE;
}


/**** END OF LQR_RASTER CLASS FUNCTIONS ****/
