/* GIMP LiquidRescale Plug-in
 * Copyright (C) 2007-2009 Carlo Baldassi (the "Author") <carlobaldassi@gmail.com>.
 * All Rights Reserved.
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


#ifndef __PREVIEW_H__
#define __PREVIEW_H__


/*  Constants  */

#define PREVIEW_MAX_WIDTH  300
#define PREVIEW_MAX_HEIGHT 200

/*  Preview data struct */

typedef struct
{
  gint32 image_ID;
  gint32 orig_layer_ID;
  gint32 layer_ID;
  GimpImageType type;
  gint width;
  gint height;
  gint old_width;
  gint old_height;
  gint x_off;
  gint y_off;
  gfloat factor;
  guchar *buffer;
  guchar *pres_buffer;
  guchar *disc_buffer;
  guchar *rigmask_buffer;
  guchar *preview_buffer;
  PlugInVals *vals;
  PlugInUIVals *ui_vals;
  GdkPixbuf *pixbuf;
  GtkWidget *area;
  GtkWidget *pres_combo;
  GtkWidget *disc_combo;
  GtkWidget *rigmask_combo;
  gboolean pres_combo_awaked;
  gboolean disc_combo_awaked;
  gboolean rigmask_combo_awaked;
  GtkWidget *coordinates;
  GtkWidget *disc_warning_image;
} PreviewData;

#define PREVIEW_DATA(data) ((PreviewData*)data)


/*  Functions  */

void callback_pres_combo_set_sensitive_preview (GtkWidget * button,
                                                       gpointer data);
void callback_disc_combo_set_sensitive_preview (GtkWidget * button,
                                                       gpointer data);
void callback_rigmask_combo_set_sensitive_preview (GtkWidget * button,
                                                          gpointer data);

void preview_init_mem (PreviewData * preview_data);
guchar * preview_build_buffer (gint32 layer_ID);
void preview_build_pixbuf (PreviewData * preview_data);
void preview_free_pixbuf (guchar * buffer, gpointer data);

gboolean
callback_preview_expose_event (GtkWidget * preview_area,
                               GdkEventExpose * event, gpointer data);

#endif /* __PREVIEW_H__ */

