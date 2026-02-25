#ifndef __INTERFACE_I_H__
#define __INTERFACE_I_H__

/* Data structs for callbacks */

typedef struct {
    GtkWidget *coordinates;
    GtkWidget *info_label;
    GtkWidget *dump_button;
    PlugInColVals *col_vals;
    CarverData *carver_data;
    gint orig_width;
    gint orig_height;
    gint32 vmap_layer_ID;
} InterfaceIData;

#define INTERFACE_I_DATA(data) ((InterfaceIData*) data)

/*  Public functions  */

gint
dialog_I(
        GimpImage *image,
        GimpDrawable **drawables,
        PlugInImageVals *image_vals,
        PlugInDrawableVals *drawable_vals,
        PlugInVals *vals,
        PlugInUIVals *ui_vals,
        PlugInColVals *col_vals,
        PlugInDialogVals *dialog_vals);

#endif /* __INTERFACE_I_H__ */
