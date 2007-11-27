/* GIMP LiquidRescaling Plug-in
 * Copyright (C) 2007 Carlo Baldassi (the "Author") <carlobaldassi@yahoo.it>.
 * (implementation based on the GIMP Plug-in Template by Michael Natterer)
 * All Rights Reserved.
 *
 * This plug-in implements the algorithm described in the paper
 * "Seam Carving for Content-Aware Image Resizing"
 * by Shai Avidan and Ariel Shamir
 * which can be found at http://www.faculty.idc.ac.il/arik/imret.pdf
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 dated June, 1991.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 */


#include "config.h"

#include <string.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "plugin-intl.h"

#include "lqr_base.h"
#include "lqr_gradient.h"

#include "main.h"
#include "interface.h"
#include "render.h"



/*  Constants  */

#define PLUG_IN_NAME   "plug-in-lqr"

#define DATA_KEY_VALS    "plug_in_lqr"
#define DATA_KEY_UI_VALS "plug_in_lqr_ui"
#define DATA_KEY_COL_VALS "plug_in_lqr_col"

#define PARASITE_KEY     "plug_in_lqr_options"


/*  Local function prototypes  */

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
  100,                          /* pres coeff */
  0,                            /* disc layer ID */
  100,                          /* disc coeff */
  0,                            /* rigidity coeff */
  1,                            /* delta x */
  TRUE,                         /* resize canvas */
  TRUE,                         /* resize aux layers */
  FALSE,                        /* output on a new layer */
  FALSE,                        /* output seams */
  LQR_GF_XABS,                  /* grad func */
  LQR_RES_ORDER_HOR,		/* resize order */
  GIMP_MASK_APPLY,              /* mask behavior */
  LQR_MODE_NORMAL		/* operational mode */
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
  0
};

const PlugInDrawableVals default_drawable_vals = {
  0,
};

const PlugInUIVals default_ui_vals = {
  FALSE,
  FALSE,
  FALSE,
  0,
};

static PlugInVals vals;
static PlugInImageVals image_vals;
static PlugInDrawableVals drawable_vals;
static PlugInUIVals ui_vals;
static PlugInColVals col_vals;


GimpPlugInInfo PLUG_IN_INFO = {
  NULL,                         /* init_proc  */
  NULL,                         /* quit_proc  */
  query,                        /* query_proc */
  run,                          /* run_proc   */
};

MAIN ()

static void
query (void)
{
  gchar *help_path;
  gchar *help_uri;

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
    {GIMP_PDB_INT32, "delta_x", "max displacement of seams"},
    {GIMP_PDB_INT32, "resize_aux_layers",
     "Whether to resize auxiliary layers"},
    {GIMP_PDB_INT32, "resize_canvas", "Whether to resize canvas"},
    {GIMP_PDB_INT32, "new_layer", "Whether to output on a new layer"},
    {GIMP_PDB_INT32, "seams", "Whether to output the seam map"},
    {GIMP_PDB_INT32, "mask_behavior", "What to do with masks"},
    {GIMP_PDB_INT32, "grad_func", "Gradient function to use"},
    {GIMP_PDB_INT32, "res_order", "Resize order"},
    {GIMP_PDB_INT32, "oper_mode", "Operational mode"},
  };

  gimp_plugin_domain_register (GETTEXT_PACKAGE, LOCALEDIR);

  help_path = g_build_filename (DATADIR, "help", NULL);
  help_uri = g_filename_to_uri (help_path, NULL, NULL);
  g_free (help_path);

  gimp_plugin_help_register ("plug-in-lqr-help", help_uri);

  gimp_install_procedure (PLUG_IN_NAME,
                          N_("scaling which keeps layer features (or removes them)"),
                          "Resize a layer preserving (or removing) content",
                          "Carlo Baldassi <carlobaldassi@yahoo.it>",
                          "Carlo Baldassi <carlobaldassi@yahoo.it>", "2007",
                          N_("Li_quid rescale..."), "RGB*, GRAY*",
                          GIMP_PLUGIN, G_N_ELEMENTS (args), 0, args, NULL);
  /* what about INDEXED* images ? */

  gimp_plugin_menu_register (PLUG_IN_NAME, "<Image>/Layer/");
}

