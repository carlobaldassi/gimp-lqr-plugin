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


#ifndef __LQR_GRADIENT_H__
#define __LQR_GRADIENT_H__

/**** gradient functions for energy evluation ****/
typedef double (*p_to_f) (double, double);
typedef enum _LqrGradFunc LqrGradFunc;

enum _LqrGradFunc
{
  LQR_GF_NORM,                  /* gradient norm : sqrt(x^2 + y^2)            */
  LQR_GF_NORM_BIAS,             /* gradient biased norm : sqrt(x^2 + 0.1 y^2) */
  LQR_GF_SUMABS,                /* sum of absulte values : |x| + |y|          */
  LQR_GF_XABS,                  /* x absolute value : |x|                     */
  LQR_GF_YABS,                  /* y absolute value : |y|                     */
  LQR_GF_NULL                   /* 0 */
};

double lqr_grad_norm (double x, double y);
double lqr_grad_norm_bias (double x, double y);
double lqr_grad_sumabs (double x, double y);
double lqr_grad_xabs (double x, double y);
double lqr_grad_yabs (double x, double y);
double lqr_grad_zero (double x, double y);

#endif // __LQR_GRADIENT_H__
