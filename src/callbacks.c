//
// Created by tomhodder on 30/05/25.
//

#include "callbacks.h"

/* External variables */
extern gint dialog_response;
extern PlugInUIVals *ui_state;
extern PlugInVals *state;
extern PlugInDialogVals *dialog_state;
extern NotebookData *notebook_data;
extern gboolean features_are_sensitive;
extern PreviewData preview_data;
extern PresDiscStatus presdisc_status;
extern ToggleData pres_toggle_data;
extern ToggleData disc_toggle_data;
extern ToggleData rigmask_toggle_data;
extern GtkWidget *nrg_func_combo_box;
extern GtkWidget *res_order_combo_box;
extern GtkWidget *dlg;


/* Callbacks */

void
callback_dialog_response(GtkWidget *dialog, gint response_id, gpointer data) {
    NotebookData *n_data = NOTEBOOK_DATA (data);
    switch (response_id) {
        case RESPONSE_WORK_ON_AUX_LAYER:
        case RESPONSE_INTERACTIVE:
        case RESPONSE_REFRESH:
        case GTK_RESPONSE_OK:
        case RESPONSE_FEAT_REFRESH:
        case RESPONSE_ADV_REFRESH:
        case RESPONSE_RESET:
            LAYER_CHECK_ACTION (n_data->layer_ID, gtk_dialog_response(GTK_DIALOG(dialog), RESPONSE_FATAL),);
            gtk_window_get_position(GTK_WINDOW(dialog), &(dialog_state->x), &(dialog_state->y));
            dialog_state->has_pos = TRUE;
            break;
        default:
            break;
    }
    switch (response_id) {
        case RESPONSE_REFRESH:
            refresh_advanced_page(n_data);
            refresh_features_page(n_data);
            break;
        case RESPONSE_FEAT_REFRESH:
            refresh_features_page(n_data);
            break;
        case RESPONSE_ADV_REFRESH:
            refresh_advanced_page(n_data);
            break;
        default:
            dialog_response = response_id;
            gtk_main_quit();
            break;
    }
}

void
callback_set_disc_warning(GtkWidget *dummy, gpointer data) {
    PreviewData *p_data = PREVIEW_DATA (data);
    gboolean issue_warn;
    gint old_w, old_h;
    gint new_w, new_h;

    if ((p_data->vals->no_disc_on_enlarge == FALSE) ||
        (p_data->ui_vals->disc_status == FALSE) ||
        (p_data->vals->disc_coeff == 0)) {
        gtk_widget_hide(GTK_WIDGET (p_data->disc_warning_image));
    } else {
        old_w = p_data->old_width;
        old_h = p_data->old_height;
        new_w = p_data->vals->new_width;
        new_h = p_data->vals->new_height;
        issue_warn = FALSE;
        switch (p_data->vals->res_order) {
            case LQR_RES_ORDER_HOR:
                if ((new_w > old_w) || ((new_w == old_w) && (new_h > old_h))) {
                    issue_warn = TRUE;
                }
                break;
            case LQR_RES_ORDER_VERT:
                if ((new_h > old_h) || ((new_h == old_h) && (new_w > old_w))) {
                    issue_warn = TRUE;
                }
                break;
        }
        if (issue_warn == TRUE) {
            gtk_widget_show(GTK_WIDGET (p_data->disc_warning_image));
        } else {
            gtk_widget_hide(GTK_WIDGET (p_data->disc_warning_image));
        }
    }
}

void
callback_size_changed(GtkWidget *size_entry, gpointer data) {
    gint new_width, new_height;
    PreviewData *p_data = PREVIEW_DATA (data);
    new_width =
            ROUND (alt_size_entry_get_refval(ALT_SIZE_ENTRY(size_entry), 0));
    new_height =
            ROUND (alt_size_entry_get_refval(ALT_SIZE_ENTRY(size_entry), 1));
    p_data->vals->new_width = new_width;
    p_data->vals->new_height = new_height;
    callback_set_disc_warning(NULL, data);
}

void
callback_res_order_changed(GtkWidget *res_order, gpointer data) {
    gint order;
    PreviewData *p_data = PREVIEW_DATA (data);
    gimp_int_combo_box_get_active(GIMP_INT_COMBO_BOX(res_order), &order);
    p_data->vals->res_order = order;
    callback_set_disc_warning(NULL, data);
}

void
callback_output_target_changed(GtkWidget *output_target_combo, gpointer data) {
    gint mode;
    PreviewData *p_data = PREVIEW_DATA (data);
    gimp_int_combo_box_get_active(GIMP_INT_COMBO_BOX(output_target_combo), &mode);
    p_data->vals->output_target = mode;
}

void
callback_scaleback_button(GtkWidget *button, gpointer data) {
    gboolean button_status =
            gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON (button));
    state->scaleback = button_status;
    if (button_status) {
        gtk_widget_show(GTK_WIDGET (data));
    } else {
        gtk_widget_hide(GTK_WIDGET (data));
    }
}


void
callback_scaleback_mode_changed(GtkWidget *scaleback_mode_combo, gpointer data) {
    gint mode;
    PreviewData *p_data = PREVIEW_DATA (data);
    gimp_int_combo_box_get_active(GIMP_INT_COMBO_BOX(scaleback_mode_combo), &mode);
    p_data->vals->scaleback_mode = mode;
}

