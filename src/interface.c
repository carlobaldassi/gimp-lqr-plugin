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

#include <gtk/gtk.h>
#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>
#include "compat.h"
#include "altsizeentry.h"
#include "altcoordinates.h"

#include "callbacks.h"

#include <lqr.h>

#include <math.h>
#include <string.h>

#include "plugin-intl.h"
#include "main.h"
#include "interface.h"
#include "preview.h"
#include "layers_combo.h"


/***  Constants  ***/

#define SCALE_WIDTH         (80)
#define SPIN_BUTTON_WIDTH   (75)
#define BOX_INDENT          (22)
#define MAX_COEFF          (3000)
#define MAX_RIGIDITY      (1000)
#define MAX_DELTA_X         (10)
#define MAX_STRING_SIZE   (2048)

/***  Local variables  ***/

gint dialog_response = GTK_RESPONSE_CANCEL;

PlugInUIVals *ui_state;
PlugInVals *state;
PlugInDialogVals *dialog_state;

NotebookData *notebook_data;
gboolean features_are_sensitive;

PreviewData preview_data;
PresDiscStatus presdisc_status;

ToggleData pres_toggle_data;
ToggleData disc_toggle_data;
ToggleData rigmask_toggle_data;

GtkWidget *nrg_func_combo_box;
GtkWidget *res_order_combo_box;

GtkWidget *dlg;


/***  Public functions  ***/

