//
// Created by tomhodder on 30/05/25.
//

#ifndef GIMP_LQR_PLUGIN_CALLBACKS_H
#define GIMP_LQR_PLUGIN_CALLBACKS_H

#include <gtk/gtk.h>
#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>
#include "notebook.h"
#include "main.h"
#include "preview.h"
#include "interface.h"
#include "altsizeentry.h"

/* Private functions */
GtkWidget *features_page_new(gint32 image_ID, gint32 layer_ID);
GtkWidget *advanced_page_new(gint32 image_ID, gint32 layer_ID);

/* Callback functions */
void callback_dialog_response(GtkWidget *dialog, gint response_id, gpointer data);
void callback_lastvalues_button(GtkWidget *button, gpointer data);
void callback_resetvalues_button(GtkWidget *button, gpointer data);
void callback_interactive_button(GtkWidget *button, gpointer data);
void callback_set_disc_warning(GtkWidget *dummy, gpointer data);
void callback_size_changed(GtkWidget *size_entry, gpointer data);
void callback_res_order_changed(GtkWidget *res_order, gpointer data);
void callback_output_target_changed(GtkWidget *res_order, gpointer data);
void callback_scaleback_mode_changed(GtkWidget *res_order, gpointer data);
void callback_expander_changed(GtkWidget *expander, gpointer data);
void callback_out_seams_button(GtkWidget *button, gpointer data);
void callback_out_seams_col_button1(GtkWidget *button, gpointer data);
void callback_out_seams_col_button2(GtkWidget *button, gpointer data);
void callback_resize_aux_layers_button_set_sensitive(GtkWidget *button, gpointer data);
void callback_scaleback_button(GtkWidget *button, gpointer data);

/* Refresh functions */
void refresh_features_page(NotebookData *data);
void refresh_advanced_page(NotebookData *data);


#endif //GIMP_LQR_PLUGIN_CALLBACKS_H
