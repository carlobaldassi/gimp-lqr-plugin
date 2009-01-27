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
