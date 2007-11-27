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

#include <glib.h>

#include "lqr_base.h"
#include "lqr_seams_buffer.h"
#include "lqr_seams_buffer_list.h"

#ifdef __LQR_DEBUG__
#include <assert.h>
#endif /* __LQR_DEBUG__ */


/**** SEAMS BUFFER LIST FUNCTIONS ****/

LqrSeamsBufferList * 
lqr_seams_buffer_list_append (LqrSeamsBufferList * list, LqrSeamsBuffer * buffer)
{
  LqrSeamsBufferList * prev = NULL;
  LqrSeamsBufferList * now = list;
  while (now != NULL)
    {
      prev = now;
      now = now->next;
    }
  TRY_N_N (now = g_try_new(LqrSeamsBufferList, 1));
  now->next = NULL;
  now->current = buffer;
  if (prev)
    {
      prev->next = now;
    }
  if (list == NULL)
    {
      return now;
    }
  else
    {
      return list;
    }
}

void
lqr_seams_buffer_list_destroy(LqrSeamsBufferList * list)
{
  LqrSeamsBufferList * now = list;
  if (now != NULL)
    {
      lqr_seams_buffer_list_destroy(now->next);
      lqr_seams_buffer_destroy(now->current);
    }
}

gboolean
lqr_seams_buffer_list_foreach (LqrSeamsBufferList * list, LqrSeamsBufferFunc func, gpointer data)
{
  LqrSeamsBufferList * now = list;
  if (now != NULL)
    {
      TRY_F_F (func(now->current, data));
      return lqr_seams_buffer_list_foreach (now->next, func, data);
    }
  return TRUE;
}

