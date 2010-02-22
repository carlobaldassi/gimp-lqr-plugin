/* GIMP LiquidRescale Plug-in
 * Copyright (C) 2007-2010 Carlo Baldassi (the "Author") <carlobaldassi@gmail.com>.
 * All Rights Reserved.
 *
 * The code in this file is taken from gimpsizeentry.h
 * Copyright (C) 1999-2000 Sven Neumann <sven@gimp.org>
 *                         Michael Natterer <mitch@gimp.org>
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

#ifndef __ALT_SIZE_ENTRY_H__
#define __ALT_SIZE_ENTRY_H__

#define ALT_TYPE_SIZE_ENTRY            (alt_size_entry_get_type ())
#define ALT_SIZE_ENTRY(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), ALT_TYPE_SIZE_ENTRY, AltSizeEntry))
#define ALT_SIZE_ENTRY_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), ALT_TYPE_SIZE_ENTRY, AltSizeEntryClass))
#define ALT_IS_SIZE_ENTRY(obj)         (G_TYPE_CHECK_INSTANCE_TYPE (obj, ALT_TYPE_SIZE_ENTRY))
#define ALT_IS_SIZE_ENTRY_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), ALT_TYPE_SIZE_ENTRY))
#define ALT_SIZE_ENTRY_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), ALT_TYPE_SIZE_ENTRY, AltSizeEntryClass))
#define ALT_TYPE_SIZE_ENTRY_UPDATE_POLICY (alt_size_entry_update_policy_get_type ())

GType alt_size_entry_update_policy_get_type (void) G_GNUC_CONST;

typedef enum
{
  ALT_SIZE_ENTRY_UPDATE_NONE       = 0,
  ALT_SIZE_ENTRY_UPDATE_SIZE       = 1,
  ALT_SIZE_ENTRY_UPDATE_RESOLUTION = 2
} AltSizeEntryUpdatePolicy;

typedef struct _AltSizeEntryClass  AltSizeEntryClass;

typedef struct _AltSizeEntryField  AltSizeEntryField;

struct _AltSizeEntry
{
  GtkTable   parent_instance;

  GSList    *fields;
  gint       number_of_fields;

  GtkWidget *unitmenu;
  GimpUnit   unit;
  gboolean   menu_show_pixels;
  gboolean   menu_show_percent;

  gboolean                   show_refval;
  AltSizeEntryUpdatePolicy  update_policy;
};

typedef struct _AltSizeEntry AltSizeEntry;

struct _AltSizeEntryClass
{
  GtkTableClass  parent_class;

  void (* value_changed)  (AltSizeEntry);// *gse);
  void (* refval_changed) (AltSizeEntry *gse);
  void (* unit_changed)   (AltSizeEntry *gse);

  /* Padding for future expansion */
  /*
  void (* _gimp_reserved1) (void);
  void (* _gimp_reserved2) (void);
  void (* _gimp_reserved3) (void);
  void (* _gimp_reserved4) (void);
  */
};


GType       alt_size_entry_get_type (void) G_GNUC_CONST;

GtkWidget * alt_size_entry_new (gint                       number_of_fields,
                                 GimpUnit                   unit,
                                 const gchar               *unit_format,
                                 gboolean                   menu_show_pixels,
                                 gboolean                   menu_show_percent,
                                 gboolean                   show_refval,
                                 gint                       spinbutton_width,
                                 AltSizeEntryUpdatePolicy  update_policy);

void        alt_size_entry_add_field  (AltSizeEntry   *gse,
                                        GtkSpinButton   *value_spinbutton,
                                        GtkSpinButton   *refval_spinbutton);

GtkWidget * alt_size_entry_attach_label          (AltSizeEntry *gse,
                                                   const gchar   *text,
                                                   gint           row,
                                                   gint           column,
                                                   gfloat         alignment);

void        alt_size_entry_set_resolution        (AltSizeEntry *gse,
                                                   gint           field,
                                                   gdouble        resolution,
                                                   gboolean       keep_size);

void        alt_size_entry_set_size              (AltSizeEntry *gse,
                                                   gint           field,
                                                   gdouble        lower,
                                                   gdouble        upper);

void        alt_size_entry_set_value_boundaries  (AltSizeEntry *gse,
                                                   gint           field,
                                                   gdouble        lower,
                                                   gdouble        upper);

gdouble     alt_size_entry_get_value             (AltSizeEntry *gse,
                                                   gint           field);
void        alt_size_entry_set_value             (AltSizeEntry *gse,
                                                   gint           field,
                                                   gdouble        value);

void        alt_size_entry_set_refval_boundaries (AltSizeEntry *gse,
                                                   gint           field,
                                                   gdouble        lower,
                                                   gdouble        upper);
void        alt_size_entry_set_refval_digits     (AltSizeEntry *gse,
                                                   gint           field,
                                                   gint           digits);

gdouble     alt_size_entry_get_refval            (AltSizeEntry *gse,
                                                   gint           field);
void        alt_size_entry_set_refval            (AltSizeEntry *gse,
                                                   gint           field,
                                                   gdouble        refval);

GimpUnit    alt_size_entry_get_unit              (AltSizeEntry *gse);
void        alt_size_entry_set_unit              (AltSizeEntry *gse,
                                                   GimpUnit       unit);
void        alt_size_entry_show_unit_menu        (AltSizeEntry *gse,
                                                   gboolean       show);

void        alt_size_entry_set_pixel_digits      (AltSizeEntry *gse,
                                                   gint           digits);

void        alt_size_entry_grab_focus            (AltSizeEntry *gse);
void        alt_size_entry_set_activates_default (AltSizeEntry *gse,
                                                   gboolean       setting);
GtkWidget * alt_size_entry_get_help_widget       (AltSizeEntry *gse,
                                                   gint           field);


#endif /* __ALT_SIZE_ENTRY_H__ */
