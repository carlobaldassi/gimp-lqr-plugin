
#ifndef __IO_FUNCTIONS__
#define __IO_FUNCTIONS__

#ifndef __LQR_H__
#error "lqr/lqr.h must be included prior to io_functions.h"
#endif /* __LQR_H__ */

struct _VMapFuncArg;

typedef struct _VMapFuncArg VMapFuncArg;

struct _VMapFuncArg
{
  gint32 image_ID;
  gchar *name;
  gint x_off;
  gint y_off;
  GeglColor *colour_start;
  GeglColor *colour_end;
  gint32 * vmap_layer_ID_p;
};

#define VMAP_FUNC_ARG(data) ((VMapFuncArg*)(data))

/* INPUT/OUTPUT FUNCTIONS */

guchar *rgb_buffer_from_layer (gint32 layer_ID);
LqrRetVal update_bias (LqrCarver * r, gint32 layer_ID, gint bias_factor,
                       gint base_x_off, gint base_y_off);
LqrRetVal set_rigmask (LqrCarver * r, gint32 layer_ID, gint base_x_off, gint base_y_off);
LqrRetVal write_carver_to_layer (LqrCarver * r, gint32 layer_ID);
LqrRetVal write_vmap_to_layer (LqrVMap * vmap, gpointer data);
LqrRetVal write_all_vmaps (LqrVMapList * list, gint32 image_ID,
                           gchar * orig_name, gint x_off, gint y_off,
                           GeglColor *col_start, GeglColor *col_end);

#endif /* __IO_FUNCTIONS__ */
