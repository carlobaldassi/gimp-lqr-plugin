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
#include <stdio.h>

#include <string.h>

#include <glib.h>
#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>
#include <glib-object.h>
#include <lqr.h>

#include "altsizeentry.h"
#include "plugin-intl.h"

#include "main.h"
#include "interface.h"
#include "render.h"
#include "interface_I.h"
#include "interface_aux.h"
#include "defaults.h"

/* Local function prototypes */
static gint32           layer_from_name                    (gint32 image_ID,
                                                            gchar *name);
static void             set_aux_layer_name                 (GimpLayer *layer,
                                                            gboolean status,
                                                            gchar *name);
static void             save_vals                          (void);
static void             retrieve_vals                      (void);
static void             retrieve_vals_use_aux_layers_names (gint32 image_ID);
static void             noninteractive_read_vals           (GimpProcedureConfig *config,
                                                            GimpImage *image);
static void             install_custom_signals             (void);
static void             cancel_work_on_aux_layer           (void);
static GList           *lqr_query_procedures               (GimpPlugIn *plug_in);
static GimpProcedure   *lqr_create_procedure               (GimpPlugIn *plug_in,
                                                            const gchar *name);
static GimpValueArray  *lqr_run                            (GimpProcedure *procedure,
                                                            GimpRunMode run_mode,
                                                            GimpImage *image,
                                                            GimpDrawable **drawables,
                                                            GimpProcedureConfig *config,
                                                            gpointer run_data);
#if defined(G_OS_WIN32)
static gchar           *get_gimp_share_directory_on_windows(void);
#endif

/*  Local variables  */


GeglColor *default_pres_col = NULL;
GeglColor *default_disc_col = NULL;
GeglColor *default_rigmask_col = NULL;
GeglColor *default_gray_col = NULL;

/* Initialize default colors */
static void
initialize_default_colors(void) {
    if (!default_pres_col) {
        default_pres_col = gegl_color_new("rgb(0.0, 1.0, 0.0)");
    }
    if (!default_disc_col) {
        default_disc_col = gegl_color_new("rgb(1.0, 0.0, 0.0)");
    }
    if (!default_rigmask_col) {
        default_rigmask_col = gegl_color_new("rgb(0.0, 0.0, 1.0)");
    }
    if (!default_gray_col) {
        default_gray_col = gegl_color_new("rgb(0.333333, 0.333333, 0.333333)");
    }
}

static PlugInVals vals;
static PlugInImageVals image_vals;
static PlugInDrawableVals drawable_vals;
static GimpDrawable *drawable = NULL;
static PlugInUIVals ui_vals;
static PlugInColVals col_vals;
static PlugInDialogVals dialog_vals;


/* Modern GIMP 3.0 plugin class */
typedef struct _LqrPlugin LqrPlugin;
typedef struct _LqrPluginClass LqrPluginClass;

struct _LqrPlugin {
    GimpPlugIn parent_instance;
};

struct _LqrPluginClass {
    GimpPlugInClass parent_class;
};

#define LQR_TYPE_PLUGIN  (lqr_plugin_get_type ())
#define LQR_PLUGIN(obj)  (G_TYPE_CHECK_INSTANCE_CAST ((obj), LQR_TYPE_PLUGIN, LqrPlugin))


G_DEFINE_TYPE (LqrPlugin, lqr_plugin, GIMP_TYPE_PLUG_IN)

GIMP_MAIN (LQR_TYPE_PLUGIN)

static void
lqr_plugin_class_init(LqrPluginClass *klass) {
    GimpPlugInClass *plug_in_class = GIMP_PLUG_IN_CLASS(klass);

    plug_in_class->query_procedures = lqr_query_procedures;
    plug_in_class->create_procedure = lqr_create_procedure;
//  plug_in_class->set_i18n          = STD_SET_I18N;
}

static void
lqr_plugin_init(LqrPlugin *lqr) {

}

static GList *
lqr_query_procedures(GimpPlugIn *plug_in) {
    return g_list_append(NULL, g_strdup (PLUG_IN_NAME));
}

