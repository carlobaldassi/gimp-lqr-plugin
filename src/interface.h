/* GIMP LiquidRescaling Plug-in
 * Copyright (C) 2007 Carlo Baldassi (the "Author") <carlobaldassi@yahoo.it>.
 * All Rights Reserved.
 *
 * This plugin implements the algorithm described in the paper
 * "Seam Carving for Content-Aware Image Resizing"
 * by Shai Avidan and Ariel Shamir
 * which can be found at http://www.faculty.idc.ac.il/arik/imret.pdf
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
  GimpDrawable *drawable;
} NotebookData;

#define NOTEBOOK_DATA(data) ((NotebookData*)data)


/*  Public functions  */

gint dialog (gint32 image_ID,
             GimpDrawable * drawable,
             PlugInVals * vals,
             PlugInImageVals * image_vals,
             PlugInDrawableVals * drawable_vals,
             PlugInUIVals * ui_vals, PlugInColVals * col_vals);


#endif /* __INTERFACE_H__ */
