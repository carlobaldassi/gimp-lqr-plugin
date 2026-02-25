
#include "config.h"

#include <string.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

//#include "libgimpbase/gimpbase.h"

//#include "gimpwidgets.h"

#include "altsizeentry.h"


#define SIZE_MAX_VALUE 500000.0

#define ALT_SIZE_ENTRY_DIGITS(unit) (MIN (gimp_unit_get_digits (unit), 5) + 1)

enum {
    VALUE_CHANGED,
    REFVAL_CHANGED,
    UNIT_CHANGED,
    LAST_SIGNAL
};


struct _AltSizeEntryField {
    AltSizeEntry *gse;

    gdouble resolution;
    gdouble lower;
    gdouble upper;

    GtkAdjustment *value_adjustment;
    GtkWidget *value_spinbutton;
    gdouble value;
    gdouble min_value;
    gdouble max_value;

    GtkAdjustment *refval_adjustment;
    GtkWidget *refval_spinbutton;
    gdouble refval;
    gdouble min_refval;
    gdouble max_refval;
    gint refval_digits;

    gint stop_recursion;
};


static void alt_size_entry_finalize(GObject *object);

static void alt_size_entry_update_value(AltSizeEntryField *gsef,
                                        gdouble value);

static void alt_size_entry_value_callback(GtkWidget *widget,
                                          gpointer data);

static void alt_size_entry_update_refval(AltSizeEntryField *gsef,
                                         gdouble refval);

static void alt_size_entry_refval_callback(GtkWidget *widget,
                                           gpointer data);

static void alt_size_entry_update_unit(AltSizeEntry *gse,
                                       GimpUnit *unit);

static void alt_size_entry_unit_callback(GtkWidget *widget,
                                         AltSizeEntry *sizeentry);


G_DEFINE_TYPE (AltSizeEntry, alt_size_entry, GTK_TYPE_GRID)

#define parent_class alt_size_entry_parent_class

static guint alt_size_entry_signals[LAST_SIGNAL] = {0};


static void
alt_size_entry_class_init(AltSizeEntryClass *klass) {
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    alt_size_entry_signals[VALUE_CHANGED] =
            g_signal_new("value-changed",
                         G_TYPE_FROM_CLASS (klass),
                         G_SIGNAL_RUN_FIRST,
                         G_STRUCT_OFFSET (AltSizeEntryClass, value_changed),
                         NULL, NULL,
                         g_cclosure_marshal_VOID__VOID,
                         G_TYPE_NONE, 0);

    alt_size_entry_signals[REFVAL_CHANGED] =
            g_signal_new("refval-changed",
                         G_TYPE_FROM_CLASS (klass),
                         G_SIGNAL_RUN_FIRST,
                         G_STRUCT_OFFSET (AltSizeEntryClass, refval_changed),
                         NULL, NULL,
                         g_cclosure_marshal_VOID__VOID,
                         G_TYPE_NONE, 0);

    alt_size_entry_signals[UNIT_CHANGED] =
            g_signal_new("unit-changed",
                         G_TYPE_FROM_CLASS (klass),
                         G_SIGNAL_RUN_FIRST,
                         G_STRUCT_OFFSET (AltSizeEntryClass, unit_changed),
                         NULL, NULL,
                         g_cclosure_marshal_VOID__VOID,
                         G_TYPE_NONE, 0);

    object_class->finalize = alt_size_entry_finalize;

    klass->value_changed = NULL;
    klass->refval_changed = NULL;
    klass->unit_changed = NULL;
}

static void
alt_size_entry_init(AltSizeEntry *gse) {
    gse->fields = NULL;
    gse->number_of_fields = 0;
    gse->unitmenu = NULL;
    gse->unit = gimp_unit_percent();
    gse->menu_show_pixels = TRUE;
    gse->menu_show_percent = TRUE;
    gse->show_refval = FALSE;
    gse->update_policy = ALT_SIZE_ENTRY_UPDATE_NONE;
}

static void
alt_size_entry_finalize(GObject *object) {
    AltSizeEntry *gse = ALT_SIZE_ENTRY (object);

    if (gse->fields) {
        GSList *list;

        for (list = gse->fields; list; list = list->next)
            g_slice_free (AltSizeEntryField, list->data);

        g_slist_free(gse->fields);
        gse->fields = NULL;
    }

    G_OBJECT_CLASS (parent_class)->finalize(object);
}

/**
 * alt_size_entry_new:
 * @number_of_fields:  The number of input fields.
 * @unit:              The initial unit.
 * @unit_format:       A printf-like unit-format string as is used with
 *                     gimp_unit_menu_new().
 * @menu_show_pixels:  %TRUE if the unit menu shold contain an item for
 *                     GIMP_UNIT_PIXEL (ignored if the @update_policy is not
 *                     ALT_SIZE_ENTRY_UPDATE_NONE).
 * @menu_show_percent: %TRUE if the unit menu shold contain an item for
 *                     GIMP_UNIT_PERCENT.
 * @show_refval:       %TRUE if you want an extra "reference value"
 *                     spinbutton per input field.
 * @spinbutton_width:  The minimal horizontal size of the #GtkSpinButton's.
 * @update_policy:     How the automatic pixel <-> real-world-unit
 *                     calculations should be done.
 *
 * Creates a new #AltSizeEntry widget.
 *
 * To have all automatic calculations performed correctly, set up the
 * widget in the following order:
 *
 * 1. alt_size_entry_new()
 *
 * 2. (for each additional input field) alt_size_entry_add_field()
 *
 * 3. alt_size_entry_set_unit()
 *
 * For each input field:
 *
 * 4. alt_size_entry_set_resolution()
 *
 * 5. alt_size_entry_set_refval_boundaries()
 *    (or alt_size_entry_set_value_boundaries())
 *
 * 6. alt_size_entry_set_size()
 *
 * 7. alt_size_entry_set_refval() (or alt_size_entry_set_value())
 *
 * The #AltSizeEntry is derived from #GtkGrid and will have
 * an empty border of one cell width on each side plus an empty column left
 * of the #GimpUnitMenu to allow the caller to add labels or a
 * #GimpChainButton.
 *
 * Returns: A Pointer to the new #AltSizeEntry widget.
 **/
