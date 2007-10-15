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


#include <math.h>
#include "lqr_gradient.h"

/**** GRADIENT FUNCTIONS ****/

inline double
norm (double x, double y)
{
  return sqrt (x * x + y * y);
}
inline double
norm_bias (double x, double y)
{
  return sqrt (x * x + 0.1 * y * y);
}
inline double
sumabs (double x, double y)
{
  return (fabs (x) + fabs (y)) / 2;
}
inline double
xabs (double x, double y)
{
  return fabs (x);
}
inline double
yabs (double x, double y)
{
  return fabs (y);
}

inline double
zero (double x, double y)
{
  return 0;
}


/**** END OF GRADIENT FUNCTIONS ****/
