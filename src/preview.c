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

#include "config.h"

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "plugin-intl.h"
#include "main.h"
#include "preview.h"

extern GtkWidget * dlg;

void
callback_pres_combo_set_sensitive_preview (GtkWidget * button, gpointer data)
{
  PreviewData *p_data = PREVIEW_DATA (data);

  IMAGE_CHECK_ACTION(p_data->image_ID, gtk_dialog_response (GTK_DIALOG (dlg), RESPONSE_FATAL), );
  LAYER_CHECK_ACTION(p_data->vals->pres_layer_ID, gtk_dialog_response (GTK_DIALOG (dlg), RESPONSE_REFRESH), );

  if (p_data->pres_combo_awaked == FALSE)
    {
      g_signal_emit_by_name (G_OBJECT (p_data->pres_combo), "changed");
      p_data->pres_combo_awaked = TRUE;
    }
  preview_build_pixbuf (p_data);
  gtk_widget_queue_draw (p_data->area);
}

void
callback_disc_combo_set_sensitive_preview (GtkWidget * button, gpointer data)
{
  PreviewData *p_data = PREVIEW_DATA (data);

  IMAGE_CHECK_ACTION(p_data->image_ID, gtk_dialog_response (GTK_DIALOG (dlg), RESPONSE_FATAL), );
  LAYER_CHECK_ACTION(p_data->vals->disc_layer_ID, gtk_dialog_response (GTK_DIALOG (dlg), RESPONSE_REFRESH), );

  if (p_data->disc_combo_awaked == FALSE)
    {
      g_signal_emit_by_name (G_OBJECT (p_data->disc_combo), "changed");
      p_data->disc_combo_awaked = TRUE;
    }
  preview_build_pixbuf (p_data);
  gtk_widget_queue_draw (p_data->area);
}

void
callback_rigmask_combo_set_sensitive_preview (GtkWidget * button,
					      gpointer data)
{
  PreviewData *p_data = PREVIEW_DATA (data);

  IMAGE_CHECK_ACTION(p_data->image_ID, gtk_dialog_response (GTK_DIALOG (dlg), RESPONSE_FATAL), );
  LAYER_CHECK_ACTION(p_data->vals->rigmask_layer_ID, gtk_dialog_response (GTK_DIALOG (dlg), RESPONSE_REFRESH), );

  if (p_data->rigmask_combo_awaked == FALSE)
    {
      g_signal_emit_by_name (G_OBJECT (p_data->rigmask_combo), "changed");
      p_data->rigmask_combo_awaked = TRUE;
    }
  preview_build_pixbuf (p_data);
  gtk_widget_queue_draw (p_data->area);
}


/* Preview construction */

void
preview_init_mem (PreviewData * p_data)
{
  p_data->buffer = NULL;
  p_data->pres_buffer = NULL;
  p_data->disc_buffer = NULL;
  p_data->rigmask_buffer = NULL;
  p_data->pixbuf = NULL;
}

guchar *
preview_build_buffer (gint32 layer_ID)
{

  gint x, y, k;
  gint width, height;
  GimpPixelRgn rgn_in;
  guchar *inrow;
  GimpDrawable *drawable;
  gboolean isgray;
  guchar *buffer;

  drawable = gimp_drawable_get (layer_ID);
  width = gimp_drawable_width (layer_ID);
  height = gimp_drawable_height (layer_ID);
  isgray = gimp_drawable_is_gray (layer_ID);

  gimp_pixel_rgn_init (&rgn_in, drawable, 0, 0, width, height, FALSE, FALSE);

  inrow = g_new (guchar, drawable->bpp * width);
  buffer = g_new (guchar, 4 * width * height);

  for (y = 0; y < height; y++)
    {
      gimp_pixel_rgn_get_row (&rgn_in, inrow, 0, y, width);

      for (x = 0; x < width; x++)
	{
	  for (k = 0; k < 3; k++)
	    {
	      if (isgray)
		{
		  buffer[(y * width + x) * 4 + k] = inrow[2 * x];
		}
	      else
		{
		  buffer[(y * width + x) * 4 + k] = inrow[4 * x + k];
		}
	    }
	  if (isgray)
	    {
	      buffer[(y * width + x) * 4 + 3] = inrow[2 * x + 1];
	    }
	  else
	    {
	      buffer[(y * width + x) * 4 + 3] = inrow[4 * x + 3];
	    }
	}

    }

  g_free (inrow);
  gimp_drawable_detach (drawable);
  return buffer;
}