static GimpProcedure *
lqr_create_procedure(GimpPlugIn *plug_in,
                     const gchar *name) {

    GimpProcedure *procedure = NULL;

    g_message ("calling create_proceduresure for %s", name);

    if (g_strcmp0(name, PLUG_IN_NAME) == 0) {
        procedure = gimp_image_procedure_new(plug_in, name,
                                             GIMP_PDB_PROC_TYPE_PLUGIN,
                                             lqr_run, NULL, NULL);

        gimp_procedure_set_image_types(procedure, "RGB*, GRAY*");
        gimp_procedure_set_sensitivity_mask(procedure,
                                            GIMP_PROCEDURE_SENSITIVE_DRAWABLE);

        gimp_procedure_set_menu_label(procedure, N_("Li_quid rescale..."));
        gimp_procedure_add_menu_path(procedure, "<Image>/Layer/");

        gimp_procedure_set_documentation(procedure,
                                         N_("scaling which keeps layer features (or removes them)"),
                                         "Resize a layer preserving (or removing) content",
                                         name);
        gimp_procedure_set_attribution(procedure,
                                       "Carlo Baldassi <carlobaldassi@gmail.com>",
                                       "Carlo Baldassi <carlobaldassi@gmail.com>",
                                       "2010");

        /* Add arguments */
        gimp_procedure_add_int_argument(procedure, "width",
                                        "Final width",
                                        "Final width",
                                        1, GIMP_MAX_IMAGE_SIZE, 100,
                                        G_PARAM_READWRITE);

        gimp_procedure_add_int_argument(procedure, "height",
                                        "Final height",
                                        "Final height",
                                        1, GIMP_MAX_IMAGE_SIZE, 100,
                                        G_PARAM_READWRITE);

        gimp_procedure_add_layer_argument(procedure, "pres_layer",
                                          "Preservation layer",
                                          "Layer that marks preserved areas",
                                          TRUE,
                                          G_PARAM_READWRITE);

        gimp_procedure_add_int_argument(procedure, "pres_coeff",
                                        "Preservation coefficient",
                                        "Preservation coefficient",
                                        0, 10000, 1000,
                                        G_PARAM_READWRITE);

        gimp_procedure_add_layer_argument(procedure, "disc_layer",
                                          "Discard layer",
                                          "Layer that marks areas to discard",
                                          TRUE,
                                          G_PARAM_READWRITE);

        gimp_procedure_add_int_argument(procedure, "disc_coeff",
                                        "Discard coefficient",
                                        "Discard coefficient",
                                        0, 10000, 1000,
                                        G_PARAM_READWRITE);

        gimp_procedure_add_double_argument(procedure, "rigidity",
                                           "Rigidity coefficient",
                                           "Rigidity coefficient",
                                           0.0, 100.0, 0.0,
                                           G_PARAM_READWRITE);

        gimp_procedure_add_layer_argument(procedure, "rigidity_mask_layer",
                                          "Rigidity mask layer",
                                          "Layer used as rigidity mask",
                                          TRUE,
                                          G_PARAM_READWRITE);

        gimp_procedure_add_int_argument(procedure, "delta_x",
                                        "Max displacement",
                                        "max displacement of seams",
                                        0, 100, 1,
                                        G_PARAM_READWRITE);

        gimp_procedure_add_double_argument(procedure, "enl_step",
                                           "Enlargement step",
                                           "enlargment step (ratio)",
                                           1.0, 500.0, 150.0,
                                           G_PARAM_READWRITE);

        gimp_procedure_add_boolean_argument(procedure, "resize_aux_layers",
                                            "Resize auxiliary layers",
                                            "Whether to resize auxiliary layers",
                                            TRUE,
                                            G_PARAM_READWRITE);

        gimp_procedure_add_boolean_argument(procedure, "resize_canvas",
                                            "Resize canvas",
                                            "Whether to resize canvas",
                                            TRUE,
                                            G_PARAM_READWRITE);

        gimp_procedure_add_int_argument(procedure, "output_target",
                                        "Output target",
                                        "Output target (same layer, new layer, new image)",
                                        0, 2, 0,
                                        G_PARAM_READWRITE);

        gimp_procedure_add_boolean_argument(procedure, "seams",
                                            "Output seams",
                                            "Whether to output the seam map",
                                            FALSE,
                                            G_PARAM_READWRITE);

        gimp_procedure_add_int_argument(procedure, "nrg_func",
                                        "Energy function",
                                        "Energy function to use",
                                        0, 10, 0,
                                        G_PARAM_READWRITE);

        gimp_procedure_add_int_argument(procedure, "res_order",
                                        "Resize order",
                                        "Resize order",
                                        0, 1, 0,
                                        G_PARAM_READWRITE);

        gimp_procedure_add_int_argument(procedure, "mask_behavior",
                                        "Mask behavior",
                                        "What to do with masks",
                                        0, 2, 0,
                                        G_PARAM_READWRITE);

        gimp_procedure_add_boolean_argument(procedure, "scaleback",
                                            "Scale back",
                                            "Whether to scale back when done",
                                            FALSE,
                                            G_PARAM_READWRITE);

        gimp_procedure_add_int_argument(procedure, "scaleback_mode",
                                        "Scale back mode",
                                        "Scale back mode",
                                        0, 1, 0,
                                        G_PARAM_READWRITE);

        gimp_procedure_add_boolean_argument(procedure, "no_disc_on_enlarge",
                                            "No discard on enlarge",
                                            "Ignore discard layer upon enlargement",
                                            TRUE,
                                            G_PARAM_READWRITE);

        gimp_procedure_add_string_argument(procedure, "pres_layer_name",
                                           "Preservation layer name",
                                           "Preservation layer name (for noninteractive mode only)",
                                           "",
                                           G_PARAM_READWRITE);

        gimp_procedure_add_string_argument(procedure, "disc_layer_name",
                                           "Discard layer name",
                                           "Discard layer name (for noninteractive mode only)",
                                           "",
                                           G_PARAM_READWRITE);

        gimp_procedure_add_string_argument(procedure, "rigmask_layer_name",
                                           "Rigidity mask layer name",
                                           "Rigidity mask layer name (for noninteractive mode only)",
                                           "",
                                           G_PARAM_READWRITE);

        gimp_procedure_add_string_argument(procedure, "selected_layer_name",
                                           "Selected layer name",
                                           "Selected layer name (for noninteractive mode only)",
                                           "",
                                           G_PARAM_READWRITE);
    }

    return procedure;
}


