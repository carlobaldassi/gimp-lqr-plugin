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
#include <stdlib.h>

#include "io_functions.h"

#include "main.h"
#include "render.h"
#include "plugin-intl.h"


#if 0
#define __CLOCK_IT__
#endif

#define MEMCHECK_N(x) if ((x) == NULL) { g_message(_("Not enough memory")); return NULL; }
#define MEMCHECK1_N(x) if ((x) == LQR_NOMEM) { g_message(_("Not enough memory")); return NULL; }
#define MEMCHECK(x) if ((x) == FALSE) { g_message(_("Not enough memory")); return FALSE; }
#define MEMCHECK1(x) if ((x) == LQR_NOMEM) { g_message(_("Not enough memory")); return FALSE; }

gboolean
my_progress_end (const gchar * message)
{
  return gimp_progress_end ();
}

CarverData *
render_init_carver (gint32 image_ID,
        GimpDrawable * drawable,
        PlugInVals * vals,
        PlugInDrawableVals * drawable_vals,
        gboolean interactive)
{
  CarverData *carver_data;
  LqrCarver *carver, *aux_carver;
  gint32 mask_ID;
  gint32 layer_ID;
  gchar layer_name[LQR_MAX_NAME_LENGTH];
  gchar new_layer_name[LQR_MAX_NAME_LENGTH];
  guchar *rgb_buffer;
  gboolean alpha_lock;
  gboolean alpha_lock_pres = FALSE, alpha_lock_disc = FALSE, alpha_lock_rigmask = FALSE;
  gint ntiles;
  gfloat rigidity;
  gint old_width, old_height;
  gint new_width, new_height;
  gint bpp;
  gint x_off, y_off, aux_x_off, aux_y_off;
  gboolean ignore_disc_mask = FALSE;
  LqrProgress *lqr_progress;
#ifdef __CLOCK_IT__
  double clock1, clock2;
#endif /* __CLOCK_IT__ */

  if (!gimp_image_is_valid (image_ID))
    {
      g_message (_("Error: it seems that the selected image "
                   "is no longer valid"));
      return NULL;
    }

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
      return NULL;
    }

  if ((vals->pres_layer_ID != 0) && 
      (!gimp_drawable_is_valid (vals->pres_layer_ID)))
    {
      g_message (_("Error: it seems that the preservation "
                   "features layer is no longer valid"));
      return NULL;
    }

  if ((vals->disc_layer_ID != 0) &&
      (!gimp_drawable_is_valid (vals->disc_layer_ID)))
    {
      g_message (_("Error: it seems that the discard features "
                   "layer is no longer valid"));
      return NULL;
    }

  if ((vals->rigmask_layer_ID != 0) &&
      (!gimp_drawable_is_valid (vals->rigmask_layer_ID)))
    {
      g_message (_("Error: it seems that the rigidity mask "
                   "layer is no longer valid"));
      return NULL;
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

  if ((!interactive) && ((vals->output_seams) && (gimp_image_base_type(image_ID) != GIMP_RGB)))
    {
      gimp_image_convert_rgb (image_ID);
    }

  if ((!interactive) && (vals->new_layer == TRUE))
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
      if (vals->rigmask_layer_ID != 0)
        {
          alpha_lock_rigmask = gimp_layer_get_lock_alpha (vals->rigmask_layer_ID);
          gimp_layer_set_lock_alpha (vals->rigmask_layer_ID, FALSE);
          gimp_drawable_offsets (vals->rigmask_layer_ID, &aux_x_off, &aux_y_off);
          gimp_layer_resize (vals->rigmask_layer_ID, old_width, old_height,
                             aux_x_off - x_off, aux_y_off - y_off);
        }
    }


  ntiles = old_width / gimp_tile_width () + 1;
  gimp_tile_cache_size ((gimp_tile_width () * gimp_tile_height () * ntiles *
                         4 * 2) / 1024 + 1);

  lqr_progress = lqr_progress_new ();
  MEMCHECK_N (lqr_progress);
  lqr_progress_set_init (lqr_progress, (LqrProgressFuncInit) gimp_progress_init);
  lqr_progress_set_update (lqr_progress, (LqrProgressFuncUpdate) gimp_progress_update);
  lqr_progress_set_end (lqr_progress, (LqrProgressFuncEnd) my_progress_end);
  lqr_progress_set_init_width_message (lqr_progress, _("Resizing width..."));
  lqr_progress_set_init_height_message (lqr_progress,
                                        _("Resizing height..."));

  if (vals->rigmask_layer_ID != 0)
    {
      rigidity = 3 * vals->rigidity;
    }
  else
    {
      rigidity = vals->rigidity;
    }

  if ((!interactive) && (vals->no_disc_on_enlarge == TRUE))
    {
      switch (vals->res_order)
        {
          case LQR_RES_ORDER_HOR:
            if ((new_width > old_width) || ((new_width == old_width) && (new_height > old_height)))
              {
                ignore_disc_mask = TRUE;
              }
            break;
          case LQR_RES_ORDER_VERT:
            if ((new_height > old_height) || ((new_height == old_height) && (new_width > old_width)))
              {
                ignore_disc_mask = TRUE;
              }
            break;
          default:
            g_message ("Error: unknown resize order index");
            return NULL;
        }
    }


