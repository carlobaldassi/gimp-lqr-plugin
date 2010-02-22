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


#ifndef __INTERFACE_AUX_H__
#define __INTERFACE_AUX_H__

typedef struct
{
	gint32 image_ID;
	gint32 layer_ID;
} InterfaceAuxData;

#define INTERFACE_AUX_DATA(data) ((InterfaceAuxData*) data)

/*  Public functions  */

gint
dialog_aux (gint32 image_ID, gint32 layer_ID,
	PlugInVals * vals,
	PlugInImageVals * image_vals,
	PlugInDrawableVals * drawable_vals, PlugInUIVals * ui_vals,
     	PlugInColVals * col_vals, PlugInDialogVals * dialog_vals);

GimpRGB * colour_from_type (gint32 image_ID, AuxLayerType layer_type);

#endif /* __INTERFACE_AUX_H__ */
