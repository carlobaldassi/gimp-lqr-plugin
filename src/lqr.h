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


#ifndef __LQR_H__
#define __LQR_H__

#define LQR_MAX_NAME_LENGTH (1024)

#define TRY_N_N(assign) if ((assign) == NULL) { return NULL; }
#define TRY_N_F(assign) if ((assign) == NULL) { return FALSE; }
#define TRY_F_N(assign) if ((assign) == FALSE) { return NULL; }
#define TRY_F_F(assign) if ((assign) == FALSE) { return FALSE; }

#if 0
#define __LQR_DEBUG__
#endif

#if 1
#define __LQR_CLOCK__
#endif

#if 0
#define __LQR_VERBOSE__
#endif

/**** CLASSES DECLARATIONS ****/
typedef struct _LqrCursor LqrCursor;
typedef struct _LqrRaster LqrRaster;

struct _LqrCursor;              /* a "smart" index to read the raster */
struct _LqrRaster;              /* the multisize image raster         */

#endif /* __LQR_H__ */
