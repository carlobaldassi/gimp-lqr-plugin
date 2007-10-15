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

#include <string.h>
#include <libgimp/gimp.h>

#include "lqr.h"
#include "lqr_data.h"

/**** DATA CLASS FUNCTIONS ****/

/* reset data */
void
lqr_data_reset (LqrData * d, int bpp)
{
  int k;
  d->e = 0;
  d->b = 0;
  d->m = 0;
  d->vs = 0;
  d->least = NULL;
  d->least_x = 0;
  for (k = 0; k < bpp; k++)
    {
      d->rgb[k] = 0;
    }
}


/* read average color value */
double
lqr_data_read (LqrData * d, int bpp)
{
  double sum = 0;
  int k;
  for (k = 0; k < bpp; k++)
    {
      sum += d->rgb[k];
    }
  return sum / (255 * bpp);
}

/**** END OF LQR_DATA CLASS FUNCTIONS ****/
