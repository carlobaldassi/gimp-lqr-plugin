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

/**** LQR_CARVER CLASS FUNCTIONS ****/

/*** constructor & destructor ***/

/* constructor */
LqrCarver *
lqr_carver_new (gint32 image_ID, GimpDrawable * drawable, gchar * name,
                gint32 pres_layer_ID, gint pres_coeff,
                gint32 disc_layer_ID, gint disc_coeff,
                LqrGradFunc gf_ind, gint rigidity,
                gboolean resize_aux_layers,
                gboolean output_seams,
                GimpRGB seam_color_start, GimpRGB seam_color_end)
{
  LqrCarver *r;
  gint dx;

  TRY_N_N (r = g_try_new (LqrCarver, 1));

  r->image_ID = image_ID;
  strncpy (r->name, name, LQR_MAX_NAME_LENGTH);

  r->transposed = 0;
  r->aux = 0;
  r->rigidity = rigidity;
  r->resize_aux_layers = resize_aux_layers;
  r->output_seams = output_seams;
  r->seam_color_start = seam_color_start;
  r->seam_color_end = seam_color_end;
  r->pres_raster = NULL;
  r->disc_raster = NULL;

  r->h = gimp_drawable_height (drawable->drawable_id);
  r->w = gimp_drawable_width (drawable->drawable_id);
  r->bpp = gimp_drawable_bpp (drawable->drawable_id);

  r->w_start = r->w;
  r->h_start = r->h;

  /* allocate memory for internal structures */
  TRY_N_N (r->rgb = g_try_new (guchar, r->w * r->h * r->bpp));
  TRY_N_N (r->en = g_try_new (gdouble, r->w * r->h));
  TRY_N_N (r->bias = g_try_new (gdouble, r->w * r->h));
  TRY_N_N (r->m = g_try_new (gdouble, r->w * r->h));
  TRY_N_N (r->least_x = g_try_new (gint, r->w * r->h));

  TRY_N_N (r->vpath_x = g_try_new (gint, r->h));

  /* read input layer */
  TRY_F_N (lqr_carver_external_readimage (r, drawable));

  /* read bias layers */
  TRY_F_N (lqr_carver_external_readbias (r, pres_layer_ID, pres_coeff));
  TRY_F_N (lqr_carver_external_readbias (r, disc_layer_ID, -disc_coeff));

  /* rasterize bias layers */
  if (resize_aux_layers == TRUE)
    {
      if (pres_layer_ID != 0)
        {
          TRY_N_N (r->pres_raster =
                   lqr_carver_aux_new (image_ID,
                                       gimp_drawable_get
                                       (pres_layer_ID), ""));
        }
      if (disc_layer_ID != 0)
        {
          TRY_N_N (r->disc_raster =
                   lqr_carver_aux_new (image_ID,
                                       gimp_drawable_get
                                       (disc_layer_ID), ""));
        }
    }

  /* set gradient function */
  lqr_carver_set_gf (r, gf_ind);

  /* set rigidity map */
  r->delta_x = 1;               /* currently only 0 or 1 are meaningful values */

  r->rigidity_map = g_try_new0 (gdouble, 2 * r->delta_x + 1);
  r->rigidity_map += r->delta_x;
  for (dx = -r->delta_x; dx <= r->delta_x; dx++)
    {
      r->rigidity_map[dx] = (gdouble) r->rigidity * dx * dx;
    }
  r->rigidity_map[0] -= 1e-5;

  return r;
}

/* aux constructor (for preserve/discard layers) */
LqrCarver *
lqr_carver_aux_new (gint32 image_ID, GimpDrawable * drawable, gchar * name)
{
  LqrCarver *r;

  TRY_N_N (r = g_try_new (LqrCarver, 1));

  r->image_ID = image_ID;
  strncpy (r->name, name, LQR_MAX_NAME_LENGTH);

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
  r->least_x = NULL;
  r->vpath_x = NULL;
  r->rigidity_map = NULL;

  r->h = gimp_drawable_height (drawable->drawable_id);
  r->w = gimp_drawable_width (drawable->drawable_id);
  r->bpp = gimp_drawable_bpp (drawable->drawable_id);

  r->w_start = r->w;
  r->h_start = r->h;

  /* allocate memory for internal structures */
  TRY_N_N (r->rgb = g_try_new (guchar, r->w * r->h * r->bpp));

  /* read input layer */
  TRY_F_N (lqr_carver_external_readimage (r, drawable));

  return r;
}

