/* GIMP LiquidRescale Plug-in
 * Copyright (C) 2007-2010 Carlo Baldassi (the "Author") <carlobaldassi@gmail.com>.
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

/* Convenience macros for checking and general cleanup */
;

#define MEM_CHECK_N(x) if ((x) == NULL) { g_message(_("Not enough memory")); return NULL; }
#define MEM_CHECK1_N(x) if ((x) == LQR_NOMEM) { g_message(_("Not enough memory")); return NULL; }
#define MEM_CHECK(x) if ((x) == NULL) { g_message(_("Not enough memory")); return FALSE; }
#define MEM_CHECK1(x) if ((x) == LQR_NOMEM) { g_message(_("Not enough memory")); return FALSE; }
#define MEM_CHECK2(x) if ((x) == FALSE) { g_message(_("Not enough memory")); return FALSE; }

#define BPP_CHECK(layer_ID, carver) G_STMT_START { \
  if (gimp_drawable_bpp(layer_ID) != lqr_carver_get_channels(carver)) \
    { \
      g_message(_("Error: number of colour channels changed")); \
      return FALSE; \
    } \
  } G_STMT_END

#define IMAGE_TYPE_CHECK(image_ID, base_type) G_STMT_START { \
  if (gimp_image_base_type(image_ID) != base_type) \
    { \
      g_message(_("Error: image type changed")); \
      return FALSE; \
    } \
  } G_STMT_END

#define UNFLOAT(layer_ID) G_STMT_START { \
  if (gimp_layer_is_floating_sel (layer_ID)) \
    { \
      gimp_floating_sel_to_layer (layer_ID); \
    } \
  } G_STMT_END

#define UNMASK(layer_ID) G_STMT_START { \
  if (gimp_layer_get_mask (layer_ID) != -1) \
    { \
      gimp_layer_remove_mask (layer_ID, vals->mask_behavior); \
    } \
  } G_STMT_END

#define SELECTION_SAVE(image_ID) G_STMT_START { \
  if (!gimp_selection_is_empty (image_ID)) \
    { \
      gimp_selection_save (image_ID); \
      gimp_selection_none (image_ID); \
      gimp_image_unset_active_channel (image_ID); \
    } \
  } G_STMT_END


/* static functions declarations */

static gboolean my_progress_end (const gchar * message);
static LqrProgress * progress_init ();
static gfloat rigidity_init (PlugInVals * vals);
static gboolean compute_ignore_disc_mask (PlugInVals * vals, gint old_width, gint old_height, gint new_width, gint new_height);
static void set_tiles (gint width);
static gboolean check_aux_layer_bpp (LqrCarverList ** carver_list_p, gint32 layer_ID);
static gboolean resize_unlock_aux_layer (gint32 layer_ID, gint width, gint height, gint x_off, gint y_off);
static LqrCarver* attach_aux_carver (LqrCarver * carver, gint32 layer_ID, gint width, gint height);
static gboolean write_aux_carver (LqrCarverList ** carver_list_p, gint32 layer_ID, gint width, gint height);
static void scale_layer_translated (gint32 layer_ID, gint width, gint height, gint x_off, gint y_off);

/* render functions */

