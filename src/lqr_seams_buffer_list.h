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

#ifndef __LQR_SEAMS_BUFFER_LIST_H__
#define __LQR_SEAMS_BUFFER_LIST_H__

#ifndef __LQR_BASE_H__
#error "lqr_base.h must be included prior to lqr_seams_buffer_list.h"
#endif /* __LQR_BASE_H__ */

#ifndef __LQR_SEAMS_BUFFER_H__
#error "lqr_seams_buffer.h must be included prior to lqr_seams_buffer_list.h"
#endif

/**** LQR_SEAMS_BUFFER_LIST CLASS DEFINITION ****/
struct _LqrSeamsBufferList;

typedef struct _LqrSeamsBufferList LqrSeamsBufferList;

struct _LqrSeamsBufferList
{
  LqrSeamsBuffer * current;
  LqrSeamsBufferList * next;
};

/* LQR_SEAMS_BUFFER_LIST FUNCTIONS */

LqrSeamsBufferList * lqr_seams_buffer_list_append (LqrSeamsBufferList * list, LqrSeamsBuffer * buffer);
void lqr_seams_buffer_list_destroy (LqrSeamsBufferList * list);

gboolean lqr_seams_buffer_list_foreach (LqrSeamsBufferList * list, LqrSeamsBufferFunc func, gpointer data);

#endif /* __LQR_SEAMS_BUFFER__ */