GtkWidget *
alt_size_entry_new(gint number_of_fields,
                   GimpUnit *unit,
                   const gchar *unit_format,
                   gboolean menu_show_pixels,
                   gboolean menu_show_percent,
                   gboolean show_refval,
                   gint spinbutton_width,
                   AltSizeEntryUpdatePolicy update_policy) {
    AltSizeEntry *gse;
    gint i;

    g_return_val_if_fail ((number_of_fields >= 0) && (number_of_fields <= 16),
                          NULL);

    gse = g_object_new(ALT_TYPE_SIZE_ENTRY, NULL);

    gse->number_of_fields = number_of_fields;
    gse->unit = unit;
    gse->show_refval = show_refval;
    gse->update_policy = update_policy;

    /** CUSTOMIZATION BEGIN **/
    /* GtkGrid doesn't need explicit resizing */
    /** CUSTOMIZATION END **/

    /*  show the 'pixels' menu entry only if we are a 'size' sizeentry and
     *  don't have the reference value spinbutton
     */
    if ((update_policy == ALT_SIZE_ENTRY_UPDATE_RESOLUTION) ||
        (show_refval == TRUE))
        gse->menu_show_pixels = FALSE;
    else
        gse->menu_show_pixels = menu_show_pixels;

    /*  show the 'percent' menu entry only if we are a 'size' sizeentry
     */
    if (update_policy == ALT_SIZE_ENTRY_UPDATE_RESOLUTION)
        gse->menu_show_percent = FALSE;
    else
        gse->menu_show_percent = menu_show_percent;

    for (i = 0; i < number_of_fields; i++) {
        AltSizeEntryField *gsef = g_slice_new0 (AltSizeEntryField);
        gint digits;

        gse->fields = g_slist_append(gse->fields, gsef);

        gsef->gse = gse;
        gsef->resolution = 1.0; /*  just to avoid division by zero  */
        gsef->lower = 0.0;
        gsef->upper = 100.0;
        gsef->value = 0;
        gsef->min_value = 0;
        gsef->max_value = SIZE_MAX_VALUE;
        gsef->refval_adjustment = NULL;
        gsef->value_adjustment = NULL;
        gsef->refval = 0;
        gsef->min_refval = 0;
        gsef->max_refval = SIZE_MAX_VALUE;
        gsef->refval_digits =
                (update_policy == ALT_SIZE_ENTRY_UPDATE_SIZE) ? 0 : 3;
        gsef->stop_recursion = 0;

        digits = ((unit == GIMP_UNIT_PIXEL) ?
                  gsef->refval_digits : ((unit == gimp_unit_percent()) ?
                                         2 : ALT_SIZE_ENTRY_DIGITS (unit)));

        gsef->value_spinbutton = gimp_spin_button_new(gsef->value_adjustment,
                                                      1.0, digits);
        gsef->value_adjustment =
                gtk_spin_button_get_adjustment(GTK_SPIN_BUTTON(gsef->value_spinbutton));

        if (spinbutton_width > 0) {
            if (spinbutton_width < 17)
                gtk_entry_set_width_chars(GTK_ENTRY (gsef->value_spinbutton),
                                          spinbutton_width);
            else
                gtk_widget_set_size_request(gsef->value_spinbutton,
                                            spinbutton_width, -1);
        }

        gtk_grid_attach(GTK_GRID (gse), gsef->value_spinbutton,
                        i + 1, gse->show_refval + 1, 1, 1);
        g_signal_connect (gsef->value_adjustment, "value-changed",
                          G_CALLBACK(alt_size_entry_value_callback),
                          gsef);

        gtk_widget_show(gsef->value_spinbutton);

        if (gse->show_refval) {
            gsef->refval_spinbutton =
                    gimp_spin_button_new(gsef->refval_adjustment,
                                         1.0, gsef->refval_digits);
            gsef->refval_adjustment =
                    gtk_spin_button_get_adjustment(GTK_SPIN_BUTTON(gsef->refval_spinbutton));

            gtk_widget_set_size_request(gsef->refval_spinbutton,
                                        spinbutton_width, -1);
            gtk_grid_attach(GTK_GRID (gse), gsef->refval_spinbutton,
                            i + 1, 1, 1, 1);
            g_signal_connect (gsef->refval_adjustment,
                              "value-changed",
                              G_CALLBACK(alt_size_entry_refval_callback),
                              gsef);

            gtk_widget_show(gsef->refval_spinbutton);
        }

        if (gse->menu_show_pixels && (unit == GIMP_UNIT_PIXEL) &&
            !gse->show_refval)
            gtk_spin_button_set_digits(GTK_SPIN_BUTTON (gsef->value_spinbutton),
                                       gsef->refval_digits);
    }

    GimpUnitStore *store;
    store = gimp_unit_store_new(0); // 0 for default pixel digits

// Configure what units to show
    gimp_unit_store_set_has_pixels(store, gse->menu_show_pixels);
    gimp_unit_store_set_has_percent(store, gse->menu_show_percent);

// Create the combo box with the configured store
    gse->unitmenu = gimp_unit_combo_box_new_with_model(store);
    g_object_unref(store);

// Set the initial unit
    gimp_unit_combo_box_set_active(GIMP_UNIT_COMBO_BOX(gse->unitmenu), unit);

    /** CUSTOMIZATION BEGIN **/
    /*
    gtk_table_attach (GTK_TABLE (gse), gse->unitmenu,
                      i+2, i+3,
                      gse->show_refval+1, gse->show_refval+2,
                      GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
                      */
    gtk_grid_attach(GTK_GRID (gse), gse->unitmenu,
                    0, gse->show_refval + 2, 2, 1);
    /** CUSTOMIZATION END **/

    g_signal_connect (gse->unitmenu, "changed",
                      G_CALLBACK(alt_size_entry_unit_callback),
                      gse);
    gtk_widget_show(gse->unitmenu);

    return GTK_WIDGET (gse);
}


