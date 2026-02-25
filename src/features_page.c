//
// Created by tomhodder on 30/05/25.
//

#include "features_page.h"

extern NotebookData *notebook_data;
extern PreviewData preview_data;
extern PlugInUIVals *ui_state;
extern PlugInVals *state;
extern gboolean features_are_sensitive;
extern ToggleData pres_toggle_data;
extern ToggleData disc_toggle_data;
extern PresDiscStatus presdisc_status;

/* Generate features page */

GtkWidget *
features_page_new(gint32 image_ID, gint32 layer_ID) {
    gint num_extra_layers;
    GtkWidget *label;
    GtkWidget *thispage;
    gchar pres_inactive_tip_string[MAX_STRING_SIZE];
    gchar disc_inactive_tip_string[MAX_STRING_SIZE];
    gchar *disc_strength_tip_string;
    gchar disc_strength_tip_string_[MAX_STRING_SIZE];
    gchar *pres_strength_tip_string;
    gchar pres_strength_tip_string_[MAX_STRING_SIZE];
    NewLayerData *new_pres_layer_data;
    NewLayerData *new_disc_layer_data;
    GtkWidget *pres_frame_event_box1;
    GtkWidget *pres_frame_event_box2;
    GtkWidget *disc_frame_event_box1;
    GtkWidget *disc_frame_event_box2;
    GtkWidget *pres_combo_event_box;
    GtkWidget *disc_combo_event_box;
    gint32 old_layer_ID;
    GtkWidget *frame;
    GtkWidget *pres_vbox;
    GtkWidget *pres_vbox2;
    GtkWidget *hbox;
    GtkWidget *new_hbox;
    GtkWidget *new_icon;
    GtkWidget *new_label;
    GtkWidget *edit_hbox;
    GtkWidget *edit_icon;
    GtkWidget *edit_label;
    GtkWidget *pres_button;
    GtkWidget *pres_new_button;
    GtkWidget *pres_edit_button;
    GtkWidget *disc_vbox;
    GtkWidget *disc_vbox2;
    GtkWidget *disc_button;
    GtkWidget *disc_new_button;
    GtkWidget *disc_edit_button;
    GtkWidget *disc_warning_image;
    GtkWidget *guess_label;
    GtkWidget *guess_button_hor;
    GtkWidget *guess_button_ver;
    GtkWidget *combo;
    GtkWidget *pres_layer_label;
    GtkWidget *disc_layer_label;

    label = gtk_label_new(_("Feature masks"));
    notebook_data->label = label;

    new_pres_layer_data = g_new (NewLayerData, 1);
    new_disc_layer_data = g_new (NewLayerData, 1);

    new_pres_layer_data->preview_data = &preview_data;
    new_pres_layer_data->layer_ID = &(state->pres_layer_ID);
    new_pres_layer_data->status = &(ui_state->pres_status);
    /* The name of a newly created layer for preservation */
    /* (here "%s" represents the selected layer's name) */
    g_snprintf(new_pres_layer_data->name, LQR_MAX_NAME_LENGTH, _("%s pres mask"),
               gimp_item_get_name(gimp_item_get_by_id(preview_data.orig_layer_ID)));

    new_pres_layer_data->colour = gegl_color_new("black");
    gegl_color_set_rgba(new_pres_layer_data->colour, 0.0, 1.0, 0.0, 1.0);
    new_pres_layer_data->layer_type = AUX_LAYER_PRES;

    new_disc_layer_data->preview_data = &preview_data;
    new_disc_layer_data->layer_ID = &(state->disc_layer_ID);
    new_disc_layer_data->status = &(ui_state->disc_status);
    /* The name of a newly created layer for discard */
    /* (here "%s" represents the selected layer's name) */
    g_snprintf(new_disc_layer_data->name, LQR_MAX_NAME_LENGTH, _("%s disc mask"),
               gimp_item_get_name(gimp_item_get_by_id(preview_data.orig_layer_ID)));
    new_disc_layer_data->colour = gegl_color_new("black");
    gegl_color_set_rgba(new_disc_layer_data->colour, 1.0, 0.0, 0.0, 1.0);
    new_disc_layer_data->layer_type = AUX_LAYER_DISC;

    num_extra_layers = count_extra_layers(image_ID);
    features_are_sensitive = (num_extra_layers > 0 ? TRUE : FALSE);

    if (!features_are_sensitive ||
        !gimp_item_is_valid(gimp_item_get_by_id(state->pres_layer_ID)) ||
        !gimp_item_is_layer(gimp_item_get_by_id(state->pres_layer_ID)) ||
        (state->pres_layer_ID == layer_ID)) {
        ui_state->pres_status = FALSE;
        state->pres_layer_ID = 0;
        preview_data.pres_combo_awaked = FALSE;
    }
    if (!features_are_sensitive ||
        !gimp_item_is_valid(gimp_item_get_by_id(state->disc_layer_ID)) ||
        !gimp_item_is_layer(gimp_item_get_by_id(state->disc_layer_ID)) ||
        (state->disc_layer_ID == layer_ID)) {
        ui_state->disc_status = FALSE;
        state->disc_layer_ID = 0;
        preview_data.disc_combo_awaked = FALSE;
    }

    thispage = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
    gtk_container_set_border_width(GTK_CONTAINER (thispage), 12);
    notebook_data->features_page = thispage;


    /*  Feature preservation  */

    frame = gimp_frame_new(_("Feature preservation mask"));
    gtk_box_pack_start(GTK_BOX (thispage), frame, FALSE, FALSE, 0);
    gtk_widget_show(frame);

    g_snprintf(pres_inactive_tip_string, MAX_STRING_SIZE,
               _("Extra layers are needed to activate feature preservation.\n"
                 "You can create one with the \"New\" button and paint on it, "
                 "then press the \"Refresh\" button.\n"
                 "Note that painting in black has no effect"));

    pres_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
    gtk_container_add(GTK_CONTAINER (frame), pres_vbox);
    gtk_widget_show(pres_vbox);

    hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
    gtk_box_pack_start(GTK_BOX (pres_vbox), hbox, FALSE, FALSE, 0);
    gtk_widget_show(hbox);

    pres_frame_event_box1 = gtk_event_box_new();
    gtk_event_box_set_visible_window(GTK_EVENT_BOX (pres_frame_event_box1),
                                     FALSE);
    gtk_box_pack_start(GTK_BOX (hbox), pres_frame_event_box1, FALSE, FALSE, 0);
    gtk_widget_show(pres_frame_event_box1);


    if (!features_are_sensitive) {
        gtk_event_box_set_above_child(GTK_EVENT_BOX (pres_frame_event_box1),
                                      TRUE);
        gtk_widget_set_tooltip_text(pres_frame_event_box1, pres_inactive_tip_string);
    }


    pres_button = gtk_check_button_new_with_label(_("Preserve features"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON (pres_button),
                                 ui_state->pres_status);

    gtk_widget_set_sensitive(pres_button, features_are_sensitive);


    gtk_container_add(GTK_CONTAINER (pres_frame_event_box1), pres_button);
    gtk_widget_show(pres_button);

    g_signal_connect (pres_button, "toggled",
                      G_CALLBACK
                              (callback_status_button),
                      (gpointer) (&ui_state->pres_status));

    gimp_help_set_help_data(pres_button,
                            _("Use an extra layer to preserve "
                              "selected areas from distortion"), NULL);

    pres_edit_button = gtk_button_new();
    gtk_box_pack_end(GTK_BOX (hbox), pres_edit_button, FALSE, FALSE, 0);
    gtk_widget_show(pres_edit_button);

    edit_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
    gtk_container_add(GTK_CONTAINER(pres_edit_button), edit_hbox);
    gtk_widget_show(edit_hbox);

    edit_icon = gtk_image_new_from_stock(GTK_STOCK_EDIT, GTK_ICON_SIZE_MENU);
    gtk_box_pack_start(GTK_BOX(edit_hbox), edit_icon, TRUE, TRUE, 0);
    gtk_widget_show(edit_icon);
    edit_label = gtk_label_new(_("Edit"));
    gtk_box_pack_end(GTK_BOX(edit_hbox), edit_label, TRUE, TRUE, 0);
    gtk_widget_show(edit_label);

    gimp_help_set_help_data(pres_edit_button,
                            _("Edit the currently selected preservation layer"),
                            NULL);

    pres_new_button = gtk_button_new();
    gtk_box_pack_end(GTK_BOX (hbox), pres_new_button, FALSE, FALSE, 0);
    gtk_widget_show(pres_new_button);

    new_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
    gtk_container_add(GTK_CONTAINER(pres_new_button), new_hbox);
    gtk_widget_show(new_hbox);

    new_icon = gtk_image_new_from_stock(GTK_STOCK_NEW, GTK_ICON_SIZE_MENU);
    gtk_box_pack_start(GTK_BOX(new_hbox), new_icon, TRUE, TRUE, 0);
    gtk_widget_show(new_icon);
    new_label = gtk_label_new(_("New"));
    gtk_box_pack_end(GTK_BOX(new_hbox), new_label, TRUE, TRUE, 0);
    gtk_widget_show(new_label);

    gimp_help_set_help_data(pres_new_button,
                            _("Creates a new transparent layer "
                              "ready to be used as a preservation mask"),
                            NULL);


    g_signal_connect (pres_new_button, "clicked",
                      G_CALLBACK
                              (callback_new_mask_button),
                      (gpointer) (new_pres_layer_data));

    g_signal_connect (pres_edit_button, "clicked",
                      G_CALLBACK
                              (callback_edit_mask_button),
                      (gpointer) (new_pres_layer_data));


    pres_frame_event_box2 = gtk_event_box_new();
    gtk_event_box_set_visible_window(GTK_EVENT_BOX (pres_frame_event_box2),
                                     FALSE);
    gtk_box_pack_start(GTK_BOX (pres_vbox), pres_frame_event_box2, FALSE,
                       FALSE, 0);
    gtk_widget_show(pres_frame_event_box2);


    if (!features_are_sensitive) {
        gtk_event_box_set_above_child(GTK_EVENT_BOX (pres_frame_event_box2),
                                      TRUE);
        gtk_widget_set_tooltip_text(pres_frame_event_box2, pres_inactive_tip_string);
    }

    pres_vbox2 = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
    gtk_container_add(GTK_CONTAINER (pres_frame_event_box2), pres_vbox2);
    gtk_widget_show(pres_vbox2);


    pres_combo_event_box = gtk_event_box_new();
    gtk_box_pack_start(GTK_BOX (pres_vbox2), pres_combo_event_box, FALSE,
                       FALSE, 0);
    gtk_widget_show(pres_combo_event_box);

    if (features_are_sensitive) {
        gimp_help_set_help_data(pres_combo_event_box,
                                _("Layer to be used as a mask for "
                                  "feature preservation.\n"
                                  "Use the \"Refresh\" button to update the list"),
                                NULL);
    }

    GtkWidget *pres_combo_grid = gtk_grid_new();
    gtk_container_set_border_width(GTK_CONTAINER (pres_combo_grid), 4);
    gtk_grid_set_column_spacing(GTK_GRID (pres_combo_grid), 4);
    gtk_grid_set_row_spacing(GTK_GRID (pres_combo_grid), 2);
    gtk_container_add(GTK_CONTAINER (pres_combo_event_box), pres_combo_grid);
    gtk_widget_show(pres_combo_grid);

    GimpLayer *layer = GIMP_LAYER(gimp_item_get_by_id(layer_ID));

    combo = gimp_layer_combo_box_new(dialog_layer_constraint_func,
                                     layer,
                                     NULL);

    g_object_set(combo, "ellipsize", PANGO_ELLIPSIZE_START, NULL);

    old_layer_ID = state->pres_layer_ID;

    gimp_int_combo_box_connect(GIMP_INT_COMBO_BOX(combo),
                               layer_ID,
                               G_CALLBACK (callback_pres_combo_get_active),
                               (gpointer) (&preview_data),
                               NULL);

    gimp_int_combo_box_set_active(GIMP_INT_COMBO_BOX(combo), old_layer_ID);

    pres_layer_label = gtk_label_new(_("Layer:"));
    gtk_widget_set_halign(pres_layer_label, GTK_ALIGN_START);
    gtk_widget_set_valign(pres_layer_label, GTK_ALIGN_CENTER);
    gtk_grid_attach(GTK_GRID(pres_combo_grid), pres_layer_label, 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(pres_combo_grid), combo, 1, 0, 1, 1);
    gtk_widget_show(pres_layer_label);

    gtk_widget_set_sensitive(pres_layer_label, ui_state->pres_status
                                    && features_are_sensitive);

    gtk_widget_set_sensitive(combo, ui_state->pres_status
                                    && features_are_sensitive);

    gtk_widget_set_sensitive(pres_edit_button, ui_state->pres_status
                                               && features_are_sensitive);

    pres_toggle_data.combo = combo;
    pres_toggle_data.combo_label = pres_layer_label;
    pres_toggle_data.edit_button = pres_edit_button;
    preview_data.pres_combo = combo;

    gtk_widget_show(combo);

    if (features_are_sensitive) {
        g_snprintf(pres_strength_tip_string_, MAX_STRING_SIZE,
                   _("Overall coefficient for "
                     "feature preservation intensity"));
        pres_strength_tip_string = pres_strength_tip_string_;
    } else {
        pres_strength_tip_string = NULL;
    }

    GtkWidget *pres_coeff_scale_entry;
    GtkWidget *pres_coeff_spin_button;
    GtkAdjustment *pres_coeff_adj;

    pres_coeff_scale_entry = gimp_scale_entry_new(_("Strength:"),
                                                  state->pres_coeff,
                                                  0,
                                                  MAX_COEFF,
                                                  0);
    pres_coeff_spin_button =
            gimp_label_spin_get_spin_button(GIMP_LABEL_SPIN(pres_coeff_scale_entry));
    pres_coeff_adj =
            gtk_spin_button_get_adjustment(GTK_SPIN_BUTTON(pres_coeff_spin_button));

    gtk_widget_set_tooltip_text(pres_coeff_scale_entry, pres_strength_tip_string);
    gtk_box_pack_start(GTK_BOX (pres_vbox2), pres_coeff_scale_entry, FALSE, FALSE, 0);
    gtk_widget_show(pres_coeff_scale_entry);

    g_signal_connect (pres_coeff_adj, "value_changed",
                      G_CALLBACK(gimp_int_adjustment_update),
                      (gpointer) &(state->pres_coeff));

    gtk_widget_set_sensitive(pres_coeff_scale_entry,
                             (ui_state->pres_status && features_are_sensitive));

    pres_toggle_data.status = &(ui_state->pres_status);

    g_signal_connect (G_OBJECT(pres_button), "toggled",
                      G_CALLBACK(callback_combo_set_sensitive),
                      (gpointer) (&pres_toggle_data));

    g_signal_connect (G_OBJECT(pres_button), "toggled",
                      G_CALLBACK(callback_pres_combo_set_sensitive_preview),
                      (gpointer) (&preview_data));

    pres_toggle_data.guess_label = NULL;
    pres_toggle_data.guess_button_hor = NULL;
    pres_toggle_data.guess_button_ver = NULL;


    /*  Feature discard  */

    frame = gimp_frame_new(_("Feature discard mask"));
    gtk_box_pack_start(GTK_BOX (thispage), frame, FALSE, FALSE, 0);
    gtk_widget_show(frame);

    g_snprintf(disc_inactive_tip_string, MAX_STRING_SIZE,
               _("Extra layers are needed to activate feature discard.\n"
                 "You can create one with the \"New\" button and paint on it, "
                 "then press the \"Refresh\" button.\n"
                 "Note that painting in black has no effect"));


    disc_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
    gtk_container_add(GTK_CONTAINER (frame), disc_vbox);
    gtk_widget_show(disc_vbox);

    hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
    gtk_box_pack_start(GTK_BOX (disc_vbox), hbox, FALSE, FALSE, 0);
    gtk_widget_show(hbox);

    disc_frame_event_box1 = gtk_event_box_new();
    gtk_event_box_set_visible_window(GTK_EVENT_BOX (disc_frame_event_box1),
                                     FALSE);
    gtk_box_pack_start(GTK_BOX (hbox), disc_frame_event_box1, FALSE, FALSE, 0);
    gtk_widget_show(disc_frame_event_box1);

    if (!features_are_sensitive) {
        gtk_event_box_set_above_child(GTK_EVENT_BOX (disc_frame_event_box1),
                                      TRUE);
        gtk_widget_set_tooltip_text(disc_frame_event_box1, disc_inactive_tip_string);
    }

    disc_button = gtk_check_button_new_with_label(_("Discard features"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON (disc_button),
                                 ui_state->disc_status);

    gtk_widget_set_sensitive(disc_button, features_are_sensitive);


    gtk_container_add(GTK_CONTAINER (disc_frame_event_box1), disc_button);
    gtk_widget_show(disc_button);

    g_signal_connect (disc_button, "toggled",
                      G_CALLBACK
                              (callback_status_button),
                      (gpointer) (&ui_state->disc_status));


    gimp_help_set_help_data(disc_button,
                            _("Use an extra layer to treat selected "
                              "areas as if they were meaningless "
                              "(useful to remove parts of the image "
                              "when shrinking)"), NULL);

    disc_warning_image = gtk_image_new_from_icon_name("dialog-warning", GTK_ICON_SIZE_MENU);
    gtk_box_pack_start(GTK_BOX (hbox), disc_warning_image, FALSE, FALSE, 0);
    gimp_help_set_help_data(disc_warning_image,
                            _
                                    ("Warning: the discard mask information will be ignored with the current settings.\n"
                                     "(If you know what you're doing you can override this behaviour by unchecking the "
                                     "corrensponding option in the \"Advanced\" tab)"),
                            NULL);

    preview_data.disc_warning_image = disc_warning_image;
    callback_set_disc_warning(NULL, (gpointer) &preview_data);

    disc_edit_button = gtk_button_new();
    gtk_box_pack_end(GTK_BOX (hbox), disc_edit_button, FALSE, FALSE, 0);
    gtk_widget_show(disc_edit_button);

    edit_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
    gtk_container_add(GTK_CONTAINER(disc_edit_button), edit_hbox);
    gtk_widget_show(edit_hbox);

    edit_icon = gtk_image_new_from_icon_name("document-edit", GTK_ICON_SIZE_MENU);
    // edit_icon = gtk_image_new_from_stock(GTK_STOCK_EDIT, GTK_ICON_SIZE_MENU);
    gtk_box_pack_start(GTK_BOX(edit_hbox), edit_icon, TRUE, TRUE, 0);
    gtk_widget_show(edit_icon);
    edit_label = gtk_label_new(_("Edit"));
    gtk_box_pack_end(GTK_BOX(edit_hbox), edit_label, TRUE, TRUE, 0);
    gtk_widget_show(edit_label);

    gimp_help_set_help_data(disc_edit_button,
                            _("Edit the currently selected discard layer"),
                            NULL);

    disc_new_button = gtk_button_new();
    gtk_box_pack_end(GTK_BOX (hbox), disc_new_button, FALSE, FALSE, 0);
    gtk_widget_show(disc_new_button);

    new_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
    gtk_container_add(GTK_CONTAINER(disc_new_button), new_hbox);
    gtk_widget_show(new_hbox);

    new_icon = gtk_image_new_from_stock(GTK_STOCK_NEW, GTK_ICON_SIZE_MENU);
    gtk_box_pack_start(GTK_BOX(new_hbox), new_icon, TRUE, TRUE, 0);
    gtk_widget_show(new_icon);
    new_label = gtk_label_new(_("New"));
    gtk_box_pack_end(GTK_BOX(new_hbox), new_label, TRUE, TRUE, 0);
    gtk_widget_show(new_label);

    gimp_help_set_help_data(disc_new_button,
                            _("Creates a new transparent layer "
                              "ready to be used as a discard mask"), NULL);


    g_signal_connect (disc_new_button, "clicked",
                      G_CALLBACK
                              (callback_new_mask_button),
                      (gpointer) (new_disc_layer_data));

    g_signal_connect (disc_edit_button, "clicked",
                      G_CALLBACK
                              (callback_edit_mask_button),
                      (gpointer) (new_disc_layer_data));


    disc_frame_event_box2 = gtk_event_box_new();
    gtk_event_box_set_visible_window(GTK_EVENT_BOX (disc_frame_event_box2),
                                     FALSE);
    gtk_box_pack_start(GTK_BOX (disc_vbox), disc_frame_event_box2, FALSE,
                       FALSE, 0);
    gtk_widget_show(disc_frame_event_box2);


    if (!features_are_sensitive) {
        gtk_event_box_set_above_child(GTK_EVENT_BOX (disc_frame_event_box2),
                                      TRUE);
        gtk_widget_set_tooltip_text(disc_frame_event_box2, disc_inactive_tip_string);
    }

    disc_vbox2 = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
    gtk_container_add(GTK_CONTAINER (disc_frame_event_box2), disc_vbox2);
    gtk_widget_show(disc_vbox2);


    disc_combo_event_box = gtk_event_box_new();
    gtk_box_pack_start(GTK_BOX (disc_vbox2), disc_combo_event_box, FALSE,
                       FALSE, 0);
    gtk_widget_show(disc_combo_event_box);

    if (features_are_sensitive) {
        gimp_help_set_help_data(disc_combo_event_box,
                                _("Layer to be used as a mask "
                                  "for feature discard.\n"
                                  "Use the \"Refresh\" button "
                                  "to update the list"), NULL);
    }

    GtkWidget *disc_combo_grid = gtk_grid_new();
    gtk_container_set_border_width(GTK_CONTAINER (disc_combo_grid), 4);
    gtk_grid_set_column_spacing(GTK_GRID (disc_combo_grid), 4);
    gtk_grid_set_row_spacing(GTK_GRID (disc_combo_grid), 2);
    gtk_container_add(GTK_CONTAINER (disc_combo_event_box), disc_combo_grid);
    gtk_widget_show(disc_combo_grid);

    combo =
            gimp_layer_combo_box_new(dialog_layer_constraint_func,
                                     layer,
                                     NULL);

    g_object_set(combo, "ellipsize", PANGO_ELLIPSIZE_START, NULL);

    old_layer_ID = state->disc_layer_ID;

    gimp_int_combo_box_connect(GIMP_INT_COMBO_BOX(combo),
                               layer_ID,
                               G_CALLBACK (callback_disc_combo_get_active),
                               (gpointer) (&preview_data),
                               NULL);

    gimp_int_combo_box_set_active(GIMP_INT_COMBO_BOX(combo), old_layer_ID);

    disc_layer_label = gtk_label_new(_("Layer:"));
    gtk_widget_set_halign(disc_layer_label, GTK_ALIGN_START);
    gtk_widget_set_valign(disc_layer_label, GTK_ALIGN_CENTER);
    gtk_grid_attach(GTK_GRID(disc_combo_grid), disc_layer_label, 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(disc_combo_grid), combo, 1, 0, 1, 1);
    gtk_widget_show(disc_layer_label);

    gtk_widget_set_sensitive(combo, ui_state->disc_status
                                    && features_are_sensitive);

    gtk_widget_set_sensitive(disc_layer_label, ui_state->disc_status
                                    && features_are_sensitive);

    gtk_widget_set_sensitive(disc_edit_button, ui_state->disc_status
                                               && features_are_sensitive);

    disc_toggle_data.combo = combo;
    disc_toggle_data.combo_label = disc_layer_label;
    disc_toggle_data.edit_button = disc_edit_button;
    preview_data.disc_combo = combo;

    gtk_widget_show(combo);

    if (features_are_sensitive) {
        g_snprintf(disc_strength_tip_string_, MAX_STRING_SIZE,
                   _("Overall coefficient for "
                     "feature discard intensity"));
        disc_strength_tip_string = disc_strength_tip_string_;
    } else {
        disc_strength_tip_string = NULL;
    }

    GtkWidget *disc_coeff_scale_entry;
    GtkAdjustment *disc_coeff_adj;

// Create the scale entry widget
    disc_coeff_scale_entry = gimp_scale_entry_new(_("Strength:"),    // label text
                                                  state->disc_coeff, // initial value
                                                  0,                 // lower bound
                                                  MAX_COEFF,         // upper bound
                                                  0);                // digits

    GtkWidget *disc_coeff_spin_button =
            gimp_label_spin_get_spin_button(GIMP_LABEL_SPIN(disc_coeff_scale_entry));
    disc_coeff_adj = gtk_spin_button_get_adjustment(GTK_SPIN_BUTTON(disc_coeff_spin_button));

// Set tooltip if needed
    gtk_widget_set_tooltip_text(disc_coeff_scale_entry, disc_strength_tip_string);
    gtk_box_pack_start(GTK_BOX (disc_vbox2), disc_coeff_scale_entry, FALSE, FALSE, 0);
    gtk_widget_show(disc_coeff_scale_entry);

    g_signal_connect (disc_coeff_adj, "value_changed",
                      G_CALLBACK(gimp_int_adjustment_update),
                      (gpointer) &(state->disc_coeff));
    g_signal_connect (disc_coeff_adj, "value_changed",
                      G_CALLBACK(callback_set_disc_warning),
                      (gpointer) &preview_data);


    gtk_widget_set_sensitive(disc_coeff_scale_entry,
                             (ui_state->disc_status
                              && features_are_sensitive));

    disc_toggle_data.status = &(ui_state->disc_status);

    g_signal_connect (G_OBJECT(disc_button), "toggled",
                      G_CALLBACK(callback_combo_set_sensitive),
                      (gpointer) (&disc_toggle_data));

    g_signal_connect (G_OBJECT(disc_button), "toggled",
                      G_CALLBACK(callback_disc_combo_set_sensitive_preview),
                      (gpointer) (&preview_data));

    g_signal_connect (pres_button, "toggled",
                      G_CALLBACK
                              (callback_resize_aux_layers_button_set_sensitive),
                      (gpointer) (&presdisc_status));
    g_signal_connect (disc_button, "toggled",
                      G_CALLBACK
                              (callback_resize_aux_layers_button_set_sensitive),
                      (gpointer) (&presdisc_status));

    g_signal_connect (disc_button, "toggled",
                      G_CALLBACK
                              (callback_set_disc_warning), (gpointer) (&preview_data));


    hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
    gtk_box_pack_start(GTK_BOX (disc_vbox2), hbox, FALSE, FALSE, 0);
    gtk_widget_show(hbox);

    // Auto-size buttons
    guess_label = gtk_label_new(_("Auto size:"));
    gtk_box_pack_start(GTK_BOX (hbox), guess_label, FALSE, FALSE, 0);
    gtk_widget_show(guess_label);

    disc_toggle_data.guess_label = guess_label;

    gtk_widget_set_sensitive(guess_label,
                             (ui_state->disc_status
                              && features_are_sensitive));

    // Width auto-size button
    guess_button_hor = gtk_button_new_with_label(_("Width"));
    gtk_box_pack_start(GTK_BOX (hbox), guess_button_hor, FALSE, FALSE, 0);
    gtk_widget_show(guess_button_hor);

    disc_toggle_data.guess_button_hor = guess_button_hor;

    gtk_widget_set_sensitive(guess_button_hor,
                             (ui_state->disc_status
                              && features_are_sensitive));

    if (features_are_sensitive) {
        gimp_help_set_help_data(guess_button_hor,
                                _
                                        ("Try to set the final width as needed to remove the masked areas.\n"
                                         "Only use with simple masks"), NULL);
    }

    g_signal_connect (guess_button_hor, "clicked",
                      G_CALLBACK(callback_guess_button_hor),
                      (gpointer) &preview_data);


    // Height auto-size button
    guess_button_ver = gtk_button_new_with_label(_("Height"));
    gtk_box_pack_start(GTK_BOX (hbox), guess_button_ver, FALSE, FALSE, 0);
    gtk_widget_show(guess_button_ver);

    disc_toggle_data.guess_button_ver = guess_button_ver;

    gtk_widget_set_sensitive(guess_button_ver,
                             (ui_state->disc_status
                              && features_are_sensitive));

    if (features_are_sensitive) {
        gimp_help_set_help_data(guess_button_ver,
                                _
                                        ("Try to set the final height as needed to remove the masked areas.\n"
                                         "Only use with simple masks"), NULL);
    }

    g_signal_connect (guess_button_ver, "clicked",
                      G_CALLBACK(callback_guess_button_ver),
                      (gpointer) &preview_data);

    return thispage;
}
