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

#ifndef __LQR_CARVER_H__
#define __LQR_CARVER_H__

#ifndef __LQR_H__
#error "lqr.h must be included prior to lqr_carver.h"
#endif /* __LQR_H__ */

#ifndef __LQR_GRADIENT_H__
#error "lqr_gradient.h must be included prior to lqr_carver.h"
#endif /* __LQR_GRADIENT_H__ */

/**** LQR_CARVER CLASS DEFINITION ****/
struct _LqrCarver
{
  gint32 image_ID;              /* the GIMP image ID of the layer */
  gchar name[LQR_MAX_NAME_LENGTH];      /* the layer name */
  gint w_start, h_start;        /* original width & height */
  gint w, h;                    /* current width & height */
  gint x_off, y_off;            /* layer offsets in the GIMP coordinate system */

  gint bpp;                     /* number of bpp of the image */

  gint transposed;              /* flag to set transposed state */
  gint aux;			/* flag to set if raster is auxiliary */

  gboolean resize_aux_layers;   /* flag to determine whether the auxiliary layers are resized */
  gboolean output_seams;        /* flag to determine whether to output the seam map */
  GimpRGB seam_color_start;     /* start color for the seam map */
  GimpRGB seam_color_end;       /* end color for the seam map */

  LqrCarver *pres_raster;       /* preservation layer raster */
  LqrCarver *disc_raster;       /* discard layer raster */

  gint rigidity;                /* rigidity value (can straighten seams) */
  gdouble *rigidity_map;        /* the rigidity function */
  gint delta_x;                 /* max displacement of seams (currently is only meaningful if 0 or 1 */

  guchar *rgb;                  /* array of rgb points */
  gdouble *en;                  /* array of energy levels */
  gdouble *bias;                /* array of bias levels */
  gdouble *m;			/* array of auxiliary energy values */
  gint *least_x;		/* array of x coordinates of the least array */

  gint *vpath_x;                /* array of abscisses representing a vertical seam */

  p_to_f gf;                    /* pointer to a gradient function */

};


/* LQR_CARVER CLASS FUNCTIONS */

/* build maps */
void lqr_carver_shrink (LqrCarver * r, gint depth);      /* build all */
void lqr_carver_build_emap (LqrCarver * r);     /* energy */
void lqr_carver_build_mmap (LqrCarver * r);     /* minpath */

/* internal functions for maps computation */
inline gdouble lqr_carver_read (LqrCarver * r, gint x, gint y); /* read the average value at given point */
void lqr_carver_compute_e (LqrCarver * r, gint x, gint y);      /* compute energy of point at c (fast) */
void lqr_carver_update_emap (LqrCarver * r);    /* update energy map after seam removal */
void lqr_carver_update_mmap (LqrCarver * r);    /* minpath */
void lqr_carver_build_vpath (LqrCarver * r);    /* compute seam path */
void lqr_carver_carve (LqrCarver * r);  /* updates the "raw" buffer */
void lqr_carver_copy_vpath (LqrCarver * r, LqrCarver * dest);   /* copy vpath on another raster */

/* image manipulations */
gboolean lqr_carver_transpose (LqrCarver * r);

/* constructor & destructor */
LqrCarver *lqr_carver_new (gint32 image_ID, GimpDrawable * drawable,
                           gchar * name, gint32 pres_layer_ID,
                           gint pres_coeff, gint32 disc_layer_ID,
                           gint disc_coeff, LqrGradFunc gf_int,
                           gint rigidity, gboolean resize_aux_layers,
                           gboolean output_seams, GimpRGB seam_color_start,
                           GimpRGB seam_color_end);
LqrCarver *lqr_carver_aux_new (gint32 image_ID, GimpDrawable * drawable,
                               gchar * name);
void lqr_carver_destroy (LqrCarver * r);

/* set gradient function */
void lqr_carver_set_gf (LqrCarver * r, LqrGradFunc gf_ind);

/* image manipulations */
gboolean lqr_carver_resize (LqrCarver * r, gint w1, gint h1);     /* liquid resize */

#endif /* __LQR_CARVER_H__ */