static GimpValueArray *
lqr_run(
        GimpProcedure *procedure,
        GimpRunMode run_mode,
        GimpImage *image,
        GimpDrawable **drawables,
        GimpProcedureConfig *config,
        gpointer run_data
) {
    GimpDrawable *drawable;

    gegl_init (NULL, NULL);

    GimpPDBStatusType status = GIMP_PDB_SUCCESS;
    gint32 layer_ID;
    gint32 image_ID;

    gboolean run_dialog = TRUE;
    gboolean run_render = TRUE;
    gint dialog_resp;
    gint dialog_I_resp;
    gint dialog_aux_resp;
    gboolean render_success = FALSE;

    g_message ("got here1");

    /*  Initialize i18n support  */
#if defined(G_OS_WIN32)
    bindtextdomain (GETTEXT_PACKAGE, gimp_locale_directory());
#else
    bindtextdomain(GETTEXT_PACKAGE, LOCALEDIR);
#endif
#ifdef HAVE_BIND_TEXTDOMAIN_CODESET
    bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
#endif
    textdomain(GETTEXT_PACKAGE);

    /* Initialize default colors */
    initialize_default_colors();


    g_message ("got here1 initialize_default_colors");


    /*  Initialize with default values  */
    vals = default_vals;
    image_vals = default_image_vals;
    drawable_vals = default_drawable_vals;
//    image = gimp_image_get_by_id(image_ID);
//    drawable = gimp_drawable_get_by_id(layer_ID);

    ui_vals = default_ui_vals;
    col_vals = default_col_vals;
    dialog_vals = default_dialog_vals;


    /* Get the first drawable (layer) */
    drawable = drawables[0];
    layer_ID = gimp_item_get_id(GIMP_ITEM(drawable));


    if (!layer_ID) {
        g_message("Warning: layer_ID is not populated");
    }
    image_ID = gimp_image_get_id(image);
    if (!image_ID) {
        g_message("Warning: image_ID is not populated");
    }

    if (gimp_item_is_channel(GIMP_ITEM(drawable))) {
        gimp_image_unset_active_channel(image);
    }
    if (!gimp_item_is_layer(GIMP_ITEM(drawable))) {
        GimpLayer **selected_layers;
        selected_layers = gimp_image_get_selected_layers(image);
        if (selected_layers && selected_layers[0] != NULL)
            layer_ID = gimp_item_get_id(GIMP_ITEM(selected_layers[0]));
        g_free(selected_layers);
    }

    g_message ("got here");

    image_vals.image_ID = image_ID;
    drawable_vals.layer_ID = layer_ID;

    switch (run_mode) {
        case GIMP_RUN_NONINTERACTIVE:
            noninteractive_read_vals(config, image);
            break;

        case GIMP_RUN_INTERACTIVE:
            retrieve_vals();

            install_custom_signals();

            while (run_dialog == TRUE) {
                dialog_resp = dialog(image,
                                     drawables,
                                     &image_vals,
                                     &drawable_vals,
                                     &vals,
                                     &ui_vals,
                                     &col_vals,
                                     &dialog_vals
                );
                switch (dialog_resp) {

                    case GTK_RESPONSE_OK:
                        run_dialog = FALSE;
                        break;

                    case RESPONSE_RESET:
                        vals = default_vals;
                        ui_vals = default_ui_vals;
                        col_vals = default_col_vals;
                        break;

                    case RESPONSE_INTERACTIVE:
                        dialog_I_resp = dialog_I(
                                image,
                                drawables,
                                &image_vals,
                                &drawable_vals,
                                &vals,
                                &ui_vals,
                                &col_vals,
                                &dialog_vals
                        );
                        switch (dialog_I_resp) {
                            case GTK_RESPONSE_OK:
                                run_dialog = FALSE;
                                run_render = FALSE;
                                break;
                            case RESPONSE_NONINTERACTIVE:
                                save_vals();
                                run_dialog = TRUE;
                                break;
                            default:
                                run_dialog = FALSE;
                                run_render = FALSE;
                                status = GIMP_PDB_CANCEL;
                                break;
                        }
                        break;
                    case RESPONSE_WORK_ON_AUX_LAYER:
                        dialog_aux_resp = dialog_aux(
                                image,
                                drawables,
                                &image_vals,
                                &drawable_vals,
                                &vals,
                                &ui_vals,
                                &col_vals,
                                &dialog_vals);
                        switch (dialog_aux_resp) {
                            case GTK_RESPONSE_OK:
                                break;
                            default:
                                cancel_work_on_aux_layer();
                                run_dialog = FALSE;
                                run_render = FALSE;
                                status = GIMP_PDB_CANCEL;
                                break;
                        }
                        break;
                    case RESPONSE_FATAL:
                        run_dialog = FALSE;
                        status = GIMP_PDB_CALLING_ERROR;
                        break;
                    default:
                        run_dialog = FALSE;
                        status = GIMP_PDB_CANCEL;
                        break;
                }
            }
            break;

        case GIMP_RUN_WITH_LAST_VALS:
            retrieve_vals_use_aux_layers_names(image_ID);
            break;

        default:
            break;
    }

    if (status == GIMP_PDB_SUCCESS) {
        // @TODO find migration path for image_ID
        IMAGE_CHECK (image_ID, NULL);
        AUX_LAYER_STATUS(vals.pres_layer_ID, ui_vals.pres_status);
        AUX_LAYER_STATUS(vals.disc_layer_ID, ui_vals.disc_status);
        AUX_LAYER_STATUS(vals.rigmask_layer_ID, ui_vals.rigmask_status);
        ui_vals.last_used_width = vals.new_width;
        ui_vals.last_used_height = vals.new_height;
        ui_vals.last_layer_ID = layer_ID;
        gimp_image_undo_group_start(image);
        render_success = TRUE;
        if (run_render) {
            CarverData *carver_data;

            render_success = FALSE;
            carver_data = render_init_carver(
                    &image_vals,
                    &drawable_vals,
                    &vals,
                    FALSE);
            if (carver_data) {
                image = gimp_image_get_by_id(carver_data->image_ID);
                drawable = gimp_drawable_get_by_id(carver_data->layer_ID);
                if (image_ID != gimp_image_get_id(image)) {
                    gimp_image_undo_group_end(image);
                    image_ID = gimp_image_get_id(image);
                    gimp_image_undo_group_start(image);
                }
                render_success = render_noninteractive(&vals, &col_vals, carver_data);
            }
        }

        if (run_mode != GIMP_RUN_NONINTERACTIVE)
            gimp_displays_flush();

        if ((run_mode == GIMP_RUN_INTERACTIVE) && render_success) {
            save_vals();
        }

        // IMAGE_CHECK (image_ID, return gimp_procedure_new_return_values (procedure, GIMP_PDB_EXECUTION_ERROR, NULL));
        gimp_image_undo_group_end(image);
    }

    return gimp_procedure_new_return_values(procedure, status, NULL);
}