/**
 * alt_size_entry_add_field:
 * @gse:               The sizeentry you want to add a field to.
 * @value_spinbutton:  The spinbutton to display the field's value.
 * @refval_spinbutton: The spinbutton to display the field's reference value.
 *
 * Adds an input field to the #AltSizeEntry.
 *
 * The new input field will have the index 0. If you specified @show_refval
 * as %TRUE in alt_size_entry_new() you have to pass an additional
 * #GtkSpinButton to hold the reference value. If @show_refval was %FALSE,
 * @refval_spinbutton will be ignored.
 **/
void
alt_size_entry_add_field(AltSizeEntry *gse,
                         GtkSpinButton *value_spinbutton,
                         GtkSpinButton *refval_spinbutton) {
    AltSizeEntryField *gsef;
    gint digits;

    g_return_if_fail (ALT_IS_SIZE_ENTRY(gse));
    g_return_if_fail (GTK_IS_SPIN_BUTTON(value_spinbutton));

    if (gse->show_refval) {
        g_return_if_fail (GTK_IS_SPIN_BUTTON(refval_spinbutton));
    }

    gsef = g_slice_new0 (AltSizeEntryField);

    gse->fields = g_slist_prepend(gse->fields, gsef);
    gse->number_of_fields++;

    gsef->gse = gse;
    gsef->resolution = 1.0; /*  just to avoid division by zero  */
    gsef->lower = 0.0;
    gsef->upper = 100.0;
    gsef->value = 0;
    gsef->min_value = 0;
    gsef->max_value = SIZE_MAX_VALUE;
    gsef->refval = 0;
    gsef->min_refval = 0;
    gsef->max_refval = SIZE_MAX_VALUE;
    gsef->refval_digits =
            (gse->update_policy == ALT_SIZE_ENTRY_UPDATE_SIZE) ? 0 : 3;
    gsef->stop_recursion = 0;

    gsef->value_adjustment =
            gtk_spin_button_get_adjustment(value_spinbutton);
    gsef->value_spinbutton = GTK_WIDGET (value_spinbutton);
    g_signal_connect (gsef->value_adjustment, "value-changed",
                      G_CALLBACK(alt_size_entry_value_callback),
                      gsef);

    if (gse->show_refval) {
        gsef->refval_adjustment =
                gtk_spin_button_get_adjustment(refval_spinbutton);
        gsef->refval_spinbutton = GTK_WIDGET (refval_spinbutton);
        g_signal_connect (gsef->refval_adjustment, "value-changed",
                          G_CALLBACK(alt_size_entry_refval_callback),
                          gsef);
    }

    digits = ((gse->unit == gimp_unit_pixel()) ? gsef->refval_digits :
              (gse->unit == gimp_unit_percent()) ? 2 :
              ALT_SIZE_ENTRY_DIGITS (gse->unit));

    gtk_spin_button_set_digits(GTK_SPIN_BUTTON (value_spinbutton), digits);

    if (gse->menu_show_pixels &&
        !gse->show_refval &&
        (gse->unit == GIMP_UNIT_PIXEL)) {
        gtk_spin_button_set_digits(GTK_SPIN_BUTTON (gsef->value_spinbutton),
                                   gsef->refval_digits);
    }
}

/**
 * alt_size_entry_attach_label:
 * @gse:       The sizeentry you want to add a label to.
 * @text:      The text of the label.
 * @row:       The row where the label will be attached.
 * @column:    The column where the label will be attached.
 * @alignment: The horizontal alignment of the label.
 *
 * Attaches a #GtkLabel to the #AltSizeEntry (which is a #GtkTable).
 *
 * Returns: A pointer to the new #GtkLabel widget.
 **/
GtkWidget *
alt_size_entry_attach_label(AltSizeEntry *gse,
                            const gchar *text,
                            gint row,
                            gint column,
                            gfloat alignment) {
    GtkWidget *label;

    g_return_val_if_fail (ALT_IS_SIZE_ENTRY(gse), NULL);
    g_return_val_if_fail (text != NULL, NULL);

    label = gtk_label_new_with_mnemonic(text);

    if (column == 0) {
        GList *children;
        GList *list;

        children = gtk_container_get_children(GTK_CONTAINER (gse));

        for (list = children; list; list = g_list_next (list)) {
            GtkWidget *child = list->data;
            gint left_attach;
            gint top_attach;

            gtk_container_child_get(GTK_CONTAINER (gse), child,
                                    "left-attach", &left_attach,
                                    "top-attach", &top_attach,
                                    NULL);

            if (left_attach == 1 && top_attach == row) {
                gtk_label_set_mnemonic_widget(GTK_LABEL (label), child);
                break;
            }
        }

        g_list_free(children);
    }

    gtk_widget_set_halign(label,
                          alignment < 0.5 ? GTK_ALIGN_START : (alignment > 0.5 ? GTK_ALIGN_END : GTK_ALIGN_CENTER));
    gtk_widget_set_valign(label, GTK_ALIGN_CENTER);

    gtk_grid_attach(GTK_GRID (gse), label, column, row, 1, 1);
    gtk_widget_show(label);

    return label;
}


