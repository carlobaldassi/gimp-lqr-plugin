/* GIMP LiquidRescale Plug-in
 * Copyright (C) 2007-2009 Carlo Baldassi (the "Author") <carlobaldassi@gmail.com>.
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

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>
#include <lqr.h>
#include "altsizeentry.h"

#include "plugin-intl.h"

#include "main.h"
#include "interface.h"
#include "render.h"
#include "interface_I.h"

/*  Local function prototypes  */

gint32 layer_from_name(gint32 image_ID, gchar * name);
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
  FALSE,                        /* output on a new layer */
  FALSE,                        /* output seams */
  LQR_GF_XABS,                  /* grad func */
  LQR_RES_ORDER_HOR,            /* resize order */
  GIMP_MASK_APPLY,              /* mask behavior */
  FALSE,                        /* scaleback */
  SCALEBACK_MODE_LQRBACK,       /* scaleback mode */
  TRUE,                         /* no disc upon enlarging */
  "",	                        /* pres_layer_name */
  "",                           /* disc_layer_name */
  ""                            /* rigmask_layer_name */
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
  FALSE,        /* chain active */
  FALSE,        /* pres status */
  FALSE,        /* disc status */
  FALSE,        /* rigmask status */
  -1,           /* last used width */
  -1,           /* last used height */
  0,            /* last layer */
  FALSE,	/* seams control expanded */
  FALSE,	/* operations expanded */
  FALSE,        /* dialog has position */
  0,            /* dialog root position x */
  0,            /* dialog root position y */
};

const PlugInDialogVals default_dialog_vals = {
  FALSE,        /* dialog has position */
  0,            /* dialog root position x */
  0,            /* dialog root position y */
};

static PlugInVals vals;
static PlugInImageVals image_vals;
static PlugInDrawableVals drawable_vals;
static PlugInUIVals ui_vals;
static PlugInColVals col_vals;
static PlugInDialogVals dialog_vals;
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
  gchar *help_path;
  gchar *help_uri;

  static GimpParamDef args[] = {
    {GIMP_PDB_INT32, "run_mode", "Interactive, non-interactive"},
    {GIMP_PDB_IMAGE, "image", "Input image"},
    {GIMP_PDB_DRAWABLE, "drawable", "Input drawable"},
    {GIMP_PDB_INT32, "width", "Final width"},
    {GIMP_PDB_INT32, "height", "Final height"},
    {GIMP_PDB_INT32, "pres_layer", "Layer that marks preserved areas (for interactive mode only)"},
    {GIMP_PDB_INT32, "pres_coeff", "Preservation coefficient (for interactive mode only)"},
    {GIMP_PDB_INT32, "disc_layer", "Layer that marks areas to discard (for interactive mode only)"},
    {GIMP_PDB_INT32, "disc_coeff", "Discard coefficient"},
    {GIMP_PDB_FLOAT, "rigidity", "Rigidity coefficient"},
    {GIMP_PDB_INT32, "rigidity_mask_layer", "Layer used as rigidity mask"},
    {GIMP_PDB_INT32, "delta_x", "max displacement of seams"},
    {GIMP_PDB_FLOAT, "enl_step", "enlargment step (ratio)"},
    {GIMP_PDB_INT32, "resize_aux_layers",
     "Whether to resize auxiliary layers"},
    {GIMP_PDB_INT32, "resize_canvas", "Whether to resize canvas"},
    {GIMP_PDB_INT32, "new_layer", "Whether to output on a new layer"},
    {GIMP_PDB_INT32, "seams", "Whether to output the seam map"},
    {GIMP_PDB_INT32, "grad_func", "Gradient function to use"},
    {GIMP_PDB_INT32, "res_order", "Resize order"},
    {GIMP_PDB_INT32, "mask_behavior", "What to do with masks"},
    {GIMP_PDB_INT32, "scaleback", "Whether to scale back when done"},
    {GIMP_PDB_INT32, "scaleback_mode", "Scale back mode"},
    {GIMP_PDB_INT32, "no_disc_on_enlarge", "Ignore discard layer upon enlargement"},
    {GIMP_PDB_STRING, "pres_layer_name", "Preservation layer name (for noninteractive mode only)"},
    {GIMP_PDB_STRING, "disc_layer_name", "Discard layer name (for noninteractive mode only)"},
    {GIMP_PDB_STRING, "rigmask_layer_name", "Rigidity mask layer name (for noninteractive mode only)"},
  };

  args_num = G_N_ELEMENTS (args);

  gimp_plugin_domain_register (GETTEXT_PACKAGE, LOCALEDIR);

  help_path = g_build_filename (DATADIR, "help", NULL);
  help_uri = g_filename_to_uri (help_path, NULL, NULL);
  g_free (help_path);

  gimp_plugin_help_register ("plug-in-lqr-help", help_uri);

  gimp_install_procedure (PLUG_IN_NAME,
                          N_("scaling which keeps layer features (or removes them)"),
                          "Resize a layer preserving (or removing) content",
                          "Carlo Baldassi <carlobaldassi@gmail.com>",
                          "Carlo Baldassi <carlobaldassi@gmail.com>", "2008",
                          N_("Li_quid rescale..."), "RGB*, GRAY*",
                          GIMP_PLUGIN, args_num, 0, args, NULL);

  gimp_plugin_menu_register (PLUG_IN_NAME, "<Image>/Layer/");
}

