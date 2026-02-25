#ifndef __MAIN_COMMON_H__
#define __MAIN_COMMON_H__

#include <gtk/gtk.h>

/*  Constants  */

#define PLUG_IN_NAME   "plug-in-lqr"

#define DATA_KEY_VALS    "plug_in_lqr"
#define DATA_KEY_UI_VALS "plug_in_lqr_ui"
#define DATA_KEY_COL_VALS "plug_in_lqr_col"
#define PARASITE_KEY     "plug_in_lqr_options"

#define VALS_MAX_NAME_LENGTH (1024)
#define MAX_STRING_SIZE   (2048)

/**
 * PlugInVals
 *
 *
 *
 */
typedef struct
{
  gint new_width;
  gint new_height;
  gint32 pres_layer_ID;
  gint pres_coeff;
  gint32 disc_layer_ID;
  gint disc_coeff;
  gfloat rigidity;
  gint32 rigmask_layer_ID;
  gint delta_x;
  gfloat enl_step;
  gboolean resize_aux_layers;
  gboolean resize_canvas;
  gint32 output_target;
  gboolean output_seams;
  gint nrg_func;
  gint res_order;
  gint mask_behavior;
  gboolean scaleback;
  gint scaleback_mode;
  gboolean no_disc_on_enlarge;
  gchar pres_layer_name[VALS_MAX_NAME_LENGTH];
  gchar disc_layer_name[VALS_MAX_NAME_LENGTH];
  gchar rigmask_layer_name[VALS_MAX_NAME_LENGTH];
  gchar selected_layer_name[VALS_MAX_NAME_LENGTH];
} PlugInVals;

static inline gint32
gimp_image_get_active_layer(GimpImage *image) {
    GList *layers = gimp_image_list_layers(image);

    if (layers) {
        gint32 layer_id = gimp_item_get_id(GIMP_ITEM(layers->data));
        g_list_free(layers);
        return layer_id;
    }
    return -1;
}

static inline gint32
gimp_image_get_active_layer_id(gint32 image_id) {
    GimpImage *image = gimp_image_get_by_id(image_id);
    return gimp_image_get_active_layer(image);
}

static inline void
gimp_image_set_active_layer_id(gint32 image_id, gint32 layer_id) {
    GimpImage *image = gimp_image_get_by_id(image_id);
    GimpLayer *layer = gimp_layer_get_by_id(layer_id);
    GimpLayer *layers[] = {layer, NULL};
    gimp_image_set_selected_layers(image, (const GimpLayer **)layers);
}

static inline gint32*
gimp_image_get_layers_id(gint32 image_id, gint *num_layers) {
    GimpImage *image = gimp_image_get_by_id(image_id);
    GList *layers = gimp_image_list_layers(image);
    GList *iter = layers;

    *num_layers = g_list_length(layers);
    gint32 *layer_ids = g_new(gint32, *num_layers);

    for (int i = 0; iter; iter = iter->next, i++) {
        layer_ids[i] = gimp_item_get_id(GIMP_ITEM(iter->data));
    }

    g_list_free(layers);
    return layer_ids;
}

// Wrapper functions for ID-based calls
static inline gint
gimp_drawable_get_width_id(gint32 drawable_id) {
    GimpDrawable *drawable = gimp_drawable_get_by_id(drawable_id);
    return gimp_drawable_get_width(drawable);
}

// Wrapper functions for ID-based calls
static inline gint
gimp_drawable_get_height_id(gint32 drawable_id) {
    GimpDrawable *drawable = gimp_drawable_get_by_id(drawable_id);
    return gimp_drawable_get_height(drawable);
}

static inline const gchar *
gimp_drawable_get_name_id(gint32 drawable_id) {
    GimpDrawable *drawable = gimp_drawable_get_by_id(drawable_id);
    return gimp_item_get_name(GIMP_ITEM(drawable));
}

static inline const gchar *
gimp_item_get_name_id(gint32 item_id) {
    GimpItem *item = NULL;

    // Try as layer first
    GimpLayer *layer = gimp_layer_get_by_id(item_id);
    if (layer) item = GIMP_ITEM(layer);

    // Try as channel if not layer
    if (!item) {
        GimpChannel *channel = gimp_channel_get_by_id(item_id);
        if (channel) item = GIMP_ITEM(channel);
    }

    return item ? gimp_item_get_name(item) : "Unknown";
}

static inline GdkPixbuf*
gimp_drawable_get_thumbnail_id(gint32 drawable_id, gint width, gint height, GimpPixbufTransparency alpha) {
    GimpDrawable *drawable = gimp_drawable_get_by_id(drawable_id);
    return gimp_drawable_get_thumbnail(drawable, width, height, alpha);
}

//static inline gint32
//gimp_image_get_active_layer_id(gint32 image_id) {
//    GimpImage *image = gimp_image_get_by_id(image_id);
//    GimpDrawable *drawable = gimp_image_get_active_drawable(image);
//    if (drawable && GIMP_IS_LAYER(drawable)) {
//        return gimp_item_get_id(GIMP_ITEM(drawable));
//    }
//    return -1;
//}