#ifdef __CLOCK_IT__
  clock1 = (double) clock () / CLOCKS_PER_SEC;
  printf ("[ begin ]\n");
#endif /* __CLOCK_IT__ */

  /* lqr carver initialization */
  rgb_buffer = rgb_buffer_from_layer (layer_ID);
  MEMCHECK_N (rgb_buffer);
  carver = lqr_carver_new (rgb_buffer, old_width, old_height, bpp);
  MEMCHECK_N (carver);
  MEMCHECK1_N (lqr_carver_init (carver, vals->delta_x, rigidity));
  MEMCHECK1_N (update_bias
               (carver, vals->pres_layer_ID, vals->pres_coeff, x_off, y_off));
  if (ignore_disc_mask == FALSE)
    {
      MEMCHECK1_N (update_bias
                 (carver, vals->disc_layer_ID, -vals->disc_coeff, x_off, y_off));
    }
  MEMCHECK1_N (set_rigmask
               (carver, vals->rigmask_layer_ID, x_off, y_off));
  lqr_carver_set_gradient_function (carver, vals->grad_func);
  lqr_carver_set_resize_order (carver, vals->res_order);
  lqr_carver_set_progress (carver, lqr_progress);
  lqr_carver_set_side_switch_frequency (carver, 2);
  lqr_carver_set_enl_step (carver, vals->enl_step / 100);
  if ((!interactive) && (vals->output_seams))
    {
      lqr_carver_set_dump_vmaps (carver);
    }
  if (vals->resize_aux_layers)
    {
      if (vals->pres_layer_ID)
        {
          rgb_buffer = rgb_buffer_from_layer (vals->pres_layer_ID);
          MEMCHECK_N (rgb_buffer);
          aux_carver =
            lqr_carver_new (rgb_buffer, old_width, old_height,
                            gimp_drawable_bpp (vals->pres_layer_ID));
          MEMCHECK_N (aux_carver);
          MEMCHECK1_N (lqr_carver_attach (carver, aux_carver));
        }
      if (vals->disc_layer_ID)
        {
          rgb_buffer = rgb_buffer_from_layer (vals->disc_layer_ID);
          MEMCHECK_N (rgb_buffer);
          aux_carver =
            lqr_carver_new (rgb_buffer, old_width, old_height,
                            gimp_drawable_bpp (vals->disc_layer_ID));
          MEMCHECK_N (aux_carver);
          MEMCHECK1_N (lqr_carver_attach (carver, aux_carver));
        }
      if (vals->rigmask_layer_ID)
        {
          rgb_buffer = rgb_buffer_from_layer (vals->rigmask_layer_ID);
          MEMCHECK_N (rgb_buffer);
          aux_carver =
            lqr_carver_new (rgb_buffer, old_width, old_height,
                            gimp_drawable_bpp (vals->rigmask_layer_ID));
          MEMCHECK_N (aux_carver);
          MEMCHECK1_N (lqr_carver_attach (carver, aux_carver));
        }
    }

#ifdef __CLOCK_IT__
  clock2 = (double) clock () / CLOCKS_PER_SEC;
  printf ("[ read: %g ]\n", clock2 - clock1);
