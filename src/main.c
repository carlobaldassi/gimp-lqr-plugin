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

#include <string.h>

#include <glib.h>
#include <glib/gi18n.h>
#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>
#include <lqr.h>
#include "altsizeentry.h"

#include "plugin-intl.h"

#include "main.h"
#include "interface.h"
#include "render.h"
#include "interface_I.h"
#include "interface_aux.h"

/*  Local function prototypes  */

static gint32 layer_from_name(gint32 image_ID, gchar * name);
static void set_aux_layer_name(gint layer_ID, gboolean status, gchar * name);
static void save_vals (void);
static void retrieve_vals (void);
static void retrieve_vals_use_aux_layers_names (gint32 image_ID);
static void noninteractive_read_vals (const GimpParam * param);
static void install_custom_signals();
static void cancel_work_on_aux_layer(void);
#if defined(G_OS_WIN32)
static gchar * get_gimp_share_directory_on_windows();
#endif

static void query (void);
static void run (const gchar * name,
                 gint nparams,
                 const GimpParam * param,
                 gint * nreturn_vals, GimpParam ** return_vals);


/*  Local variables  */

const PlugInVals default_vals = {
  100,                          /* new width */
  100,                          /* new height */
  0,                            /* pres layer ID */
  1000,                         /* pres coeff */
  0,                            /* disc layer ID */
  1000,                         /* disc coeff */
  0,                            /* rigidity coeff */
  0,				/* rigmask layer ID */
  1,                            /* delta x */
  150,				/* enl step */
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
  "",	                        /* pres_layer_name */
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

const PlugInImageVals default_image_vals = {
  0             /* image ID */
};

const PlugInDrawableVals default_drawable_vals = {
  0             /* layer ID */
};

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

const GimpRGB default_pres_col = {
  0.0,
  1.0,
  0.0,
  1.0
};

const GimpRGB default_disc_col = {
  1.0,
  0.0,
  0.0,
  1.0
};

const GimpRGB default_rigmask_col = {
  0.0,
  0.0,
  1.0,
  1.0
};

const GimpRGB default_gray_col = {
  0.333333,
  0.333333,
  0.333333,
  1.0
};


static PlugInVals vals;
static PlugInImageVals image_vals;
static PlugInDrawableVals drawable_vals;
static PlugInUIVals ui_vals;
static PlugInColVals col_vals;
static PlugInDialogVals dialog_vals;
static GimpParamDef args[] = {
  {GIMP_PDB_INT32, "run_mode", "Interactive, non-interactive"},
  {GIMP_PDB_IMAGE, "image", "Input image"},
  {GIMP_PDB_DRAWABLE, "drawable", "Input drawable"},
  {GIMP_PDB_INT32, "width", "Final width"},
  {GIMP_PDB_INT32, "height", "Final height"},
  {GIMP_PDB_INT32, "pres_layer", "Layer that marks preserved areas"},
  {GIMP_PDB_INT32, "pres_coeff", "Preservation coefficient"},
  {GIMP_PDB_INT32, "disc_layer", "Layer that marks areas to discard"},
  {GIMP_PDB_INT32, "disc_coeff", "Discard coefficient"},
  {GIMP_PDB_FLOAT, "rigidity", "Rigidity coefficient"},
  {GIMP_PDB_INT32, "rigidity_mask_layer", "Layer used as rigidity mask"},
  {GIMP_PDB_INT32, "delta_x", "max displacement of seams"},
  {GIMP_PDB_FLOAT, "enl_step", "enlargment step (ratio)"},
  {GIMP_PDB_INT32, "resize_aux_layers",
   "Whether to resize auxiliary layers"},
  {GIMP_PDB_INT32, "resize_canvas", "Whether to resize canvas"},
  {GIMP_PDB_INT32, "output target", "Output target (same layer, new layer, new image)"},
  {GIMP_PDB_INT32, "seams", "Whether to output the seam map"},
  {GIMP_PDB_INT32, "nrg_func", "Energy function to use"},
  {GIMP_PDB_INT32, "res_order", "Resize order"},
  {GIMP_PDB_INT32, "mask_behavior", "What to do with masks"},
  {GIMP_PDB_INT32, "scaleback", "Whether to scale back when done"},
  {GIMP_PDB_INT32, "scaleback_mode", "Scale back mode"},
  {GIMP_PDB_INT32, "no_disc_on_enlarge", "Ignore discard layer upon enlargement"},
  {GIMP_PDB_STRING, "pres_layer_name", "Preservation layer name (for noninteractive mode only)"},
  {GIMP_PDB_STRING, "disc_layer_name", "Discard layer name (for noninteractive mode only)"},
  {GIMP_PDB_STRING, "rigmask_layer_name", "Rigidity mask layer name (for noninteractive mode only)"},
  {GIMP_PDB_STRING, "selected_layer_name", "Selected layer name (for noninteractive mode only)"},
};

static int args_num;

GimpPlugInInfo PLUG_IN_INFO = {
  NULL,                         /* init_proc  */
  NULL,                         /* quit_proc  */
  query,                        /* query_proc */
  run,                          /* run_proc   */
};

MAIN ()

static void query (void)
{
#if defined(G_OS_WIN32)
  gchar *gimp_share_directory;
#endif
  gchar *help_path;
  gchar *help_uri;

  args_num = G_N_ELEMENTS (args);

#if defined(G_OS_WIN32)
  gimp_share_directory = get_gimp_share_directory_on_windows();
#endif

#if defined(G_OS_WIN32)
  gimp_plugin_domain_register (GETTEXT_PACKAGE, gimp_locale_directory());
  help_path = g_build_filename (gimp_share_directory, PACKAGE_NAME, "help", NULL); 
#else 
  gimp_plugin_domain_register (GETTEXT_PACKAGE, LOCALEDIR);
  help_path = g_build_filename (PLUGIN_DATADIR, "help", NULL);
#endif
  help_uri = g_filename_to_uri (help_path, NULL, NULL);

  gimp_plugin_help_register ("plug-in-lqr-help", help_uri); 
  g_free (help_uri); 

  gimp_install_procedure (PLUG_IN_NAME,
                          N_("scaling which keeps layer features (or removes them)"),
                          "Resize a layer preserving (or removing) content",
                          "Carlo Baldassi <carlobaldassi@gmail.com>",
                          "Carlo Baldassi <carlobaldassi@gmail.com>", "2010",
                          N_("Li_quid rescale..."), "RGB*, GRAY*",
                          GIMP_PLUGIN, args_num, 0, args, NULL);

  gimp_plugin_menu_register (PLUG_IN_NAME, "<Image>/Layer/");
}


static void
run (const gchar * name,
     gint n_params,
     const GimpParam * param, gint * nreturn_vals, GimpParam ** return_vals)
{
  static GimpParam values[1];
  gint32 layer_ID;
  //gint32 active_channel_ID;

  gint32 image_ID;
  GimpRunMode run_mode;
  GimpPDBStatusType status = GIMP_PDB_SUCCESS;

  gboolean run_dialog = TRUE;
  gboolean run_render = TRUE;
  gint dialog_resp;
  gint dialog_I_resp;
  gint dialog_aux_resp;
  gboolean render_success = FALSE;

  *nreturn_vals = 1;
  *return_vals = values;

  /*  Initialize i18n support  */
#if defined(G_OS_WIN32)
  bindtextdomain (GETTEXT_PACKAGE, gimp_locale_directory());
#else
  bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
#endif
#ifdef HAVE_BIND_TEXTDOMAIN_CODESET
  bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
#endif
  textdomain (GETTEXT_PACKAGE);

  args_num = G_N_ELEMENTS (args);

  run_mode = param[0].data.d_int32;
  image_ID = param[1].data.d_int32;
  layer_ID = param[2].data.d_drawable;
  if (gimp_drawable_is_channel (layer_ID))
    {
      gimp_image_unset_active_channel (image_ID);
    }
  if (!gimp_drawable_is_layer (layer_ID))
    {
      layer_ID = gimp_image_get_active_layer (image_ID);
    }


  /*  Initialize with default values  */
  vals = default_vals;
  image_vals = default_image_vals;
  drawable_vals = default_drawable_vals;
  ui_vals = default_ui_vals;
  col_vals = default_col_vals;
  dialog_vals = default_dialog_vals;

  image_vals.image_ID = image_ID;
  drawable_vals.layer_ID = layer_ID;

  if (strcmp (name, PLUG_IN_NAME) == 0)
    {
      switch (run_mode)
        {
        case GIMP_RUN_NONINTERACTIVE:
          if (n_params != args_num)
            {
              fprintf(stderr, "gimp-lqr-plugin: error: wrong number of arguments\n");
              fflush(stderr);
              status = GIMP_PDB_CALLING_ERROR;
            }
          else
            {
              noninteractive_read_vals (param);
              layer_ID = drawable_vals.layer_ID;
            }
          break;

        case GIMP_RUN_INTERACTIVE:
          retrieve_vals();

          install_custom_signals();

          while (run_dialog == TRUE)
            {
              dialog_resp = dialog (&image_vals, &drawable_vals,
                             &vals, &ui_vals, &col_vals, &dialog_vals);
              switch (dialog_resp)
                {
                  case GTK_RESPONSE_OK:
                    run_dialog = FALSE;
                    break;
                  case RESPONSE_RESET:
                    vals = default_vals;
                    ui_vals = default_ui_vals;
                    col_vals = default_col_vals;
                    break;
		  case RESPONSE_INTERACTIVE:
		    dialog_I_resp = dialog_I (&image_vals, &drawable_vals,
                                &vals, &ui_vals, &col_vals, &dialog_vals);
                    switch (dialog_I_resp)
                      {
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
                    dialog_aux_resp = dialog_aux (&image_vals, &drawable_vals,
                        &vals, &ui_vals, &col_vals, &dialog_vals);
                    switch(dialog_aux_resp)
                      {
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
    }
  else
    {
      status = GIMP_PDB_CALLING_ERROR;
    }

  image_ID = image_vals.image_ID;
  layer_ID = drawable_vals.layer_ID;

  if (status == GIMP_PDB_SUCCESS)
    {
      IMAGE_CHECK (image_ID, );
      AUX_LAYER_STATUS(vals.pres_layer_ID, ui_vals.pres_status);
      AUX_LAYER_STATUS(vals.disc_layer_ID, ui_vals.disc_status);
      AUX_LAYER_STATUS(vals.rigmask_layer_ID, ui_vals.rigmask_status);
      ui_vals.last_used_width = vals.new_width;
      ui_vals.last_used_height = vals.new_height;
      ui_vals.last_layer_ID = layer_ID;
      gimp_image_undo_group_start (image_ID);
      render_success = TRUE;
      if (run_render)
        {
          CarverData * carver_data;

          render_success = FALSE;
          carver_data = render_init_carver (&image_vals, &drawable_vals, &vals, FALSE);
          if (carver_data)
            {
              image_vals.image_ID = carver_data->image_ID;
              drawable_vals.layer_ID = carver_data->layer_ID;
              if (image_ID != image_vals.image_ID)
                {
                  gimp_image_undo_group_end (image_ID);
                  image_ID = image_vals.image_ID;
                  gimp_image_undo_group_start (image_ID);
                }
              render_success = render_noninteractive (&vals, &col_vals, carver_data);
            }
        }

      if (run_mode != GIMP_RUN_NONINTERACTIVE)
        gimp_displays_flush ();

      if ((run_mode == GIMP_RUN_INTERACTIVE) && render_success)
        {
          save_vals();
        }

      IMAGE_CHECK (image_ID, );
      gimp_image_undo_group_end (image_ID);
    }

  values[0].type = GIMP_PDB_STATUS;
  values[0].data.d_status = status;

}

static gint32
layer_from_name(gint32 image_ID, gchar * name)
{
  gint i;
  gint num_layers;
  gint * layer_list;

  if ((name == NULL) || (strncmp(name, "", VALS_MAX_NAME_LENGTH) == 0))
    {
      return 0;
    }

  layer_list = gimp_image_get_layers(image_ID, &num_layers);
  for (i = 0; i < num_layers; i++) {
    if (strncmp(name, gimp_drawable_get_name(layer_list[i]), VALS_MAX_NAME_LENGTH) == 0)
      {
        return layer_list[i];
      }
  }
  return 0;
}

static void
set_aux_layer_name(gint layer_ID, gboolean status, gchar * name)
{
  if ((layer_ID == -1) || (status == FALSE))
    {
      name[0] = '\0';
    }
  else
    {
      g_strlcpy(name, gimp_drawable_get_name(layer_ID), VALS_MAX_NAME_LENGTH);
    }
}

static void
save_vals (void)
{
  set_aux_layer_name (vals.pres_layer_ID, ui_vals.pres_status, vals.pres_layer_name);
  set_aux_layer_name (vals.disc_layer_ID, ui_vals.disc_status, vals.disc_layer_name);
  set_aux_layer_name (vals.rigmask_layer_ID, ui_vals.rigmask_status, vals.rigmask_layer_name);

  gimp_set_data (DATA_KEY_VALS, &vals, sizeof (vals));
  gimp_set_data (DATA_KEY_UI_VALS, &ui_vals, sizeof (ui_vals));
  gimp_set_data (DATA_KEY_COL_VALS, &col_vals, sizeof (col_vals));
}

static void
retrieve_vals (void)
{
  /*  Possibly retrieve data  */
  gimp_get_data (DATA_KEY_VALS, &vals);
  gimp_get_data (DATA_KEY_UI_VALS, &ui_vals);
  gimp_get_data (DATA_KEY_COL_VALS, &col_vals);
}

static void
retrieve_vals_use_aux_layers_names (gint32 image_ID)
{
  /*  Possibly retrieve data and set aux layers from names */
  retrieve_vals();

  vals.pres_layer_ID = layer_from_name(image_ID, vals.pres_layer_name);
  vals.disc_layer_ID = layer_from_name(image_ID, vals.disc_layer_name);
  vals.rigmask_layer_ID = layer_from_name(image_ID, vals.rigmask_layer_name);
}

static void
noninteractive_read_vals (const GimpParam * param)
{
  gint32 image_ID;
  gint32 aux_pres_layer_ID;
  gint32 aux_disc_layer_ID;
  gint32 aux_rigmask_layer_ID;
  gint32 aux_selected_layer_ID;
  int val_ind = 3;

  image_ID = image_vals.image_ID;

  vals.new_width = param[val_ind++].data.d_int32;
  vals.new_height = param[val_ind++].data.d_int32;
  vals.pres_layer_ID = param[val_ind++].data.d_int32;
  vals.pres_coeff = param[val_ind++].data.d_int32;
  vals.disc_layer_ID = param[val_ind++].data.d_int32;
  vals.disc_coeff = param[val_ind++].data.d_int32;
  vals.rigidity = param[val_ind++].data.d_float;
  vals.rigmask_layer_ID = param[val_ind++].data.d_int32;
  vals.delta_x = param[val_ind++].data.d_int32;
  vals.enl_step = param[val_ind++].data.d_float;
  vals.resize_aux_layers = param[val_ind++].data.d_int32;
  vals.resize_canvas = param[val_ind++].data.d_int32;
  vals.output_target = param[val_ind++].data.d_int32;
  vals.output_seams = param[val_ind++].data.d_int32;
  vals.nrg_func = param[val_ind++].data.d_int32;
  vals.res_order = param[val_ind++].data.d_int32;
  vals.mask_behavior = param[val_ind++].data.d_int32;
  vals.scaleback = param[val_ind++].data.d_int32;
  vals.scaleback_mode = param[val_ind++].data.d_int32;
  vals.no_disc_on_enlarge = param[val_ind++].data.d_int32;
  g_strlcpy(vals.pres_layer_name, param[val_ind++].data.d_string, VALS_MAX_NAME_LENGTH);
  g_strlcpy(vals.disc_layer_name, param[val_ind++].data.d_string, VALS_MAX_NAME_LENGTH);
  g_strlcpy(vals.rigmask_layer_name, param[val_ind++].data.d_string, VALS_MAX_NAME_LENGTH);
  g_strlcpy(vals.selected_layer_name, param[val_ind++].data.d_string, VALS_MAX_NAME_LENGTH);

  aux_pres_layer_ID = layer_from_name(image_ID, vals.pres_layer_name);
  aux_disc_layer_ID = layer_from_name(image_ID, vals.disc_layer_name);
  aux_rigmask_layer_ID = layer_from_name(image_ID, vals.rigmask_layer_name);
  aux_selected_layer_ID = layer_from_name(image_ID, vals.selected_layer_name);

  if (aux_pres_layer_ID)
    {
      vals.pres_layer_ID = aux_pres_layer_ID;
    }
  if (aux_disc_layer_ID)
    {
      vals.disc_layer_ID = aux_disc_layer_ID;
    }
  if (aux_rigmask_layer_ID)
    {
      vals.rigmask_layer_ID = aux_rigmask_layer_ID;
    }
  if (aux_selected_layer_ID)
    {
      drawable_vals.layer_ID = aux_selected_layer_ID;
    }

  if (vals.pres_layer_ID)
    {
      ui_vals.pres_status = TRUE;
    }
  if (vals.disc_layer_ID)
    {
      ui_vals.disc_status = TRUE;
    }
  if (vals.rigmask_layer_ID)
    {
      ui_vals.rigmask_status = TRUE;
    }
  //printf("RNIN: lid=%i pres=%i disc=%i rmask=%i\n", aux_selected_layer_ID, aux_pres_layer_ID, aux_disc_layer_ID, aux_rigmask_layer_ID); fflush(stdout);
  //printf("RNIN: lid=%i pres=%i disc=%i rmask=%i\n", drawable_vals.layer_ID, vals.pres_layer_ID, vals.disc_layer_ID, vals.rigmask_layer_ID); fflush(stdout);

}

static void
install_custom_signals()
{
  /* Install a new signal needed by interface_I */
  g_signal_newv("coordinates-alarm", ALT_TYPE_SIZE_ENTRY, G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
      0, NULL, NULL, g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0, NULL);
}

static void
cancel_work_on_aux_layer(void)
{
  if (!gimp_image_is_valid(image_vals.image_ID))
    {
      return;
    }
  gimp_image_set_active_layer(image_vals.image_ID, drawable_vals.layer_ID);
  if (ui_vals.layer_on_edit_is_new && gimp_drawable_is_valid (ui_vals.layer_on_edit_ID))
    {
      gimp_image_remove_layer(image_vals.image_ID, ui_vals.layer_on_edit_ID);
    }
  gimp_displays_flush ();
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