/**
 * alt_size_entry_set_resolution:
 * @gse:        The sizeentry you want to set a resolution for.
 * @field:      The index of the field you want to set the resolution for.
 * @resolution: The new resolution (in dpi) for the chosen @field.
 * @keep_size:  %TRUE if the @field's size in pixels should stay the same.
 *              %FALSE if the @field's size in units should stay the same.
 *
 * Sets the resolution (in dpi) for field # @field of the #AltSizeEntry.
 *
 * The @resolution passed will be clamped to fit in
 * [#GIMP_MIN_RESOLUTION..#GIMP_MAX_RESOLUTION].
 *
 * This function does nothing if the #AltSizeEntryUpdatePolicy specified in
 * alt_size_entry_new() doesn't equal to #ALT_SIZE_ENTRY_UPDATE_SIZE.
 **/
void
alt_size_entry_set_resolution(AltSizeEntry *gse,
                              gint field,
                              gdouble resolution,
                              gboolean keep_size) {
    AltSizeEntryField *gsef;
    gfloat val;

    g_return_if_fail (ALT_IS_SIZE_ENTRY(gse));
    g_return_if_fail ((field >= 0) && (field < gse->number_of_fields));

    resolution = CLAMP (resolution, GIMP_MIN_RESOLUTION, GIMP_MAX_RESOLUTION);

    gsef = (AltSizeEntryField *) g_slist_nth_data(gse->fields, field);
    gsef->resolution = resolution;

    val = gsef->value;

    gsef->stop_recursion = 0;
    alt_size_entry_set_refval_boundaries(gse, field,
                                         gsef->min_refval, gsef->max_refval);

    if (!keep_size)
        alt_size_entry_set_value(gse, field, val);
}


/**
 * alt_size_entry_set_size:
 * @gse:   The sizeentry you want to set a size for.
 * @field: The index of the field you want to set the size for.
 * @lower: The reference value which will be treated as 0%.
 * @upper: The reference value which will be treated as 100%.
 *
 * Sets the pixel values for field # @field of the #AltSizeEntry
 * which will be treated as 0% and 100%.
 *
 * These values will be used if you specified @menu_show_percent as %TRUE
 * in alt_size_entry_new() and the user has selected GIMP_UNIT_PERCENT in
 * the #AltSizeEntry's #GimpUnitMenu.
 *
 * This function does nothing if the #AltSizeEntryUpdatePolicy specified in
 * alt_size_entry_new() doesn't equal to ALT_SIZE_ENTRY_UPDATE_SIZE.
 **/
void
alt_size_entry_set_size(AltSizeEntry *gse,
                        gint field,
                        gdouble lower,
                        gdouble upper) {
    AltSizeEntryField *gsef;

    g_return_if_fail (ALT_IS_SIZE_ENTRY(gse));
    g_return_if_fail ((field >= 0) && (field < gse->number_of_fields));
    g_return_if_fail (lower <= upper);

    gsef = (AltSizeEntryField *) g_slist_nth_data(gse->fields, field);
    gsef->lower = lower;
    gsef->upper = upper;

    alt_size_entry_set_refval(gse, field, gsef->refval);
}


/**
 * alt_size_entry_set_value_boundaries:
 * @gse:   The sizeentry you want to set value boundaries for.
 * @field: The index of the field you want to set value boundaries for.
 * @lower: The new lower boundary of the value of the chosen @field.
 * @upper: The new upper boundary of the value of the chosen @field.
 *
 * Limits the range of possible values which can be entered in field # @field
 * of the #AltSizeEntry.
 *
 * The current value of the @field will be clamped to fit in the @field's
 * new boundaries.
 *
 * NOTE: In most cases you won't be interested in this function because the
 *       #AltSizeEntry's purpose is to shield the programmer from unit
 *       calculations. Use alt_size_entry_set_refval_boundaries() instead.
 *       Whatever you do, don't mix these calls. A size entry should either
 *       be clamped by the value or the reference value.
 **/
void
alt_size_entry_set_value_boundaries(AltSizeEntry *gse,
                                    gint field,
                                    gdouble lower,
                                    gdouble upper) {
    AltSizeEntryField *gsef;

    g_return_if_fail (ALT_IS_SIZE_ENTRY(gse));
    g_return_if_fail ((field >= 0) && (field < gse->number_of_fields));
    g_return_if_fail (lower <= upper);

    gsef = (AltSizeEntryField *) g_slist_nth_data(gse->fields, field);
    gsef->min_value = lower;
    gsef->max_value = upper;

    gtk_adjustment_set_lower(GTK_ADJUSTMENT (gsef->value_adjustment), gsef->min_value);
    gtk_adjustment_set_upper(GTK_ADJUSTMENT (gsef->value_adjustment), gsef->max_value);

    if (gsef->stop_recursion) /* this is a hack (but useful ;-) */
        return;

    gsef->stop_recursion++;
    switch (gsef->gse->update_policy) {
        case ALT_SIZE_ENTRY_UPDATE_NONE:
            break;

        case ALT_SIZE_ENTRY_UPDATE_SIZE:
            switch (gimp_unit_get_id(gse->unit)) {
                case GIMP_UNIT_PIXEL:
                    alt_size_entry_set_refval_boundaries(gse, field,
                                                         gsef->min_value,
                                                         gsef->max_value);
                    break;
                case GIMP_UNIT_PERCENT:
                    alt_size_entry_set_refval_boundaries(gse, field,
                                                         gsef->lower +
                                                         (gsef->upper - gsef->lower) *
                                                         gsef->min_value / 100,
                                                         gsef->lower +
                                                         (gsef->upper - gsef->lower) *
                                                         gsef->max_value / 100);
                    break;
                default:
                    alt_size_entry_set_refval_boundaries(gse, field,
                                                         gsef->min_value *
                                                         gsef->resolution /
                                                         gimp_unit_get_factor(gse->unit),
                                                         gsef->max_value *
                                                         gsef->resolution /
                                                         gimp_unit_get_factor(gse->unit));
                    break;
            }
            break;

        case ALT_SIZE_ENTRY_UPDATE_RESOLUTION:
            alt_size_entry_set_refval_boundaries(gse, field,
                                                 gsef->min_value *
                                                 gimp_unit_get_factor(gse->unit),
                                                 gsef->max_value *
                                                 gimp_unit_get_factor(gse->unit));
            break;

        default:
            break;
    }
    gsef->stop_recursion--;

    alt_size_entry_set_value(gse, field, gsef->value);
}