void
callback_lastvalues_button(GtkWidget *button, gpointer data) {
    gint new_width, new_height;
    PreviewData *p_data = PREVIEW_DATA (data);
    new_width = p_data->ui_vals->last_used_width;
    new_height = p_data->ui_vals->last_used_height;

    alt_size_entry_set_refval(ALT_SIZE_ENTRY
                                      (p_data->coordinates), 0, new_width);
    alt_size_entry_set_refval(ALT_SIZE_ENTRY
                                      (p_data->coordinates), 1, new_height);
}

void
callback_resetvalues_button(GtkWidget *button, gpointer data) {
    gint new_width, new_height;
    PreviewData *p_data = PREVIEW_DATA (data);

    new_width = gimp_drawable_get_width(gimp_drawable_get_by_id(p_data->orig_layer_ID));
    new_height = gimp_drawable_get_height(gimp_drawable_get_by_id(p_data->orig_layer_ID));

    alt_size_entry_set_refval(ALT_SIZE_ENTRY (p_data->coordinates), 0,
                              new_width);
    alt_size_entry_set_refval(ALT_SIZE_ENTRY (p_data->coordinates), 1,
                              new_height);
}

void
callback_interactive_button(GtkWidget *button, gpointer data) {
    gtk_dialog_response(GTK_DIALOG (data), RESPONSE_INTERACTIVE);
}


void
callback_out_seams_button(GtkWidget *button, gpointer data) {
    *((gboolean *) data) =
            gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON (button));
}

void
callback_out_seams_col_button1(GtkWidget *button, gpointer data) {
    GeglColor *colour;
    PlugInColVals *col_data = (PlugInColVals *) data;
    gdouble rgba[4];

    colour = gegl_color_new("rgba(0,0,0,1)");
    colour = gimp_color_button_get_color(GIMP_COLOR_BUTTON(button));

    // Extract RGBA components from GeglColor
    gegl_color_get_rgba(colour, &rgba[0], &rgba[1], &rgba[2], &rgba[3]);

    col_data->r1 = rgba[0];
    col_data->g1 = rgba[1];
    col_data->b1 = rgba[2];

    g_object_unref(colour);  // Use g_object_unref instead of g_free
}

void
callback_out_seams_col_button2(GtkWidget *button, gpointer data) {
    GeglColor *colour;
    PlugInColVals *col_data = (PlugInColVals *) data;
    gdouble rgba[4];

    colour = gegl_color_new("rgba(0,0,0,1)");
    colour = gimp_color_button_get_color(GIMP_COLOR_BUTTON
                                                 (button));

    // Extract RGBA components from GeglColor
    gegl_color_get_rgba(colour, &rgba[0], &rgba[1], &rgba[2], &rgba[3]);

    col_data->r1 = rgba[0];
    col_data->g1 = rgba[1];
    col_data->b1 = rgba[2];

    g_free(colour);
}

void
callback_resize_aux_layers_button_set_sensitive(GtkWidget *button,
                                                gpointer data) {
    PresDiscStatus *pd_status = PRESDISC_STATUS (data);
    PlugInUIVals *ui = PLUGIN_UI_VALS (pd_status->ui_vals);
    if ((ui->pres_status == TRUE) || (ui->disc_status == TRUE)
        || (ui->rigmask_status == TRUE)) {
        gtk_widget_set_sensitive((GtkWidget *) (pd_status->button), TRUE);
    } else {
        gtk_widget_set_sensitive((GtkWidget *) (pd_status->button), FALSE);
    }
}

void callback_expander_changed(GtkWidget *expander, gpointer data) {
    gboolean *b_data = (gboolean *) data;
    *b_data = !gtk_expander_get_expanded(GTK_EXPANDER(expander));
}


/* Refresh */

void
refresh_features_page(NotebookData *data) {
    GtkWidget *new_page;
    gint current_page;

    current_page =
            gtk_notebook_get_current_page(GTK_NOTEBOOK (data->notebook));
    gtk_notebook_remove_page(GTK_NOTEBOOK (data->notebook),
                             data->features_page_ID);
    new_page = features_page_new(data->image_ID, data->layer_ID);
    gtk_widget_show(new_page);
    data->features_page_ID =
            gtk_notebook_prepend_page_menu(GTK_NOTEBOOK (data->notebook), new_page,
                                           data->label, NULL);
    data->features_page = new_page;
    gtk_notebook_set_current_page(GTK_NOTEBOOK (data->notebook), current_page);
    callback_resize_aux_layers_button_set_sensitive(NULL,
                                                    (gpointer)
                                                            (&presdisc_status));
}

void
refresh_advanced_page(NotebookData *data) {
    GtkWidget *new_page;
    gint current_page;

    current_page =
            gtk_notebook_get_current_page(GTK_NOTEBOOK (data->notebook));
    gtk_notebook_remove_page(GTK_NOTEBOOK (data->notebook),
                             data->advanced_page_ID);
    new_page = advanced_page_new(data->image_ID, data->layer_ID);
    gtk_widget_show(new_page);
    data->advanced_page_ID =
            gtk_notebook_append_page_menu(GTK_NOTEBOOK (data->notebook), new_page,
                                          data->label, NULL);
    data->advanced_page = new_page;
    gtk_notebook_set_current_page(GTK_NOTEBOOK (data->notebook), current_page);
    callback_resize_aux_layers_button_set_sensitive(NULL,
                                                    (gpointer)
                                                            (&presdisc_status));
}


