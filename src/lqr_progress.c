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

#include "lqr_progress.h"

gboolean
lqr_progress_init(LqrProgress * p, const gchar * message)
{
  if (p != NULL)
    {
      return p->init(message);
    }
  else
    {
      return TRUE;
    }
}

gboolean
lqr_progress_update(LqrProgress * p, gdouble percentage)
{
  if (p != NULL)
    {
      return p->update(percentage);
    }
  else
    {
      return TRUE;
    }
}

gboolean
lqr_progress_end(LqrProgress * p)
{
  if (p != NULL)
    {
      return p->end();
    }
  else
    {
      return TRUE;
    }
}