/**
 * alt_size_entry_get_value;
 * @gse:   The sizeentry you want to know a value of.
 * @field: The index of the field you want to know the value of.
 *
 * Returns the value of field # @field of the #AltSizeEntry.
 *
 * The @value returned is a distance or resolution
 * in the #GimpUnit the user has selected in the #AltSizeEntry's
 * #GimpUnitMenu.
 *
 * NOTE: In most cases you won't be interested in this value because the
 *       #AltSizeEntry's purpose is to shield the programmer from unit
 *       calculations. Use alt_size_entry_get_refval() instead.
 *
 * Returns: The value of the chosen @field.
 **/
gdouble
alt_size_entry_get_value(AltSizeEntry *gse,
                         gint field) {
    AltSizeEntryField *gsef;

    g_return_val_if_fail (ALT_IS_SIZE_ENTRY(gse), 0);
    g_return_val_if_fail ((field >= 0) && (field < gse->number_of_fields), 0);

    gsef = (AltSizeEntryField *) g_slist_nth_data(gse->fields, field);
    return gsef->value;
}

static void
alt_size_entry_update_value(AltSizeEntryField *gsef,
                            gdouble value) {
    if (gsef->stop_recursion > 1)
        return;

    gsef->value = value;

    switch (gsef->gse->update_policy) {
        case ALT_SIZE_ENTRY_UPDATE_NONE:
            break;

        case ALT_SIZE_ENTRY_UPDATE_SIZE:
            switch (gimp_unit_get_id(gsef->gse->unit)) {
                case GIMP_UNIT_PIXEL:
                    gsef->refval = value;
                    break;
                case GIMP_UNIT_PERCENT:
                    gsef->refval =
                            CLAMP (gsef->lower + (gsef->upper - gsef->lower) * value / 100,
                                   gsef->min_refval, gsef->max_refval);
                    break;
                default:
                    gsef->refval =
                            CLAMP (value * gsef->resolution /
                                   gimp_unit_get_factor(gsef->gse->unit),
                                   gsef->min_refval, gsef->max_refval);
                    break;
            }
            if (gsef->gse->show_refval)
                gtk_adjustment_set_value(GTK_ADJUSTMENT (gsef->refval_adjustment),
                                         gsef->refval);
            break;

        case ALT_SIZE_ENTRY_UPDATE_RESOLUTION:
            gsef->refval =
                    CLAMP (value * gimp_unit_get_factor(gsef->gse->unit),
                           gsef->min_refval, gsef->max_refval);
            if (gsef->gse->show_refval)
                gtk_adjustment_set_value(GTK_ADJUSTMENT (gsef->refval_adjustment),
                                         gsef->refval);
            break;

        default:
            break;
    }

    g_signal_emit(gsef->gse, alt_size_entry_signals[VALUE_CHANGED], 0);
}

/**
 * alt_size_entry_set_value;
 * @gse:   The sizeentry you want to set a value for.
 * @field: The index of the field you want to set a value for.
 * @value: The new value for @field.
 *
 * Sets the value for field # @field of the #AltSizeEntry.
 *
 * The @value passed is treated to be a distance or resolution
 * in the #GimpUnit the user has selected in the #AltSizeEntry's
 * #GimpUnitMenu.
 *
 * NOTE: In most cases you won't be interested in this value because the
 *       #AltSizeEntry's purpose is to shield the programmer from unit
 *       calculations. Use alt_size_entry_set_refval() instead.
 **/
void
alt_size_entry_set_value(AltSizeEntry *gse,
                         gint field,
                         gdouble value) {
    AltSizeEntryField *gsef;

    g_return_if_fail (ALT_IS_SIZE_ENTRY(gse));
    g_return_if_fail ((field >= 0) && (field < gse->number_of_fields));

    gsef = (AltSizeEntryField *) g_slist_nth_data(gse->fields, field);

    value = CLAMP (value, gsef->min_value, gsef->max_value);

    gtk_adjustment_set_value(GTK_ADJUSTMENT (gsef->value_adjustment), value);
    alt_size_entry_update_value(gsef, value);
}


static void
alt_size_entry_value_callback(GtkWidget *widget,
                              gpointer data) {
    AltSizeEntryField *gsef;
    gdouble new_value;

    gsef = (AltSizeEntryField *) data;

    new_value = gtk_adjustment_get_value(GTK_ADJUSTMENT (widget));

    if (gsef->value != new_value)
        alt_size_entry_update_value(gsef, new_value);
}