static gint32
layer_from_name(gint32 image_ID, gchar *name) {
    gint i;
    GimpLayer **layers;

    if ((name == NULL) || (strncmp(name, "", VALS_MAX_NAME_LENGTH) == 0)) {
        return 0;
    }

    GimpImage *image = gimp_image_get_by_id(image_ID);
    if (!image)
        return 0;

    layers = gimp_image_get_layers(image);
    if (!layers)
        return 0;

    for (i = 0; layers[i] != NULL; i++) {
        if (strncmp(name, gimp_item_get_name(GIMP_ITEM(layers[i])), VALS_MAX_NAME_LENGTH) == 0) {
            gint32 layer_id = gimp_item_get_id(GIMP_ITEM(layers[i]));
            g_free(layers);
            return layer_id;
        }
    }
    g_free(layers);
    return 0;
}

static void
set_aux_layer_name(GimpLayer *layer, gboolean status, gchar *name) {
    if ((layer == NULL) || (status == FALSE)) {
        name[0] = '\0';
    } else {
        g_strlcpy(name, gimp_item_get_name(GIMP_ITEM(layer)), VALS_MAX_NAME_LENGTH);
    }
}

static void
save_vals(void) {
    GimpLayer *pres_layer = gimp_layer_get_by_id(vals.pres_layer_ID);
    GimpLayer *disc_layer = gimp_layer_get_by_id(vals.disc_layer_ID);
    GimpLayer *rigmask_layer = gimp_layer_get_by_id(vals.rigmask_layer_ID);

    set_aux_layer_name(pres_layer, ui_vals.pres_status, vals.pres_layer_name);
    set_aux_layer_name(disc_layer, ui_vals.disc_status, vals.disc_layer_name);
    set_aux_layer_name(rigmask_layer, ui_vals.rigmask_status, vals.rigmask_layer_name);

    // TODO: Implement proper data storage in GIMP 3.0
    // For now, just store in memory
    static PlugInVals saved_vals;
    static PlugInUIVals saved_ui_vals;
    static PlugInColVals saved_col_vals;

    memcpy(&saved_vals, &vals, sizeof(vals));
    memcpy(&saved_ui_vals, &ui_vals, sizeof(ui_vals));
    memcpy(&saved_col_vals, &col_vals, sizeof(col_vals));
}