CarverData *
render_init_carver (gint32 image_ID,
        PlugInVals * vals,
        PlugInDrawableVals * drawable_vals,
        gboolean interactive)
{
  CarverData *carver_data;
  LqrCarver *carver;
  gint32 layer_ID;
  gchar layer_name[LQR_MAX_NAME_LENGTH];
  gchar new_layer_name[LQR_MAX_NAME_LENGTH];
  guchar *rgb_buffer;
  gboolean alpha_lock;
  gboolean alpha_lock_pres = FALSE, alpha_lock_disc = FALSE, alpha_lock_rigmask = FALSE;
  gfloat rigidity;
  gint old_width, old_height;
  gint new_width, new_height;
  gint bpp;
  gint x_off, y_off;
  gboolean ignore_disc_mask = FALSE;
  LqrProgress *progress;
#ifdef __CLOCK_IT__
  double clock1, clock2;
#endif /* __CLOCK_IT__ */

  IMAGE_CHECK (image_ID, NULL);

  if (drawable_vals->layer_ID)
    {
      layer_ID = drawable_vals->layer_ID;
    }
  else
    {
      layer_ID = gimp_image_get_active_layer (image_ID);
    }

  LAYER_CHECK (layer_ID, NULL);
  LAYER_CHECK0 (vals->pres_layer_ID, NULL);
  LAYER_CHECK0 (vals->disc_layer_ID, NULL);
  LAYER_CHECK0 (vals->rigmask_layer_ID, NULL);

  UNFLOAT (layer_ID);
  SELECTION_SAVE (image_ID);
  UNMASK (layer_ID);

  g_snprintf (layer_name, LQR_MAX_NAME_LENGTH, "%s",
            gimp_drawable_get_name (layer_ID));

  old_width = gimp_drawable_width (layer_ID);
  old_height = gimp_drawable_height (layer_ID);
  gimp_drawable_offsets (layer_ID, &x_off, &y_off);
  bpp = gimp_drawable_bpp (layer_ID);

  new_width = vals->new_width;
  new_height = vals->new_height;
  rigidity = rigidity_init(vals);

  if (!interactive)
    {
      ignore_disc_mask = compute_ignore_disc_mask (vals, old_width, old_height, new_width, new_height);
      if ((vals->output_seams) && (gimp_image_base_type(image_ID) != GIMP_RGB))
        {
          gimp_image_convert_rgb (image_ID);
        }
    }

  if (vals->new_layer)
    {
      g_snprintf (new_layer_name, LQR_MAX_NAME_LENGTH, "%s LqR", layer_name);
      layer_ID = gimp_layer_copy (layer_ID);
      gimp_image_add_layer (image_ID, layer_ID, -1);
      gimp_drawable_set_name (layer_ID, new_layer_name);
      gimp_drawable_set_visible (layer_ID, FALSE);
    }

  /* unset lock alpha */
  alpha_lock = gimp_layer_get_lock_alpha (layer_ID);
  gimp_layer_set_lock_alpha (layer_ID, FALSE);

  if (vals->resize_aux_layers == TRUE)
    {
      alpha_lock_pres = resize_unlock_aux_layer (vals->pres_layer_ID, old_width, old_height, x_off, y_off);
      alpha_lock_disc = resize_unlock_aux_layer (vals->disc_layer_ID, old_width, old_height, x_off, y_off);
      alpha_lock_rigmask = resize_unlock_aux_layer (vals->rigmask_layer_ID, old_width, old_height, x_off, y_off);
    }

  set_tiles (old_width);

  progress = progress_init();
  MEM_CHECK_N (progress);

#ifdef __CLOCK_IT__
  clock1 = (double) clock () / CLOCKS_PER_SEC;
  printf ("[ begin ]\n");
#endif /* __CLOCK_IT__ */

  /* lqr carver initialization */
  rgb_buffer = rgb_buffer_from_layer (layer_ID);
  MEM_CHECK_N (rgb_buffer);
  carver = lqr_carver_new (rgb_buffer, old_width, old_height, bpp);
  MEM_CHECK_N (carver);
  MEM_CHECK1_N (lqr_carver_init (carver, vals->delta_x, rigidity));
  MEM_CHECK1_N (update_bias
               (carver, vals->pres_layer_ID, vals->pres_coeff, x_off, y_off));
  if (!ignore_disc_mask)
    {
      MEM_CHECK1_N (update_bias
                 (carver, vals->disc_layer_ID, -vals->disc_coeff, x_off, y_off));
    }
  MEM_CHECK1_N (set_rigmask
               (carver, vals->rigmask_layer_ID, x_off, y_off));
  lqr_carver_set_energy_function_builtin (carver, vals->nrg_func);
  lqr_carver_set_resize_order (carver, vals->res_order);
  lqr_carver_set_progress (carver, progress);
  lqr_carver_set_side_switch_frequency (carver, 2);
  lqr_carver_set_enl_step (carver, vals->enl_step / 100);
  if ((!interactive) && (vals->output_seams))
    {
      lqr_carver_set_dump_vmaps (carver);
    }
  if (vals->resize_aux_layers)
    {
      attach_aux_carver (carver, vals->pres_layer_ID, old_width, old_height);
      attach_aux_carver (carver, vals->disc_layer_ID, old_width, old_height);
      attach_aux_carver (carver, vals->rigmask_layer_ID, old_width, old_height);
    }

#ifdef __CLOCK_IT__
  clock2 = (double) clock () / CLOCKS_PER_SEC;
  printf ("[ read: %g ]\n", clock2 - clock1);
#endif /* __CLOCK_IT__ */

  MEM_CHECK_N(carver_data = calloc(1, sizeof(CarverData)));

  carver_data->carver = carver;
  carver_data->layer_ID = layer_ID;
  carver_data->base_type = gimp_image_base_type (image_ID);
  carver_data->alpha_lock = alpha_lock;
  carver_data->alpha_lock_pres = alpha_lock_pres;
  carver_data->alpha_lock_disc = alpha_lock_disc;
  carver_data->alpha_lock_rigmask = alpha_lock_rigmask;

  carver_data->ref_w = old_width;
  carver_data->ref_h = old_height;
  carver_data->orientation = 0;
  carver_data->depth = 0;
  carver_data->enl_step = vals->enl_step / 100;

  return carver_data;
}

