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

#include "config.h"

#include <gtk/gtk.h>

#include <libgimp/gimp.h>

#include <lqr.h>

#include "io_functions.h"

#include "main.h"
#include "render.h"
#include "plugin-intl.h"


#if 0
#define __CLOCK_IT__
#endif

#define MEMCHECK(x) if ((x) == FALSE) { g_message(_("Not enough memory")); return FALSE; }
#define MEMCHECK1(x) if ((x) == LQR_NOMEM) { g_message(_("Not enough memory")); return FALSE; }

gboolean
my_progress_end (const gchar * message)
{
  return gimp_progress_end ();
}

gboolean
render (gint32 image_ID,
        GimpDrawable * drawable,
        PlugInVals * vals,
        PlugInImageVals * image_vals, PlugInDrawableVals * drawable_vals,
        PlugInColVals * col_vals)
{
  LqrCarver *carver, *aux_carver;
  LqrCarverList *carver_list;
  gint32 mask_ID;
  gint32 layer_ID;
  gchar layer_name[LQR_MAX_NAME_LENGTH];
  gchar new_layer_name[LQR_MAX_NAME_LENGTH];
  guchar *rgb_buffer;
  gboolean alpha_lock;
  gboolean alpha_lock_pres = FALSE, alpha_lock_disc = FALSE;
  gint ntiles;
  gint old_width, old_height;
  gint new_width, new_height;
  gint bpp;
  gint x_off, y_off, aux_x_off, aux_y_off;
  GimpDrawable *drawable_pres, *drawable_disc;
  GimpRGB colour_start, colour_end;
  LqrProgress *lqr_progress;
#ifdef __CLOCK_IT__
  double clock1, clock2, clock3, clock4;
#endif /* __CLOCK_IT__ */

  if (drawable_vals->layer_ID)
    {
      layer_ID = drawable_vals->layer_ID;
    }
  else
    {
      layer_ID = gimp_image_get_active_layer (image_ID);
    }


  if (!gimp_drawable_is_valid (layer_ID))
    {
      g_message (_("Error: it seems that the selected layer "
                   "is no longer valid"));
      return FALSE;
    }

  if (vals->pres_layer_ID != 0)
    {
      if (!gimp_drawable_is_valid (vals->pres_layer_ID))
        {
          g_message (_("Error: it seems that the preservation "
                       "features layer is no longer valid"));
          return FALSE;
        }
    }

  if (vals->disc_layer_ID != 0)
    {
      if (!gimp_drawable_is_valid (vals->disc_layer_ID))
        {
          g_message (_("Error: it seems that the discard features "
                       "layer is no longer valid"));
          return FALSE;
        }
    }

  if (gimp_layer_is_floating_sel (layer_ID))
    {
      gimp_floating_sel_to_layer (layer_ID);
    }

  drawable = gimp_drawable_get (layer_ID);

  snprintf (layer_name, LQR_MAX_NAME_LENGTH, "%s",
            gimp_drawable_get_name (drawable->drawable_id));

  if (gimp_selection_is_empty (image_ID) == FALSE)
    {
      gimp_selection_save (image_ID);
      gimp_selection_none (image_ID);
      gimp_image_unset_active_channel (image_ID);
    }

  if (vals->output_seams)
    {
      gimp_image_convert_rgb (image_ID);
    }


  if (vals->new_layer == TRUE)
    {
      snprintf (new_layer_name, LQR_MAX_NAME_LENGTH, "%s LqR", layer_name);
      layer_ID = gimp_layer_copy (drawable->drawable_id);
      gimp_image_add_layer (image_ID, layer_ID, -1);
      gimp_drawable_detach (drawable);
      drawable = gimp_drawable_get (layer_ID);
      gimp_drawable_set_name (layer_ID, new_layer_name);
      gimp_drawable_set_visible (layer_ID, FALSE);
    }

  mask_ID = gimp_layer_get_mask (drawable->drawable_id);
  if (mask_ID != -1)
    {
      gimp_layer_remove_mask (drawable->drawable_id, vals->mask_behavior);
    }

  /* unset lock alpha */
  alpha_lock = gimp_layer_get_lock_alpha (drawable->drawable_id);
  gimp_layer_set_lock_alpha (drawable->drawable_id, FALSE);

  old_width = gimp_drawable_width (drawable->drawable_id);
  old_height = gimp_drawable_height (drawable->drawable_id);
  new_width = vals->new_width;
  new_height = vals->new_height;

  gimp_drawable_offsets (drawable->drawable_id, &x_off, &y_off);

  bpp = gimp_drawable_bpp (drawable->drawable_id);

  if (vals->resize_aux_layers == TRUE)
    {
      if (vals->pres_layer_ID != 0)
        {
          alpha_lock_pres = gimp_layer_get_lock_alpha (vals->pres_layer_ID);
          gimp_layer_set_lock_alpha (vals->pres_layer_ID, FALSE);
          gimp_drawable_offsets (vals->pres_layer_ID, &aux_x_off, &aux_y_off);
          gimp_layer_resize (vals->pres_layer_ID, old_width, old_height,
                             aux_x_off - x_off, aux_y_off - y_off);
        }
      if (vals->disc_layer_ID != 0)
        {
          alpha_lock_disc = gimp_layer_get_lock_alpha (vals->disc_layer_ID);
          gimp_layer_set_lock_alpha (vals->disc_layer_ID, FALSE);
          gimp_drawable_offsets (vals->disc_layer_ID, &aux_x_off, &aux_y_off);
          gimp_layer_resize (vals->disc_layer_ID, old_width, old_height,
                             aux_x_off - x_off, aux_y_off - y_off);
        }
    }


  gimp_rgba_set (&colour_start, col_vals->r1, col_vals->g1, col_vals->b1, 1);
  gimp_rgba_set (&colour_end, col_vals->r2, col_vals->g2, col_vals->b2, 1);


  ntiles = old_width / gimp_tile_width () + 1;
  gimp_tile_cache_size ((gimp_tile_width () * gimp_tile_height () * ntiles *
                         4 * 2) / 1024 + 1);

  lqr_progress = lqr_progress_new ();
  MEMCHECK (lqr_progress != NULL);
  lqr_progress->init = (LqrProgressFuncInit) gimp_progress_init;
  lqr_progress->update = (LqrProgressFuncUpdate) gimp_progress_update;
  lqr_progress->end = (LqrProgressFuncEnd) my_progress_end;
  lqr_progress_set_init_width_message (lqr_progress, _("Resizing width..."));
  lqr_progress_set_init_height_message (lqr_progress,
                                        _("Resizing height..."));

#ifdef __CLOCK_IT__
  clock1 = (double) clock () / CLOCKS_PER_SEC;
  printf ("[ begin: clock: %g ]\n", clock1);
#endif /* __CLOCK_IT__ */

  /* lqr carver initialization */
  rgb_buffer = rgb_buffer_from_layer (layer_ID);
  MEMCHECK (rgb_buffer != NULL);
  carver = lqr_carver_new (rgb_buffer, old_width, old_height, bpp);
  MEMCHECK (carver != NULL);
  MEMCHECK1 (lqr_carver_init (carver, vals->delta_x, vals->rigidity));
  MEMCHECK1 (update_bias
             (carver, vals->pres_layer_ID, vals->pres_coeff, x_off, y_off));
  MEMCHECK1 (update_bias
             (carver, vals->disc_layer_ID, -vals->disc_coeff, x_off, y_off));
  lqr_carver_set_gradient_function (carver, vals->grad_func);
  lqr_carver_set_resize_order (carver, vals->res_order);
  lqr_carver_set_progress (carver, lqr_progress);
  if (vals->output_seams)
    {
      lqr_carver_set_dump_vmaps (carver);
    }
  if (vals->resize_aux_layers)
    {
      if (vals->pres_layer_ID)
        {
          rgb_buffer = rgb_buffer_from_layer (vals->pres_layer_ID);
          MEMCHECK (rgb_buffer != NULL);
          aux_carver =
            lqr_carver_new (rgb_buffer, old_width, old_height,
                            gimp_drawable_bpp (vals->pres_layer_ID));
          MEMCHECK (aux_carver != NULL);
          MEMCHECK1 (lqr_carver_attach (carver, aux_carver));
        }
      if (vals->disc_layer_ID)
        {
          rgb_buffer = rgb_buffer_from_layer (vals->disc_layer_ID);
          MEMCHECK (rgb_buffer != NULL);
          aux_carver =
            lqr_carver_new (rgb_buffer, old_width, old_height,
                            gimp_drawable_bpp (vals->disc_layer_ID));
          MEMCHECK (aux_carver != NULL);
          MEMCHECK1 (lqr_carver_attach (carver, aux_carver));
        }
    }

#ifdef __CLOCK_IT__
  clock2 = (double) clock () / CLOCKS_PER_SEC;
  printf ("[ read: clock: %g (%g) ]\n", clock2, clock2 - clock1);
#endif /* __CLOCK_IT__ */

  MEMCHECK1 (lqr_carver_resize (carver, new_width, new_height));

  switch (vals->oper_mode)
    {
    case OPER_MODE_NORMAL:
      break;
    case OPER_MODE_LQRBACK:
      MEMCHECK1 (lqr_carver_flatten (carver));
#if 0
      if (vals->resize_aux_layers == TRUE)
        {
          carver_list = lqr_carver_list_start (carver);
          if (vals->pres_layer_ID != 0)
            {
              carver_list = lqr_carver_list_next (carver_list);
            }
          if (vals->disc_layer_ID != 0)
            {
              MEMCHECK1 (update_bias
                         (carver, vals->disc_layer_ID, 2 * vals->disc_coeff,
                          x_off, y_off));
              carver_list = lqr_carver_list_next (carver_list);
            }
        }
#endif
      new_width = old_width;
      new_height = old_height;

#if 0
      switch (vals->res_order)
        {
        case LQR_RES_ORDER_HOR:
          carver->resize_order = LQR_RES_ORDER_VERT;
          break;
        case LQR_RES_ORDER_VERT:
          carver->resize_order = LQR_RES_ORDER_HOR;
          break;
        }
#endif
      MEMCHECK1 (lqr_carver_resize (carver, new_width, new_height));
      break;
    case OPER_MODE_SCALEBACK:
      break;
    default:
      g_message ("error: unknown mode");
      return FALSE;
    }

  write_all_vmaps (lqr_vmap_list_start (carver), image_ID, layer_name, x_off,
                   y_off, colour_start, colour_end);

  if (vals->resize_canvas == TRUE)
    {
      gimp_image_resize (image_ID, new_width, new_height, -x_off, -y_off);
      gimp_layer_resize_to_image_size (layer_ID);
    }
  else
    {
      gimp_layer_resize (layer_ID, new_width, new_height, 0, 0);
    }
  gimp_drawable_detach (drawable);
  drawable = gimp_drawable_get (layer_ID);

#ifdef __CLOCK_IT__
  clock3 = (double) clock () / CLOCKS_PER_SEC;
  printf ("[ resized: clock : %g (%g) ]\n", clock3, clock3 - clock2);
  fflush (stdout);
#endif /* __CLOCK_IT__ */

  ntiles = vals->new_width / gimp_tile_width () + 1;
  gimp_tile_cache_size ((gimp_tile_width () * gimp_tile_height () * ntiles *
                         4 * 2) / 1024 + 1);

  MEMCHECK1 (write_carver_to_layer (carver, drawable));

  if (vals->resize_aux_layers == TRUE)
    {
      carver_list = lqr_carver_list_start (carver);
      if (vals->pres_layer_ID != 0)
        {
          gimp_layer_resize (vals->pres_layer_ID, new_width, new_height, 0,
                             0);
          drawable_pres = gimp_drawable_get (vals->pres_layer_ID);
          aux_carver = lqr_carver_list_current (carver_list);
          MEMCHECK1 (write_carver_to_layer (aux_carver, drawable_pres));
          gimp_drawable_detach (drawable_pres);
          carver_list = lqr_carver_list_next (carver_list);
        }
      if (vals->disc_layer_ID != 0)
        {
          gimp_layer_resize (vals->disc_layer_ID, new_width, new_height, 0,
                             0);
          drawable_disc = gimp_drawable_get (vals->disc_layer_ID);
          aux_carver = lqr_carver_list_current (carver_list);
          MEMCHECK1 (write_carver_to_layer (aux_carver, drawable_disc));
          gimp_drawable_detach (drawable_disc);
          carver_list = lqr_carver_list_next (carver_list);
        }
    }

  lqr_carver_destroy (carver);
  switch (vals->oper_mode)
    {
    case OPER_MODE_NORMAL:
    case OPER_MODE_LQRBACK:
      break;
    case OPER_MODE_SCALEBACK:
      if (vals->resize_canvas == TRUE)
        {
          gimp_image_resize (image_ID, old_width, old_height, 0, 0);
          gimp_layer_scale (layer_ID, old_width, old_height, FALSE);
        }
      else
        {
          gimp_layer_translate (layer_ID, -x_off, -y_off);
          gimp_layer_scale (layer_ID, old_width, old_height, FALSE);
          gimp_layer_translate (layer_ID, x_off, y_off);
        }
      gimp_drawable_detach (drawable);
      drawable = gimp_drawable_get (layer_ID);
      if (vals->resize_aux_layers == TRUE)
        {
          if (vals->pres_layer_ID != 0)
            {
              gimp_layer_translate (vals->pres_layer_ID, -x_off, -y_off);
              gimp_layer_scale (vals->pres_layer_ID, old_width,
                                old_height, FALSE);
              gimp_layer_translate (vals->pres_layer_ID, x_off, y_off);
            }
          if (vals->disc_layer_ID != 0)
            {
              gimp_layer_translate (vals->disc_layer_ID, -x_off, -y_off);
              gimp_layer_scale (vals->disc_layer_ID, old_width,
                                old_height, FALSE);
              gimp_layer_translate (vals->disc_layer_ID, x_off, y_off);
            }
        }
      break;
    default:
      g_message ("error: unknown mode");
      return FALSE;
    }



#ifdef __CLOCK_IT__
  clock4 = (double) clock () / CLOCKS_PER_SEC;
  printf ("[ finish: clock: %g (%g) ]\n\n", clock4, clock4 - clock3);
#endif /* __CLOCK_IT__ */

  gimp_drawable_set_visible (layer_ID, TRUE);
  gimp_image_set_active_layer (image_ID, layer_ID);

  gimp_layer_set_lock_alpha (drawable->drawable_id, alpha_lock);
  if (vals->resize_aux_layers == TRUE)
    {
      if (vals->pres_layer_ID != 0)
        {
          gimp_layer_set_lock_alpha (vals->pres_layer_ID, alpha_lock_pres);
        }
      if (vals->disc_layer_ID != 0)
        {
          gimp_layer_set_lock_alpha (vals->disc_layer_ID, alpha_lock_disc);
        }
    }

  return TRUE;
}