static void
retrieve_vals(void) {
    /* Possibly retrieve data  */
    // TODO: Implement proper data retrieval in GIMP 3.0
    // For now, just use defaults
    vals = default_vals;
    ui_vals = default_ui_vals;
    col_vals = default_col_vals;
}

static void
retrieve_vals_use_aux_layers_names(gint32 image_ID) {
    /* Possibly retrieve data and set aux layers from names */
    retrieve_vals();

    vals.pres_layer_ID = layer_from_name(image_ID, vals.pres_layer_name);
    vals.disc_layer_ID = layer_from_name(image_ID, vals.disc_layer_name);
    vals.rigmask_layer_ID = layer_from_name(image_ID, vals.rigmask_layer_name);
}

static void
noninteractive_read_vals(GimpProcedureConfig *config, GimpImage *image) {
    gint32 image_ID;
    gint32 aux_pres_layer_ID;
    gint32 aux_disc_layer_ID;
    gint32 aux_rigmask_layer_ID;
    gint32 aux_selected_layer_ID;

    image_ID = gimp_image_get_id(image);

    /* Read parameters using GimpProcedureConfig */
    g_object_get(config,
                 "width", &vals.new_width,
                 "height", &vals.new_height,
                 "pres_coeff", &vals.pres_coeff,
                 "disc_coeff", &vals.disc_coeff,
                 "rigidity", &vals.rigidity,
                 "delta_x", &vals.delta_x,
                 "enl_step", &vals.enl_step,
                 "resize_aux_layers", &vals.resize_aux_layers,
                 "resize_canvas", &vals.resize_canvas,
                 "output_target", &vals.output_target,
                 "seams", &vals.output_seams,
                 "nrg_func", &vals.nrg_func,
                 "res_order", &vals.res_order,
                 "mask_behavior", &vals.mask_behavior,
                 "scaleback", &vals.scaleback,
                 "scaleback_mode", &vals.scaleback_mode,
                 "no_disc_on_enlarge", &vals.no_disc_on_enlarge,
                 NULL);

    /* Get string parameters */
    g_object_get(config,
                 "pres_layer_name", &vals.pres_layer_name,
                 "disc_layer_name", &vals.disc_layer_name,
                 "rigmask_layer_name", &vals.rigmask_layer_name,
                 "selected_layer_name", &vals.selected_layer_name,
                 NULL);

    /* Get layer parameters */
    GimpLayer *pres_layer = NULL;
    GimpLayer *disc_layer = NULL;
    GimpLayer *rigmask_layer = NULL;

    g_object_get(config,
                 "pres_layer", &pres_layer,
                 "disc_layer", &disc_layer,
                 "rigidity_mask_layer", &rigmask_layer,
                 NULL);

    /* Convert GimpLayer objects to IDs if available */
    if (pres_layer)
        vals.pres_layer_ID = gimp_item_get_id(GIMP_ITEM(pres_layer));
    else
        vals.pres_layer_ID = layer_from_name(image_ID, vals.pres_layer_name);

    if (disc_layer)
        vals.disc_layer_ID = gimp_item_get_id(GIMP_ITEM(disc_layer));
    else
        vals.disc_layer_ID = layer_from_name(image_ID, vals.disc_layer_name);

    if (rigmask_layer)
        vals.rigmask_layer_ID = gimp_item_get_id(GIMP_ITEM(rigmask_layer));
    else
        vals.rigmask_layer_ID = layer_from_name(image_ID, vals.rigmask_layer_name);

    /* Find auxiliary layers by name if needed */
    aux_selected_layer_ID = layer_from_name(image_ID, vals.selected_layer_name);
    if (aux_selected_layer_ID) {
        drawable = gimp_drawable_get_by_id(aux_selected_layer_ID);
    }

    /* Update status flags */
    if (vals.pres_layer_ID) {
        ui_vals.pres_status = TRUE;
    }
    if (vals.disc_layer_ID) {
        ui_vals.disc_status = TRUE;
    }
    if (vals.rigmask_layer_ID) {
        ui_vals.rigmask_status = TRUE;
    }
}