gboolean
render_noninteractive (gint32 image_ID,
        PlugInVals * vals,
        PlugInDrawableVals * drawable_vals,
        PlugInColVals * col_vals,
        CarverData * carver_data)
{
  LqrCarver *carver;
  LqrCarverList *carver_list;
  gint32 layer_ID;
  gchar layer_name[LQR_MAX_NAME_LENGTH];
  gboolean alpha_lock;
  gboolean alpha_lock_pres = FALSE, alpha_lock_disc = FALSE, alpha_lock_rigmask = FALSE;
  gint old_width, old_height;
  gint new_width, new_height;
  gint sb_width, sb_height;
  gint x_off, y_off;
  GimpRGB colour_start, colour_end;
#ifdef __CLOCK_IT__
  double clock1, clock2, clock3;
#endif /* __CLOCK_IT__ */

  carver = carver_data->carver;
  layer_ID = carver_data->layer_ID;
  alpha_lock = carver_data->alpha_lock;
  alpha_lock_pres = carver_data->alpha_lock_pres;
  alpha_lock_disc = carver_data->alpha_lock_disc;
  alpha_lock_rigmask = carver_data->alpha_lock_rigmask;

  g_snprintf (layer_name, LQR_MAX_NAME_LENGTH, "%s",
              gimp_drawable_get_name (layer_ID));

  old_width = gimp_drawable_width (layer_ID);
  old_height = gimp_drawable_height (layer_ID);
  gimp_drawable_offsets (layer_ID, &x_off, &y_off);

  new_width = vals->new_width;
  new_height = vals->new_height;

#ifdef __CLOCK_IT__
  clock1 = (double) clock () / CLOCKS_PER_SEC;
#endif /* __CLOCK_IT__ */

  MEM_CHECK1 (lqr_carver_resize (carver, new_width, new_height));

  if (vals->scaleback)
    {
      switch (vals->scaleback_mode)
        {
        case SCALEBACK_MODE_LQRBACK:
          MEM_CHECK1 (lqr_carver_flatten (carver));
          new_width = old_width;
          new_height = old_height;
          MEM_CHECK1 (lqr_carver_resize (carver, new_width, new_height));
          break;
        case SCALEBACK_MODE_STD:
        case SCALEBACK_MODE_STDW:
        case SCALEBACK_MODE_STDH:
          break;
        default:
          g_message ("error: unknown mode");
          return FALSE;
        }
    }

  if (vals->output_seams) {
    gimp_rgba_set (&colour_start, col_vals->r1, col_vals->g1, col_vals->b1, 1);
    gimp_rgba_set (&colour_end, col_vals->r2, col_vals->g2, col_vals->b2, 1);

    MEM_CHECK1 (write_all_vmaps (lqr_vmap_list_start (carver), image_ID, layer_name, x_off,
                     y_off, colour_start, colour_end));
  }

  if (vals->resize_canvas)
    {
      gimp_image_resize (image_ID, new_width, new_height, -x_off, -y_off);
      gimp_layer_resize_to_image_size (layer_ID);
    }
  else
    {
      gimp_layer_resize (layer_ID, new_width, new_height, 0, 0);
    }

#ifdef __CLOCK_IT__
  clock2 = (double) clock () / CLOCKS_PER_SEC;
  printf ("[ resized: %g ]\n", clock2 - clock1);
  fflush (stdout);
#endif /* __CLOCK_IT__ */

  set_tiles (new_width);

  MEM_CHECK1 (write_carver_to_layer (carver, layer_ID));

  if (vals->resize_aux_layers)
    {
      carver_list = lqr_carver_list_start (carver);
      MEM_CHECK2 (write_aux_carver (&carver_list, vals->pres_layer_ID, new_width, new_height));
      MEM_CHECK2 (write_aux_carver (&carver_list, vals->disc_layer_ID, new_width, new_height));
      MEM_CHECK2 (write_aux_carver (&carver_list, vals->rigmask_layer_ID, new_width, new_height));
    }

  lqr_carver_destroy (carver);

  if (vals->scaleback)
    {
      switch (vals->scaleback_mode)
        {
        case SCALEBACK_MODE_LQRBACK:
          break;
        case SCALEBACK_MODE_STD:
        case SCALEBACK_MODE_STDW:
        case SCALEBACK_MODE_STDH:
          switch (vals->scaleback_mode)
            {
              case SCALEBACK_MODE_STD:
                sb_width = old_width;
                sb_height = old_height;
                break;
              case SCALEBACK_MODE_STDW:
                sb_width = old_width;
                sb_height = (int) ((double) new_height * old_width / new_width);
                break;
              case SCALEBACK_MODE_STDH:
                sb_width = (int) ((double) new_width * old_height / new_height);
                sb_height = old_height;
                break;
              default:
                return FALSE;
            }

          if (vals->resize_canvas == TRUE)
            {
              gimp_image_resize (image_ID, sb_width, sb_height, 0, 0);
              gimp_layer_scale (layer_ID, sb_width, sb_height, FALSE);
            }
          else
            {
              scale_layer_translated (layer_ID, sb_width, sb_height, x_off, y_off);
            }
          if (vals->resize_aux_layers == TRUE)
            {
              if (vals->pres_layer_ID != 0)
                {
                  scale_layer_translated (vals->pres_layer_ID, sb_width, sb_height, x_off, y_off);
                }
              if (vals->disc_layer_ID != 0)
                {
                  scale_layer_translated (vals->disc_layer_ID, sb_width, sb_height, x_off, y_off);
                }
              if (vals->rigmask_layer_ID != 0)
                {
                  scale_layer_translated (vals->rigmask_layer_ID, sb_width, sb_height, x_off, y_off);
                }
            }
          break;
        default:
          g_message ("error: unknown mode");
          return FALSE;
        }
    }


#ifdef __CLOCK_IT__
  clock3 = (double) clock () / CLOCKS_PER_SEC;
  printf ("[ finish: %g ]\n\n", clock3 - clock2);
#endif /* __CLOCK_IT__ */

  gimp_drawable_set_visible (layer_ID, TRUE);
  gimp_image_set_active_layer (image_ID, layer_ID);

  gimp_layer_set_lock_alpha (layer_ID, alpha_lock);
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
        PlugInVals * vals,
        PlugInDrawableVals * drawable_vals,
        CarverData * carver_data)
{
  LqrCarver *carver;
  LqrCarverList *carver_list;
  gint32 layer_ID;
  gchar layer_name[LQR_MAX_NAME_LENGTH];
  gint old_width, old_height;
  gint new_width, new_height;
  gint x_off, y_off;
#ifdef __CLOCK_IT__
  double clock1, clock2, clock3;
#endif /* __CLOCK_IT__ */

  carver = carver_data->carver;
  layer_ID = carver_data->layer_ID;

  IMAGE_CHECK (image_ID, FALSE);

  LAYER_CHECK (layer_ID, FALSE);
  LAYER_CHECK0 (vals->pres_layer_ID, FALSE);
  LAYER_CHECK0 (vals->disc_layer_ID, FALSE);
  LAYER_CHECK0 (vals->rigmask_layer_ID, FALSE);

  IMAGE_TYPE_CHECK (image_ID, carver_data->base_type);
  BPP_CHECK (layer_ID, carver);
  if (vals->resize_aux_layers == TRUE)
    {
      carver_list = lqr_carver_list_start (carver);
      MEM_CHECK2 (check_aux_layer_bpp (&carver_list, vals->pres_layer_ID));
      MEM_CHECK2 (check_aux_layer_bpp (&carver_list, vals->disc_layer_ID));
      MEM_CHECK2 (check_aux_layer_bpp (&carver_list, vals->rigmask_layer_ID));
    }

  UNFLOAT (layer_ID);
  SELECTION_SAVE (image_ID);
  UNMASK (layer_ID);

  g_snprintf (layer_name, LQR_MAX_NAME_LENGTH, "%s",
            gimp_drawable_get_name (layer_ID));

  old_width = gimp_drawable_width (layer_ID);
  old_height = gimp_drawable_height (layer_ID);
  gimp_drawable_offsets (layer_ID, &x_off, &y_off);

  new_width = vals->new_width;
  new_height = vals->new_height;

  gimp_layer_set_lock_alpha (layer_ID, FALSE);

  if (vals->resize_aux_layers == TRUE)
    {
      resize_unlock_aux_layer (vals->pres_layer_ID, old_width, old_height, x_off, y_off);
      resize_unlock_aux_layer (vals->disc_layer_ID, old_width, old_height, x_off, y_off);
      resize_unlock_aux_layer (vals->rigmask_layer_ID, old_width, old_height, x_off, y_off);
    }

#ifdef __CLOCK_IT__
  clock1 = (double) clock () / CLOCKS_PER_SEC;
#endif /* __CLOCK_IT__ */

  MEM_CHECK1 (lqr_carver_resize (carver, new_width, new_height));

  if (vals->resize_canvas == TRUE)
    {
      gimp_image_resize (image_ID, new_width, new_height, -x_off, -y_off);
      gimp_layer_resize_to_image_size (layer_ID);
    }
  else
    {
      gimp_layer_resize (layer_ID, new_width, new_height, 0, 0);
    }

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

  set_tiles (new_width);

  MEM_CHECK1 (write_carver_to_layer (carver, layer_ID));

  if (vals->resize_aux_layers)
    {
      carver_list = lqr_carver_list_start (carver);
      MEM_CHECK2 (write_aux_carver (&carver_list, vals->pres_layer_ID, new_width, new_height));
      MEM_CHECK2 (write_aux_carver (&carver_list, vals->disc_layer_ID, new_width, new_height));
      MEM_CHECK2 (write_aux_carver (&carver_list, vals->rigmask_layer_ID, new_width, new_height));
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
        PlugInVals * vals,
        PlugInDrawableVals * drawable_vals,
        CarverData * carver_data)
{
  LqrCarver *carver;
  LqrCarverList *carver_list;
  gint32 layer_ID;
  gchar layer_name[LQR_MAX_NAME_LENGTH];
  gint old_width, old_height;
  gint x_off, y_off;
#ifdef __CLOCK_IT__
  double clock1, clock2, clock3;
#endif /* __CLOCK_IT__ */

  carver = carver_data->carver;
  layer_ID = carver_data->layer_ID;

  IMAGE_CHECK (image_ID, FALSE);

  LAYER_CHECK (layer_ID, FALSE);
  LAYER_CHECK0 (vals->pres_layer_ID, FALSE);
  LAYER_CHECK0 (vals->disc_layer_ID, FALSE);
  LAYER_CHECK0 (vals->rigmask_layer_ID, FALSE);

  IMAGE_TYPE_CHECK (image_ID, carver_data->base_type);
  BPP_CHECK (layer_ID, carver);
  if (vals->resize_aux_layers == TRUE)
    {
      carver_list = lqr_carver_list_start (carver);
      MEM_CHECK2 (check_aux_layer_bpp (&carver_list, vals->pres_layer_ID));
      MEM_CHECK2 (check_aux_layer_bpp (&carver_list, vals->disc_layer_ID));
      MEM_CHECK2 (check_aux_layer_bpp (&carver_list, vals->rigmask_layer_ID));
    }

  UNFLOAT (layer_ID);
  SELECTION_SAVE (image_ID);
  UNMASK (layer_ID);

  g_snprintf (layer_name, LQR_MAX_NAME_LENGTH, "%s",
            gimp_drawable_get_name (layer_ID));

  old_width = gimp_drawable_width (layer_ID);
  old_height = gimp_drawable_height (layer_ID);
  gimp_drawable_offsets (layer_ID, &x_off, &y_off);

  gimp_layer_set_lock_alpha (layer_ID, FALSE);

  if (vals->resize_aux_layers == TRUE)
    {
      resize_unlock_aux_layer (vals->pres_layer_ID, old_width, old_height, x_off, y_off);
      resize_unlock_aux_layer (vals->disc_layer_ID, old_width, old_height, x_off, y_off);
      resize_unlock_aux_layer (vals->rigmask_layer_ID, old_width, old_height, x_off, y_off);
    }

#ifdef __CLOCK_IT__
  clock1 = (double) clock () / CLOCKS_PER_SEC;
#endif /* __CLOCK_IT__ */

  MEM_CHECK1 (lqr_carver_flatten (carver));

  if (vals->resize_canvas == TRUE)
    {
      gimp_image_resize (image_ID, old_width, old_height, -x_off, -y_off);
      gimp_layer_resize_to_image_size (layer_ID);
    }
  else
    {
      gimp_layer_resize (layer_ID, old_width, old_height, 0, 0);
    }

#ifdef __CLOCK_IT__
  clock2 = (double) clock () / CLOCKS_PER_SEC;
  printf ("[ flattened: %g ]\n", clock2 - clock1);
  fflush (stdout);
#endif /* __CLOCK_IT__ */

  carver_data->ref_w = lqr_carver_get_ref_width (carver);
  carver_data->ref_h = lqr_carver_get_ref_height (carver);
  carver_data->orientation = lqr_carver_get_orientation (carver);
  carver_data->depth = lqr_carver_get_depth (carver);
  carver_data->enl_step = lqr_carver_get_enl_step (carver);

  set_tiles (old_width);

  MEM_CHECK1 (write_carver_to_layer (carver, layer_ID));

  if (vals->resize_aux_layers)
    {
      carver_list = lqr_carver_list_start (carver);
      MEM_CHECK2 (write_aux_carver (&carver_list, vals->pres_layer_ID, old_width, old_height));
      MEM_CHECK2 (write_aux_carver (&carver_list, vals->disc_layer_ID, old_width, old_height));
      MEM_CHECK2 (write_aux_carver (&carver_list, vals->rigmask_layer_ID, old_width, old_height));
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
render_dump_vmap (gint32 image_ID,
        PlugInVals * vals,
        PlugInDrawableVals * drawable_vals,
        PlugInColVals * col_vals,
        CarverData * carver_data,
        gint32 * vmap_layer_ID_p)
{
  LqrCarver *carver;
  LqrVMap *vmap;
  VMapFuncArg vmap_data;
  gint32 layer_ID;
  gchar layer_name[LQR_MAX_NAME_LENGTH];
  gchar vmap_name[LQR_MAX_NAME_LENGTH];
  gint old_width, old_height;
  gint x_off, y_off;
  GimpRGB colour_start, colour_end;
#ifdef __CLOCK_IT__
  double clock1, clock2, clock3;
#endif /* __CLOCK_IT__ */

  carver = carver_data->carver;
  layer_ID = carver_data->layer_ID;

  IMAGE_CHECK (image_ID, FALSE);

  LAYER_CHECK (layer_ID, FALSE);

  IMAGE_TYPE_CHECK (image_ID, carver_data->base_type);

  UNFLOAT (layer_ID);
  SELECTION_SAVE (image_ID);
  UNMASK (layer_ID);

  g_snprintf (layer_name, LQR_MAX_NAME_LENGTH, "%s",
            gimp_drawable_get_name (layer_ID));

  old_width = gimp_drawable_width (layer_ID);
  old_height = gimp_drawable_height (layer_ID);

  gimp_drawable_offsets (layer_ID, &x_off, &y_off);

#ifdef __CLOCK_IT__
  clock1 = (double) clock () / CLOCKS_PER_SEC;
#endif /* __CLOCK_IT__ */

  vmap = lqr_vmap_dump (carver);
  MEM_CHECK (vmap);

  g_snprintf (vmap_name, LQR_MAX_NAME_LENGTH, _("%s seam map"), layer_name);

#ifdef __CLOCK_IT__
  clock2 = (double) clock () / CLOCKS_PER_SEC;
  printf ("[ dumped: %g ]\n", clock2 - clock1);
  fflush (stdout);
#endif /* __CLOCK_IT__ */

  gimp_rgba_set (&colour_start, col_vals->r1, col_vals->g1, col_vals->b1, 1);
  gimp_rgba_set (&colour_end, col_vals->r2, col_vals->g2, col_vals->b2, 1);

  vmap_data.image_ID = image_ID;
  vmap_data.name = vmap_name;
  vmap_data.x_off = x_off;
  vmap_data.y_off = y_off;
  vmap_data.colour_start = colour_start;
  vmap_data.colour_end = colour_end;
  vmap_data.vmap_layer_ID_p = vmap_layer_ID_p;

  set_tiles (lqr_vmap_get_width(vmap));

  MEM_CHECK1 (write_vmap_to_layer (vmap, (gpointer) (&vmap_data)));

#ifdef __CLOCK_IT__
  clock3 = (double) clock () / CLOCKS_PER_SEC;
  printf ("[ finish: %g ]\n\n", clock3 - clock2);
#endif /* __CLOCK_IT__ */

  //gimp_drawable_set_visible (layer_ID, TRUE);
  gimp_image_set_active_layer (image_ID, layer_ID);

  return TRUE;
}

static gboolean
my_progress_end (const gchar * message)
{
  return gimp_progress_end ();
}

static LqrProgress*
progress_init ()
{
  LqrProgress * progress = lqr_progress_new ();
  MEM_CHECK_N (progress);
  lqr_progress_set_init (progress, (LqrProgressFuncInit) gimp_progress_init);
  lqr_progress_set_update (progress, (LqrProgressFuncUpdate) gimp_progress_update);
  lqr_progress_set_end (progress, (LqrProgressFuncEnd) my_progress_end);
  lqr_progress_set_init_width_message (progress, _("Resizing width..."));
  lqr_progress_set_init_height_message (progress,
                                        _("Resizing height..."));
  return progress;
}

static gfloat
rigidity_init (PlugInVals * vals)
{
  if (vals->rigmask_layer_ID != 0)
    {
      return 3 * vals->rigidity;
    }
  else
    {
      return vals->rigidity;
    }
}

static gboolean
compute_ignore_disc_mask (PlugInVals * vals, gint old_width, gint old_height, gint new_width, gint new_height)
{
  if (!vals->no_disc_on_enlarge)
    {
      return FALSE;
    }

  switch (vals->res_order)
    {
      case LQR_RES_ORDER_HOR:
        if ((new_width > old_width) || ((new_width == old_width) && (new_height > old_height)))
          {
            return TRUE;
          }
        break;
      case LQR_RES_ORDER_VERT:
        if ((new_height > old_height) || ((new_height == old_height) && (new_width > old_width)))
          {
            return TRUE;
          }
        break;
      default:
        g_message ("Error: unknown resize order index");
        abort();
    }
  return FALSE;
}

static void
set_tiles (gint width)
{
  gint ntiles = width / gimp_tile_width () + 1;
  gimp_tile_cache_size ((gimp_tile_width () * gimp_tile_height () * ntiles *
                         4 * 2) / 1024 + 1);
}

static gboolean
check_aux_layer_bpp (LqrCarverList ** carver_list_p, gint32 layer_ID)
{
  LqrCarver * aux_carver;
  if (!layer_ID)
    {
      return TRUE;
    }
  aux_carver = lqr_carver_list_current (*carver_list_p);
  BPP_CHECK (layer_ID, aux_carver);
  *carver_list_p = lqr_carver_list_next (*carver_list_p);
  return TRUE;
}

static gboolean
resize_unlock_aux_layer (gint32 layer_ID, gint width, gint height, gint x_off, gint y_off)
{
  gboolean alpha_lock = FALSE;
  gint aux_x_off, aux_y_off;
  if (layer_ID)
    {
      alpha_lock = gimp_layer_get_lock_alpha (layer_ID);
      gimp_layer_set_lock_alpha (layer_ID, FALSE);
      gimp_drawable_offsets (layer_ID, &aux_x_off, &aux_y_off);
      gimp_layer_resize (layer_ID, width, height,
                         aux_x_off - x_off, aux_y_off - y_off);
    }
  return alpha_lock;
}

static LqrCarver*
attach_aux_carver (LqrCarver * carver, gint32 layer_ID, gint width, gint height)
{
  guchar *rgb_buffer;
  LqrCarver * aux_carver;
  gint bpp;

  if (layer_ID)
    {
      rgb_buffer = rgb_buffer_from_layer (layer_ID);
      MEM_CHECK_N (rgb_buffer);
      bpp = gimp_drawable_bpp (layer_ID);
      aux_carver =
        lqr_carver_new (rgb_buffer, width, height, bpp);
                        
      MEM_CHECK_N (aux_carver);
      MEM_CHECK1_N (lqr_carver_attach (carver, aux_carver));
    }
  return carver;
}

static gboolean
write_aux_carver (LqrCarverList ** carver_list_p, gint32 layer_ID, gint width, gint height)
{
  LqrCarver * aux_carver;
  LqrCarverList * carver_list = *carver_list_p;
  if (!layer_ID)
    {
      return TRUE;
    }
  gimp_layer_resize (layer_ID, width, height, 0, 0);
  aux_carver = lqr_carver_list_current (carver_list);
  MEM_CHECK1 (write_carver_to_layer (aux_carver, layer_ID));
  *carver_list_p = lqr_carver_list_next (carver_list);
  return TRUE;
}


static void
scale_layer_translated (gint32 layer_ID, gint width, gint height, gint x_off, gint y_off)
{
  gimp_layer_translate (layer_ID, -x_off, -y_off);
  gimp_layer_scale (layer_ID, width, height, FALSE);
  gimp_layer_translate (layer_ID, x_off, y_off);
}
