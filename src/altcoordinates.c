/* GIMP LiquidRescale Plug-in
 * Copyright (C) 2007-2010 Carlo Baldassi (the "Author") <carlobaldassi@gmail.com>.
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

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "altsizeentry.h"
#include "altcoordinates.h"

typedef struct
{
  GimpChainButton *chainbutton;
  gboolean         chain_constrains_ratio;
  gdouble          orig_x;
  gdouble          orig_y;
  gdouble          last_x;
  gdouble          last_y;
} AltCoordinatesData;


static void alt_coordinates_callback (GtkWidget *widget, AltCoordinatesData *data);
static void alt_coordinates_data_free (AltCoordinatesData *data);
static void alt_coordinates_chainbutton_toggled (GimpChainButton *button, AltSizeEntry   *entry);


static void
alt_coordinates_callback (GtkWidget           *widget,
                           AltCoordinatesData *data)
{
  gdouble new_x;
  gdouble new_y;

  new_x = alt_size_entry_get_refval (ALT_SIZE_ENTRY (widget), 0);
  new_y = alt_size_entry_get_refval (ALT_SIZE_ENTRY (widget), 1);

  if (gimp_chain_button_get_active (data->chainbutton))
    {
      if (data->chain_constrains_ratio)
        {
          if ((data->orig_x != 0) && (data->orig_y != 0))
            {
              if (ROUND (new_x) != ROUND (data->last_x))
                {
                  data->last_x = new_x;
                  new_y = (new_x * data->orig_y) / data->orig_x;

                  alt_size_entry_set_refval (ALT_SIZE_ENTRY (widget), 1,
                                              new_y);
                  data->last_y
                    = alt_size_entry_get_refval (ALT_SIZE_ENTRY (widget), 1);
                }
              else if (ROUND (new_y) != ROUND (data->last_y))
                {
                  data->last_y = new_y;
                  new_x = (new_y * data->orig_x) / data->orig_y;

                  alt_size_entry_set_refval (ALT_SIZE_ENTRY (widget), 0,
                                              new_x);
                  data->last_x
                    = alt_size_entry_get_refval (ALT_SIZE_ENTRY (widget), 0);
                }
            }
        }
      else
        {
          if (new_x != data->last_x)
            {
              new_y = new_x;

              alt_size_entry_set_refval (ALT_SIZE_ENTRY (widget), 1, new_x);
              data->last_y = data->last_x
                = alt_size_entry_get_refval (ALT_SIZE_ENTRY (widget), 1);
            }
          else if (new_y != data->last_y)
            {
              new_x = new_y;

              alt_size_entry_set_refval (ALT_SIZE_ENTRY (widget), 0, new_y);
              data->last_x = data->last_y
                = alt_size_entry_get_refval (ALT_SIZE_ENTRY (widget), 0);
            }
        }
    }
  else
    {
      if (new_x != data->last_x)
        data->last_x = new_x;
      if (new_y != data->last_y)
        data->last_y = new_y;
    }
}

static void
alt_coordinates_data_free (AltCoordinatesData *data)
{
  g_slice_free (AltCoordinatesData, data);
}

static void
alt_coordinates_chainbutton_toggled (GimpChainButton *button,
                                      AltSizeEntry   *entry)
{
  if (gimp_chain_button_get_active (button))
    {
      AltCoordinatesData *data;

      data = g_object_get_data (G_OBJECT (entry), "alt-coordinates-data");

      data->orig_x = alt_size_entry_get_refval (entry, 0);
      data->orig_y = alt_size_entry_get_refval (entry, 1);
    }
}

/**
 * gimp_coordinates_new:
 * @unit:                   The initial unit of the #GimpUnitMenu.
 * @unit_format:            A printf-like unit-format string as is used with
 *                          gimp_unit_menu_new().
 * @menu_show_pixels:       %TRUE if the #GimpUnitMenu should contain an item
 *                          for GIMP_UNIT_PIXEL.
 * @menu_show_percent:      %TRUE if the #GimpUnitMenu should contain an item
 *                          for GIMP_UNIT_PERCENT.
 * @spinbutton_width:       The horizontal size of the #AltSizeEntry's
 *                           #GtkSpinButton's.
 * @update_policy:          The update policy for the #AltSizeEntry.
 * @chainbutton_active:     %TRUE if the attached #GimpChainButton should be
 *                          active.
 * @chain_constrains_ratio: %TRUE if the chainbutton should constrain the
 *                          fields' aspect ratio. If %FALSE, the values will
 *                          be constrained.
 * @xlabel:                 The label for the X coordinate.
 * @x:                      The initial value of the X coordinate.
 * @xres:                   The horizontal resolution in DPI.
 * @lower_boundary_x:       The lower boundary of the X coordinate.
 * @upper_boundary_x:       The upper boundary of the X coordinate.
 * @xsize_0:                The X value which will be treated as 0%.
 * @xsize_100:              The X value which will be treated as 100%.
 * @ylabel:                 The label for the Y coordinate.
 * @y:                      The initial value of the Y coordinate.
 * @yres:                   The vertical resolution in DPI.
 * @lower_boundary_y:       The lower boundary of the Y coordinate.
 * @upper_boundary_y:       The upper boundary of the Y coordinate.
 * @ysize_0:                The Y value which will be treated as 0%.
 * @ysize_100:              The Y value which will be treated as 100%.
 *
 * Convenience function that creates a #AltSizeEntry with two fields for x/y
 * coordinates/sizes with a #GimpChainButton attached to constrain either the
 * two fields' values or the ratio between them.
 *
 * Returns: The new #AltSizeEntry.
 **/