//static inline gint32
//gimp_image_get_active_layer_id(gint32 image_id) {
//    GimpImage *image = gimp_image_get_by_id(image_id);
//    GimpLayer *layer = gimp_image_get_active_layer(image);
//    return layer ? gimp_item_get_id(GIMP_ITEM(layer)) : -1;
//}

static inline gboolean
gimp_drawable_is_layer_id(gint32 drawable_id) {
    GimpDrawable *drawable = gimp_drawable_get_by_id(drawable_id);
    return GIMP_IS_LAYER(drawable);
}

static inline void
gimp_layer_set_lock_alpha_id(gint32 layer_id, gboolean lock) {
    GimpLayer *layer = gimp_layer_get_by_id(layer_id);
    gimp_layer_set_lock_alpha(layer, lock);
}

static inline GimpUnit *
gimp_image_get_unit_id(gint32 image_id) {
    GimpImage *image = gimp_image_get_by_id(image_id);
    return gimp_image_get_unit(image);
}

static inline void
gimp_image_get_resolution_id(gint32 image_id, gdouble *xres, gdouble *yres) {
    GimpImage *image = gimp_image_get_by_id(image_id);
    gimp_image_get_resolution(image, xres, yres);
}

static inline void
gimp_image_undo_group_start_id(gint32 image_id) {
    GimpImage *image = gimp_image_get_by_id(image_id);
    gimp_image_undo_group_start(image);
}

static inline void
gimp_image_undo_group_end_id(gint32 image_id) {
    GimpImage *image = gimp_image_get_by_id(image_id);
    gimp_image_undo_group_end(image);
}

static inline gboolean
gimp_drawable_is_valid_id(gint32 drawable_id) {
    GimpDrawable *drawable = gimp_drawable_get_by_id(drawable_id);
    return drawable != NULL;
}

static inline gboolean
gimp_layer_has_mask_id(gint32 layer_id) {
    GimpLayer *layer = gimp_layer_get_by_id(layer_id);
    GimpLayerMask *mask = gimp_layer_get_mask(layer);
    return mask != NULL;
}

static inline void
gimp_layer_remove_mask_id(gint32 layer_id, GimpMaskApplyMode mode) {
    GimpLayer *layer = gimp_layer_get_by_id(layer_id);
    gimp_layer_remove_mask(layer, mode);
}

static inline gint32
gimp_layer_get_mask_id(gint32 layer_id) {
    GimpLayer *layer = gimp_layer_get_by_id(layer_id);
    GimpLayerMask *mask = gimp_layer_get_mask(layer);
    return mask ? gimp_item_get_id(GIMP_ITEM(mask)) : -1;
}

static inline void
gimp_drawable_get_offsets_id(gint32 drawable_id, gint *offset_x, gint *offset_y) {
    GimpDrawable *drawable = gimp_drawable_get_by_id(drawable_id);
    gimp_drawable_get_offsets(drawable, offset_x, offset_y);
}

static inline gboolean
gimp_drawable_has_alpha_id(gint32 drawable_id) {
    GimpDrawable *drawable = gimp_drawable_get_by_id(drawable_id);
    return gimp_drawable_has_alpha(drawable);
}

static inline gint
gimp_drawable_bpp_id(gint32 drawable_id) {
    GimpDrawable *drawable = gimp_drawable_get_by_id(drawable_id);
    return gimp_drawable_get_bpp(drawable);
}

static inline void
gimp_image_insert_layer_id(gint32 image_id, gint32 layer_id, gint32 parent_id, gint position) {
    GimpImage *image = gimp_image_get_by_id(image_id);
    GimpLayer *layer = gimp_layer_get_by_id(layer_id);
    GimpLayer *parent = parent_id > 0 ? gimp_layer_get_by_id(parent_id) : NULL;
    gimp_image_insert_layer(image, layer, parent, position);
}

static inline gboolean
gimp_layer_get_lock_alpha_id(gint32 layer_id) {
    GimpLayer *layer = gimp_layer_get_by_id(layer_id);
    return gimp_layer_get_lock_alpha(layer);
}

static inline void
gimp_image_remove_layer_id(gint32 image_id, gint32 layer_id) {
    GimpImage *image = gimp_image_get_by_id(image_id);
    GimpLayer *layer = gimp_layer_get_by_id(layer_id);
    gimp_image_remove_layer(image, layer);
}

static inline gint32
gimp_layer_new_id(gint32 image_id, const gchar *name, gint width, gint height,
                  GimpImageType type, gdouble opacity, GimpLayerMode mode) {
    GimpImage *image = gimp_image_get_by_id(image_id);
    GimpLayer *layer = gimp_layer_new(image, name, width, height, type, opacity, mode);
    return gimp_item_get_id(GIMP_ITEM(layer));
}

static inline gint32
gimp_image_new_id(gint width, gint height, GimpImageBaseType type) {
    GimpImage *image = gimp_image_new(width, height, type);
    return gimp_image_get_id(image);
}

