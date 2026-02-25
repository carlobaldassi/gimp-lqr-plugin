#ifndef __INTERFACE_AUX_H__
#define __INTERFACE_AUX_H__

typedef struct {
    gint32 image_ID;
    gint32 layer_ID;
} InterfaceAuxData;

#define INTERFACE_AUX_DATA(data) ((InterfaceAuxData*) data)

/*  Public functions  */

gint
dialog_aux(
        GimpImage *image,
        GimpDrawable **drawables,
        PlugInImageVals *image_vals,
        PlugInDrawableVals *drawable_vals,
        PlugInVals *vals,
        PlugInUIVals *ui_vals,
        PlugInColVals *col_vals,
        PlugInDialogVals *dialog_vals);

GeglColor *colour_from_type(
        gint32 image_ID,
        AuxLayerType layer_type
);

#endif /* __INTERFACE_AUX_H__ */