#endif /* __CLOCK_IT__ */

  MEMCHECK_N(carver_data = calloc(1, sizeof(CarverData)));

  carver_data->carver = carver;
  carver_data->layer_ID = layer_ID;
  carver_data->alpha_lock = alpha_lock;
  carver_data->alpha_lock_pres = alpha_lock_pres;
  carver_data->alpha_lock_disc = alpha_lock_disc;
  carver_data->alpha_lock_rigmask = alpha_lock_rigmask;

  return carver_data;
}

gboolean
render_noninteractive (gint32 image_ID,
        GimpDrawable * drawable,
        PlugInVals * vals,
        PlugInDrawableVals * drawable_vals,
        PlugInColVals * col_vals,
        CarverData * carver_data)
{
  LqrCarver *carver, *aux_carver;
  LqrCarverList *carver_list;
  gint32 layer_ID;
  gchar layer_name[LQR_MAX_NAME_LENGTH];
  gboolean alpha_lock;
  gboolean alpha_lock_pres = FALSE, alpha_lock_disc = FALSE, alpha_lock_rigmask = FALSE;
  gint ntiles;
  gint old_width, old_height;
  gint new_width, new_height;
  gint x_off, y_off;
  gint bpp;
  GimpDrawable *drawable_pres, *drawable_disc, *drawable_rigmask;
  GimpRGB colour_start, colour_end;
#ifdef __CLOCK_IT__
  double clock1, clock2, clock3;
#endif /* __CLOCK_IT__ */

  old_width = gimp_drawable_width (drawable->drawable_id);
  old_height = gimp_drawable_height (drawable->drawable_id);
  new_width = vals->new_width;
  new_height = vals->new_height;
  gimp_drawable_offsets (drawable->drawable_id, &x_off, &y_off);
  bpp = gimp_drawable_bpp (drawable->drawable_id);

  carver = carver_data->carver;
  layer_ID = carver_data->layer_ID;
  alpha_lock = carver_data->alpha_lock;
  alpha_lock_pres = carver_data->alpha_lock_pres;
  alpha_lock_disc = carver_data->alpha_lock_disc;
  alpha_lock_rigmask = carver_data->alpha_lock_rigmask;

#ifdef __CLOCK_IT__
  clock1 = (double) clock () / CLOCKS_PER_SEC;
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
              if (ignore_disc_mask == FALSE)
                {
                  MEMCHECK1 (update_bias
                               (carver, vals->disc_layer_ID, 2 * vals->disc_coeff,
                              x_off, y_off));
                }
              carver_list = lqr_carver_list_next (carver_list);
            }
          if (vals->rigmask_layer_ID != 0)
            {
              MEMCHECK1 (update_bias
                         (carver, vals->rigmask_layer_ID, 2 * vals->rigmask_coeff,
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

  if (vals->output_seams) {
    gimp_rgba_set (&colour_start, col_vals->r1, col_vals->g1, col_vals->b1, 1);
    gimp_rgba_set (&colour_end, col_vals->r2, col_vals->g2, col_vals->b2, 1);

    MEMCHECK1 (write_all_vmaps (lqr_vmap_list_start (carver), image_ID, layer_name, x_off,
                     y_off, colour_start, colour_end));
  }

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
  clock2 = (double) clock () / CLOCKS_PER_SEC;
  printf ("[ resized: %g ]\n", clock2 - clock1);
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
      if (vals->rigmask_layer_ID != 0)
        {
          gimp_layer_resize (vals->rigmask_layer_ID, new_width, new_height, 0,
                             0);
          drawable_rigmask = gimp_drawable_get (vals->rigmask_layer_ID);
          aux_carver = lqr_carver_list_current (carver_list);
          MEMCHECK1 (write_carver_to_layer (aux_carver, drawable_rigmask));
          gimp_drawable_detach (drawable_rigmask);
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
          if (vals->rigmask_layer_ID != 0)
            {
              gimp_layer_translate (vals->rigmask_layer_ID, -x_off, -y_off);
              gimp_layer_scale (vals->rigmask_layer_ID, old_width,
                                old_height, FALSE);
              gimp_layer_translate (vals->rigmask_layer_ID, x_off, y_off);
            }
        }
      break;
    default:
      g_message ("error: unknown mode");
      return FALSE;
    }



#ifdef __CLOCK_IT__
  clock3 = (double) clock () / CLOCKS_PER_SEC;
  printf ("[ finish: %g ]\n\n", clock3 - clock2);
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
      if (vals->rigmask_layer_ID != 0)
        {
          gimp_layer_set_lock_alpha (vals->rigmask_layer_ID, alpha_lock_rigmask);
        }
    }

  return TRUE;
}