gint
dialog(
        GimpImage *image,
        GimpDrawable **drawables,
        PlugInImageVals *image_vals,
        PlugInDrawableVals *drawable_vals,
        PlugInVals *vals,
        PlugInUIVals *ui_vals,
        PlugInColVals *col_vals,
        PlugInDialogVals *dialog_vals) {
    gint32 image_ID;
    gint32 layer_ID;
    gint num_extra_layers;
    gint orig_width, orig_height;
    GtkWidget *main_hbox;
    GtkWidget *vbox;
    GtkWidget *vbox2;
    GtkWidget *vbox3;
    GtkWidget *hbox;
    GtkWidget *frame;
    GtkWidget *notebook;
    gfloat wfactor, hfactor;
    GtkWidget *preview_area;
    GtkWidget *filler;
    GtkWidget *pres_use_image;
    GtkWidget *disc_use_image;
    GtkWidget *rigmask_use_image;
    GtkWidget *coordinates;
    GtkWidget *resetvalues_event_box;
    GtkWidget *resetvalues_button;
    GtkWidget *resetvalues_icon;
    GtkWidget *lastvalues_event_box;
    GtkWidget *lastvalues_button;
    GtkWidget *lastvalues_icon;
    GtkWidget *interactive_event_box;
    GtkWidget *interactive_button;
    GtkWidget *interactive_hbox;
    GtkWidget *interactive_icon;
    GtkWidget *interactive_label;
    GtkWidget *scaleback_mode_alignment;
    GtkWidget *scaleback_mode_event_box;
    GtkWidget *scaleback_mode_label;
    GtkWidget *scaleback_mode_hbox;
    GtkWidget *scaleback_mode_combo_box;
    GtkWidget *features_page;
    GtkWidget *advanced_page;
    GtkWidget *thispage;
    GtkWidget *label;
    GtkWidget *output_target_event_box;
    GtkWidget *output_target_label;
    GtkWidget *output_target_hbox;
    GtkWidget *output_target_combo_box;
    GtkWidget *resize_canvas_button;
    GtkWidget *resize_aux_layers_button;
    GtkWidget *out_seams_hbox;
    GtkWidget *out_seams_button;
    GeglColor *colour;
    GtkWidget *out_seams_col_button1;
    GtkWidget *out_seams_col_button2;
    GtkWidget *scaleback_button;
    GtkWidget *mask_behavior_combo_box = NULL;
    gboolean has_mask = FALSE;
    GimpUnit *unit;
    gdouble xres, yres;

    image_ID = gimp_image_get_id(image);
    layer_ID = drawable_vals->layer_ID;

    IMAGE_CHECK (image_ID, FALSE);

    gimp_ui_init(PLUGIN_NAME);

    dialog_state = dialog_vals;

    state = g_new (PlugInVals, 1);
    memcpy(state, vals, sizeof(PlugInVals));

    ui_state = g_new (PlugInUIVals, 1);
    memcpy(ui_state, ui_vals, sizeof(PlugInUIVals));

    notebook_data = g_new (NotebookData, 1);

    if (!gimp_drawable_is_valid_id(layer_ID)) {
        layer_ID = gimp_image_get_active_layer(image);
    }

    pres_toggle_data.ui_toggled = &(ui_state->pres_status);
    disc_toggle_data.ui_toggled = &(ui_state->disc_status);

    preview_data.pres_combo_awaked = FALSE;
    preview_data.disc_combo_awaked = FALSE;

    if (ui_state->pres_status == TRUE) {
        if (gimp_drawable_is_valid_id(state->pres_layer_ID) &&
            gimp_drawable_is_layer_id(state->pres_layer_ID) &&
            (state->pres_layer_ID != layer_ID)) {
            preview_data.pres_combo_awaked = TRUE;
        } else {
            state->pres_layer_ID = 0;
            ui_state->pres_status = FALSE;
        }
    }

    if (ui_state->disc_status == TRUE) {
        if (gimp_drawable_is_valid_id(state->disc_layer_ID) &&
            gimp_drawable_is_layer_id(state->disc_layer_ID) &&
            (state->disc_layer_ID != layer_ID)) {
            preview_data.disc_combo_awaked = TRUE;
        } else {
            state->disc_layer_ID = 0;
            ui_state->disc_status = FALSE;
        }
        preview_data.disc_combo_awaked = TRUE;
    }

    orig_width = gimp_drawable_get_width_id(layer_ID);
    orig_height = gimp_drawable_get_height_id(layer_ID);

    if (layer_ID != ui_state->last_layer_ID) {
        state->new_width = orig_width;
        state->new_height = orig_height;
    }

    g_assert (gimp_drawable_is_layer_id(layer_ID) == TRUE);

    drawable_vals->layer_ID = layer_ID;
    preview_data.orig_layer_ID = layer_ID;

    if (gimp_layer_has_mask_id(layer_ID)) {
        has_mask = TRUE;
    }

    num_extra_layers = count_extra_layers(image_ID);
    features_are_sensitive = (num_extra_layers > 0 ? TRUE : FALSE);
    if (!features_are_sensitive) {
        ui_state->pres_status = FALSE;
        ui_state->disc_status = FALSE;
        preview_data.pres_combo_awaked = FALSE;
        preview_data.disc_combo_awaked = FALSE;
    }

    dlg = gimp_dialog_new(
            _("GIMP LiquidRescale Plug-In"),
            PLUGIN_NAME,
            NULL,
            0,
            gimp_standard_help_func,
            "plug-in-lqr",
            "_Reset",
            RESPONSE_RESET,
            "_Refresh",
            RESPONSE_REFRESH,
            "_Cancel",
            GTK_RESPONSE_CANCEL,
            "_OK",
            GTK_RESPONSE_OK,
            NULL);

    gtk_window_set_resizable(GTK_WINDOW (dlg), FALSE);

    if (dialog_state->has_pos) {
        gtk_window_move(GTK_WINDOW(dlg), dialog_state->x, dialog_state->y);
        dialog_state->has_pos = FALSE;
    }

    g_signal_connect (
            dlg,
            "response",
            G_CALLBACK(callback_dialog_response),
            (gpointer) (notebook_data)
    );

    preview_data.dlg = dlg;

    /*
    ┌─────────────────────────┬──────────────────────────┐
    ┼─────────────────────────┼──────────────────────────┼
    │                         │                          │
    │          hbox           │            hbox          │
    │                         │                          │
    │                         │                          │
    │                         │                          │
    └─────────────────────────┴──────────────────────────┘
     */

    main_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
    gtk_container_set_border_width(GTK_CONTAINER (main_hbox), 12);
    gtk_container_add(GTK_CONTAINER (gtk_dialog_get_content_area(GTK_DIALOG(dlg))), main_hbox);

    /*
     * ┌─────────────────────────┬──────────────────────────┐
     * ┼─────────────────────────┼──────────────────────────┼
     * │┌───────────────────────┐│                          │
     * ││                       ││                          │
     * ││                       ││                          │
     * ││                       ││                          │
     * ││     vbox              ││                          │
     * ││                       ││                          │
     * ││                       ││                          │
     * ││                       ││                          │
     * ││                       ││                          │
     * │└───────────────────────┘│                          │
     * └─────────────────────────┴──────────────────────────┘
     */

    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
    gtk_box_pack_start(GTK_BOX (main_hbox), vbox, TRUE, TRUE, 0);
    gtk_widget_show(vbox);

/*
 * ┌─────────────────────────┬──────────────────────────┐
 * ┼─────────────────────────┼──────────────────────────┼
 * │┌───────────────────────┐│                          │
 * ││┌─────────────────────┐││                          │
 * │││                     │││                          │
 * │││                     │││                          │
 * │││   frame             │││                          │
 * │││                     │││                          │
 * │││                     │││                          │
 * │││                     │││                          │
 * ││└─────────────────────┘││                          │
 * │└───────────────────────┘│                          │
 * └─────────────────────────┴──────────────────────────┘
 */

    frame = gimp_frame_new(_("Selected layer"));
    gtk_box_pack_start(GTK_BOX (vbox), frame, FALSE, FALSE, 0);
    gtk_widget_show(frame);

    /*
 * ┌─────────────────────────┬──────────────────────────┐
 * ┼─────────────────────────┼──────────────────────────┼
 * │┌───────────────────────┐│                          │
 * ││┌─────────────────────┐││                          │
 * │││┌───────────────────┐│││                          │
 * ││││                   ││││                          │
 * ││││  vbox2            ││││                          │
 * ││││                   ││││                          │
 * ││││                   ││││                          │
 * │││└───────────────────┘│││                          │
 * ││└─────────────────────┘││                          │
 * │└───────────────────────┘│                          │
 * └─────────────────────────┴──────────────────────────┘
 */

    vbox2 = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
    gtk_container_set_border_width(GTK_CONTAINER (vbox2), 0);
    gtk_container_add(GTK_CONTAINER (frame), vbox2);
    gtk_widget_show(vbox2);

    /* Preview */

    preview_data.image_ID = image_ID;
    preview_data.vals = state;
    preview_data.ui_vals = ui_state;

    wfactor = (gfloat) gimp_drawable_get_width_id(layer_ID) / PREVIEW_MAX_WIDTH;
    hfactor = (gfloat) gimp_drawable_get_height_id(layer_ID) / PREVIEW_MAX_HEIGHT;

    preview_data.factor = MAX (wfactor, hfactor);
    preview_data.factor = MAX (preview_data.factor, 1);

    preview_data.old_width = orig_width;
    preview_data.old_height = orig_height;


    gimp_drawable_get_offsets_id(
            layer_ID,
            &(preview_data.x_off),
            &(preview_data.y_off));

    preview_data.width =
            gimp_drawable_get_width_id(preview_data.orig_layer_ID) / preview_data.factor;

    preview_data.height =
            gimp_drawable_get_height_id(preview_data.orig_layer_ID) / preview_data.factor;


    preview_data_create(image_ID, layer_ID, &preview_data);

    preview_build_pixbuf(&preview_data);

    preview_area = preview_area_create(&preview_data);

    /*
 * ┌─────────────────────────┬──────────────────────────┐
 * ┼─────────────────────────┼──────────────────────────┼
 * │┌───────────────────────┐│                          │
 * ││┌─────────────────────┐││                          │
 * │││┌───────────────────┐│││                          │
 * ││││┌────────────────┐ ││││                          │
 * │││││ preview area   │ ││││                          │
 * │││││                │ ││││                          │
 * ││││└────────────────┘ ││││                          │
 * │││└───────────────────┘│││                          │
 * ││└─────────────────────┘││                          │
 * │└───────────────────────┘│                          │
 * └─────────────────────────┴──────────────────────────┘
 */

    gtk_box_pack_start(GTK_BOX (vbox2), preview_area, FALSE, FALSE, 0);
    gtk_widget_show(preview_area);

    hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);

    gtk_container_set_border_width(GTK_CONTAINER (hbox), 4);
    gtk_box_pack_start(GTK_BOX (vbox2), hbox, FALSE, FALSE, 0);
    gtk_widget_show(hbox);

    filler = gtk_image_new();

    gtk_box_pack_start(GTK_BOX (hbox), filler, TRUE, TRUE, 0);
    gtk_widget_show(filler);

    filler = gtk_image_new();
    gtk_box_pack_end(GTK_BOX (hbox), filler, TRUE, TRUE, 0);
    gtk_widget_show(filler);

