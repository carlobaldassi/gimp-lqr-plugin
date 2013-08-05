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

static gboolean preview_has_pres_buffer(PreviewData * p_data);
static gboolean preview_has_disc_buffer(PreviewData * p_data);
static gboolean preview_has_rigmask_buffer(PreviewData * p_data);
void preview_composite(PreviewData * p_data, GdkPixbuf * src_pixbuf, SizeInfo * size_info);

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
  p_data->base_pixbuf = NULL;
  p_data->pres_pixbuf = NULL;
  p_data->disc_pixbuf = NULL;
  p_data->rigmask_pixbuf = NULL;
  p_data->pixbuf = NULL;
}

void
preview_data_create(gint32 image_ID, gint32 layer_ID, PreviewData * p_data)
{
  g_assert(p_data != NULL);

  preview_init_mem (p_data);

  p_data->base_pixbuf = gimp_drawable_get_thumbnail(layer_ID, p_data->width, p_data->height, GIMP_PIXBUF_SMALL_CHECKS);
  p_data->width = gdk_pixbuf_get_width(p_data->base_pixbuf);
  p_data->height = gdk_pixbuf_get_height(p_data->base_pixbuf);
}

GtkWidget *
preview_area_create(PreviewData * p_data)
{
  p_data->area = gtk_drawing_area_new ();
  gtk_widget_set_size_request (p_data->area, PREVIEW_MAX_WIDTH,
                               PREVIEW_MAX_HEIGHT);
  return p_data->area;
}

void
size_info_scale(SizeInfo * size_info, gdouble factor)
{
  size_info->x_off = (guint) (size_info->x_off / factor);
  size_info->y_off = (guint) (size_info->y_off / factor);
  size_info->width = (guint) (size_info->width / factor);
  size_info->height = (guint) (size_info->height / factor);
}

void
preview_composite(PreviewData * p_data, GdkPixbuf * src_pixbuf, SizeInfo * size_info)
{
  gint x_off = size_info->x_off;
  gint y_off = size_info->y_off;
  gint dest_x = MAX(0, x_off);
  gint dest_y = MAX(0, y_off);
  gint dest_width = MIN(p_data->width, size_info->width + x_off) - dest_x;
  gint dest_height = MIN(p_data->height, size_info->height + y_off) - dest_y;
  gdk_pixbuf_composite(src_pixbuf, p_data->pixbuf, dest_x, dest_y, dest_width, dest_height, (gdouble) x_off, (gdouble) y_off, 1.0, 1.0, GDK_INTERP_BILINEAR, 127);
}

static gboolean
preview_has_pres_buffer(PreviewData * p_data)
{
  return ((p_data->pres_pixbuf) && (p_data->ui_vals->pres_status));
}

static gboolean
preview_has_disc_buffer(PreviewData * p_data)
{
  return ((p_data->disc_pixbuf) && (p_data->ui_vals->disc_status));
}

static gboolean
preview_has_rigmask_buffer(PreviewData * p_data)
{
  return ((p_data->rigmask_pixbuf) && (p_data->ui_vals->rigmask_status));
}

void
preview_build_pixbuf (PreviewData * p_data)
{
  if (p_data->pixbuf)
    {
      g_object_unref (G_OBJECT (p_data->pixbuf));
    }

  p_data->pixbuf = gdk_pixbuf_copy(p_data->base_pixbuf);

  if (preview_has_pres_buffer(p_data))
    {
      preview_composite(p_data, p_data->pres_pixbuf, &p_data->pres_size_info);
    }
  if (preview_has_disc_buffer(p_data))
    {
      preview_composite(p_data, p_data->disc_pixbuf, &p_data->disc_size_info);
    }
  if (preview_has_rigmask_buffer(p_data))
    {
      preview_composite(p_data, p_data->rigmask_pixbuf, &p_data->rigmask_size_info);
    }
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
