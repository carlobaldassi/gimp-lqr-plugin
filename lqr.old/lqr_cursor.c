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

#include "lqr.h"

#ifdef __LQR_DEBUG__
#include <assert.h>
#endif

/**** LQR_CURSOR STRUCT FUNTIONS ****/

/*** constructor and destructor ***/

LqrCursor *
lqr_cursor_create (LqrRaster * owner, gint * vs)
{
  LqrCursor *c;

  TRY_N_N (c = g_try_new (LqrCursor, 1));
  c->o = owner;
  c->vs = vs;
#ifdef __LQR_DEBUG__
  c->initialized = 1;
#endif
  lqr_cursor_reset (c);

  return c;
}

void
lqr_cursor_destroy (LqrCursor * c)
{
  g_free (c);
}

/*** functions for moving around ***/

/* resets to starting point */
void
lqr_cursor_reset (LqrCursor * c)
{
  /* make sure the pointers are initialized */
#ifdef __LQR_DEBUG__
  assert (c->initialized);
#endif /* __LQR_DEBUG__ */

  /* reset coordinates */
  c->x = 0;
  c->y = 0;

  /* set the current point to the beginning of the map */
  c->now = 0;

  /* skip invisible points */
  while ((c->vs[c->now] != 0) && (c->vs[c->now] < c->o->level))
    {
      c->now++;
#ifdef __LQR_DEBUG__
      assert (c->now < c->o->w0);
#endif /* __LQR_DEBUG__ */
    }
}

/* go to next data (first rows, then columns;
 * does nothing if we are already at the top-right corner) */
void
lqr_cursor_next (LqrCursor * c)
{
#ifdef __LQR_DEBUG__
  assert (c->initialized);
#endif /* __LQR_DEBUG__ */

  /* update coordinates */
  if (c->x == c->o->w - 1)
    {
      if (c->y == c->o->h - 1)
        {
          /* top-right corner, do nothing */
          return;
        }
      /* end-of-line, carriage return */
      c->x = 0;
      c->y++;
    }
  else
    {
      /* simple right move */
      c->x++;
    }

  /* first move */
  c->now++;
#ifdef __LQR_DEBUG__
  assert (c->now < (c->o->w0 * c->o->h0));
#endif /* __LQR_DEBUG__ */

  /* skip invisible points */
  while ((c->vs[c->now] != 0) && (c->vs[c->now] < c->o->level))
    {
      c->now++;
#ifdef __LQR_DEBUG__
      assert (c->now < (c->o->w0 * c->o->h0));
#endif /* __LQR_DEBUG__ */
    }
}

/* go to previous data (behaves opposite to next) */
void
lqr_cursor_prev (LqrCursor * c)
{
  /* update coordinates */
  if (c->x == 0)
    {
      if (c->y == 0)
        {
          /* bottom-right corner, do nothing */
          return;
        }
      /* carriage return */
      c->x = c->o->w - 1;
      c->y--;
    }
  else
    {
      /* simple left move */
      c->x--;
    }

  /* first move */
  c->now--;
#ifdef __LQR_DEBUG__
  assert (c->now >= 0);
#endif /* __LQR_DEBUG__ */

  /* skip invisible points */
  while ((c->vs[c->now] != 0) && (c->vs[c->now] < c->o->level))
    {
      c->now--;
#ifdef __LQR_DEBUG__
      assert (c->now >= 0);
#endif /* __LQR_DEBUG__ */
    }
}

/*** methods for exploring neighborhoods ***/

/* these return pointers to neighboring data
 * it is an error to ask for out-of-bounds data */

gint
lqr_cursor_left (LqrCursor * c)
{
  /* create an auxiliary pointer */
  gint ret = c->now;

#ifdef __LQR_DEBUG__
  assert (c->initialized);
  assert (c->x > 0);
#endif /* __LQR_DEBUG__ */

  /* first move */
  ret--;
#ifdef __LQR_DEBUG__
  assert (ret >= 0);
#endif /* __LQR_DEBUG__ */

  /* skip invisible points */
  while ((c->vs[ret] != 0) && c->vs[ret] < c->o->level)
    {
      ret--;
#ifdef __LQR_DEBUG__
      assert (ret >= 0);
#endif /* __LQR_DEBUG__ */
    }
  return ret;
}


/**** END OF LQR_CURSOR_CURSOR CLASS FUNCTIONS ****/