/* destructor */
void
lqr_carver_destroy (LqrCarver * r)
{
  g_free (r->rgb);
  g_free (r->en);
  g_free (r->bias);
  g_free (r->m);
  g_free (r->least_x);

  g_free (r->vpath_x);
  if (r->pres_raster != NULL)
    {
      lqr_carver_destroy (r->pres_raster);
    }
  if (r->disc_raster != NULL)
    {
      lqr_carver_destroy (r->disc_raster);
    }
  if (r->rigidity_map != NULL)
    {
      r->rigidity_map -= r->delta_x;
      g_free (r->rigidity_map);
    }
  g_free (r);
}


/*** gradient related functions ***/

/* set the gradient function for energy computation 
 * choosing among those those listed in enum LqrGradFunc
 * the arguments are the x, y components of the gradient */
void
lqr_carver_set_gf (LqrCarver * r, LqrGradFunc gf_ind)
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
lqr_carver_read (LqrCarver * r, gint x, gint y)
{
  gdouble sum = 0;
  gint k;
  for (k = 0; k < r->bpp; k++)
    {
      sum += r->rgb[(y * r->w_start + x) * r->bpp + k];
    }
  return sum / (255 * r->bpp);
}

void
lqr_carver_carve (LqrCarver * r)
{
  gint x, y, z0;

  for (y = 0; y < r->h_start; y++)
    {
      for (x = r->vpath_x[y]; x < r->w; x++)
        {
          z0 = y * r->w_start + x;
          memcpy(r->rgb + (z0 * r->bpp), r->rgb + ((z0 + 1) * r->bpp), r->bpp * sizeof(guchar));
	  if (r->aux == 0)
	    {
	      r->en[z0] = r->en[z0 + 1];
	      r->m[z0] = r->m[z0 + 1];
	      r->bias[z0] = r->bias[z0 + 1];
	      r->least_x[z0] = r->least_x[z0 + 1];
	    }
        }
    }
}


/*** compute maps (energy, minpath & visibility ***/

void
lqr_carver_shrink (LqrCarver * r, gint w_new)
{
  gint update_step;
  if (w_new < r->w)
    {
      /* update step for progress */
      update_step = MAX ((r->w - w_new) / 50, 1);

      /* compute energy & minpath maps */
      lqr_carver_build_emap (r);
      lqr_carver_build_mmap (r);

      while (r->w > w_new)
        {
          if ((r->w - w_new) % update_step == 0)
            {
              gimp_progress_update ((gdouble) (r->w - w_new) /
                                    (gdouble) (r->w_start - w_new));
            }

          /* compute vertical seam */
          lqr_carver_build_vpath (r);

          r->w--;

          /* update raw data */
          lqr_carver_carve (r);

          if (r->w > 1)
            {
              /* update the energy */
              //lqr_carver_build_emap (r);
              lqr_carver_update_emap (r);

              /* recalculate the minpath map */
              //lqr_carver_update_mmap (r);
              lqr_carver_build_mmap (r);
            }

#if 0
          /* copy visibility maps on auxiliary layers */
          if (r->resize_aux_layers)
            {
              if (r->pres_raster != NULL)
                {
                  lqr_carver_copy_vpath (r, r->pres_raster);
                  r->pres_raster->w--;
                  lqr_carver_carve (r->pres_raster);
                }
              if (r->disc_raster != NULL)
                {
                  lqr_carver_copy_vpath (r, r->disc_raster);
                  r->disc_raster->w--;
                  lqr_carver_carve (r->disc_raster);
                }
            }
#endif
        }
    }
}