/*
 * ┌──────────────────────────────────┐
 * │┌──────┐  hbox           ┌──────┐ │
 * ││      │ ┌───┐┌───┐┌──┐  │      │ │
 * ││filler│ │   ││   ││  │  │filler│ │
 * ││      │ └───┘└───┘└──┘  │      │ │
 * │└──────┘                 └──────┘ │
 * └──────────────────────────────────┘
 *
 *
 */

    pres_use_image = gtk_image_new_from_icon_name(GIMP_ICON_CHANNEL_GREEN, GTK_ICON_SIZE_MENU);
    gtk_box_pack_start(GTK_BOX(hbox), pres_use_image, FALSE, FALSE, 0);
    gtk_widget_show(pres_use_image);

    disc_use_image = gtk_image_new_from_icon_name(GIMP_ICON_CHANNEL_RED, GTK_ICON_SIZE_MENU);
    gtk_box_pack_start(GTK_BOX(hbox), disc_use_image, FALSE, FALSE, 0);
    gtk_widget_show(disc_use_image);

    rigmask_use_image = gtk_image_new_from_icon_name(GIMP_ICON_CHANNEL_BLUE, GTK_ICON_SIZE_MENU);
    gtk_box_pack_start(GTK_BOX (hbox), rigmask_use_image, FALSE, FALSE, 0);
    gtk_widget_show(rigmask_use_image);


    preview_data.pres_use_image = pres_use_image;
    preview_data.disc_use_image = disc_use_image;
    preview_data.rigmask_use_image = rigmask_use_image;

    g_signal_connect(
            G_OBJECT(preview_area),
            "draw",
            G_CALLBACK(callback_preview_draw),
            (gpointer) (&preview_data)
    );


    /*  New size  */

    frame = gimp_frame_new(_("Select new width and height"));
    gtk_box_pack_start(GTK_BOX (vbox), frame, FALSE, FALSE, 0);
    gtk_widget_show(frame);

    vbox2 = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
    gtk_container_set_border_width(GTK_CONTAINER (vbox2), 4);
    gtk_container_add(GTK_CONTAINER (frame), vbox2);
    gtk_widget_show(vbox2);

    hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
    gtk_container_set_border_width(GTK_CONTAINER (hbox), 4);
    gtk_box_pack_start(GTK_BOX (vbox2), hbox, FALSE, FALSE, 0);
    gtk_widget_show(hbox);

    unit = gimp_image_get_unit_id(image_ID);
    gimp_image_get_resolution_id(image_ID, &xres, &yres);

    coordinates =
            alt_coordinates_new(unit, "%p", TRUE, TRUE, SPIN_BUTTON_WIDTH,
                                ALT_SIZE_ENTRY_UPDATE_SIZE, ui_state->chain_active,
                                TRUE, _("Width:"), state->new_width, xres, 2,
                                GIMP_MAX_IMAGE_SIZE, 0, orig_width,
                                _("Height:"), state->new_height, yres, 2,
                                GIMP_MAX_IMAGE_SIZE, 0, orig_height);

    if (layer_ID != ui_state->last_layer_ID) {
        alt_size_entry_set_refval(ALT_SIZE_ENTRY (coordinates), 0,
                                  state->new_width);
        alt_size_entry_set_refval(ALT_SIZE_ENTRY (coordinates), 1,
                                  state->new_height);
    }

    g_signal_connect (ALT_SIZE_ENTRY(coordinates), "value-changed",
                      G_CALLBACK(callback_size_changed),
                      (gpointer) &preview_data);

    g_signal_connect (ALT_SIZE_ENTRY(coordinates), "refval-changed",
                      G_CALLBACK(callback_size_changed),
                      (gpointer) &preview_data);

    gtk_box_pack_start(GTK_BOX (hbox), coordinates, FALSE, FALSE, 0);
    gtk_widget_show(coordinates);

    preview_data.coordinates = (gpointer) coordinates;

    /* Aux buttons */

    vbox3 = gtk_box_new(GTK_ORIENTATION_VERTICAL, 1);
    gtk_box_pack_start(GTK_BOX (hbox), vbox3, FALSE, FALSE, 0);
    gtk_widget_show(vbox3);

    resetvalues_event_box = gtk_event_box_new();
    gtk_box_pack_start(GTK_BOX (vbox3), resetvalues_event_box, FALSE, FALSE,
                       0);
    gtk_widget_show(resetvalues_event_box);

    gimp_help_set_help_data(
            resetvalues_event_box,
            _("Reset width and height to their original values"),
            NULL);

    resetvalues_button = gtk_button_new();
    resetvalues_icon = gtk_image_new_from_icon_name(
            GIMP_ICON_RESET,
            GTK_ICON_SIZE_MENU
    );

    gtk_container_add(GTK_CONTAINER (resetvalues_button), resetvalues_icon);
    gtk_widget_show(resetvalues_icon);
    gtk_container_add(GTK_CONTAINER (resetvalues_event_box),
                      resetvalues_button);
    gtk_widget_show(resetvalues_button);

    g_signal_connect (resetvalues_button, "clicked",
                      G_CALLBACK(callback_resetvalues_button),
                      (gpointer) &preview_data);

    lastvalues_event_box = gtk_event_box_new();
    gtk_box_pack_start(GTK_BOX (vbox3), lastvalues_event_box, FALSE, FALSE, 0);
    gtk_widget_show(lastvalues_event_box);

    gimp_help_set_help_data(lastvalues_event_box,
                            _("Set width and height to the last used values"),
                            NULL);

    lastvalues_button = gtk_button_new();
    lastvalues_icon =
            gtk_image_new_from_stock(GTK_STOCK_REVERT_TO_SAVED, GTK_ICON_SIZE_MENU);

    gtk_container_add(GTK_CONTAINER (lastvalues_button), lastvalues_icon);
    gtk_widget_show(lastvalues_icon);
    gtk_container_add(GTK_CONTAINER (lastvalues_event_box), lastvalues_button);
    gtk_widget_show(lastvalues_button);

    g_signal_connect (lastvalues_button, "clicked",
                      G_CALLBACK(callback_lastvalues_button),
                      (gpointer) &preview_data);

    gtk_widget_set_sensitive(lastvalues_button,
                             ((ui_state->last_used_width != -1)
                              && (ui_state->last_used_height !=
                                  -1)) ? TRUE : FALSE);

    vbox3 = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
    gtk_box_pack_start(GTK_BOX (hbox), vbox3, FALSE, FALSE, 0);
    gtk_widget_show(vbox3);

    interactive_event_box = gtk_event_box_new();
    gtk_box_pack_start(GTK_BOX (vbox3), interactive_event_box, FALSE, FALSE,
                       0);
    gtk_widget_show(interactive_event_box);

    gimp_help_set_help_data(interactive_event_box,
                            _("Switch to interactive mode. "
                              "Note that the current settings will be applied."),
                            NULL);

    interactive_button = gtk_button_new();
    interactive_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
    gtk_container_add(GTK_CONTAINER (interactive_button), interactive_hbox);
    gtk_widget_show(interactive_hbox);
    interactive_icon =
            gtk_image_new_from_stock(GTK_STOCK_EXECUTE, GTK_ICON_SIZE_LARGE_TOOLBAR);
    gtk_box_pack_start(GTK_BOX(interactive_hbox), interactive_icon, TRUE, FALSE, 0);
    gtk_widget_show(interactive_icon);
    interactive_label = gtk_label_new_with_mnemonic(_("_Interactive"));
    gtk_box_pack_start(GTK_BOX (interactive_hbox), interactive_label, TRUE, FALSE, 0);
    gtk_widget_show(interactive_label);
    gtk_container_add(GTK_CONTAINER (interactive_event_box),
                      interactive_button);
    gtk_widget_show(interactive_button);

    g_signal_connect (interactive_button, "clicked",
                      G_CALLBACK(callback_interactive_button),
                      (gpointer) dlg);

    /* Notebook */

    /*
    *       ┌─────────────────────────┬──────────────────────────┐
    *       ┼─────────────────────────┼──────────────────────────┼
    *       │┌───────────────────────┐│ ┌─────────────────────┐  │
    *       ││┌─────────────────────┐││ │                     │  │
    *       │││┌───────────────────┐│││ │                     │  │
    *       ││││┌────────────────┐ ││││ │                     │  │
    *       │││││ preview area   │ ││││ │                     │  │
    *       │││││                │ ││││ │   notebook          │  │
    *       ││││└────────────────┘ ││││ │                     │  │
    *       │││└───────────────────┘│││ │                     │  │
    *       ││└─────────────────────┘││ │                     │  │
    *       │└───────────────────────┘│ └─────────────────────┘  │
    *       └─────────────────────────┴──────────────────────────┘
    *
            */

    notebook = gtk_notebook_new();
    gtk_box_pack_start(GTK_BOX (main_hbox), notebook, TRUE, TRUE, 5);
    gtk_widget_show(notebook);
    notebook_data->notebook = notebook;
    notebook_data->image_ID = image_ID;
    notebook_data->layer_ID = layer_ID;

    /* Fature masks page */

    features_page = features_page_new(image_ID, layer_ID);
    gtk_widget_show(features_page);
    notebook_data->features_page_ID =
            gtk_notebook_prepend_page_menu(GTK_NOTEBOOK (notebook), features_page,
                                           notebook_data->label, NULL);

    /* Output settings page */

    label = gtk_label_new(_("Output"));

    thispage = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
    gtk_container_set_border_width(GTK_CONTAINER (thispage), 12);
    gtk_notebook_append_page_menu(GTK_NOTEBOOK (notebook), thispage, label,
                                  NULL);
    gtk_widget_show(thispage);

    /* Output checkboxes */

    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
    gtk_box_pack_start(GTK_BOX (thispage), vbox, FALSE, FALSE, 0);
    gtk_widget_show(vbox);

    output_target_event_box = gtk_event_box_new();
    gtk_box_pack_start(GTK_BOX (vbox), output_target_event_box, FALSE, FALSE, 0);
    gtk_widget_show(output_target_event_box);

    gimp_help_set_help_data(output_target_event_box,
                            _
                            ("The result of rescaling can be put in the current layer, in a new one "
                             "or in a new image"),
                            NULL);

    output_target_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
    gtk_container_add(GTK_CONTAINER (output_target_event_box), output_target_hbox);
    gtk_widget_show(output_target_hbox);

    output_target_label = gtk_label_new(_("Output target:"));
    gtk_box_pack_start(GTK_BOX(output_target_hbox), output_target_label, FALSE, FALSE, 0);
    gtk_widget_show(output_target_label);

    output_target_combo_box =
            gimp_int_combo_box_new(_("selected layer"), OUTPUT_TARGET_SAME_LAYER,
                                   _("new layer"), OUTPUT_TARGET_NEW_LAYER,
                                   _("new image"), OUTPUT_TARGET_NEW_IMAGE,
                                   NULL);
    gimp_int_combo_box_set_active(GIMP_INT_COMBO_BOX(output_target_combo_box),
                                  state->output_target);

    gimp_int_combo_box_connect(GIMP_INT_COMBO_BOX(output_target_combo_box),
                               state->output_target,
                               G_CALLBACK (callback_output_target_changed),
                               (gpointer) &preview_data,
                               NULL);

    gtk_box_pack_start(GTK_BOX (output_target_hbox), output_target_combo_box, FALSE, FALSE, 0);
    gtk_widget_show(output_target_combo_box);
    /**/

    resize_canvas_button =
            gtk_check_button_new_with_label(_("Resize image canvas"));

    gtk_box_pack_start(GTK_BOX (vbox), resize_canvas_button, FALSE, FALSE, 0);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON (resize_canvas_button),
                                 state->resize_canvas);
    gtk_widget_show(resize_canvas_button);

    gimp_help_set_help_data(resize_canvas_button,
                            _("Resize and translate the image "
                              "canvas to fit the resized layer"), NULL);

    resize_aux_layers_button =
            gtk_check_button_new_with_label(_("Resize auxiliary layers"));

    gtk_box_pack_start(GTK_BOX (vbox), resize_aux_layers_button, FALSE, FALSE,
                       0);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON (resize_aux_layers_button),
                                 state->resize_aux_layers);

    presdisc_status.ui_vals = (gpointer) ui_state;
    presdisc_status.button = (gpointer) resize_aux_layers_button;

    callback_resize_aux_layers_button_set_sensitive(NULL,
                                                    (gpointer)
                                                            (&presdisc_status));

    gtk_widget_show(resize_aux_layers_button);

    gimp_help_set_help_data(resize_aux_layers_button,
                            _
                            ("Resize the layers used as features or rigidity masks "
                             "along with the active layer"), NULL);

    out_seams_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
    gtk_box_pack_start(GTK_BOX (vbox), out_seams_hbox, FALSE, FALSE, 0);
    gtk_widget_show(out_seams_hbox);

    out_seams_button = gtk_check_button_new_with_label(_("Output the seams"));

    gtk_box_pack_start(GTK_BOX (out_seams_hbox), out_seams_button, FALSE,
                       FALSE, 0);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON (out_seams_button),
                                 state->output_seams);
    gtk_widget_show(out_seams_button);

    gimp_help_set_help_data(out_seams_button,
                            _("Creates an extra output layer with the seams, "
                              "for visual inspection of what the plugin did. "
                              "Use it together with \"Output on a new layer\", "
                              "and resize in one direction at a time.\n"
                              "Note that this option is ignored in interactive mode"), NULL);

    g_signal_connect (out_seams_button, "toggled",
                      G_CALLBACK(callback_out_seams_button),
                      (gpointer) &(state->output_seams));

    colour = gegl_color_new("black");
    gegl_color_set_rgba(colour, col_vals->r2, col_vals->g2, col_vals->b2, 1.0);