GtkWidget *
alt_coordinates_new (GimpUnit         unit,
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
                      gdouble          ysize_100  /* % */)
{
  AltCoordinatesData *data;
  GtkObject           *adjustment;
  GtkWidget           *spinbutton;
  GtkWidget           *sizeentry;
  GtkWidget           *chainbutton;

  spinbutton = gimp_spin_button_new (&adjustment, 1, 0, 1, 1, 10, 0, 1, 2);

  if (spinbutton_width > 0)
    {
      if (spinbutton_width < 17)
        gtk_entry_set_width_chars (GTK_ENTRY (spinbutton), spinbutton_width);
      else
        gtk_widget_set_size_request (spinbutton, spinbutton_width, -1);
    }

  sizeentry = alt_size_entry_new (1, unit, unit_format,
                                   menu_show_pixels,
                                   menu_show_percent,
                                   FALSE,
                                   spinbutton_width,
                                   update_policy);
  gtk_table_set_col_spacing (GTK_TABLE (sizeentry), 0, 4);
  gtk_table_set_col_spacing (GTK_TABLE (sizeentry), 2, 4);
  alt_size_entry_add_field (ALT_SIZE_ENTRY (sizeentry),
                             GTK_SPIN_BUTTON (spinbutton), NULL);
  gtk_table_attach_defaults (GTK_TABLE (sizeentry), spinbutton, 1, 2, 0, 1);
  gtk_widget_show (spinbutton);

  alt_size_entry_set_unit (ALT_SIZE_ENTRY (sizeentry),
                            (update_policy == ALT_SIZE_ENTRY_UPDATE_RESOLUTION) ||
                            (menu_show_pixels == FALSE) ?
                            GIMP_UNIT_INCH : GIMP_UNIT_PIXEL);

  alt_size_entry_set_resolution (ALT_SIZE_ENTRY (sizeentry), 0, xres, TRUE);
  alt_size_entry_set_resolution (ALT_SIZE_ENTRY (sizeentry), 1, yres, TRUE);
  alt_size_entry_set_refval_boundaries (ALT_SIZE_ENTRY (sizeentry), 0,
                                         lower_boundary_x, upper_boundary_x);
  alt_size_entry_set_refval_boundaries (ALT_SIZE_ENTRY (sizeentry), 1,
                                         lower_boundary_y, upper_boundary_y);

  if (menu_show_percent)
    {
      alt_size_entry_set_size (ALT_SIZE_ENTRY (sizeentry), 0,
                                xsize_0, xsize_100);
      alt_size_entry_set_size (ALT_SIZE_ENTRY (sizeentry), 1,
                                ysize_0, ysize_100);
    }

  alt_size_entry_set_refval (ALT_SIZE_ENTRY (sizeentry), 0, x);
  alt_size_entry_set_refval (ALT_SIZE_ENTRY (sizeentry), 1, y);

  alt_size_entry_attach_label (ALT_SIZE_ENTRY (sizeentry),
                                xlabel, 0, 0, 0.0);
  alt_size_entry_attach_label (ALT_SIZE_ENTRY (sizeentry),
                                ylabel, 1, 0, 0.0);

  chainbutton = gimp_chain_button_new (GIMP_CHAIN_RIGHT);

  if (chainbutton_active)
    gimp_chain_button_set_active (GIMP_CHAIN_BUTTON (chainbutton), TRUE);

  gtk_table_attach (GTK_TABLE (sizeentry), chainbutton, 2, 3, 0, 2,
                    GTK_SHRINK | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_widget_show (chainbutton);

  data = g_slice_new (AltCoordinatesData);

  data->chainbutton            = GIMP_CHAIN_BUTTON (chainbutton);
  data->chain_constrains_ratio = chain_constrains_ratio;
  data->orig_x                 = x;
  data->orig_y                 = y;
  data->last_x                 = x;
  data->last_y                 = y;

  g_object_set_data_full (G_OBJECT (sizeentry), "alt-coordinates-data",
                          data,
                          (GDestroyNotify) alt_coordinates_data_free);

  g_signal_connect (sizeentry, "value-changed",
                    G_CALLBACK (alt_coordinates_callback),
                    data);

  g_object_set_data (G_OBJECT (sizeentry), "chainbutton", chainbutton);

  g_signal_connect (chainbutton, "toggled",
                    G_CALLBACK (alt_coordinates_chainbutton_toggled),
                    sizeentry);

  return sizeentry;
}

