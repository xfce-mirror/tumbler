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
#include <config.h>
#endif

#include <glib.h>
#include <glib-object.h>

#include <tumbler/tumbler-enum-types.h>
#include <tumbler/tumbler-png-thumbnail-info.h>
#include <tumbler/tumbler-thumbnail-info.h>



#define TUMBLER_PNG_THUMBNAIL_INFO_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), TUMBLER_TYPE_PNG_THUMBNAIL_INFO, TumblerPNGThumbnailInfoPrivate))



/* property identifiers */
enum
{
  PROP_0,
  PROP_FORMAT,
  PROP_URI,
};



static void                   tumbler_png_thumbnail_info_class_init   (TumblerPNGThumbnailInfoClass *klass);
static void                   tumbler_png_thumbnail_info_iface_init   (TumblerThumbnailInfoIface    *iface);
static void                   tumbler_png_thumbnail_info_init         (TumblerPNGThumbnailInfo      *info);
static void                   tumbler_png_thumbnail_info_constructed  (GObject                      *object);
static void                   tumbler_png_thumbnail_info_finalize     (GObject                      *object);
static void                   tumbler_png_thumbnail_info_get_property (GObject                      *object,
                                                                       guint                         prop_id,
                                                                       GValue                       *value,
                                                                       GParamSpec                   *pspec);
static void                   tumbler_png_thumbnail_info_set_property (GObject                      *object,
                                                                       guint                         prop_id,
                                                                       const GValue                 *value,
                                                                       GParamSpec                   *pspec);
static TumblerThumbnailFormat tumbler_png_thumbnail_info_get_format   (TumblerThumbnailInfo         *info);
static const gchar           *tumbler_png_thumbnail_info_get_uri      (TumblerThumbnailInfo         *info);



struct _TumblerPNGThumbnailInfoClass
{
  GObjectClass __parent__;
};

struct _TumblerPNGThumbnailInfo
{
  GObject __parent__;

  TumblerPNGThumbnailInfoPrivate *priv;
};

struct _TumblerPNGThumbnailInfoPrivate
{
  TumblerThumbnailFormat format;
  gchar                 *uri;
};



static GObjectClass *tumbler_png_thumbnail_info_parent_class = NULL;



GType
tumbler_png_thumbnail_info_get_type (void)
{
  static GType type = G_TYPE_INVALID;

  if (G_UNLIKELY (type == G_TYPE_INVALID))
    {
      static const GInterfaceInfo info = 
      {
        (GClassInitFunc) tumbler_png_thumbnail_info_iface_init,
        NULL,
        NULL,
      };

      type = g_type_register_static_simple (G_TYPE_OBJECT, 
                                            "TumblerPNGThumbnailInfo",
                                            sizeof (TumblerPNGThumbnailInfoClass),
                                            (GClassInitFunc) tumbler_png_thumbnail_info_class_init,
                                            sizeof (TumblerPNGThumbnailInfo),
                                            (GInstanceInitFunc) tumbler_png_thumbnail_info_init,
                                            0);

      g_type_add_interface_static (type, TUMBLER_TYPE_THUMBNAIL_INFO, &info);
    }

  return type;
}



static void
tumbler_png_thumbnail_info_class_init (TumblerPNGThumbnailInfoClass *klass)
{
  GObjectClass *gobject_class;

  g_type_class_add_private (klass, sizeof (TumblerPNGThumbnailInfoPrivate));

  /* Determine the parent type class */
  tumbler_png_thumbnail_info_parent_class = g_type_class_peek_parent (klass);

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->constructed = tumbler_png_thumbnail_info_constructed; 
  gobject_class->finalize = tumbler_png_thumbnail_info_finalize; 
  gobject_class->get_property = tumbler_png_thumbnail_info_get_property;
  gobject_class->set_property = tumbler_png_thumbnail_info_set_property;

  g_object_class_override_property (gobject_class, PROP_FORMAT, "format");
  g_object_class_override_property (gobject_class, PROP_URI, "uri");
}



static void
tumbler_png_thumbnail_info_iface_init (TumblerThumbnailInfoIface *iface)
{
  iface->get_format = tumbler_png_thumbnail_info_get_format;
  iface->get_uri = tumbler_png_thumbnail_info_get_uri;
}



static void
tumbler_png_thumbnail_info_init (TumblerPNGThumbnailInfo *info)
{
  info->priv = TUMBLER_PNG_THUMBNAIL_INFO_GET_PRIVATE (info);
}



static void
tumbler_png_thumbnail_info_constructed (GObject *object)
{
  TumblerPNGThumbnailInfo *info = TUMBLER_PNG_THUMBNAIL_INFO (object);
}



static void
tumbler_png_thumbnail_info_finalize (GObject *object)
{
  TumblerPNGThumbnailInfo *info = TUMBLER_PNG_THUMBNAIL_INFO (object);

  g_free (info->priv->uri);

  (*G_OBJECT_CLASS (tumbler_png_thumbnail_info_parent_class)->finalize) (object);
}



static void
tumbler_png_thumbnail_info_get_property (GObject    *object,
                                         guint       prop_id,
                                         GValue     *value,
                                         GParamSpec *pspec)
{
  TumblerPNGThumbnailInfo *info = TUMBLER_PNG_THUMBNAIL_INFO (object);

  switch (prop_id)
    {
    case PROP_FORMAT:
      g_value_set_enum (value, info->priv->format);
      break;
    case PROP_URI:
      g_value_set_string (value, info->priv->uri);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}



static void
tumbler_png_thumbnail_info_set_property (GObject      *object,
                                         guint         prop_id,
                                         const GValue *value,
                                         GParamSpec   *pspec)
{
  TumblerPNGThumbnailInfo *info = TUMBLER_PNG_THUMBNAIL_INFO (object);

  switch (prop_id)
    {
    case PROP_FORMAT:
      info->priv->format = g_value_get_enum (value);
      break;
    case PROP_URI:
      info->priv->uri = g_value_dup_string (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}



static TumblerThumbnailFormat
tumbler_png_thumbnail_info_get_format (TumblerThumbnailInfo *info)
{
  g_return_val_if_fail (TUMBLER_IS_PNG_THUMBNAIL_INFO (info), TUMBLER_THUMBNAIL_FORMAT_INVALID);
  return TUMBLER_PNG_THUMBNAIL_INFO (info)->priv->format;
}



static const gchar *
tumbler_png_thumbnail_info_get_uri (TumblerThumbnailInfo *info)
{
  g_return_val_if_fail (TUMBLER_IS_PNG_THUMBNAIL_INFO (info), NULL);
  return TUMBLER_PNG_THUMBNAIL_INFO (info)->priv->uri;
}