/* compute energy map */
void
lqr_carver_build_emap (LqrCarver * r)
{
  gint x, y;

  for (y = 0; y < r->h; y++)
    {
      for (x = 0; x < r->w; x++)
        {
          lqr_carver_compute_e (r, x, y);
        }
    }
}

/* compute auxiliary minpath map
 * defined as
 * y = 1 : m(x,y) = e(x,y)
 * y > 1 : m(x,y) = min{ m(x-1,y-1), m(x,y-1), m(x+1,y-1) } + e(x,y) */
void
lqr_carver_build_mmap (LqrCarver * r)
{
  gint x, y, x1;
  gint z0, z1;
  double m, m1;

  /* span first row */
  for (x = 0; x < r->w; x++)
    {
      r->m[x] = r->en[x];
      r->least_x[x] = 0;
    }

  /* span all other rows */
  for (y = 1; y < r->h; y++)
    {
      for (x = 0; x < r->w; x++)
        {
          z0 = (y * r->w_start) + x;
          m = (1 << 29);
          for (x1 = MAX (-x, -r->delta_x);
               x1 <= MIN (r->w - 1 - x, r->delta_x); x1++)
            {
	      z1 = ((y - 1) * r->w_start) + x + x1;
              /* find the min among the neighbors
               * in the last row */
              //m1 = r->m[z1] + r->rigidity_map[x1] / r->h;
              m1 = r->m[z1] ;//+ r->rigidity_map[x1] / r->h;
              if (m1 < m)
                {
                  m = m1;
                  r->least_x[z0] = x1;
                }
            }
#ifdef __LQR_DEBUG__
          assert (m < 1 << 29);
#endif // __LQR_DEBUG__

          /* set current m */
          r->m[z0] = r->en[z0] + m;
        }
    }
}

#if 0
/* enlarge the image by seam insertion
 * visibility map is updated and the resulting multisize image
 * is complete in both directions */
gboolean
lqr_carver_inflate (LqrCarver * r, gint l)
{
  gint w1, z0, vs, k;
  gint z1, x, y;
  gint c_left;
  guchar *new_rgb;
  gint *new_vs;
  gdouble *new_bias = NULL;

#ifdef __LQR_DEBUG__
  assert (l + 1 > r->max_level);        /* otherwise is useless */
#endif // __LQR_DEBUG__

  /* scale to current maximum size
   * (this is the original size the first time) */
  lqr_carver_set_width (r, r->w0);

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
          z1 = y * r->w_start + x;
#ifdef __LQR_DEBUG__
          assert (y < r->h_start);
          assert (x < r->w_start - l);
          assert (z1 <= z0);
#endif // __LQR_DEBUG__
          r->raw[z1] = z0;
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
      assert ((y == r->h_start) || (printf("y=%i hst=%i\n", y, r->h_start) && fflush(stdout) && 0) );
    }
#endif // __LQR_DEBUG__

  /* substitute maps */
  g_free (r->rgb);
  g_free (r->vs);
  g_free (r->en);
  g_free (r->bias);
  g_free (r->m);
  g_free (r->least);
  g_free (r->least_x);

  r->rgb = new_rgb;
  r->vs = new_vs;
  if (r->aux == 0)
    {
      r->bias = new_bias;
      TRY_N_F (r->en = g_try_new0 (gdouble, w1 * r->h0));
      TRY_N_F (r->m = g_try_new0 (gdouble, w1 * r->h0));
      TRY_N_F (r->least = g_try_new0 (gint, w1 * r->h0));
      TRY_N_F (r->least_x = g_try_new0 (gint, w1 * r->h0));
    }

  /* set new widths & levels (w_start is kept for reference) */
  r->level = l + 1;
  r->w0 = w1;
  r->w = r->w_start;

  /* reset seam path and cursors */
  lqr_cursor_destroy (r->c);
  r->c = lqr_cursor_create (r, r->vs);

  return TRUE;
}
#endif