gint32
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

  *nreturn_vals = 1;
  *return_vals = values;
  gboolean run_dialog = TRUE;
  gboolean run_render = TRUE;
  gint dialog_resp;
  gint dialog_I_resp;
  gboolean render_success = FALSE;

  /*  Initialize i18n support  */
  bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
#ifdef HAVE_BIND_TEXTDOMAIN_CODESET
  bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
#endif
  textdomain (GETTEXT_PACKAGE);

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
              status = GIMP_PDB_CALLING_ERROR;
            }
          else
            {
              vals.new_width = param[3].data.d_int32;
              vals.new_height = param[4].data.d_int32;
              /* vals.pres_layer_ID = param[5].data.d_int32; */
              vals.pres_coeff = param[6].data.d_int32;
              /* vals.disc_layer_ID = param[7].data.d_int32; */
              vals.disc_coeff = param[8].data.d_int32;
              vals.rigidity = param[9].data.d_float;
              /* vals.rigmask_layer_ID = param[10].data.d_int32; */
              vals.delta_x = param[11].data.d_int32;
              vals.enl_step = param[12].data.d_float;
              vals.resize_aux_layers = param[13].data.d_int32;
              vals.resize_canvas = param[14].data.d_int32;
              vals.new_layer = param[15].data.d_int32;
              vals.output_seams = param[16].data.d_int32;
              vals.grad_func = param[17].data.d_int32;
              vals.res_order = param[18].data.d_int32;
              vals.mask_behavior = param[19].data.d_int32;
              vals.scaleback = param[20].data.d_int32;
              vals.scaleback_mode = param[21].data.d_int32;
              vals.no_disc_on_enlarge = param[22].data.d_int32;
              g_strlcpy(vals.pres_layer_name, param[23].data.d_string, VALS_MAX_NAME_LENGTH);
              g_strlcpy(vals.disc_layer_name, param[24].data.d_string, VALS_MAX_NAME_LENGTH);
              g_strlcpy(vals.rigmask_layer_name, param[25].data.d_string, VALS_MAX_NAME_LENGTH);
              vals.pres_layer_ID = layer_from_name(image_ID, vals.pres_layer_name);
              vals.disc_layer_ID = layer_from_name(image_ID, vals.disc_layer_name);
              vals.rigmask_layer_ID = layer_from_name(image_ID, vals.rigmask_layer_name);
            }
          break;

        case GIMP_RUN_INTERACTIVE:
          /*  Possibly retrieve data  */
          gimp_get_data (DATA_KEY_VALS, &vals);
          gimp_get_data (DATA_KEY_UI_VALS, &ui_vals);
          gimp_get_data (DATA_KEY_COL_VALS, &col_vals);

          /* Install a new signal needed by interface_I */
          g_signal_newv("coordinates-alarm", ALT_TYPE_SIZE_ENTRY, G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
              0, NULL, NULL, g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0, NULL);

          /* gimp_context_push(); */
          while (run_dialog == TRUE)
            {
              dialog_resp = dialog (image_ID, layer_ID,
                             &vals, &image_vals, &drawable_vals, &ui_vals,
                             &col_vals, &dialog_vals);
              switch (dialog_resp)
                {
                  case GTK_RESPONSE_OK:
                    //printf("OK\n"); fflush(stdout);
                    run_dialog = FALSE;
                    break;
                  case RESPONSE_RESET:
                    //printf("RESET\n"); fflush(stdout);
                    vals = default_vals;
                    image_vals = default_image_vals;
                    drawable_vals = default_drawable_vals;
                    ui_vals = default_ui_vals;
                    col_vals = default_col_vals;
                    image_vals.image_ID = image_ID;
                    drawable_vals.layer_ID = layer_ID;
                    break;
		  case RESPONSE_INTERACTIVE:
                    //printf("INTERACTIVE\n"); fflush(stdout);
		    dialog_I_resp = dialog_I (image_ID, drawable_vals.layer_ID,
                                &vals, &image_vals, &drawable_vals,
                                &ui_vals, &col_vals, &dialog_vals);
                    switch (dialog_I_resp)
                      {
                        case GTK_RESPONSE_OK:
                          run_dialog = FALSE;
                          run_render = FALSE;
                          break;
                        case RESPONSE_NONINTERACTIVE:
                          run_dialog = TRUE;
                          break;
                        default:
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
                    //printf("DEFAULT\n"); fflush(stdout);
                    run_dialog = FALSE;
                    status = GIMP_PDB_CANCEL;
                    break;
                }
            }
          /* gimp_context_pop(); */
          break;

        case GIMP_RUN_WITH_LAST_VALS:
          /*  Possibly retrieve data  */
          gimp_get_data (DATA_KEY_VALS, &vals);
          gimp_get_data (DATA_KEY_UI_VALS, &ui_vals);
          gimp_get_data (DATA_KEY_COL_VALS, &col_vals);

          vals.pres_layer_ID = layer_from_name(image_ID, vals.pres_layer_name);
          vals.disc_layer_ID = layer_from_name(image_ID, vals.disc_layer_name);
          vals.rigmask_layer_ID = layer_from_name(image_ID, vals.rigmask_layer_name);

          break;

        default:
          break;
        }
    }
  else
    {
      status = GIMP_PDB_CALLING_ERROR;
    }

  if (status == GIMP_PDB_SUCCESS)
    {
      if ((vals.pres_layer_ID == -1) || (ui_vals.pres_status == FALSE))
        {
          vals.pres_layer_ID = 0;
        }
      if ((vals.disc_layer_ID == -1) || (ui_vals.disc_status == FALSE))
        {
          vals.disc_layer_ID = 0;
        }
      if ((vals.rigmask_layer_ID == -1) || (ui_vals.rigmask_status == FALSE))
        {
          vals.rigmask_layer_ID = 0;
        }
      ui_vals.last_used_width = vals.new_width;
      ui_vals.last_used_height = vals.new_height;
      ui_vals.last_layer_ID = layer_ID;
      gimp_image_undo_group_start (image_ID);
      if (run_render)
        {
          CarverData * carver_data;
          carver_data = render_init_carver (image_ID, &vals, &drawable_vals, FALSE);
          if (carver_data)
            {
              render_success = render_noninteractive (image_ID, &vals, &drawable_vals,
                &col_vals, carver_data);
            }
          else
            {
              render_success = FALSE;
            }
        }
      else
        {
          render_success = TRUE;
        }

      if (run_mode != GIMP_RUN_NONINTERACTIVE)
        gimp_displays_flush ();

      if ((run_mode == GIMP_RUN_INTERACTIVE) && render_success)
        {
          if ((vals.pres_layer_ID == -1) || (ui_vals.pres_status == FALSE))
            {
              vals.pres_layer_name[0] = '\0';
            }
          else
            {
              g_strlcpy(vals.pres_layer_name, gimp_drawable_get_name(vals.pres_layer_ID), VALS_MAX_NAME_LENGTH);
            }
          if ((vals.disc_layer_ID == -1) || (ui_vals.disc_status == FALSE))
            {
              vals.disc_layer_name[0] = '\0';
            }
          else
            {
              g_strlcpy(vals.disc_layer_name, gimp_drawable_get_name(vals.disc_layer_ID), VALS_MAX_NAME_LENGTH);
            }
          if ((vals.rigmask_layer_ID == -1) || (ui_vals.rigmask_status == FALSE))
            {
              vals.rigmask_layer_name[0] = '\0';
            }
          else
            {
              g_strlcpy(vals.rigmask_layer_name, gimp_drawable_get_name(vals.rigmask_layer_ID), VALS_MAX_NAME_LENGTH);
            }
          gimp_set_data (DATA_KEY_VALS, &vals, sizeof (vals));
          gimp_set_data (DATA_KEY_UI_VALS, &ui_vals, sizeof (ui_vals));
          gimp_set_data (DATA_KEY_COL_VALS, &col_vals, sizeof (col_vals));
        }

      gimp_image_undo_group_end (image_ID);
    }

  values[0].type = GIMP_PDB_STATUS;
  values[0].data.d_status = status;

}
