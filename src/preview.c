/* GIMP LiquidRescale Plug-in
 * Copyright (C) 2007-2010 Carlo Baldassi (the "Author") <carlobaldassi@gmail.com>.
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

/***  Local functions declarations ***/

static gdouble compute_tfactor(guchar * buffer, gdouble alpha, gint ind, gint bpp);
static gboolean preview_has_pres_buffer(PreviewData * p_data);
static gboolean preview_has_disc_buffer(PreviewData * p_data);
static gboolean preview_has_rigmask_buffer(PreviewData * p_data);

/***  Functions definitions ***/

void
callback_pres_combo_set_sensitive_preview (GtkWidget * button, gpointer data)
{
  PreviewData *p_data = PREVIEW_DATA (data);

  IMAGE_CHECK_ACTION(p_data->image_ID, gtk_dialog_response (GTK_DIALOG (p_data->dlg), RESPONSE_FATAL), );
  LAYER_CHECK_ACTION(p_data->vals->pres_layer_ID, gtk_dialog_response (GTK_DIALOG (p_data->dlg), RESPONSE_REFRESH), );

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

  IMAGE_CHECK_ACTION(p_data->image_ID, gtk_dialog_response (GTK_DIALOG (p_data->dlg), RESPONSE_FATAL), );
  LAYER_CHECK_ACTION(p_data->vals->disc_layer_ID, gtk_dialog_response (GTK_DIALOG (p_data->dlg), RESPONSE_REFRESH), );

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

  IMAGE_CHECK_ACTION(p_data->image_ID, gtk_dialog_response (GTK_DIALOG (p_data->dlg), RESPONSE_FATAL), );
  LAYER_CHECK_ACTION(p_data->vals->rigmask_layer_ID, gtk_dialog_response (GTK_DIALOG (p_data->dlg), RESPONSE_REFRESH), );

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

void
preview_data_create(gint32 image_ID, gint32 layer_ID, PreviewData * p_data)
{
  g_assert(p_data != NULL);

  preview_init_mem (p_data);

  gimp_image_undo_freeze (image_ID);
  p_data->layer_ID = gimp_layer_copy (layer_ID);
  gimp_image_add_layer (image_ID, p_data->layer_ID, 1);

  gimp_layer_scale (p_data->layer_ID, p_data->width,
                    p_data->height, TRUE);
  gimp_layer_add_alpha (p_data->layer_ID);

  p_data->buffer = preview_build_buffer (p_data->layer_ID);

  gimp_image_remove_layer (image_ID, p_data->layer_ID);
  gimp_image_undo_thaw (image_ID);
}

GtkWidget *
preview_area_create(PreviewData * p_data)
{
  p_data->area = gtk_drawing_area_new ();
  gtk_widget_set_size_request (p_data->area, PREVIEW_MAX_WIDTH,
                               PREVIEW_MAX_HEIGHT);
  return p_data->area;
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

static gdouble
compute_tfactor(guchar * buffer, gdouble alpha, gint ind, gint bpp)
{
  return (gdouble) (255 - alpha * buffer[(ind + 1) * bpp - 1]) / 255;
}

static gboolean
preview_has_pres_buffer(PreviewData * p_data)
{
  return ((p_data->pres_buffer) && (p_data->ui_vals->pres_status));
}

static gboolean
preview_has_disc_buffer(PreviewData * p_data)
{
  return ((p_data->disc_buffer) && (p_data->ui_vals->disc_status));
}

static gboolean
preview_has_rigmask_buffer(PreviewData * p_data)
{
  return ((p_data->rigmask_buffer) && (p_data->ui_vals->rigmask_status));
}

void
preview_build_pixbuf (PreviewData * p_data)
{
  gint bpp;
  gint x, y, k;
  gint index, index1;
  gdouble tfactor_orig, tfactor_pres, tfactor_disc, tfactor_rigmask, tfactor;
  gdouble value;

  if (p_data->pixbuf != NULL)
    {
      g_object_unref (G_OBJECT (p_data->pixbuf));
    }

  p_data->preview_buffer =
    g_new (guchar, p_data->width * p_data->height * 4);

  bpp = 4;

  for (y = 0; y < p_data->height; y++)
    {
      for (x = 0; x < p_data->width; x++)
	{
	  index1 = (y * p_data->width + x);
	  tfactor_orig = 0;
	  tfactor_pres = 1;
	  tfactor_disc = 1;
	  tfactor_rigmask = 1;

          tfactor_orig = compute_tfactor(p_data->buffer, 1.0, index1, bpp);
	  if (preview_has_pres_buffer(p_data))
	    {
	      tfactor_pres = compute_tfactor(p_data->pres_buffer, 0.5, index1, bpp);
	    }
	  if (preview_has_disc_buffer(p_data))
	    {
	      tfactor_disc = compute_tfactor(p_data->disc_buffer, 0.5, index1, bpp);
	    }
	  if (preview_has_rigmask_buffer(p_data))
	    {
	      tfactor_rigmask = compute_tfactor(p_data->rigmask_buffer, 0.5, index1, bpp);
	    }
	  tfactor = (1 - tfactor_orig) *
            tfactor_pres * tfactor_disc * tfactor_rigmask;

	  for (k = 0; k < bpp; k++)
	    {
	      index = index1 * bpp + k;
	      value = (tfactor * p_data->buffer[index]);
	      if (tfactor_pres < 1)
		{
		  value +=
		    (guchar) (tfactor_rigmask * tfactor_disc * (1 - tfactor_pres) *
			      p_data->pres_buffer[index]);
		}
	      if (tfactor_disc < 1)
		{
		  value +=
		    (guchar) (tfactor_rigmask * (1 - tfactor_disc) *
			      p_data->disc_buffer[index]);
		}
	      if (tfactor_rigmask < 1)
		{
		  value +=
		    (guchar) ((1 - tfactor_rigmask) *
			      p_data->rigmask_buffer[index]);
		}
              value = MIN(value, 255);
	      p_data->preview_buffer[index] = (guchar) value;
	    }
	}
    }
  /* p_data->pixbuf = gimp_drawable_get_thumbnail(p_data->orig_layer_ID, p_data->width, p_data->height, GIMP_PIXBUF_SMALL_CHECKS); */
  
  p_data->pixbuf =
    gdk_pixbuf_new_from_data (p_data->preview_buffer,
			      GDK_COLORSPACE_RGB, TRUE, 8,
			      p_data->width, p_data->height,
			      bpp * p_data->width * sizeof (guchar),
			      preview_free_pixbuf, NULL);
}

void
preview_free_pixbuf (guchar * buffer, gpointer data)
{
  g_free (buffer);
}

void
callback_preview_expose_event (GtkWidget * preview_area,
			       GdkEventExpose * event, gpointer data)
{
  PreviewData *p_data = PREVIEW_DATA (data);

  gdk_draw_pixbuf (gtk_widget_get_window(p_data->area), NULL,
		   p_data->pixbuf, 0, 0,
		   (PREVIEW_MAX_WIDTH - p_data->width) / 2,
		   (PREVIEW_MAX_HEIGHT - p_data->height) / 2,
		   -1, -1, GDK_RGB_DITHER_NORMAL, 0, 0);

  update_info_aux_use_icons(p_data->vals, p_data->ui_vals, p_data->pres_use_image, p_data->disc_use_image, p_data->rigmask_use_image);
}

void
update_info_aux_use_icons(PlugInVals *vals, PlugInUIVals *ui_vals, GtkWidget *pres_use_image, GtkWidget *disc_use_image, GtkWidget *rigmask_use_image)
{
  gchar help_text_common[MAX_STRING_SIZE];
  gchar help_text[MAX_STRING_SIZE];
  if (ui_vals->pres_status == TRUE)
    {
      g_snprintf(help_text_common, MAX_STRING_SIZE, _("Layer in use as preservation mask: "));
      g_snprintf(help_text, MAX_STRING_SIZE, "%s %s", help_text_common, gimp_drawable_get_name(vals->pres_layer_ID));
      gimp_help_set_help_data (pres_use_image,
                               help_text,
                               NULL);

      gtk_widget_set_sensitive (pres_use_image, TRUE);
    }
  else
    {
      gimp_help_set_help_data (pres_use_image,
                               _("No preservation mask is currently in use"),
                               NULL);

      gtk_widget_set_sensitive (pres_use_image, FALSE);
    }

  if (ui_vals->disc_status == TRUE)
    {
      g_snprintf(help_text_common, MAX_STRING_SIZE, _("Layer in use as discard mask: "));
      g_snprintf(help_text, MAX_STRING_SIZE, "%s %s", help_text_common, gimp_drawable_get_name(vals->disc_layer_ID));
      gimp_help_set_help_data (disc_use_image,
                               help_text,
                               NULL);

      gtk_widget_set_sensitive (disc_use_image, TRUE);
    }
  else
    {
      gtk_widget_set_sensitive (disc_use_image, FALSE);
      gimp_help_set_help_data (disc_use_image,
                               _("No discard mask is currently in use"),
                               NULL);
    }

  if (ui_vals->rigmask_status == TRUE)
    {
      g_snprintf(help_text_common, MAX_STRING_SIZE, _("Layer in use as rigidity mask: "));
      g_snprintf(help_text, MAX_STRING_SIZE, "%s %s", help_text_common, gimp_drawable_get_name(vals->rigmask_layer_ID));
      gimp_help_set_help_data (rigmask_use_image,
                               help_text,
                               NULL);

      gtk_widget_set_sensitive (rigmask_use_image, TRUE);
    }
  else
    {
      gtk_widget_set_sensitive (rigmask_use_image, FALSE);
      gimp_help_set_help_data (rigmask_use_image,
                               _("No rigidity mask is currently in use"),
                               NULL);
    }
}