gboolean
render_interactive (gint32 image_ID,
        GimpDrawable * drawable,
        PlugInVals * vals,
        PlugInDrawableVals * drawable_vals,
        CarverData * carver_data)
{
  LqrCarver *carver, *aux_carver;
  LqrCarverList *carver_list;
  gint32 layer_ID;
  gint32 mask_ID;
  gchar layer_name[LQR_MAX_NAME_LENGTH];
  gint ntiles;
  gint old_width, old_height;
  gint new_width, new_height;
  gint x_off, y_off, aux_x_off, aux_y_off;
  gint bpp;
  GimpDrawable *drawable_pres, *drawable_disc, *drawable_rigmask;
#ifdef __CLOCK_IT__
  double clock1, clock2, clock3;
#endif /* __CLOCK_IT__ */

  carver = carver_data->carver;
  layer_ID = carver_data->layer_ID;

  if (!gimp_image_is_valid (image_ID))
    {
      g_message (_("Error: it seems that the selected image "
                   "is no longer valid"));
      return FALSE;
    }

  if (!gimp_drawable_is_valid (layer_ID))
    {
      g_message (_("Error: it seems that the selected layer "
                   "is no longer valid"));
      return FALSE;
    }

  if ((vals->pres_layer_ID != 0) && 
      (!gimp_drawable_is_valid (vals->pres_layer_ID)))
    {
      g_message (_("Error: it seems that the preservation "
                   "features layer is no longer valid"));
      return FALSE;
    }

  if ((vals->disc_layer_ID != 0) &&
      (!gimp_drawable_is_valid (vals->disc_layer_ID)))
    {
      g_message (_("Error: it seems that the discard features "
                   "layer is no longer valid"));
      return FALSE;
    }

  if ((vals->rigmask_layer_ID != 0) &&
      (!gimp_drawable_is_valid (vals->rigmask_layer_ID)))
    {
      g_message (_("Error: it seems that the rigidity mask "
                   "layer is no longer valid"));
      return FALSE;
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

  mask_ID = gimp_layer_get_mask (drawable->drawable_id);
  if (mask_ID != -1)
    {
      gimp_layer_remove_mask (drawable->drawable_id, vals->mask_behavior);
    }

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
          gimp_layer_set_lock_alpha (vals->pres_layer_ID, FALSE);
          gimp_drawable_offsets (vals->pres_layer_ID, &aux_x_off, &aux_y_off);
          gimp_layer_resize (vals->pres_layer_ID, old_width, old_height,
                             aux_x_off - x_off, aux_y_off - y_off);
        }
      if (vals->disc_layer_ID != 0)
        {
          gimp_layer_set_lock_alpha (vals->disc_layer_ID, FALSE);
          gimp_drawable_offsets (vals->disc_layer_ID, &aux_x_off, &aux_y_off);
          gimp_layer_resize (vals->disc_layer_ID, old_width, old_height,
                             aux_x_off - x_off, aux_y_off - y_off);
        }
      if (vals->rigmask_layer_ID != 0)
        {
          gimp_layer_set_lock_alpha (vals->rigmask_layer_ID, FALSE);
          gimp_drawable_offsets (vals->rigmask_layer_ID, &aux_x_off, &aux_y_off);
          gimp_layer_resize (vals->rigmask_layer_ID, old_width, old_height,
                             aux_x_off - x_off, aux_y_off - y_off);
        }
    }

#ifdef __CLOCK_IT__
  clock1 = (double) clock () / CLOCKS_PER_SEC;
#endif /* __CLOCK_IT__ */

  MEMCHECK1 (lqr_carver_resize (carver, new_width, new_height));

  gimp_image_resize (image_ID, new_width, new_height, -x_off, -y_off);
  gimp_layer_resize_to_image_size (layer_ID);
  gimp_drawable_detach (drawable);
  drawable = gimp_drawable_get (layer_ID);

#ifdef __CLOCK_IT__
  clock2 = (double) clock () / CLOCKS_PER_SEC;
  printf ("[ resized: %g ]\n", clock2 - clock1);
  fflush (stdout);