//    //gimp_rgba_set(colour, col_vals->r2, col_vals->g2, col_vals->b2, 1);

    out_seams_col_button2 =
            gimp_color_button_new(_("Last seams colour"), 14, 14, colour,
                                  GIMP_COLOR_AREA_FLAT);
    gtk_box_pack_end(GTK_BOX (out_seams_hbox), out_seams_col_button2, FALSE,
                     FALSE, 0);
    gtk_widget_show(out_seams_col_button2);

    g_signal_connect (out_seams_col_button2, "color-changed",
                      G_CALLBACK(callback_out_seams_col_button2),
                      (gpointer) (col_vals));

    gimp_help_set_help_data(out_seams_col_button2,
                            _("Colour to use for the last seams"), NULL);

    // gimp_rgba_set(colour, col_vals->r1, col_vals->g1, col_vals->b1, 1);
    gegl_color_set_rgba(colour, col_vals->r1, col_vals->g1, col_vals->b1, 1.0);

    out_seams_col_button1 =
            gimp_color_button_new(_("First seams colour"), 14, 14, colour,
                                  GIMP_COLOR_AREA_FLAT);
    gtk_box_pack_end(GTK_BOX (out_seams_hbox), out_seams_col_button1, FALSE,
                     FALSE, 0);
    gtk_widget_show(out_seams_col_button1);

    g_signal_connect (out_seams_col_button1, "color-changed",
                      G_CALLBACK(callback_out_seams_col_button1),
                      (gpointer) (col_vals));

    gimp_help_set_help_data(out_seams_col_button1,
                            _("Colour to use for the first seams"), NULL);

