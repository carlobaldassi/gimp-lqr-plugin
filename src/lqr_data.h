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


#ifndef __LQR_DATA_H__
#define __LQR_DATA_H__

#ifndef __LQR_H__
#error "lqr.h must be included prior to lqr_data.h"
#endif /* __LQR_H__ */

#define _LQR_DATA_MAX_BPP (4)

/**** LQR_DATA CLASS DEFINITION ****/
/* This is the basic class representing a point of the multisize image.
 * It holds RGB values, visibility levels and other auxiliary
 * quantities */
struct _LqrData
{
  guchar rgb[_LQR_DATA_MAX_BPP];        /* color map
                                         * (chars between 0 and 255)
                                         */

  gdouble e;                     /* energy */
  gdouble b;                     /* energy bias */
  gdouble m;                     /* auxiliary value for seam construction */
  LqrData *least;               /* pointer to the predecessor with least m-value */
  gint least_x;                  /* offset of "least" with respect to current */
  gint vs;                       /* vertical seam level (determines visibiliy)
                                 *   vs = 0       -> uninitialized (visible)
                                 *   vs < level   -> invisible
                                 *   vs >= level  -> visible
                                 */
};

/* LQR_DATA CLASS FUNCTIONS */

/* reset */
void lqr_data_reset (LqrData * d, gint bpp);

/* read average color value */
double lqr_data_read (LqrData * d, gint bpp);

#endif /* __LQR_DATA_H__ */