/**
 * alt_size_entry_set_refval_boundaries:
 * @gse:   The sizeentry you want to set the reference value boundaries for.
 * @field: The index of the field you want to set the reference value
 *         boundaries for.
 * @lower: The new lower boundary of the reference value of the chosen @field.
 * @upper: The new upper boundary of the reference value of the chosen @field.
 *
 * Limits the range of possible reference values which can be entered in
 * field # @field of the #AltSizeEntry.
 *
 * The current reference value of the @field will be clamped to fit in the
 * @field's new boundaries.
 **/
void
alt_size_entry_set_refval_boundaries(AltSizeEntry *gse,
                                     gint field,
                                     gdouble lower,
                                     gdouble upper) {
    AltSizeEntryField *gsef;

    g_return_if_fail (ALT_IS_SIZE_ENTRY(gse));
    g_return_if_fail ((field >= 0) && (field < gse->number_of_fields));
    g_return_if_fail (lower <= upper);

    gsef = (AltSizeEntryField *) g_slist_nth_data(gse->fields, field);
    gsef->min_refval = lower;
    gsef->max_refval = upper;

    if (gse->show_refval) {
        gtk_adjustment_set_lower(GTK_ADJUSTMENT (gsef->refval_adjustment), gsef->min_refval);
        gtk_adjustment_set_upper(GTK_ADJUSTMENT (gsef->refval_adjustment), gsef->max_refval);
    }

    if (gsef->stop_recursion) /* this is a hack (but useful ;-) */
        return;

    gsef->stop_recursion++;
    switch (gsef->gse->update_policy) {
        case ALT_SIZE_ENTRY_UPDATE_NONE:
            break;

        case ALT_SIZE_ENTRY_UPDATE_SIZE:
            switch (gimp_unit_get_id(gse->unit)) {
                case GIMP_UNIT_PIXEL:
                    alt_size_entry_set_value_boundaries(gse, field,
                                                        gsef->min_refval,
                                                        gsef->max_refval);
                    break;
                case GIMP_UNIT_PERCENT:
                    alt_size_entry_set_value_boundaries(gse, field,
                                                        100 * (gsef->min_refval -
                                                               gsef->lower) /
                                                        (gsef->upper - gsef->lower),
                                                        100 * (gsef->max_refval -
                                                               gsef->lower) /
                                                        (gsef->upper - gsef->lower));
                    break;
                default:
                    alt_size_entry_set_value_boundaries(gse, field,
                                                        gsef->min_refval *
                                                        gimp_unit_get_factor(gse->unit) /
                                                        gsef->resolution,
                                                        gsef->max_refval *
                                                        gimp_unit_get_factor(gse->unit) /
                                                        gsef->resolution);
                    break;
            }
            break;

        case ALT_SIZE_ENTRY_UPDATE_RESOLUTION:
            alt_size_entry_set_value_boundaries(gse, field,
                                                gsef->min_refval /
                                                gimp_unit_get_factor(gse->unit),
                                                gsef->max_refval /
                                                gimp_unit_get_factor(gse->unit));
            break;

        default:
            break;
    }
    gsef->stop_recursion--;

    alt_size_entry_set_refval(gse, field, gsef->refval);
}

/**
 * alt_size_entry_set_refval_digits:
 * @gse:    The sizeentry you want to set the reference value digits for.
 * @field:  The index of the field you want to set the reference value for.
 * @digits: The new number of decimal digits for the #GtkSpinButton which
 *          displays @field's reference value.
 *
 * Sets the decimal digits of field # @field of the #AltSizeEntry to
 * @digits.
 *
 * If you don't specify this value explicitly, the reference value's number
 * of digits will equal to 0 for #ALT_SIZE_ENTRY_UPDATE_SIZE and to 2 for
 * #ALT_SIZE_ENTRY_UPDATE_RESOLUTION.
 **/
void
alt_size_entry_set_refval_digits(AltSizeEntry *gse,
                                 gint field,
                                 gint digits) {
    AltSizeEntryField *gsef;

    g_return_if_fail (ALT_IS_SIZE_ENTRY(gse));
    g_return_if_fail ((field >= 0) && (field < gse->number_of_fields));
    g_return_if_fail ((digits >= 0) && (digits <= 6));

    gsef = (AltSizeEntryField *) g_slist_nth_data(gse->fields, field);
    gsef->refval_digits = digits;

    if (gse->update_policy == ALT_SIZE_ENTRY_UPDATE_SIZE) {
        if (gse->show_refval)
            gtk_spin_button_set_digits(GTK_SPIN_BUTTON (gsef->refval_spinbutton),
                                       gsef->refval_digits);
        else if (gse->unit == GIMP_UNIT_PIXEL)
            gtk_spin_button_set_digits(GTK_SPIN_BUTTON (gsef->value_spinbutton),
                                       gsef->refval_digits);
    }
}

/**
 * alt_size_entry_get_refval;
 * @gse:   The sizeentry you want to know a reference value of.
 * @field: The index of the field you want to know the reference value of.
 *
 * Returns the reference value for field # @field of the #AltSizeEntry.
 *
 * The reference value is either a distance in pixels or a resolution
 * in dpi, depending on which #AltSizeEntryUpdatePolicy you chose in
 * alt_size_entry_new().
 *
 * Returns: The reference value of the chosen @field.
 **/
gdouble
alt_size_entry_get_refval(AltSizeEntry *gse,
                          gint field) {
    AltSizeEntryField *gsef;

    /*  return 1.0 to avoid division by zero  */
    g_return_val_if_fail (ALT_IS_SIZE_ENTRY(gse), 1.0);
    g_return_val_if_fail ((field >= 0) && (field < gse->number_of_fields), 1.0);

    gsef = (AltSizeEntryField *) g_slist_nth_data(gse->fields, field);
    return gsef->refval;
}

