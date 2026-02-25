//
// Created by tomhodder on 30/05/25.
//

#ifndef GIMP_LQR_PLUGIN_DEFAULTS_H
#define GIMP_LQR_PLUGIN_DEFAULTS_H

/**
 * Some state related to the LiquidRescale plugin.
 *
 * Detailed description.
 */
const PlugInVals default_vals = {
        100,                          /* new width */
        100,                          /* new height */
        0,                            /* pres layer ID */
        1000,                         /* pres coeff */
        0,                            /* disc layer ID */
        1000,                         /* disc coeff */
        0,                            /* rigidity coeff */
        0,                /* rigmask layer ID */
        1,                            /* delta x */
        150,                /* enl step */
        TRUE,                         /* resize aux layers */
        TRUE,                         /* resize canvas */
        OUTPUT_TARGET_SAME_LAYER,     /* output target (same layer, new layer, new image) */
        FALSE,                        /* output seams */
        LQR_EF_GRAD_XABS,             /* nrg func */
        LQR_RES_ORDER_HOR,            /* resize order */
        GIMP_MASK_APPLY,              /* mask behavior */
        FALSE,                        /* scaleback */
        SCALEBACK_MODE_LQRBACK,       /* scaleback mode */
        TRUE,                         /* no disc upon enlarging */
        "",                            /* pres_layer_name */
        "",                           /* disc_layer_name */
        "",                           /* rigmask_layer_name */
        "",                           /* selected layer name */
};

const PlugInColVals default_col_vals = {
        1,                            /* start colour */
        1,
        0,
        0.2,                          /* end colour */
        0,
        0
};

/**
 * Legacy thing needs to be removed in the future.
 *
 * Detailed description.
 */
const PlugInImageVals default_image_vals = {
        0             /* image ID */
};

/**
 * Legacy thing needs to be removed in the future.
 *
 * Detailed description.
 */
const PlugInDrawableVals default_drawable_vals = {
        0             /* layer ID */
};

/**
 * Holding the UI state of the LiquidRescale plugin.
 *
 * Detailed description.
 */
const PlugInUIVals default_ui_vals = {
        FALSE,                /* chain active */
        FALSE,                /* pres status */
        FALSE,                /* disc status */
        FALSE,                /* rigmask status */
        -1,                   /* last used width */
        -1,                   /* last used height */
        0,                    /* last layer */
        FALSE,                /* seams control expanded */
        FALSE,                /* operations expanded */
        FALSE,                /* dialog has position */
        0,                    /* dialog root position x */
        0,                    /* dialog root position y */
        0,                    /* layer on edit ID */
        AUX_LAYER_PRES,       /* layer on edit type */
        TRUE,                 /* layer on edit is new */
};

const PlugInDialogVals default_dialog_vals = {
        FALSE,        /* dialog has position */
        0,            /* dialog root position x */
        0,            /* dialog root position y */
};

#endif //GIMP_LQR_PLUGIN_DEFAULTS_H