void
preview_build_pixbuf (PreviewData * preview_data)
{
  gint bpp;
  gint x, y, k;
  gint index, index1;
  gdouble tfactor_orig, tfactor_pres, tfactor_disc, tfactor_rigmask, tfactor;
  gdouble value;

  if (preview_data->pixbuf != NULL)
    {
      g_object_unref (G_OBJECT (preview_data->pixbuf));
    }

  preview_data->preview_buffer =
    g_new (guchar, preview_data->width * preview_data->height * 4);

  bpp = 4;

  for (y = 0; y < preview_data->height; y++)
    {
      for (x = 0; x < preview_data->width; x++)
	{
	  index1 = (y * preview_data->width + x);
	  tfactor_orig = 0;
	  tfactor_pres = 1;
	  tfactor_disc = 1;
	  tfactor_rigmask = 1;
	  tfactor_orig =
	    (gdouble) (255 -
		       preview_data->buffer[index1 * bpp + bpp - 1]) / 255;
	  if ((preview_data->pres_buffer != NULL)
	      && (preview_data->ui_vals->pres_status == TRUE))
	    {
	      tfactor_pres =
		(gdouble) (255 -
			   preview_data->pres_buffer[index1 * bpp + bpp -
						     1] / 2) / 255;
	    }
	  if ((preview_data->disc_buffer != NULL)
	      && (preview_data->ui_vals->disc_status == TRUE))
	    {
	      tfactor_disc =
		(gdouble) (255 -
			   0.5 * preview_data->disc_buffer[(index1 + 1) *
							   bpp - 1]) / 255;
	    }
	  if ((preview_data->rigmask_buffer != NULL)
	      && (preview_data->ui_vals->rigmask_status == TRUE))
	    {
	      tfactor_rigmask =
		(gdouble) (255 -
			   0.5 * preview_data->rigmask_buffer[(index1 + 1) *
							      bpp - 1]) / 255;
	    }
	  tfactor =
	    (1 -
	     tfactor_orig) * tfactor_pres * tfactor_disc * tfactor_rigmask;
	  for (k = 0; k < bpp; k++)
	    {
	      index = index1 * bpp + k;
	      value = (tfactor * preview_data->buffer[index]);
	      if (tfactor_pres < 1)
		{
		  value +=
		    (guchar) (tfactor_disc * (1 - tfactor_pres) *
			      preview_data->pres_buffer[index]);
		}
	      if (tfactor_disc < 1)
		{
		  value +=
		    (guchar) ((1 -
			       tfactor_disc) *
			      preview_data->disc_buffer[index]);
		}
	      if (tfactor_rigmask < 1)
		{
		  value +=
		    (guchar) ((1 -
			       tfactor_rigmask) *
			      preview_data->rigmask_buffer[index]);
		}
	      if (value > 255)
		{
		  value = 255;
		}
	      preview_data->preview_buffer[index] = (guchar) value;
	    }
	}
    }
  preview_data->pixbuf =
    gdk_pixbuf_new_from_data (preview_data->preview_buffer,
			      GDK_COLORSPACE_RGB, TRUE, 8,
			      preview_data->width, preview_data->height,
			      bpp * preview_data->width * sizeof (guchar),
			      preview_free_pixbuf, NULL);
}

void
preview_free_pixbuf (guchar * buffer, gpointer data)
{
  g_free (buffer);
}

gboolean
callback_preview_expose_event (GtkWidget * preview_area,
			       GdkEventExpose * event, gpointer data)
{
  PreviewData *p_data = PREVIEW_DATA (data);

  gdk_draw_pixbuf (p_data->area->window, NULL,
		   p_data->pixbuf, 0, 0,
		   (PREVIEW_MAX_WIDTH - p_data->width) / 2,
		   (PREVIEW_MAX_HEIGHT - p_data->height) / 2,
		   -1, -1, GDK_RGB_DITHER_NORMAL, 0, 0);

  return TRUE;
}