#endif /* __CLOCK_IT__ */

  carver_data->ref_w = lqr_carver_get_ref_width (carver);
  carver_data->ref_h = lqr_carver_get_ref_height (carver);
  carver_data->orientation = lqr_carver_get_orientation (carver);
  carver_data->depth = lqr_carver_get_depth (carver);
  carver_data->enl_step = lqr_carver_get_enl_step (carver);

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
      if (vals->rigmask_layer_ID != 0)
        {
          gimp_layer_resize (vals->rigmask_layer_ID, new_width, new_height, 0,
                             0);
          drawable_rigmask = gimp_drawable_get (vals->rigmask_layer_ID);
          aux_carver = lqr_carver_list_current (carver_list);
          MEMCHECK1 (write_carver_to_layer (aux_carver, drawable_rigmask));
          gimp_drawable_detach (drawable_rigmask);
          carver_list = lqr_carver_list_next (carver_list);
        }
    }

#ifdef __CLOCK_IT__
  clock3 = (double) clock () / CLOCKS_PER_SEC;
  printf ("[ finish: %g ]\n\n", clock3 - clock2);
#endif /* __CLOCK_IT__ */

  gimp_drawable_set_visible (layer_ID, TRUE);
  gimp_image_set_active_layer (image_ID, layer_ID);


  return TRUE;
}

