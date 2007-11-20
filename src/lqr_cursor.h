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

#ifndef __LQR_CURSOR_H__
#define __LQR_CURSOR_H__

#ifndef __LQR_H__
#error "lqr.h must be included prior to lqr_cursor.h"
#endif /* __LQR_H__ */

/**** LQR_CURSOR CLASS DEFINITION ****/
/* The lqr_cursors can scan a multisize image according to its
 * current visibility level, skipping invisible points */
struct _LqrCursor
{
#ifdef __LQR_DEBUG__
  gint initialized;             /* initialization flag */
#endif
  gint x;                       /* x coordinate of current data */
  gint y;                       /* y coordinate of current data */
  gint now;                     /* current array position */
  LqrRaster *o;                 /* pointer to owner raster */
  gint *vs;                     /* pointer to owner's visibility map */
};

/* LQR_CURSOR CLASS FUNCTIONS */

/* constructor */
LqrCursor *lqr_cursor_create (LqrRaster * owner, gint * vs);

/* destructor */
void lqr_cursor_destroy (LqrCursor * c);

/* functions for moving around */
void lqr_cursor_reset (LqrCursor * c);
void lqr_cursor_next (LqrCursor * c);
void lqr_cursor_prev (LqrCursor * c);

/* methods for exploring neighborhoods */
gint lqr_cursor_left (LqrCursor * c);

#endif /* __LQR_CURSOR_H__ */