//    g_free(colour);

    scaleback_button =
            gtk_check_button_new_with_label(_("Scale back to the original size"));
    gtk_box_pack_start(GTK_BOX (vbox), scaleback_button, FALSE, FALSE, 0);

    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON (scaleback_button),
                                 state->scaleback);

    gtk_widget_show(scaleback_button);

    gimp_help_set_help_data(scaleback_button,
                            _
                            ("Select this if you want to transform back the "
                             "layer after LqR has been performed.\n"
                             "Note that this option is ignored in interactive mode"),
                            NULL);

    scaleback_mode_alignment = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_set_margin_start(scaleback_mode_alignment, BOX_INDENT);
    gtk_box_pack_start(GTK_BOX (vbox), scaleback_mode_alignment, FALSE, FALSE, 0);
    gtk_widget_show(scaleback_mode_alignment);

    scaleback_mode_event_box = gtk_event_box_new();
    gtk_container_add(GTK_CONTAINER (scaleback_mode_alignment), scaleback_mode_event_box);
    gtk_widget_show(scaleback_mode_event_box);

    gimp_help_set_help_data(scaleback_mode_event_box,
                            _
                            ("You can choose to rescale back to the original size with LqR or "
                             "standard scaling, or to use standard scaling to reach the previous width "
                             "or height while preserving the aspect ratio"),
                            NULL);

    scaleback_mode_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
    gtk_container_add(GTK_CONTAINER (scaleback_mode_event_box), scaleback_mode_hbox);
    gtk_widget_show(scaleback_mode_hbox);

    scaleback_mode_label = gtk_label_new(_("Mode:"));
    gtk_box_pack_start(GTK_BOX(scaleback_mode_hbox), scaleback_mode_label, FALSE, FALSE, 0);
    gtk_widget_show(scaleback_mode_label);

    scaleback_mode_combo_box =
            gimp_int_combo_box_new(_("liquid rescale"), SCALEBACK_MODE_LQRBACK,
                                   _("standard scaling"), SCALEBACK_MODE_STD,
                                   _("width only (uniform scaling)"), SCALEBACK_MODE_STDW,
                                   _("height only (uniform scaling)"), SCALEBACK_MODE_STDH,
                                   NULL);
    gimp_int_combo_box_set_active(GIMP_INT_COMBO_BOX(scaleback_mode_combo_box),
                                  state->scaleback_mode);

    gimp_int_combo_box_connect(GIMP_INT_COMBO_BOX(scaleback_mode_combo_box),
                               state->scaleback_mode,
                               G_CALLBACK (callback_scaleback_mode_changed),
                               (gpointer) &preview_data,
                               NULL);

    gtk_box_pack_start(GTK_BOX (scaleback_mode_hbox), scaleback_mode_combo_box, FALSE, FALSE, 0);
    gtk_widget_show(scaleback_mode_combo_box);

    g_signal_connect (scaleback_button, "toggled",
                      G_CALLBACK(callback_scaleback_button),
                      (gpointer) (scaleback_mode_alignment));

    callback_scaleback_button(scaleback_button, (gpointer) scaleback_mode_alignment);



    /* Advanced settings page */

    advanced_page = advanced_page_new(image_ID, layer_ID);
    gtk_widget_show(advanced_page);
    notebook_data->advanced_page_ID =
            gtk_notebook_append_page_menu(GTK_NOTEBOOK (notebook), advanced_page,
                                          notebook_data->label, NULL);

    /* Mask */

    if (has_mask == TRUE) {
        /* Mask page */

        label = gtk_label_new(_("Mask"));

        thispage = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
        gtk_container_set_border_width(GTK_CONTAINER (thispage), 12);
        gtk_notebook_append_page_menu(GTK_NOTEBOOK (notebook), thispage, label,
                                      NULL);
        gtk_widget_show(thispage);

        frame = gimp_frame_new(_("Select behaviour for the mask"));
        gtk_box_pack_start(GTK_BOX (thispage), frame, FALSE, FALSE, 0);
        gtk_widget_show(frame);

        mask_behavior_combo_box =
                gimp_int_combo_box_new(_("Apply"), GIMP_MASK_APPLY, _("Discard"),
                                       GIMP_MASK_DISCARD, NULL);
        gimp_int_combo_box_set_active(GIMP_INT_COMBO_BOX
                                              (mask_behavior_combo_box),
                                      state->mask_behavior);

        gtk_container_add(GTK_CONTAINER (frame), mask_behavior_combo_box);
        gtk_widget_show(mask_behavior_combo_box);
    }

    /*  Show the main containers  */

    gtk_widget_show(main_hbox);
    gtk_widget_show(dlg);
    gtk_main();

    if ((dialog_response == GTK_RESPONSE_OK) || (dialog_response == RESPONSE_INTERACTIVE) ||
        (dialog_response == RESPONSE_WORK_ON_AUX_LAYER)) {
        /*  Save ui values  */
        ui_state->chain_active =
                gimp_chain_button_get_active(GIMP_COORDINATES_CHAINBUTTON
                                             (coordinates));
        state->new_width =
                ROUND (alt_size_entry_get_refval(ALT_SIZE_ENTRY(coordinates), 0));
        state->new_height =
                ROUND (alt_size_entry_get_refval(ALT_SIZE_ENTRY(coordinates), 1));
        gimp_int_combo_box_get_active(GIMP_INT_COMBO_BOX(nrg_func_combo_box),
                                      &(state->nrg_func));
        gimp_int_combo_box_get_active(GIMP_INT_COMBO_BOX(res_order_combo_box),
                                      &(state->res_order));
        state->resize_canvas =
                gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON
                                             (resize_canvas_button));
        state->resize_aux_layers =
                gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON
                                             (resize_aux_layers_button));
        /* save mask behaviour */
        if (has_mask == TRUE) {
            gimp_int_combo_box_get_active(GIMP_INT_COMBO_BOX
                                                  (mask_behavior_combo_box),
                                          &(state->mask_behavior));
        }

        /* save all */
        memcpy(vals, state, sizeof(PlugInVals));
        memcpy(ui_vals, ui_state, sizeof(PlugInUIVals));
    }

    gtk_widget_destroy(dlg);

    g_object_unref(G_OBJECT (preview_data.pixbuf));
    g_free(state);
    g_free(ui_state);
    g_free(notebook_data);

    return dialog_response;
}


/***  Private functions  ***/
