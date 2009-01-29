/* GIMP LiquidRescale Plug-in
 * Copyright (C) 2007-2009 Carlo Baldassi (the "Author") <carlobaldassi@gmail.com>.
 * All Rights Reserved.
 *
 * The code in this file is taken from gimpwidgets.c
 * Copyright (C) 2000 Michael Natterer <mitch@gimp.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the Licence, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org.licences/>.
 */

#ifndef __ALT_COORDINATES_H__
#define __ALT_COORDINATES_H__

GtkWidget * alt_coordinates_new (GimpUnit         unit,
                      const gchar     *unit_format,
                      gboolean         menu_show_pixels,
                      gboolean         menu_show_percent,
                      gint             spinbutton_width,
                      AltSizeEntryUpdatePolicy  update_policy,

                      gboolean         chainbutton_active,
                      gboolean         chain_constrains_ratio,

                      const gchar     *xlabel,
                      gdouble          x,
                      gdouble          xres,
                      gdouble          lower_boundary_x,
                      gdouble          upper_boundary_x,
                      gdouble          xsize_0,   /* % */
                      gdouble          xsize_100, /* % */

                      const gchar     *ylabel,
                      gdouble          y,
                      gdouble          yres,
                      gdouble          lower_boundary_y,
                      gdouble          upper_boundary_y,
                      gdouble          ysize_0,   /* % */
                      gdouble          ysize_100  /* % */);

#endif // __ALT_COORDINATES_H__