static void
install_custom_signals() {
    /* Install a new signal needed by interface_I */
    g_signal_newv("coordinates-alarm", ALT_TYPE_SIZE_ENTRY, G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
                  0, NULL, NULL, g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0, NULL);
}

static void
cancel_work_on_aux_layer(void) {
    if (!gimp_image_is_valid_id(image_vals.image_ID)) {
        return;
    }
    gimp_image_set_active_layer_id(image_vals.image_ID, drawable_vals.layer_ID);
    if (ui_vals.layer_on_edit_is_new && gimp_drawable_is_valid_id(ui_vals.layer_on_edit_ID)) {
        gimp_image_remove_layer_id(image_vals.image_ID, ui_vals.layer_on_edit_ID);
    }
    gimp_displays_flush();
}

#if defined(G_OS_WIN32)
static gchar *
get_gimp_share_directory_on_windows()
{
  gchar ** tokens;
  gchar ** tokens2;
  gchar * str;
  gchar * ret;
  gint ind = 0;
  gint ind2;
  gboolean found = FALSE;

  tokens = g_strsplit(gimp_data_directory(), "\\", 1000);

  for (ind = 0; ind < 999; ++ind)
    {
      if (tokens[ind] == NULL)
        {
          break;
        }
      str = g_ascii_strdown(tokens[ind], -1);

      if (g_strcmp0(str, "share") == 0)
        {
          found = TRUE;
        }
      g_free(str);
      if (found)
        {
          break;
        }
    }

  if (!found)
    {
      g_message("GIMP share directory not found, resorting to default\n"); 
      ret = g_strdup_printf("C:\\Program Files\\GIMP-2.0\\share");
      return ret;
    }

  tokens2 = g_new(gchar*, ind + 2);
  for (ind2 = 0; ind2 <= ind; ++ind2)
    {
      tokens2[ind2] = g_strdup(tokens[ind2]);
    }
  tokens2[ind + 1] = NULL;
  g_strfreev(tokens);

  ret = g_strjoinv("\\", tokens2);

  g_strfreev(tokens2);
  if (!g_file_test(ret, G_FILE_TEST_IS_DIR))
    {
      g_message("GIMP share directory found but test for it failed, resorting to default\n"); 
      g_free(ret);
      ret = g_strdup_printf("C:\\Program Files\\GIMP-2.0\\share");
    }

  return ret;
}
#endif
