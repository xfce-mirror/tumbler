/* vi:set et ai sw=2 sts=2 ts=2: */
/*-
 * Copyright (c) 2009 Jannis Pohlmann <jannis@xfce.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "tumbler-thumbnail-flavor.h"
#include "tumbler-visibility.h"



/* property identifiers */
enum
{
  PROP_0,
  PROP_NAME,
  PROP_WIDTH,
  PROP_HEIGHT,
};



static void
tumbler_thumbnail_flavor_finalize (GObject *object);
static void
tumbler_thumbnail_flavor_get_property (GObject *object,
                                       guint prop_id,
                                       GValue *value,
                                       GParamSpec *pspec);
static void
tumbler_thumbnail_flavor_set_property (GObject *object,
                                       guint prop_id,
                                       const GValue *value,
                                       GParamSpec *pspec);



struct _TumblerThumbnailFlavor
{
  GObject __parent__;

  gchar *name;
  gint width;
  gint height;
};



G_DEFINE_TYPE (TumblerThumbnailFlavor, tumbler_thumbnail_flavor, G_TYPE_OBJECT)



static void
tumbler_thumbnail_flavor_class_init (TumblerThumbnailFlavorClass *klass)
{
  GObjectClass *gobject_class;

  /* Determine the parent type class */
  tumbler_thumbnail_flavor_parent_class = g_type_class_peek_parent (klass);

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->finalize = tumbler_thumbnail_flavor_finalize;
  gobject_class->get_property = tumbler_thumbnail_flavor_get_property;
  gobject_class->set_property = tumbler_thumbnail_flavor_set_property;

  g_object_class_install_property (gobject_class, PROP_NAME,
                                   g_param_spec_string ("name",
                                                        "name",
                                                        "name",
                                                        NULL,
                                                        G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (gobject_class, PROP_WIDTH,
                                   g_param_spec_int ("width",
                                                     "width",
                                                     "width",
                                                     -1, G_MAXINT, 0,
                                                     G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (gobject_class, PROP_HEIGHT,
                                   g_param_spec_int ("height",
                                                     "height",
                                                     "height",
                                                     -1, G_MAXINT, 0,
                                                     G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
}



static void
tumbler_thumbnail_flavor_init (TumblerThumbnailFlavor *flavor)
{
}



static void
tumbler_thumbnail_flavor_finalize (GObject *object)
{
  TumblerThumbnailFlavor *flavor = TUMBLER_THUMBNAIL_FLAVOR (object);

  g_free (flavor->name);

  (*G_OBJECT_CLASS (tumbler_thumbnail_flavor_parent_class)->finalize) (object);
}



static void
tumbler_thumbnail_flavor_get_property (GObject *object,
                                       guint prop_id,
                                       GValue *value,
                                       GParamSpec *pspec)
{
  TumblerThumbnailFlavor *flavor = TUMBLER_THUMBNAIL_FLAVOR (object);

  switch (prop_id)
    {
    case PROP_NAME:
      g_value_set_string (value, flavor->name);
      break;
    case PROP_WIDTH:
      g_value_set_int (value, flavor->width);
      break;
    case PROP_HEIGHT:
      g_value_set_int (value, flavor->height);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}



static void
tumbler_thumbnail_flavor_set_property (GObject *object,
                                       guint prop_id,
                                       const GValue *value,
                                       GParamSpec *pspec)
{
  TumblerThumbnailFlavor *flavor = TUMBLER_THUMBNAIL_FLAVOR (object);

  switch (prop_id)
    {
    case PROP_NAME:
      flavor->name = g_value_dup_string (value);
      break;
    case PROP_WIDTH:
      flavor->width = g_value_get_int (value);
      break;
    case PROP_HEIGHT:
      flavor->height = g_value_get_int (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}



TumblerThumbnailFlavor *
tumbler_thumbnail_flavor_new (const gchar *name,
                              gint width,
                              gint height)
{
  g_return_val_if_fail (name != NULL && *name != '\0', NULL);

  return g_object_new (TUMBLER_TYPE_THUMBNAIL_FLAVOR, "name", name,
                       "width", width, "height", height, NULL);
}



TumblerThumbnailFlavor *
tumbler_thumbnail_flavor_new_normal (void)
{
  return g_object_new (TUMBLER_TYPE_THUMBNAIL_FLAVOR, "name", "normal",
                       "width", 128, "height", 128, NULL);
}



TumblerThumbnailFlavor *
tumbler_thumbnail_flavor_new_large (void)
{
  return g_object_new (TUMBLER_TYPE_THUMBNAIL_FLAVOR, "name", "large",
                       "width", 256, "height", 256, NULL);
}



TumblerThumbnailFlavor *
tumbler_thumbnail_flavor_new_x_large (void)
{
  return g_object_new (TUMBLER_TYPE_THUMBNAIL_FLAVOR, "name", "x-large",
                       "width", 512, "height", 512, NULL);
}



TumblerThumbnailFlavor *
tumbler_thumbnail_flavor_new_xx_large (void)
{
  return g_object_new (TUMBLER_TYPE_THUMBNAIL_FLAVOR, "name", "xx-large",
                       "width", 1024, "height", 1024, NULL);
}



const gchar *
tumbler_thumbnail_flavor_get_name (TumblerThumbnailFlavor *flavor)
{
  g_return_val_if_fail (TUMBLER_IS_THUMBNAIL_FLAVOR (flavor), NULL);
  return flavor->name;
}



void
tumbler_thumbnail_flavor_get_size (TumblerThumbnailFlavor *flavor,
                                   gint *width,
                                   gint *height)
{
  g_return_if_fail (TUMBLER_IS_THUMBNAIL_FLAVOR (flavor));

  if (width != NULL)
    *width = flavor->width;

  if (height != NULL)
    *height = flavor->height;
}

#define __TUMBLER_THUMBNAIL_FLAVOR_C__
#include "tumbler-visibility.c"