static void
run (const gchar * name,
     gint n_params,
     const GimpParam * param, gint * nreturn_vals, GimpParam ** return_vals)
{
  static GimpParam values[1];
  GimpDrawable *drawable;
  gint32 active_channel_ID;

  gint32 image_ID;
  GimpRunMode run_mode;
  GimpPDBStatusType status = GIMP_PDB_SUCCESS;

  *nreturn_vals = 1;
  *return_vals = values;
  gboolean render_success = FALSE;

  /*  Initialize i18n support  */
  bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
#ifdef HAVE_BIND_TEXTDOMAIN_CODESET
  bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
#endif
  textdomain (GETTEXT_PACKAGE);

  run_mode = param[0].data.d_int32;
  image_ID = param[1].data.d_int32;
  drawable = gimp_drawable_get (param[2].data.d_drawable);
  if (gimp_drawable_is_channel (drawable->drawable_id))
    {
      active_channel_ID = gimp_image_get_active_channel (image_ID);
      gimp_image_unset_active_channel (image_ID);
    }
  drawable = gimp_drawable_get (gimp_image_get_active_layer (image_ID));

  /*  Initialize with default values  */
  vals = default_vals;
  image_vals = default_image_vals;
  drawable_vals = default_drawable_vals;
  ui_vals = default_ui_vals;
  col_vals = default_col_vals;

  if (strcmp (name, PLUG_IN_NAME) == 0)
    {
      switch (run_mode)
        {
        case GIMP_RUN_NONINTERACTIVE:
          if (n_params != 19)
            {
              status = GIMP_PDB_CALLING_ERROR;
            }
          else
            {
              vals.new_width = param[3].data.d_int32;
              vals.new_height = param[4].data.d_int32;
              vals.pres_layer_ID = param[5].data.d_int32;
              vals.pres_coeff = param[6].data.d_int32;
              vals.disc_layer_ID = param[7].data.d_int32;
              vals.disc_coeff = param[8].data.d_int32;
              vals.rigidity = param[9].data.d_int32;
              vals.delta_x = param[10].data.d_int32;
              vals.resize_aux_layers = param[11].data.d_int32;
              vals.resize_canvas = param[12].data.d_int32;
              vals.new_layer = param[13].data.d_int32;
              vals.output_seams = param[14].data.d_int32;
              vals.grad_func = param[15].data.d_int32;
              vals.res_order = param[16].data.d_int32;
              vals.mask_behavior = param[17].data.d_int32;
	      vals.oper_mode = param[18].data.d_int32;
            }
          break;

        case GIMP_RUN_INTERACTIVE:
          /*  Possibly retrieve data  */
          gimp_get_data (DATA_KEY_VALS, &vals);
          gimp_get_data (DATA_KEY_UI_VALS, &ui_vals);
          gimp_get_data (DATA_KEY_COL_VALS, &col_vals);

          /* gimp_context_push(); */ /* doesn't work! why? */
          if (!dialog (image_ID, drawable,
                       &vals, &image_vals, &drawable_vals, &ui_vals,
                       &col_vals))
            {
              status = GIMP_PDB_CANCEL;
            }
          /* gimp_context_pop(); */
          break;

        case GIMP_RUN_WITH_LAST_VALS:
          /*  Possibly retrieve data  */
          gimp_get_data (DATA_KEY_VALS, &vals);
          gimp_get_data (DATA_KEY_UI_VALS, &ui_vals);
          gimp_get_data (DATA_KEY_COL_VALS, &col_vals);
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
      gimp_image_undo_group_start (image_ID);
      render_success =
        render (image_ID, drawable, &vals, &image_vals, &drawable_vals,
                &col_vals);

      if (run_mode != GIMP_RUN_NONINTERACTIVE)
        gimp_displays_flush ();

      if (run_mode == GIMP_RUN_INTERACTIVE)
        {
          gimp_set_data (DATA_KEY_VALS, &vals, sizeof (vals));
          gimp_set_data (DATA_KEY_UI_VALS, &ui_vals, sizeof (ui_vals));
          gimp_set_data (DATA_KEY_COL_VALS, &col_vals, sizeof (col_vals));
        }

      drawable =
        gimp_drawable_get (gimp_image_get_active_drawable (image_ID));
      gimp_drawable_detach (drawable);

      gimp_image_undo_group_end (image_ID);
    }

  values[0].type = GIMP_PDB_STATUS;
  values[0].data.d_status = status;

}