/*** internal functions for maps computations ***/

/* compute energy at x, y */
void
lqr_carver_compute_e (LqrCarver * r, gint x, gint y)
{
  gdouble gx, gy;
  gint z0;

  if (y == 0)
    {
      gy = lqr_carver_read (r, x, y + 1) - lqr_carver_read (r, x, y);
    }
  else if (y < r->h - 1)
    {
      gy =
        (lqr_carver_read (r, x, y + 1) - lqr_carver_read (r, x, y - 1)) / 2;
    }
  else
    {
      gy = lqr_carver_read (r, x, y) - lqr_carver_read (r, x, y - 1);
    }

  if (x == 0)
    {
      gx = lqr_carver_read (r, x + 1, y) - lqr_carver_read (r, x, y);
    }
  else if (x < r->w - 1)
    {
      gx =
        (lqr_carver_read (r, x + 1, y) - lqr_carver_read (r, x - 1, y)) / 2;
    }
  else
    {
      gx = lqr_carver_read (r, x, y) - lqr_carver_read (r, x - 1, y);
    }
  z0 = y * r->w_start + x;
  r->en[z0] = (*(r->gf)) (gx, gy) + r->bias[z0] / r->w_start;
}

/* update energy map after seam removal
 * (the only affected energies are to the
 * left and right of the removed seam) */
void
lqr_carver_update_emap (LqrCarver * r)
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
          lqr_carver_compute_e (r, x + dx, y);
        }
    }
}


void
lqr_carver_update_mmap (LqrCarver * r)  /* BUGGGGGY if r->delta_x > 1 */
{
  gint x, y;
  gint x_min, x_max;
  gint x1;
  gint z0, z1;
  gdouble m, m1;
  gint old_least_x;

  /* span first row */
  x_min = MAX (r->vpath_x[0] - 1, 0);
  x_max = MIN (r->vpath_x[0], r->w - 1);

  for (x = x_min; x <= x_max; x++)
    {
      r->m[x] = r->en[x];
    }

  for (y = 1; y < r->h; y++)
    {
      x_min = MIN (x_min, r->vpath_x[y]);
      x_max = MAX (x_max, r->vpath_x[y] - 1);

      x_min = MAX (x_min - r->delta_x, 0);
      x_max = MIN (x_max + r->delta_x, r->w - 1);

      for (x = x_min; x <= x_max; x++)
        {
          z0 = y * r->w_start + x;

          old_least_x = r->least_x[z0];
          m = (1 << 29);
          for (x1 = MAX (-x, -r->delta_x);
               x1 <= MIN (r->w - 1 - x, r->delta_x); x1++)
            {
              z1 = ((y - 1) * r->w_start) + x + x1;
              /* find the min among the neighbors
               * in the last row */
              //m1 = r->m[z1] + r->rigidity_map[x1] / r->h;
              m1 = r->m[z1] ;//+ r->rigidity_map[x1] / r->h;
              if (m1 < m)
                {
                  m = m1;
                  r->least_x[z0] = x1;
                }
            }

          if ((x == x_min) && (x < r->vpath_x[y])
              && (r->least_x[z0] == old_least_x) && (r->m[z0] == r->en[z0] + m))
            {
              x_min++;
            }
          if ((x == x_max) && (x >= r->vpath_x[y])
              && (r->least_x[z0] == old_least_x) && (r->m[z0] == r->en[z0] + m))
            {
              x_max--;
            }


          /* set current m */
          r->m[z0] = r->en[z0] + m;

#ifdef __LQR_DEBUG__
          assert (m < 1 << 29);
#endif // __LQR_DEBUG__
        }

    }
}


