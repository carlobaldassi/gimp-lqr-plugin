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
#include "lqr_data.h"
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
                gboolean output_seams,
                GimpRGB seam_color_start, GimpRGB seam_color_end)
{
  LqrRaster *r;
  gint dx;

  TRY_N_N (r = g_try_new (LqrRaster, 1));

  r->image_ID = image_ID;
  strncpy (r->name, name, LQR_MAX_NAME_LENGTH);

  r->level = 1;
  r->max_level = 1;
  r->transposed = 0;
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

  g_assert (r->bpp <= _LQR_DATA_MAX_BPP);

  r->w0 = r->w;
  r->h0 = r->h;
  r->w_start = r->w;
  r->h_start = r->h;

  /* allocate memory for internal structures */
  TRY_N_N (r->map = g_try_new (LqrData, r->w * r->h));

  TRY_N_N (r->raw = g_try_new (LqrData *, r->h_start * r->w_start));


  TRY_N_N (r->vpath = g_try_new (LqrData *, r->h));
  TRY_N_N (r->vpath_x = g_try_new (gint, r->h));

  /* initialize cursors */

  TRY_N_N (r->c = lqr_cursor_create (r, r->map));

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
      r->rigidity_map[dx] = (gdouble) r->rigidity * dx * dx;
    }
  r->rigidity_map[0] -= 1e-5;

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
  r->rigidity = 0;
  r->resize_aux_layers = FALSE;
  r->output_seams = FALSE;
  r->pres_raster = NULL;
  r->disc_raster = NULL;

  r->raw = NULL;
  r->vpath = NULL;
  r->vpath_x = NULL;
  r->rigidity_map = NULL;

  r->h = gimp_drawable_height (drawable->drawable_id);
  r->w = gimp_drawable_width (drawable->drawable_id);
  r->bpp = gimp_drawable_bpp (drawable->drawable_id);

  g_assert (r->bpp <= _LQR_DATA_MAX_BPP);

  r->w0 = r->w;
  r->h0 = r->h;
  r->w_start = r->w;
  r->h_start = r->h;

  /* allocate memory for internal structures */
  TRY_N_N (r->map = g_try_new (LqrData, r->w * r->h));

  /* initialize cursors */

  TRY_N_N (r->c = lqr_cursor_create (r, r->map));

  /* read input layer */
  TRY_F_N (lqr_external_readimage (r, drawable));

  return r;
}

/* destructor */
void
lqr_raster_destroy (LqrRaster * r)
{
  g_free (r->map);
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
  return lqr_data_read (r->raw[y * r->w_start + x], r->bpp);
}

