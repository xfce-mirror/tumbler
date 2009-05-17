/* vi:set et ai sw=2 sts=2 ts=2: */
/*-
 * Copyright (c) 2009 Jannis Pohlmann <jannis@xfce.org>
 *
 * This program is free software; you can redistribute it and/or 
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of 
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public 
 * License along with this program; if not, write to the Free 
 * Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib.h>
#include <glib-object.h>

#include <tumbler/tumbler-thumbnailer.h>
#include <tumbler/tumbler-builtin-thumbnailer.h>



#define TUMBLER_BUILTIN_THUMBNAILER_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), TUMBLER_TYPE_BUILTIN_THUMBNAILER, TumblerBuiltinThumbnailerPrivate))



/* Property identifiers */
enum
{
  PROP_0,
  PROP_MIME_TYPES,
};



static void tumbler_builtin_thumbnailer_class_init   (TumblerBuiltinThumbnailerClass *klass);
static void tumbler_builtin_thumbnailer_init         (TumblerBuiltinThumbnailer      *thumbnailer);
static void tumbler_builtin_thumbnailer_iface_init   (TumblerThumbnailerIface        *iface);
static void tumbler_builtin_thumbnailer_constructed  (GObject                        *object);
static void tumbler_builtin_thumbnailer_finalize     (GObject                        *object);
static void tumbler_builtin_thumbnailer_get_property (GObject                        *object,
                                                      guint                           prop_id,
                                                      GValue                         *value,
                                                      GParamSpec                     *pspec);
static void tumbler_builtin_thumbnailer_set_property (GObject                        *object,
                                                      guint                           prop_id,
                                                      const GValue                   *value,
                                                      GParamSpec                     *pspec);
static void tumbler_builtin_thumbnailer_create       (TumblerThumbnailer             *thumbnailer,
                                                      const gchar                    *uri,
                                                      const gchar                    *mime_hint);



struct _TumblerBuiltinThumbnailerClass
{
  GObjectClass __parent__;
};

struct _TumblerBuiltinThumbnailer
{
  GObject __parent__;

  TumblerBuiltinThumbnailerPrivate *priv;
};

struct _TumblerBuiltinThumbnailerPrivate
{
  TumblerBuiltinThumbnailerFunc func;
  GStrv                         mime_types;
};



static GObjectClass *tumbler_builtin_thumbnailer_parent_class = NULL;



GType
tumbler_builtin_thumbnailer_get_type (void)
{
  static GType type = G_TYPE_INVALID;

  if (G_UNLIKELY (type == G_TYPE_INVALID))
    {
      static const GInterfaceInfo info = {
        (GInterfaceInitFunc) tumbler_builtin_thumbnailer_iface_init,
        NULL,
        NULL,
      };

      type = g_type_register_static_simple (G_TYPE_OBJECT, 
                                            "TumblerBuiltinThumbnailer",
                                            sizeof (TumblerBuiltinThumbnailerClass),
                                            (GClassInitFunc) tumbler_builtin_thumbnailer_class_init,
                                            sizeof (TumblerBuiltinThumbnailer),
                                            (GInstanceInitFunc) tumbler_builtin_thumbnailer_init,
                                            0);

      g_type_add_interface_static (type, TUMBLER_TYPE_THUMBNAILER, &info);
    }

  return type;
}



static void
tumbler_builtin_thumbnailer_class_init (TumblerBuiltinThumbnailerClass *klass)
{
  GObjectClass *gobject_class;

  g_type_class_add_private (klass, sizeof (TumblerBuiltinThumbnailerPrivate));

  /* Determine the parent type class */
  tumbler_builtin_thumbnailer_parent_class = g_type_class_peek_parent (klass);

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->finalize = tumbler_builtin_thumbnailer_finalize; 
  gobject_class->get_property = tumbler_builtin_thumbnailer_get_property;
  gobject_class->set_property = tumbler_builtin_thumbnailer_set_property;

  g_object_class_override_property (gobject_class, PROP_MIME_TYPES, "mime-types");
}



static void
tumbler_builtin_thumbnailer_iface_init (TumblerThumbnailerIface *iface)
{
  iface->create = tumbler_builtin_thumbnailer_create;
}



static void
tumbler_builtin_thumbnailer_init (TumblerBuiltinThumbnailer *thumbnailer)
{
  thumbnailer->priv = TUMBLER_BUILTIN_THUMBNAILER_GET_PRIVATE (thumbnailer);
  thumbnailer->priv->mime_types = NULL;
}



static void
tumbler_builtin_thumbnailer_finalize (GObject *object)
{
  TumblerBuiltinThumbnailer *thumbnailer = TUMBLER_BUILTIN_THUMBNAILER (object);

  g_strfreev (thumbnailer->priv->mime_types);

  (*G_OBJECT_CLASS (tumbler_builtin_thumbnailer_parent_class)->finalize) (object);
}



static void
tumbler_builtin_thumbnailer_get_property (GObject    *object,
                                          guint       prop_id,
                                          GValue     *value,
                                          GParamSpec *pspec)
{
  TumblerBuiltinThumbnailer *thumbnailer = TUMBLER_BUILTIN_THUMBNAILER (object);

  switch (prop_id)
    {
    case PROP_MIME_TYPES:
      g_value_set_pointer (value, thumbnailer->priv->mime_types);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}



static void
tumbler_builtin_thumbnailer_set_property (GObject      *object,
                                          guint         prop_id,
                                          const GValue *value,
                                          GParamSpec   *pspec)
{
  TumblerBuiltinThumbnailer *thumbnailer = TUMBLER_BUILTIN_THUMBNAILER (object);

  switch (prop_id)
    {
    case PROP_MIME_TYPES:
      if (g_value_get_pointer (value) != NULL)
        thumbnailer->priv->mime_types = g_strdupv (g_value_get_pointer (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}



static void
tumbler_builtin_thumbnailer_create (TumblerThumbnailer *thumbnailer,
                                    const gchar        *uri,
                                    const gchar        *mime_hint)
{
  /* TODO */
}



TumblerThumbnailer *
tumbler_builtin_thumbnailer_new (TumblerBuiltinThumbnailerFunc func,
                                 const GStrv                   mime_types)
{
  TumblerBuiltinThumbnailer *thumbnailer;

  g_return_val_if_fail (func != NULL, NULL);
  g_return_val_if_fail (mime_types != NULL, NULL);

  /* create the built-in thumbnailer */
  thumbnailer = g_object_new (TUMBLER_TYPE_BUILTIN_THUMBNAILER, 
                              "mime-types", mime_types, NULL);

  /* set the thumbnailer function */
  thumbnailer->priv->func = func;

  return TUMBLER_THUMBNAILER (thumbnailer);
}