static void
alt_size_entry_update_refval(AltSizeEntryField *gsef,
                             gdouble refval) {
    if (gsef->stop_recursion > 1)
        return;

    gsef->refval = refval;

    switch (gsef->gse->update_policy) {
        case ALT_SIZE_ENTRY_UPDATE_NONE:
            break;

        case ALT_SIZE_ENTRY_UPDATE_SIZE:
            switch (gimp_unit_get_id(gsef->gse->unit)) {
                case GIMP_UNIT_PIXEL:
                    gsef->value = refval;
                    break;
                case GIMP_UNIT_PERCENT:
                    gsef->value =
                            CLAMP (100 * (refval - gsef->lower) / (gsef->upper - gsef->lower),
                                   gsef->min_value, gsef->max_value);
                    break;
                default:
                    gsef->value =
                            CLAMP (refval * gimp_unit_get_factor(gsef->gse->unit) /
                                   gsef->resolution,
                                   gsef->min_value, gsef->max_value);
                    break;
            }
            gtk_adjustment_set_value(GTK_ADJUSTMENT (gsef->value_adjustment),
                                     gsef->value);
            break;

        case ALT_SIZE_ENTRY_UPDATE_RESOLUTION:
            gsef->value =
                    CLAMP (refval / gimp_unit_get_factor(gsef->gse->unit),
                           gsef->min_value, gsef->max_value);
            gtk_adjustment_set_value(GTK_ADJUSTMENT (gsef->value_adjustment),
                                     gsef->value);
            break;

        default:
            break;
    }

    g_signal_emit(gsef->gse, alt_size_entry_signals[REFVAL_CHANGED], 0);
}

/**
 * alt_size_entry_set_refval;
 * @gse:    The sizeentry you want to set a reference value for.
 * @field:  The index of the field you want to set the reference value for.
 * @refval: The new reference value for @field.
 *
 * Sets the reference value for field # @field of the #AltSizeEntry.
 *
 * The @refval passed is either a distance in pixels or a resolution in dpi,
 * depending on which #AltSizeEntryUpdatePolicy you chose in
 * alt_size_entry_new().
 **/
void
alt_size_entry_set_refval(AltSizeEntry *gse,
                          gint field,
                          gdouble refval) {
    AltSizeEntryField *gsef;

    g_return_if_fail (ALT_IS_SIZE_ENTRY(gse));
    g_return_if_fail ((field >= 0) && (field < gse->number_of_fields));

    gsef = (AltSizeEntryField *) g_slist_nth_data(gse->fields, field);

    refval = CLAMP (refval, gsef->min_refval, gsef->max_refval);

    if (gse->show_refval)
        gtk_adjustment_set_value(GTK_ADJUSTMENT (gsef->refval_adjustment),
                                 refval);

    alt_size_entry_update_refval(gsef, refval);
}

static void
alt_size_entry_refval_callback(GtkWidget *widget,
                               gpointer data) {
    AltSizeEntryField *gsef;
    gdouble new_refval;

    gsef = (AltSizeEntryField *) data;

    new_refval = gtk_adjustment_get_value(GTK_ADJUSTMENT (widget));

    if (gsef->refval != new_refval)
        alt_size_entry_update_refval(gsef, new_refval);
}


/**
 * alt_size_entry_get_unit:
 * @gse: The sizeentry you want to know the unit of.
 *
 * Returns the #GimpUnit the user has selected in the #AltSizeEntry's
 * #GimpUnitMenu.
 *
 * Returns: The sizeentry's unit.
 **/
GimpUnit *
alt_size_entry_get_unit(AltSizeEntry *gse) {

    // @TODO migration fix needed
    //g_return_val_if_fail (ALT_IS_SIZE_ENTRY(gse), GIMP_UNIT_INCH);

    return gse->unit;
}

static void
alt_size_entry_update_unit(AltSizeEntry *gse,
                           GimpUnit *unit) {
    AltSizeEntryField *gsef;
    gint i;
    gint digits;

    gse->unit = unit;

    //@TODO migrate
    //digits = gimp_unit_menu_get_pixel_digits(GIMP_UNIT_MENU(gse->unitmenu));
    digits = 2;

    for (i = 0; i < gse->number_of_fields; i++) {
        gsef = (AltSizeEntryField *) g_slist_nth_data(gse->fields, i);

        if (gse->update_policy == ALT_SIZE_ENTRY_UPDATE_SIZE) {
            if (unit == GIMP_UNIT_PIXEL)
                gtk_spin_button_set_digits(GTK_SPIN_BUTTON (gsef->value_spinbutton),
                                           gsef->refval_digits + digits);
            else if (unit == gimp_unit_percent())
                gtk_spin_button_set_digits(GTK_SPIN_BUTTON (gsef->value_spinbutton),
                                           2 + digits);
            else
                gtk_spin_button_set_digits(GTK_SPIN_BUTTON (gsef->value_spinbutton),
                                           ALT_SIZE_ENTRY_DIGITS (unit) + digits);
        } else if (gse->update_policy == ALT_SIZE_ENTRY_UPDATE_RESOLUTION) {
            digits = (gimp_unit_get_digits(unit) -
                      gimp_unit_get_digits(unit));
            gtk_spin_button_set_digits(GTK_SPIN_BUTTON (gsef->value_spinbutton),
                                       MAX (3 + digits, 3));
        }

        gsef->stop_recursion = 0; /* hack !!! */

        alt_size_entry_set_refval_boundaries(gse, i,
                                             gsef->min_refval,
                                             gsef->max_refval);
    }

    g_signal_emit(gse, alt_size_entry_signals[UNIT_CHANGED], 0);
}


