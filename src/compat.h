#ifndef GIMP_LQR_PLUGIN_COMPAT_H
#define GIMP_LQR_PLUGIN_COMPAT_H

#include <gtk/gtk.h>

typedef struct {
    GtkWidget *label;
    GtkWidget *scale;
    GtkWidget *spinbutton;
    GtkAdjustment *adjustment;
} ScaleEntry;

// Create the scale entry
ScaleEntry *create_scale_entry(GtkGrid *grid,
                               gint row,
                               const gchar *label_text,
                               gdouble value,
                               gdouble min,
                               gdouble max
) {
    ScaleEntry *entry = g_new(ScaleEntry, 1);

    entry->adjustment = gtk_adjustment_new(value, min, max, 1, 10, 0);
    entry->label = gtk_label_new(label_text);
    entry->scale = gtk_scale_new(GTK_ORIENTATION_HORIZONTAL, entry->adjustment);
    entry->spinbutton = gtk_spin_button_new(entry->adjustment, 1, 0);

    gtk_widget_set_halign(entry->label, GTK_ALIGN_START);
    gtk_widget_set_hexpand(entry->scale, TRUE);

    gtk_grid_attach(grid, entry->label, 0, row, 1, 1);
    gtk_grid_attach(grid, entry->scale, 1, row, 1, 1);
    gtk_grid_attach(grid, entry->spinbutton, 2, row, 1, 1);

    return entry;
}


#endif //GIMP_LQR_PLUGIN_COMPAT_H
