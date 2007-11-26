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

#ifndef __LQR_SEAMS_BUFFER_H__
#define __LQR_SEAMS_BUFFER_H__

#ifndef __LQR_H__
#error "lqr.h must be included prior to lqr_seams_buffer.h"
#endif /* __LQR_H__ */

/*** LQR_SEAMS_BUFFER CLASS DEFINITION ***/

struct _LqrSeamsBuffer
{
  guchar * buffer;
  gint width;
  gint height;
};

/* LQR_SEAMS_BUFFER FUNCTIONS */

LqrSeamsBuffer* lqr_seams_buffer_new (guchar * buffer, gint width, gint heigth);
void lqr_seams_buffer_destroy (LqrSeamsBuffer * seams_buffer);

gboolean lqr_seams_buffer_flush_vs (LqrRaster * r);


#endif /* __LQR_SEAMS_BUFFER__ */