/**
 * alt_size_entry_set_unit:
 * @gse:  The sizeentry you want to change the unit for.
 * @unit: The new unit.
 *
 * Sets the #AltSizeEntry's unit. The reference value for all fields will
 * stay the same but the value in units or pixels per unit will change
 * according to which #AltSizeEntryUpdatePolicy you chose in
 * alt_size_entry_new().
 **/
void
alt_size_entry_set_unit(AltSizeEntry *gse,
                        GimpUnit *unit) {
    g_return_if_fail (ALT_IS_SIZE_ENTRY(gse));
    g_return_if_fail (gse->menu_show_pixels || (unit != gimp_unit_pixel()));
    g_return_if_fail (gse->menu_show_percent || (unit != gimp_unit_percent()));;

    // @TODO fix migration
    //gimp_unit_menu_set_unit(GIMP_UNIT_MENU(gse->unitmenu), unit);
    alt_size_entry_update_unit(gse, unit);
}

static void
alt_size_entry_unit_callback(GtkWidget *widget,
                             AltSizeEntry *gse) {
    GimpUnit *new_unit;

    new_unit = gimp_unit_combo_box_get_active(GIMP_UNIT_COMBO_BOX(widget));

    if (gse->unit != new_unit)
        alt_size_entry_update_unit(gse, new_unit);
}

/**
 * alt_size_entry_show_unit_menu:
 * @gse: a #AltSizeEntry
 * @show: Boolean
 *
 * Controls whether a unit menu is shown in the size entry.  If
 * @show is #TRUE, the menu is shown; otherwise it is hidden.
 *
 * Since: GIMP 2.4
 **/
void
alt_size_entry_show_unit_menu(AltSizeEntry *gse,
                              gboolean show) {
    g_return_if_fail (ALT_IS_SIZE_ENTRY(gse));

    if (show)
        gtk_widget_show(gse->unitmenu);
    else
        gtk_widget_hide(gse->unitmenu);
}


/**
 * alt_size_entry_set_pixel_digits:
 * @gse: a #AltSizeEntry
 * @digits: the number of digits to display for a pixel size
 *
 * Similar to gimp_unit_menu_set_pixel_digits(), this function allows
 * you set up a #AltSizeEntry so that sub-pixel sizes can be entered.
 **/
void
alt_size_entry_set_pixel_digits(AltSizeEntry *gse,
                                gint digits) {
    GimpUnitComboBox *combo;

    g_return_if_fail (ALT_IS_SIZE_ENTRY(gse));

    combo = GIMP_UNIT_COMBO_BOX(gse->unitmenu);

    // @TODO no obvious way to replace this
    // gimp_unit_menu_set_pixel_digits(menu, digits);

    alt_size_entry_update_unit(gse, gimp_unit_combo_box_get_active(combo));
}


/**
 * alt_size_entry_grab_focus:
 * @gse: The sizeentry you want to grab the keyboard focus.
 *
 * This function is rather ugly and just a workaround for the fact that
 * it's impossible to implement gtk_widget_grab_focus() for a #GtkGrid.
 **/
void
alt_size_entry_grab_focus(AltSizeEntry *gse) {
    AltSizeEntryField *gsef;

    g_return_if_fail (ALT_IS_SIZE_ENTRY(gse));

    gsef = gse->fields->data;
    if (gsef)
        gtk_widget_grab_focus(gse->show_refval ?
                              gsef->refval_spinbutton : gsef->value_spinbutton);
}

/**
 * alt_size_entry_set_activates_default:
 * @gse: A #GimpSizeEntr
 * @setting: %TRUE to activate window's default widget on Enter keypress
 *
 * Iterates over all entries in the #AltSizeEntry and calls
 * gtk_entry_set_activates_default() on them.
 *
 * Since: GIMP 2.4
 **/
void
alt_size_entry_set_activates_default(AltSizeEntry *gse,
                                     gboolean setting) {
    GSList *list;

    g_return_if_fail (ALT_IS_SIZE_ENTRY(gse));

    for (list = gse->fields; list; list = g_slist_next (list)) {
        AltSizeEntryField *gsef = list->data;

        if (gsef->value_spinbutton)
            gtk_entry_set_activates_default(GTK_ENTRY (gsef->value_spinbutton),
                                            setting);

        if (gsef->refval_spinbutton)
            gtk_entry_set_activates_default(GTK_ENTRY (gsef->refval_spinbutton),
                                            setting);
    }
}

/**
 * alt_size_entry_get_help_widget:
 * @gse: a #AltSizeEntry
 * @field: the index of the widget you want to get a pointer to
 *
 * You shouldn't fiddle with the internals of a #AltSizeEntry but
 * if you want to set tooltips using gimp_help_set_help_data() you
 * can use this function to get a pointer to the spinbuttons.
 *
 * Return value: a #GtkWidget pointer that you can attach a tooltip to.
 **/
GtkWidget *
alt_size_entry_get_help_widget(AltSizeEntry *gse,
                               gint field) {
    AltSizeEntryField *gsef;

    g_return_val_if_fail (ALT_IS_SIZE_ENTRY(gse), NULL);
    g_return_val_if_fail ((field >= 0) && (field < gse->number_of_fields), NULL);

    gsef = g_slist_nth_data(gse->fields, field);
    if (!gsef)
        return NULL;

    return (gsef->refval_spinbutton ?
            gsef->refval_spinbutton : gsef->value_spinbutton);
}