void
lqr_raster_carve (LqrRaster * r)
{
  gint x, y, z0;

  for (y = 0; y < r->h_start; y++)
    {
#ifdef __LQR_DEBUG__
      assert (r->raw[y * r->w_start + r->vpath_x[y]]->vs != 0);
      for (x = 0; x < r->vpath_x[y]; x++)
        {
          z0 = y * r->w_start + x;
          assert (r->raw[z0]->vs == 0);
        }
#endif // __LQR_DEBUG__
      for (x = r->vpath_x[y]; x < r->w; x++)
        {
          z0 = y * r->w_start + x;
          r->raw[z0] = r->raw[z0 + 1];
#ifdef __LQR_DEBUG__
          assert (r->raw[z0]->vs == 0);
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
      //printf("      y=%i\n", y); fflush(stdout);
      for (x = 0; x < r->w; x++)
        {
          //printf("       x=%i\n", x); fflush(stdout);
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
  LqrData *data;
  LqrData *data_down;
  gint x1;
  double m, m1;

  /* span first row */
  for (x = 0; x < r->w; x++)
    {
      data = r->raw[x];
#ifdef __LQR_DEBUG__
      assert (data->vs == 0);
#endif //__LQR_DEBUG__
      data->m = data->e;
      data->least = NULL;
      data->least_x = 0;
    }

  /* span all other rows */
  for (y = 1; y < r->h; y++)
    {
      for (x = 0; x < r->w; x++)
        {
          data = r->raw[(y * r->w_start) + x];
#ifdef __LQR_DEBUG__
          assert (data->vs == 0);
#endif //__LQR_DEBUG__
          m = (1 << 29);
          for (x1 = MAX (-x, -r->delta_x);
               x1 <= MIN (r->w - 1 - x, r->delta_x); x1++)
            {
              data_down = r->raw[((y - 1) * r->w_start) + x + x1];
              /* find the min among the neighbors
               * in the last row */
              m1 = data_down->m + r->rigidity_map[x1] / r->h;
              if (m1 < m)
                {
                  m = m1;
                  data->least = data_down;
                  data->least_x = x1;
                }
            }
#ifdef __LQR_DEBUG__
          assert (m < 1 << 29);
#endif // __LQR_DEBUG__

          /* set current m */
          data->m = data->e + m;
        }
    }
}

/* compute (vertical) visibility map up to given depth
 * (it also calls inflate() to add image enlargment information) */
gboolean
lqr_raster_build_vsmap (LqrRaster * r, gint depth)
{
  gint z, l;
  gint update_step;

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
  if (r->max_level == 1)
    {
      for (z = 0; z < r->w0 * r->h0; z++)
        {
          r->map[z].vs = 0;
        }
    }

  /* cycle over levels */
  for (l = r->max_level; l < depth; l++)
    {

      update_step = MAX ((depth - r->max_level) / 50, 1);
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

  return TRUE;
}

/* enlarge the image by seam insertion
 * visibility map is updated and the resulting multisize image
 * is complete in both directions */
gboolean
lqr_raster_inflate (LqrRaster * r, gint l)
{
  gint w1, z0, vs, k;
  gint z1, x, y;
  LqrData *newmap;

#ifdef __LQR_DEBUG__
  assert (l + 1 > r->max_level);        /* otherwise is useless */
#endif // __LQR_DEBUG__

  /* scale to current maximum size
   * (this is the original size the first time) */
  lqr_raster_set_width (r, r->w0);

  /* final width */
  w1 = r->w0 + l - r->max_level + 1;

  /* allocate room for new map */
  TRY_N_F (newmap = g_try_new0 (LqrData, w1 * r->h0));

  /* span the image with a cursor
   * and build the new image */
  lqr_cursor_reset (r->c);
  x = 0;
  y = 0;
  for (z0 = 0; z0 < w1 * r->h0; z0++, lqr_cursor_next (r->c))
    {
      /* read visibility */
      vs = r->c->now->vs;
      if ((vs != 0) && (vs <= l + r->max_level - 1)
          && (vs >= 2 * r->max_level - 1))
        {
          /* the point belongs to a previously computed seam
           * and was not inserted during a previous
           * inflate() call : insert another seam */
          newmap[z0] = *(r->c->now);
          /* the new pixel value is equal to the average of its
           * left and right neighbors */
          if (r->c->x > 1)
            {
              for (k = 0; k < r->bpp; k++)
                {
                  newmap[z0].rgb[k] =
                    (lqr_cursor_left (r->c)->rgb[k] + r->c->now->rgb[k]) / 2;
                }
            }
          /* the first time inflate() is called
           * the new visibility should be -vs + 1 but we shift it
           * so that the final minimum visibiliy will be 1 again
           * and so that vs=0 still means "uninitialized"
           * subsequent inflations have to account for that */
          newmap[z0].vs = l - vs + r->max_level;
          z0++;
        }
      newmap[z0] = *(r->c->now);
      if (vs != 0)
        {
          /* visibility has to be shifted up */
          newmap[z0].vs = vs + l - r->max_level + 1;
        }
      else if (r->raw != NULL)
        {
          z1 = y * r->w_start + x;
#ifdef __LQR_DEBUG__
          assert (y < r->h_start);
          assert (x < r->w_start - l);
          assert (z1 <= z0);
#endif // __LQR_DEBUG__
          r->raw[z1] = newmap + z0;
          x++;
          if (x >= r->w_start - l)
            {
              x = 0;
              y++;
            }
        }
    }

#ifdef __LQR_DEBUG__
  assert (x == 0);
  assert (y == r->h_start);
#endif // __LQR_DEBUG__

  /* substitute maps */
  g_free (r->map);
  r->map = newmap;

  /* set new widths & levels (w_start is kept for reference) */
  r->level = l + 1;
  r->w0 = w1;
  r->w = r->w_start;

  /* reset seam path and cursors */
  lqr_cursor_destroy (r->c);
  r->c = lqr_cursor_create (r, r->map);

  return TRUE;
}

/*** internal functions for maps computations ***/

/* compute energy at x, y */
void
lqr_raster_compute_e (LqrRaster * r, gint x, gint y)
{
  double gx, gy;
  LqrData *data;
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
  data = r->raw[y * r->w_start + x];
  data->e = (*(r->gf)) (gx, gy) + data->b / r->w_start;
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
  LqrData *data, *data_down;
  double m, m1;
  LqrData *old_least;
  gint old_least_x;

  /* span first row */
  x_min = MAX (r->vpath_x[0] - 1, 0);
  x_max = MIN (r->vpath_x[0], r->w - 1);

  for (x = x_min; x <= x_max; x++)
    {
      r->raw[x]->m = r->raw[x]->e;
    }

  for (y = 1; y < r->h; y++)
    {
      x_min = MIN (x_min, r->vpath_x[y]);
      x_max = MAX (x_max, r->vpath_x[y] - 1);

      x_min = MAX (x_min - r->delta_x, 0);
      x_max = MIN (x_max + r->delta_x, r->w - 1);

      for (x = x_min; x <= x_max; x++)
        {
          data = r->raw[y * r->w_start + x];

          old_least = data->least;
          old_least_x = data->least_x;
          m = (1 << 29);
          for (x1 = MAX (-x, -r->delta_x);
               x1 <= MIN (r->w - 1 - x, r->delta_x); x1++)
            {
              data_down = r->raw[((y - 1) * r->w_start) + x + x1];
              /* find the min among the neighbors
               * in the last row */
              m1 = data_down->m + r->rigidity_map[x1] / r->h;
              if (m1 < m)
                {
                  m = m1;
                  data->least = data_down;
                  data->least_x = x1;
                }
            }

          if ((x == x_min) && (x < r->vpath_x[y])
              && (data->least == old_least)
              && (data->least_x == old_least_x) && (data->m == data->e + m))
            {
              x_min++;
            }
          if ((x == x_max) && (x >= r->vpath_x[y])
              && (data->least == old_least)
              && (data->least_x == old_least_x) && (data->m == data->e + m))
            {
              x_max--;
            }


          /* set current m */
          data->m = data->e + m;

#ifdef __LQR_DEBUG__
          assert (m < 1 << 29);
#endif // __LQR_DEBUG__
        }

    }
}


/* compute seam path from minpath map */
void
lqr_raster_build_vpath (LqrRaster * r)
{
  gint x, y, z0;
  gdouble m, m1;
  LqrData *last = NULL;
  gint last_x = 0;

  /* we start at last row */
  y = r->h - 1;

  /* span the last row for the minimum mmap value */
  m = (1 << 29);
  for (x = 0; x < r->w; x++)
    {
      z0 = (y * r->w_start) + x;

#ifdef __LQR_DEBUG__
      assert (r->raw[z0]->vs == 0);
#endif // __LQR_DEBUG__

      m1 = r->raw[z0]->m;
      if (m1 < m)
        {
          last = r->raw[z0];
          last_x = x;
          m = m1;
        }
    }

  /* we backtrack the seam following the min mmap */
  for (y = r->h0 - 1; y >= 0; y--)
    {
#ifdef __LQR_DEBUG__
      assert (last->vs == 0);
      assert (last_x < r->w);
#endif // __LQR_DEBUG__
      r->vpath[y] = last;
      r->vpath_x[y] = last_x;
      last_x += last->least_x;
      last = last->least;
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
      assert (r->vpath[y]->vs == 0);
      assert (r->vpath[y] == r->raw[y * r->w_start + r->vpath_x[y]]);
#endif // __LQR_DEBUG__
      r->vpath[y]->vs = l;
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
      assert (r->c->now->vs == 0);
#endif // __LQR_DEBUG__
      r->c->now->vs = r->w0;
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
          dest->map[y * r->w0 + x].vs = r->map[y * r->w0 + x].vs;
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
  LqrData *newmap;
  gint x, y;
  gint z0;

  /* allocate room for new map */
  TRY_N_F (newmap = g_try_new0 (LqrData, r->w * r->h));

  g_free (r->raw);
  TRY_N_F (r->raw = g_try_new (LqrData *, r->w * r->h));

  /* span the image with the cursor and copy
   * it in the new array  */
  lqr_cursor_reset (r->c);
  for (y = 0; y < r->h; y++)
    {
      for (x = 0; x < r->w; x++)
        {
          z0 = y * r->w + x;
          newmap[z0] = *(r->c->now);
          newmap[z0].e = 0;
          newmap[z0].m = 0;
          newmap[z0].vs = 0;
          r->raw[z0] = newmap + z0;
          lqr_cursor_next (r->c);
        }
    }

  /* substitute the map */
  g_free (r->map);
  r->map = newmap;

  /* reset widths, heights & levels */
  r->w0 = r->w;
  r->h0 = r->h;
  r->w_start = r->w;
  r->h_start = r->h;
  r->level = 1;
  r->max_level = 1;

  /* reset seam path and cursors */
  lqr_cursor_destroy (r->c);
  r->c = lqr_cursor_create (r, r->map);

  return TRUE;
}

/* transpose the image, in its current state
 * (all maps and invisible points are lost) */
gboolean
lqr_raster_transpose (LqrRaster * r)
{
  gint x, y;
  gint z0, z1;
  gint d;
  LqrData *newmap;

  if (r->level > 1)
    {
      TRY_F_F (lqr_raster_flatten (r));
    }

  /* allocate room for the new map */
  TRY_N_F (newmap = g_try_new0 (LqrData, r->w0 * r->h0));

  g_free (r->raw);
  TRY_N_F (r->raw = g_try_new0 (LqrData *, r->h0 * r->w0));

  /* compute trasposed map */
  for (x = 0; x < r->w; x++)
    {
      for (y = 0; y < r->h; y++)
        {
          z0 = y * r->w0 + x;
          z1 = x * r->h0 + y;
          newmap[z1] = r->map[z0];
          newmap[z1].vs = 0;
          r->raw[z1] = newmap + z1;
        }
    }

  /* substitute the map */
  g_free (r->map);
  r->map = newmap;

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
  g_free (r->vpath);
  TRY_N_F (r->vpath = g_try_new (LqrData *, r->h));
  g_free (r->vpath_x);
  TRY_N_F (r->vpath_x = g_try_new (gint, r->h));
  lqr_cursor_destroy (r->c);
  r->c = lqr_cursor_create (r, r->map);

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