static inline gint32
gimp_display_new_id(gint32 image_id) {
    GimpImage *image = gimp_image_get_by_id(image_id);
    GimpDisplay *display = gimp_display_new(image);
    return gimp_display_get_id(display);
}

static inline void
gimp_image_resize_id(gint32 image_id, gint new_width, gint new_height, gint offset_x, gint offset_y) {
    GimpImage *image = gimp_image_get_by_id(image_id);
    gimp_image_resize(image, new_width, new_height, offset_x, offset_y);
}

static inline void
gimp_image_convert_rgb_id(gint32 image_id) {
    GimpImage *image = gimp_image_get_by_id(image_id);
    gimp_image_convert_rgb(image);
}

static inline void
gimp_drawable_fill_id(gint32 drawable_id, GimpFillType fill_type) {
    GimpDrawable *drawable = gimp_drawable_get_by_id(drawable_id);
    gimp_drawable_fill(drawable, fill_type);
}

static inline void
gimp_layer_resize_id(gint32 layer_id, gint new_width, gint new_height, gint offset_x, gint offset_y) {
    GimpLayer *layer = gimp_layer_get_by_id(layer_id);
    gimp_layer_resize(layer, new_width, new_height, offset_x, offset_y);
}

static inline void
gimp_layer_scale_id(gint32 layer_id, gint new_width, gint new_height, gboolean local_origin) {
    GimpLayer *layer = gimp_layer_get_by_id(layer_id);
    gimp_layer_scale(layer, new_width, new_height, local_origin);
}

static inline gboolean
gimp_drawable_same_image_id(gint32 image_id, gint32 drawable_id) {
    GimpDrawable *drawable = gimp_drawable_get_by_id(drawable_id);
    GimpImage *drawable_image = gimp_item_get_image(GIMP_ITEM(drawable));
    return gimp_image_get_id(drawable_image) != image_id;
}

static inline void
gimp_layer_resize_to_image_size_id(gint32 layer_id) {
    GimpLayer *layer = gimp_layer_get_by_id(layer_id);
    gimp_layer_resize_to_image_size(layer);
}

static inline GimpImageBaseType
gimp_image_base_type_id(gint32 image_id) {
    GimpImage *image = gimp_image_get_by_id(image_id);
    return gimp_image_get_base_type(image);
}

static inline void
gimp_drawable_set_name_id(gint32 drawable_id, const gchar *name) {
    GimpDrawable *drawable = gimp_drawable_get_by_id(drawable_id);
    gimp_item_set_name(GIMP_ITEM(drawable), name);
}

static inline gboolean
gimp_image_is_valid_id(gint32 image_id) {
    GimpImage *image = gimp_image_get_by_id(image_id);
    return image != NULL;
}

static inline gint32
gimp_layer_copy_id(gint32 layer_id) {
    GimpLayer *layer = gimp_layer_get_by_id(layer_id);
    GimpLayer *copy = gimp_layer_copy(layer);
    return gimp_item_get_id(GIMP_ITEM(copy));
}

static inline gint32
gimp_layer_new_from_drawable_id(gint32 drawable_id, gint32 image_id) {
    GimpDrawable *drawable = gimp_drawable_get_by_id(drawable_id);
    GimpImage *image = gimp_image_get_by_id(image_id);
    GimpLayer *layer = gimp_layer_new_from_drawable(drawable, image);
    return gimp_item_get_id(GIMP_ITEM(layer));
}

static inline void
gimp_layer_set_opacity_id(gint32 layer_id, gdouble opacity) {
    GimpLayer *layer = gimp_layer_get_by_id(layer_id);
    gimp_layer_set_opacity(layer, opacity);
}

static inline gint32
gimp_drawable_get_image_id(gint32 drawable_id) {
    GimpDrawable *drawable = gimp_drawable_get_by_id(drawable_id);
    GimpImage *image = gimp_item_get_image(GIMP_ITEM(drawable));
    return gimp_image_get_id(image);
}

static inline void
unfloat_layer_id(gint32 layer_id) {
    GimpLayer *layer = gimp_layer_get_by_id(layer_id);
    if (gimp_layer_is_floating_sel(layer)) {
        gimp_floating_sel_to_layer(layer);
    }
}

static inline gint32
gimp_selection_save_id(gint32 image_id) {
    GimpImage *image = gimp_image_get_by_id(image_id);
    GimpChannel *channel = gimp_selection_save(image);
    return gimp_item_get_id(GIMP_ITEM(channel));
}

static inline void
gimp_selection_none_id(gint32 image_id) {
    GimpImage *image = gimp_image_get_by_id(image_id);
    gimp_selection_none(image);
}

static inline void
gimp_image_unset_active_channel_id(gint32 image_id) {
    GimpImage *image = gimp_image_get_by_id(image_id);
    gimp_image_unset_active_channel(image);
}

static inline gboolean
gimp_selection_is_empty_id(gint32 image_id) {
    GimpImage *image = gimp_image_get_by_id(image_id);
    return gimp_selection_is_empty(image);
}

#endif /* __MAIN_COMMON_H__ */
