#include <stdio.h>

#include <libgimp/gimp.h>
#include <lqr.h>

#include "config.h"
#include "plugin-intl.h"

#include "io_functions.h"

#include "main_common.h"

guchar *
rgb_buffer_from_layer(gint32 layer_ID) {
    gint y, bpp;
    gint w, h;
    GeglBuffer *buffer_in;
    guchar *buffer;
    gint update_step;

    gimp_progress_init(_("Parsing layer..."));

    w = gimp_drawable_get_width_id(layer_ID);
    h = gimp_drawable_get_height_id(layer_ID);

    bpp = gimp_drawable_bpp_id(layer_ID);

    LQR_TRY_N_N (buffer = g_try_new(guchar, bpp * w * h));

    buffer_in = gimp_drawable_get_buffer(GIMP_DRAWABLE(gimp_drawable_get_by_id(layer_ID)));

    gegl_buffer_get(buffer_in, GEGL_RECTANGLE (0, 0, w, h), 1.0, NULL, buffer, GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

    for (y = 0; y < h; y++) {

        update_step = MAX ((h - 1) / 20, 1);
        if (y % update_step == 0) {
            gimp_progress_update((gdouble) y / (h - 1));
        }
    }

    g_object_unref(buffer_in);

    gimp_progress_end();

    return buffer;
}

LqrRetVal
update_bias(LqrCarver *r, gint32 layer_ID, gint bias_factor,
            gint base_x_off, gint base_y_off) {
    guchar *rgb;
    gint w, h, bpp;
    gint x_off, y_off;

    if ((layer_ID == 0) || (bias_factor == 0)) {
        return LQR_OK;
    }

    gimp_drawable_get_offsets_id(layer_ID, &x_off, &y_off);
    x_off -= base_x_off;
    y_off -= base_y_off;

    w = gimp_drawable_get_width_id(layer_ID);
    h = gimp_drawable_get_height_id(layer_ID);

    bpp = gimp_drawable_bpp_id(layer_ID);

    rgb = rgb_buffer_from_layer(layer_ID);

    CATCH (lqr_carver_bias_add_rgb_area
                   (r, rgb, bias_factor, bpp, w, h, x_off, y_off));

    g_free(rgb);

    return LQR_OK;
}

LqrRetVal
set_rigmask(LqrCarver *r, gint32 layer_ID, gint base_x_off, gint base_y_off) {
    guchar *rgb;
    gint w, h, bpp;
    gint x_off, y_off;

    if (layer_ID == 0) {
        return LQR_OK;
    }

    gimp_drawable_get_offsets_id(layer_ID, &x_off, &y_off);
    x_off -= base_x_off;
    y_off -= base_y_off;

    w = gimp_drawable_get_width_id(layer_ID);
    h = gimp_drawable_get_height_id(layer_ID);

    bpp = gimp_drawable_bpp_id(layer_ID);

    rgb = rgb_buffer_from_layer(layer_ID);

    CATCH (lqr_carver_rigmask_add_rgb_area
                   (r, rgb, bpp, w, h, x_off, y_off));

    g_free(rgb);

    return LQR_OK;
}


LqrRetVal
write_carver_to_layer(LqrCarver *r, gint32 layer_ID) {
    GeglBuffer *buffer_out;
    gint y;
    gint w, h;
    guchar *out_line;
    gint update_step;

    gimp_progress_init(_("Applying changes..."));
    update_step = MAX ((lqr_carver_get_height(r) - 1) / 20, 1);

    w = gimp_drawable_get_width_id(layer_ID);
    h = gimp_drawable_get_height_id(layer_ID);

    buffer_out = gimp_drawable_get_buffer(GIMP_DRAWABLE(gimp_drawable_get_by_id(layer_ID)));


    while (lqr_carver_scan_line(r, &y, &out_line)) {
        if (lqr_carver_scan_by_row(r)) {
            gegl_buffer_set(buffer_out, GEGL_RECTANGLE (0, y, w, 1), 0, NULL, out_line, GEGL_AUTO_ROWSTRIDE);
        } else {
            gegl_buffer_set(buffer_out, GEGL_RECTANGLE (y, 0, 1, h), 0, NULL, out_line, GEGL_AUTO_ROWSTRIDE);
        }

        if (y % update_step == 0) {
            gimp_progress_update((gdouble) y / (lqr_carver_get_height(r) - 1));
        }

    }

    gegl_buffer_flush(buffer_out);
    gimp_drawable_update(GIMP_DRAWABLE(gimp_drawable_get_by_id(layer_ID)), 0, 0, w, h);

    g_object_unref(buffer_out);

    gimp_progress_end();

    return LQR_OK;
}

LqrRetVal
write_vmap_to_layer(LqrVMap *vmap, gpointer data) {
    gint w, h, bpp;
    gint depth;
    gint *buffer;
    gint32 seam_layer_ID;
    gint32 *seam_layer_p;
    gint32 image_ID;
    GeglBuffer *buffer_out;
    gint x_off, y_off;
    gchar *name;
    GeglColor *col_start, *col_end;
    guchar *outrow;
    gdouble value, rd, gr, bl, al;
    gint vs, y, x, k;
    gint update_step;

    image_ID = VMAP_FUNC_ARG (data)->image_ID;
    x_off = VMAP_FUNC_ARG (data)->x_off;
    y_off = VMAP_FUNC_ARG (data)->y_off;
    name = VMAP_FUNC_ARG (data)->name;
    col_start = VMAP_FUNC_ARG (data)->colour_start;
    col_end = VMAP_FUNC_ARG (data)->colour_end;
    seam_layer_ID = -1;
    seam_layer_p = VMAP_FUNC_ARG (data)->vmap_layer_ID_p;
    if (seam_layer_p) {
        seam_layer_ID = *seam_layer_p;
    }

    w = lqr_vmap_get_width(vmap);
    h = lqr_vmap_get_height(vmap);
    buffer = lqr_vmap_get_data(vmap);
    depth = lqr_vmap_get_depth(vmap);

    gimp_progress_init(_("Drawing seam map..."));
    update_step = MAX ((h - 1) / 20, 1);

    if (!gimp_drawable_is_valid_id(seam_layer_ID)) {

        seam_layer_ID =
                gimp_layer_new_id(image_ID, name, w, h, GIMP_RGBA_IMAGE, 100,
                                  GIMP_LAYER_MODE_NORMAL);

        gimp_drawable_fill_id(seam_layer_ID, GIMP_FILL_TRANSPARENT);
        gimp_image_insert_layer_id(image_ID, seam_layer_ID, 0, -1);
        gimp_layer_set_offsets(GIMP_LAYER(gimp_drawable_get_by_id(seam_layer_ID)), x_off, y_off);
        if (seam_layer_p) {
            *seam_layer_p = seam_layer_ID;
        }
    } else {
        gimp_layer_resize_id(seam_layer_ID, w, h, 0, 0);
    }
    buffer_out = gimp_drawable_get_buffer(GIMP_DRAWABLE(gimp_drawable_get_by_id(seam_layer_ID)));

    bpp = 4;

    CATCH_MEM (outrow = g_try_new(guchar, w * bpp));

    for (y = 0; y < h; y++) {
        for (x = 0; x < w; x++) {
            vs = buffer[y * w + x];
            if (vs == 0) {
                for (k = 0; k < bpp; k++) {
                    outrow[x * bpp + k] = 0;
                }
            } else {
                value = (double) (depth + 1 - vs) / (depth + 1);
                gdouble start_rgba[4], end_rgba[4];
                gegl_color_get_rgba(col_start, &start_rgba[0], &start_rgba[1], &start_rgba[2], &start_rgba[3]);
                gegl_color_get_rgba(col_end, &end_rgba[0], &end_rgba[1], &end_rgba[2], &end_rgba[3]);
                rd = value * start_rgba[0] + (1 - value) * end_rgba[0];
                gr = value * start_rgba[1] + (1 - value) * end_rgba[1];
                bl = value * start_rgba[2] + (1 - value) * end_rgba[2];
                al = 0.5 * (1 + value);
                outrow[x * bpp] = 255 * rd;
                outrow[x * bpp + 1] = 255 * gr;
                outrow[x * bpp + 2] = 255 * bl;
                outrow[x * bpp + 3] = 255 * al;
            }
        }
        gegl_buffer_set(buffer_out, GEGL_RECTANGLE (0, y, w, 1), 0, NULL, outrow, GEGL_AUTO_ROWSTRIDE);
        if (y % update_step == 0) {
            gimp_progress_update((gdouble) y / (h - 1));
        }
    }

    gegl_buffer_flush(buffer_out);
    gimp_drawable_update(GIMP_DRAWABLE(gimp_drawable_get_by_id(seam_layer_ID)), 0, 0, w, h);
    gimp_item_set_visible(GIMP_ITEM(gimp_drawable_get_by_id(seam_layer_ID)), TRUE);
    g_object_unref(buffer_out);

    gimp_progress_end();

    return LQR_OK;
}

LqrRetVal
write_all_vmaps(LqrVMapList *list, gint32 image_ID, gchar *orig_name,
                gint x_off, gint y_off, GeglColor *col_start, GeglColor *col_end) {
    gchar name[LQR_MAX_NAME_LENGTH];
    VMapFuncArg data;

    /* The name of the layer with the seams map */
    /* (here "%s" represents the selected layer's name) */
    g_snprintf(name, LQR_MAX_NAME_LENGTH, _("%s seam map"), orig_name);


    data.image_ID = image_ID;
    data.name = name;
    data.x_off = x_off;
    data.y_off = y_off;
    data.colour_start = col_start;
    data.colour_end = col_end;
    data.vmap_layer_ID_p = NULL;

    return lqr_vmap_list_foreach(list, write_vmap_to_layer,
                                 (gpointer) (&data));
}