gboolean
render_flatten (gint32 image_ID,
        GimpDrawable * drawable,
        PlugInVals * vals,
        PlugInDrawableVals * drawable_vals,
        CarverData * carver_data)
{
  LqrCarver *carver, *aux_carver;
  LqrCarverList *carver_list;
  gint32 layer_ID;
  gint32 mask_ID;
  gchar layer_name[LQR_MAX_NAME_LENGTH];
  gint ntiles;
  gint old_width, old_height;
  gint x_off, y_off, aux_x_off, aux_y_off;
  gint bpp;
  GimpDrawable *drawable_pres, *drawable_disc, *drawable_rigmask;
#ifdef __CLOCK_IT__
  double clock1, clock2, clock3;
#endif /* __CLOCK_IT__ */

  carver = carver_data->carver;
  layer_ID = carver_data->layer_ID;

  if (!gimp_image_is_valid (image_ID))
    {
      g_message (_("Error: it seems that the selected image "
                   "is no longer valid"));
      return FALSE;
    }

  if (!gimp_drawable_is_valid (layer_ID))
    {
      g_message (_("Error: it seems that the selected layer "
                   "is no longer valid"));
      return FALSE;
    }

  if ((vals->pres_layer_ID != 0) && 
      (!gimp_drawable_is_valid (vals->pres_layer_ID)))
    {
      g_message (_("Error: it seems that the preservation "
                   "features layer is no longer valid"));
      return FALSE;
    }

  if ((vals->disc_layer_ID != 0) &&
      (!gimp_drawable_is_valid (vals->disc_layer_ID)))
    {
      g_message (_("Error: it seems that the discard features "
                   "layer is no longer valid"));
      return FALSE;
    }

  if ((vals->rigmask_layer_ID != 0) &&
      (!gimp_drawable_is_valid (vals->rigmask_layer_ID)))
    {
      g_message (_("Error: it seems that the rigidity mask "
                   "layer is no longer valid"));
      return FALSE;
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

  mask_ID = gimp_layer_get_mask (drawable->drawable_id);
  if (mask_ID != -1)
    {
      gimp_layer_remove_mask (drawable->drawable_id, vals->mask_behavior);
    }

  gimp_layer_set_lock_alpha (drawable->drawable_id, FALSE);

  old_width = gimp_drawable_width (drawable->drawable_id);
  old_height = gimp_drawable_height (drawable->drawable_id);

  gimp_drawable_offsets (drawable->drawable_id, &x_off, &y_off);

  bpp = gimp_drawable_bpp (drawable->drawable_id);

  if (vals->resize_aux_layers == TRUE)
    {
      if (vals->pres_layer_ID != 0)
        {
          gimp_layer_set_lock_alpha (vals->pres_layer_ID, FALSE);
          gimp_drawable_offsets (vals->pres_layer_ID, &aux_x_off, &aux_y_off);
          gimp_layer_resize (vals->pres_layer_ID, old_width, old_height,
                             aux_x_off - x_off, aux_y_off - y_off);
        }
      if (vals->disc_layer_ID != 0)
        {
          gimp_layer_set_lock_alpha (vals->disc_layer_ID, FALSE);
          gimp_drawable_offsets (vals->disc_layer_ID, &aux_x_off, &aux_y_off);
          gimp_layer_resize (vals->disc_layer_ID, old_width, old_height,
                             aux_x_off - x_off, aux_y_off - y_off);
        }
      if (vals->rigmask_layer_ID != 0)
        {
          gimp_layer_set_lock_alpha (vals->rigmask_layer_ID, FALSE);
          gimp_drawable_offsets (vals->rigmask_layer_ID, &aux_x_off, &aux_y_off);
          gimp_layer_resize (vals->rigmask_layer_ID, old_width, old_height,
                             aux_x_off - x_off, aux_y_off - y_off);
        }
    }

#ifdef __CLOCK_IT__
  clock1 = (double) clock () / CLOCKS_PER_SEC;
#endif /* __CLOCK_IT__ */

  MEMCHECK1 (lqr_carver_flatten (carver));

  gimp_image_resize (image_ID, old_width, old_height, -x_off, -y_off);
  gimp_layer_resize_to_image_size (layer_ID);
  gimp_drawable_detach (drawable);
  drawable = gimp_drawable_get (layer_ID);

#ifdef __CLOCK_IT__
  clock2 = (double) clock () / CLOCKS_PER_SEC;
  printf ("[ resized: %g ]\n", clock2 - clock1);
  fflush (stdout);
#endif /* __CLOCK_IT__ */

  carver_data->ref_w = lqr_carver_get_ref_width (carver);
  carver_data->ref_h = lqr_carver_get_ref_height (carver);
  carver_data->orientation = lqr_carver_get_orientation (carver);
  carver_data->depth = lqr_carver_get_depth (carver);
  carver_data->enl_step = lqr_carver_get_enl_step (carver);

  ntiles = vals->new_width / gimp_tile_width () + 1;
  gimp_tile_cache_size ((gimp_tile_width () * gimp_tile_height () * ntiles *
                         4 * 2) / 1024 + 1);

  MEMCHECK1 (write_carver_to_layer (carver, drawable));

  if (vals->resize_aux_layers == TRUE)
    {
      carver_list = lqr_carver_list_start (carver);
      if (vals->pres_layer_ID != 0)
        {
          gimp_layer_resize (vals->pres_layer_ID, old_width, old_height, 0,
                             0);
          drawable_pres = gimp_drawable_get (vals->pres_layer_ID);
          aux_carver = lqr_carver_list_current (carver_list);
          MEMCHECK1 (write_carver_to_layer (aux_carver, drawable_pres));
          gimp_drawable_detach (drawable_pres);
          carver_list = lqr_carver_list_next (carver_list);
        }
      if (vals->disc_layer_ID != 0)
        {
          gimp_layer_resize (vals->disc_layer_ID, old_width, old_height, 0,
                             0);
          drawable_disc = gimp_drawable_get (vals->disc_layer_ID);
          aux_carver = lqr_carver_list_current (carver_list);
          MEMCHECK1 (write_carver_to_layer (aux_carver, drawable_disc));
          gimp_drawable_detach (drawable_disc);
          carver_list = lqr_carver_list_next (carver_list);
        }
      if (vals->rigmask_layer_ID != 0)
        {
          gimp_layer_resize (vals->rigmask_layer_ID, old_width, old_height, 0,
                             0);
          drawable_rigmask = gimp_drawable_get (vals->rigmask_layer_ID);
          aux_carver = lqr_carver_list_current (carver_list);
          MEMCHECK1 (write_carver_to_layer (aux_carver, drawable_rigmask));
          gimp_drawable_detach (drawable_rigmask);
          carver_list = lqr_carver_list_next (carver_list);
        }
    }

#ifdef __CLOCK_IT__
  clock3 = (double) clock () / CLOCKS_PER_SEC;
  printf ("[ finish: %g ]\n\n", clock3 - clock2);
#endif /* __CLOCK_IT__ */

  gimp_drawable_set_visible (layer_ID, TRUE);
  gimp_image_set_active_layer (image_ID, layer_ID);


  return TRUE;
}
