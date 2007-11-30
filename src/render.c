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

#include "config.h"

#include <gtk/gtk.h>

#include <libgimp/gimp.h>

#include <lqr/lqr.h>

#include "io_functions.h"

#include "main.h"
#include "render.h"
#include "plugin-intl.h"


#define MEMCHECK(x) if ((x) == FALSE) { g_message(_("Not enough memory")); return FALSE; }

gboolean
my_progress_end (const gchar * message)
{
  return gimp_progress_end();
}

gboolean
render (gint32 image_ID,
        GimpDrawable * drawable,
        PlugInVals * vals,
        PlugInImageVals * image_vals, PlugInDrawableVals * drawable_vals,
        PlugInColVals * col_vals)
{
  LqrCarver *rasta;
  gint32 mask_ID;
  gint32 layer_ID;
  gchar layer_name[LQR_MAX_NAME_LENGTH];
  gchar new_layer_name[LQR_MAX_NAME_LENGTH];
  guchar * rgb_buffer;
  gboolean alpha_lock;
  gboolean alpha_lock_pres = FALSE, alpha_lock_disc = FALSE;
  gint ntiles;
  gint old_width, old_height;
  gint new_width, new_height;
  gint bpp;
  gint x_off, y_off, aux_x_off, aux_y_off;
  GimpDrawable * drawable_pres, * drawable_disc;
  GimpRGB colour_start, colour_end;
  LqrProgress * lqr_progress;
#ifdef __LQR_CLOCK__
  double clock1, clock2, clock3, clock4;
#endif /* __LQR_CLOCK__ */

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

  lqr_progress = lqr_progress_new();
  MEMCHECK(lqr_progress != NULL);
  lqr_progress->init = (LqrProgressFuncInit) gimp_progress_init;
  lqr_progress->update = (LqrProgressFuncUpdate) gimp_progress_update;
  lqr_progress->end = (LqrProgressFuncEnd) my_progress_end;
  lqr_progress_set_init_width_message(lqr_progress, _("Resizing width..."));
  lqr_progress_set_init_height_message(lqr_progress, _("Resizing height..."));

#ifdef __LQR_CLOCK__
  clock1 = (double) clock () / CLOCKS_PER_SEC;
  printf ("[ begin: clock: %g ]\n", clock1);
#endif /* __LQR_CLOCK__ */

  /* lqr carver initialization */
  rgb_buffer = rgb_buffer_from_layer (layer_ID);
  MEMCHECK (rgb_buffer != NULL);
  rasta = lqr_carver_new (rgb_buffer, old_width, old_height, bpp);
  MEMCHECK (rasta != NULL);
  MEMCHECK (lqr_carver_init (rasta, vals->delta_x, vals->rigidity));
  MEMCHECK (update_bias (rasta, vals->pres_layer_ID, vals->pres_coeff, x_off, y_off));
  MEMCHECK (update_bias (rasta, vals->disc_layer_ID, -vals->disc_coeff, x_off, y_off));
  lqr_carver_set_gradient_function (rasta, vals->grad_func);
  lqr_carver_set_resize_order (rasta, vals->res_order);
  lqr_carver_set_progress (rasta, lqr_progress);
  if (vals->output_seams)
    {
      lqr_carver_set_output_seams(rasta);
    }
  if (vals->resize_aux_layers)
    {
      if (vals->pres_layer_ID)
        {
          rgb_buffer = rgb_buffer_from_layer (vals->pres_layer_ID);
	  MEMCHECK (rgb_buffer != NULL);
	  MEMCHECK (lqr_carver_attach_pres_layer(rasta, rgb_buffer, gimp_drawable_bpp(vals->pres_layer_ID)));
	}
      if (vals->disc_layer_ID)
        {
          rgb_buffer = rgb_buffer_from_layer (vals->disc_layer_ID);
	  MEMCHECK (rgb_buffer != NULL);
	  MEMCHECK (lqr_carver_attach_disc_layer(rasta, rgb_buffer, gimp_drawable_bpp(vals->disc_layer_ID)));
	}
    }

#ifdef __LQR_CLOCK__
  clock2 = (double) clock () / CLOCKS_PER_SEC;
  printf ("[ read: clock: %g (%g) ]\n", clock2, clock2 - clock1);
#endif /* __LQR_CLOCK__ */

  MEMCHECK (lqr_carver_resize (rasta, new_width, new_height));

  switch (vals->oper_mode)
    {
      case LQR_MODE_NORMAL:
	break;
      case LQR_MODE_LQRBACK:
	MEMCHECK (lqr_carver_flatten (rasta));
	if (vals->resize_aux_layers == TRUE)
	  {
	    if (vals->pres_layer_ID != 0)
	      {
		MEMCHECK (lqr_carver_flatten (rasta->pres_carver));
	      }
	    if (vals->disc_layer_ID != 0)
	      {
		MEMCHECK (lqr_carver_flatten (rasta->disc_carver));
		MEMCHECK (update_bias (rasta, vals->disc_layer_ID, 2 * vals->disc_coeff, x_off, y_off));
	      }
	  }
	new_width = old_width;
	new_height = old_height;
	
	switch (vals->res_order)
	  {
	    case LQR_RES_ORDER_HOR:
	      rasta->resize_order = LQR_RES_ORDER_VERT;
	      break;
	    case LQR_RES_ORDER_VERT:
	      rasta->resize_order = LQR_RES_ORDER_HOR;
	      break;
	  }
	MEMCHECK (lqr_carver_resize (rasta, new_width, new_height));
	break;
      case LQR_MODE_SCALEBACK:
	break;
      default:
	g_message("error: unknown mode");
	return FALSE;
    }

  write_all_vmaps (rasta->flushed_vs, image_ID, layer_name, x_off, y_off, colour_start, colour_end);

  if (vals->resize_canvas == TRUE)
    {
      gimp_image_resize (image_ID, new_width, new_height, -x_off,
                         -y_off);
      gimp_layer_resize_to_image_size (layer_ID);
    }
  else
    {
      gimp_layer_resize (layer_ID, new_width, new_height, 0, 0);
    }
  gimp_drawable_detach (drawable);
  drawable = gimp_drawable_get (layer_ID);

#ifdef __LQR_CLOCK__
  clock3 = (double) clock () / CLOCKS_PER_SEC;
  printf ("[ resized: clock : %g (%g) ]\n", clock3, clock3 - clock2);
  fflush (stdout);
#endif /* __LQR_CLOCK__ */

  ntiles = vals->new_width / gimp_tile_width () + 1;
  gimp_tile_cache_size ((gimp_tile_width () * gimp_tile_height () * ntiles *
                         4 * 2) / 1024 + 1);

  MEMCHECK (write_carver_to_layer (rasta, drawable));

  if (vals->resize_aux_layers == TRUE)
    {
      if (vals->pres_layer_ID != 0)
        {
          gimp_layer_resize (vals->pres_layer_ID, new_width, new_height, 0, 0);
	  drawable_pres = gimp_drawable_get(vals->pres_layer_ID);
          MEMCHECK (write_carver_to_layer (rasta->pres_carver, drawable_pres));
	  gimp_drawable_detach(drawable_pres);
        }
      if (vals->disc_layer_ID != 0)
        {
          gimp_layer_resize (vals->disc_layer_ID, new_width, new_height, 0, 0);
	  drawable_disc = gimp_drawable_get(vals->disc_layer_ID);
          MEMCHECK (write_carver_to_layer (rasta->disc_carver, drawable_disc));
	  gimp_drawable_detach(drawable_disc);
        }
    }

  lqr_carver_destroy (rasta);
  switch (vals->oper_mode)
    {
      case LQR_MODE_NORMAL:
      case LQR_MODE_LQRBACK:
	break;
      case LQR_MODE_SCALEBACK:
	if (vals->resize_canvas == TRUE)
	  {
	    gimp_image_resize (image_ID, old_width, old_height, 0, 0);
	    gimp_layer_scale (layer_ID, old_width, old_height, FALSE);
	  }
	else
	  {
	    gimp_layer_translate(layer_ID, -x_off, -y_off);
	    gimp_layer_scale (layer_ID, old_width, old_height, FALSE);
	    gimp_layer_translate(layer_ID, x_off, y_off);
	  }
	gimp_drawable_detach (drawable);
	drawable = gimp_drawable_get (layer_ID);
	if (vals->resize_aux_layers == TRUE)
	  {
	    if (vals->pres_layer_ID != 0)
	      {
		gimp_layer_translate(vals->pres_layer_ID, -x_off, -y_off);
		gimp_layer_scale (vals->pres_layer_ID, old_width,
				   old_height, FALSE);
		gimp_layer_translate(vals->pres_layer_ID, x_off, y_off);
	      }
	    if (vals->disc_layer_ID != 0)
	      {
		gimp_layer_translate(vals->disc_layer_ID, -x_off, -y_off);
		gimp_layer_scale (vals->disc_layer_ID, old_width,
				   old_height, FALSE);
		gimp_layer_translate(vals->disc_layer_ID, x_off, y_off);
	      }
	  }
	break;
      default:
	g_message("error: unknown mode");
	return FALSE;
    }



#ifdef __LQR_CLOCK__
  clock4 = (double) clock () / CLOCKS_PER_SEC;
  printf ("[ finish: clock: %g (%g) ]\n\n", clock4, clock4 - clock3);
#endif /* __LQR_CLOCK__ */

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
