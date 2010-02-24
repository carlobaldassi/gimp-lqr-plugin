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


#ifndef __INTERFACE_H__
#define __INTERFACE_H__

/* Data structs for callbacks */

typedef struct
{
  gpointer ui_vals;
  gpointer button;
} PresDiscStatus;

#define PRESDISC_STATUS(data) ((PresDiscStatus*)data)

typedef struct
{
  GtkWidget *notebook;
  GtkWidget *features_page;
  GtkWidget *advanced_page;
  gint features_page_ID;
  gint advanced_page_ID;
  GtkWidget *label;
  gint32 image_ID;
  gint32 layer_ID;
} NotebookData;

#define NOTEBOOK_DATA(data) ((NotebookData*)data)


/*  Public functions  */

gint dialog (PlugInImageVals * image_vals,
             PlugInDrawableVals * drawable_vals,
             PlugInVals * vals,
             PlugInUIVals * ui_vals, PlugInColVals * col_vals,
	     PlugInDialogVals * dialog_vals);


#endif /* __INTERFACE_H__ */