/* compute seam path from minpath map */
void
lqr_carver_build_vpath (LqrCarver * r)
{
  gint x, y, z0;
  gdouble m, m1;
  gint last = -1;
  gint last_x = 0;

  /* we start at last row */
  y = r->h - 1;

  /* span the last row for the minimum mmap value */
  m = (1 << 29);
  for (x = 0; x < r->w; x++)
    {
      z0 = y * r->w_start + x;

      m1 = r->m[z0];
      if (m1 < m)
        {
          last = z0;
          last_x = x;
          m = m1;
        }
    }

  /* we backtrack the seam following the min mmap */
  for (y = r->h - 1; y >= 0; y--)
    {
      r->vpath_x[y] = last_x;
      last_x += r->least_x[last];
      if (y > 0)
        {
          last = (y - 1) * r->w_start + last_x;
	}
    }
}

void
lqr_carver_copy_vpath (LqrCarver * r, LqrCarver * dest)
{
#ifdef __LQR_DEBUG__
  assert (r->w_start == dest->w_start);
  assert (r->hstart == dest->h_start);
#endif // __LQR_DEBUG__
  memcpy(dest->vpath_x, r->vpath_x, r->h_start * sizeof(gint));
}


/*** image manipulations ***/

/* transpose the image */
gboolean
lqr_carver_transpose (LqrCarver * r)
{
  gint x, y;
  gint z0, z1;
  gint d;
  guchar *new_rgb;
  gdouble *new_bias = NULL;

  /* free non needed maps first */
  g_free (r->en);
  g_free (r->m);
  g_free (r->least_x);

  /* allocate room for the new maps */
  TRY_N_F (new_rgb = g_try_new0 (guchar, r->w * r->h * r->bpp));
  if (r->aux == 0)
    {
      TRY_N_F (new_bias = g_try_new0 (gdouble, r->w * r->h));
    }

  /* compute trasposed maps */
  for (x = 0; x < r->w; x++)
    {
      for (y = 0; y < r->h; y++)
        {
          z0 = y * r->w_start + x;
          z1 = x * r->h_start + y;
	  memcpy(new_rgb + z1 * r->bpp, r->rgb + z0 + r->bpp, r->bpp * sizeof(guchar));
	  if (r->aux == 0)
	    {
	      new_bias[z1] = r->bias[z1];
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
  if (r->aux == 0)
    {
      TRY_N_F (r->en = g_try_new0 (gdouble, r->w * r->h));
      TRY_N_F (r->m = g_try_new0 (gdouble, r->w * r->h));
      TRY_N_F (r->least_x = g_try_new0 (gint, r->w * r->h));
    }

  /* switch widths & heights */
  d = r->w_start;
  r->w_start = r->h_start;
  r->h_start = d;
  r->w = r->w_start;
  r->h = r->h_start;

  /* reset seam path and cursors */
  if (r->aux == 0)
    {
      g_free (r->vpath_x);
      TRY_N_F (r->vpath_x = g_try_new (gint, r->h));
    }

  /* set transposed flag */
  r->transposed = (r->transposed ? 0 : 1);

  /* call transpose on auxiliary layers */
  if (r->resize_aux_layers == TRUE)
    {
      if (r->pres_raster != NULL)
        {
          TRY_F_F (lqr_carver_transpose (r->pres_raster));
        }
      if (r->disc_raster != NULL)
        {
          TRY_F_F (lqr_carver_transpose (r->disc_raster));
        }
    }
  return TRUE;
}


/* liquid resize : this is the main method */
gboolean
lqr_carver_resize (LqrCarver * r, gint w1, gint h1)
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
          TRY_F_F (lqr_carver_transpose (r));
        }
      gimp_progress_init (_("Resizing width..."));
      lqr_carver_shrink (r, w1);
#if 0
      if (r->output_seams)
        {
          TRY_F_F (lqr_external_write_vs (r));
        }
#endif
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
          TRY_F_F (lqr_carver_transpose (r));
        }
      gimp_progress_init (_("Resizing height..."));
      lqr_carver_shrink (r, h1);
#if 0
      if (r->output_seams)
        {
          TRY_F_F (lqr_external_write_vs (r));
        }
#endif
    }

  return TRUE;
}


/**** END OF LQR_CARVER CLASS FUNCTIONS ****/
