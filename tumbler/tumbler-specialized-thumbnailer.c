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
#include <tumbler/tumbler-specialized-thumbnailer.h>



#define TUMBLER_SPECIALIZED_THUMBNAILER_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), TUMBLER_TYPE_SPECIALIZED_THUMBNAILER, TumblerSpecializedThumbnailerPrivate))



/* Property identifiers */
enum
{
  PROP_0,
  PROP_MIME_TYPES,
  PROP_URI_SCHEMES,
  PROP_HASH_KEYS,
  PROP_NAME,
  PROP_CONNECTION,
  PROP_PROXY,
};



static void tumbler_specialized_thumbnailer_class_init   (TumblerSpecializedThumbnailerClass *klass);
static void tumbler_specialized_thumbnailer_init         (TumblerSpecializedThumbnailer      *thumbnailer);
static void tumbler_specialized_thumbnailer_iface_init   (TumblerThumbnailerIface            *iface);
static void tumbler_specialized_thumbnailer_constructed  (GObject                            *object);
static void tumbler_specialized_thumbnailer_finalize     (GObject                            *object);
static void tumbler_specialized_thumbnailer_get_property (GObject                            *object,
                                                          guint                               prop_id,
                                                          GValue                             *value,
                                                          GParamSpec                         *pspec);
static void tumbler_specialized_thumbnailer_set_property (GObject                            *object,
                                                          guint                               prop_id,
                                                          const GValue                       *value,
                                                          GParamSpec                         *pspec);
static void tumbler_specialized_thumbnailer_create       (TumblerThumbnailer                 *thumbnailer,
                                                          const gchar                        *uri,
                                                          const gchar                        *mime_hint);
static void tumbler_specialized_thumbnailer_proxy_ready  (DBusGProxy                         *proxy,
                                                          const gchar                        *uri,
                                                          TumblerSpecializedThumbnailer      *thumbnailer);
static void tumbler_specialized_thumbnailer_proxy_error  (DBusGProxy                         *proxy,
                                                          const gchar                        *uri,
                                                          gint                                error_code,
                                                          const gchar                        *message,
                                                          TumblerSpecializedThumbnailer      *thumbnailer);



struct _TumblerSpecializedThumbnailerClass
{
  GObjectClass __parent__;
};

struct _TumblerSpecializedThumbnailer
{
  GObject __parent__;

  TumblerSpecializedThumbnailerPrivate *priv;
};

struct _TumblerSpecializedThumbnailerPrivate
{
  DBusGConnection *connection;
  DBusGProxy      *proxy;

  gchar           *name;

  GStrv            uri_schemes;
  GStrv            hash_keys;
  GStrv            mime_types;
};



static GObjectClass *tumbler_specialized_thumbnailer_parent_class = NULL;



GType
tumbler_specialized_thumbnailer_get_type (void)
{
  static GType type = G_TYPE_INVALID;

  if (G_UNLIKELY (type == G_TYPE_INVALID))
    {
      static const GInterfaceInfo info = {
        (GInterfaceInitFunc) tumbler_specialized_thumbnailer_iface_init,
        NULL,
        NULL,
      };

      type = g_type_register_static_simple (G_TYPE_OBJECT, 
                                            "TumblerSpecializedThumbnailer",
                                            sizeof (TumblerSpecializedThumbnailerClass),
                                            (GClassInitFunc) tumbler_specialized_thumbnailer_class_init,
                                            sizeof (TumblerSpecializedThumbnailer),
                                            (GInstanceInitFunc) tumbler_specialized_thumbnailer_init,
                                            0);

      g_type_add_interface_static (type, TUMBLER_TYPE_THUMBNAILER, &info);
    }

  return type;
}



static void
tumbler_specialized_thumbnailer_class_init (TumblerSpecializedThumbnailerClass *klass)
{
  GObjectClass *gobject_class;

  g_type_class_add_private (klass, sizeof (TumblerSpecializedThumbnailerPrivate));

  /* Determine the parent type class */
  tumbler_specialized_thumbnailer_parent_class = g_type_class_peek_parent (klass);

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->constructed = tumbler_specialized_thumbnailer_constructed;
  gobject_class->finalize = tumbler_specialized_thumbnailer_finalize; 
  gobject_class->get_property = tumbler_specialized_thumbnailer_get_property;
  gobject_class->set_property = tumbler_specialized_thumbnailer_set_property;

  g_object_class_override_property (gobject_class, PROP_MIME_TYPES, "mime-types");
  g_object_class_override_property (gobject_class, PROP_URI_SCHEMES, "uri-schemes");
  g_object_class_override_property (gobject_class, PROP_HASH_KEYS, "hash-keys");

  g_object_class_install_property (gobject_class,
                                   PROP_NAME,
                                   g_param_spec_string ("name",
                                                        "name",
                                                        "name",
                                                        NULL,
                                                        G_PARAM_CONSTRUCT_ONLY |
                                                        G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class,
                                   PROP_CONNECTION,
                                   g_param_spec_pointer ("connection",
                                                         "connection",
                                                         "connection",
                                                         G_PARAM_CONSTRUCT_ONLY |
                                                         G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class,
                                   PROP_PROXY,
                                   g_param_spec_object ("proxy",
                                                        "proxy",
                                                        "proxy",
                                                        DBUS_TYPE_G_PROXY,
                                                        G_PARAM_READABLE));
}



static void
tumbler_specialized_thumbnailer_iface_init (TumblerThumbnailerIface *iface)
{
  iface->create = tumbler_specialized_thumbnailer_create;
}



static void
tumbler_specialized_thumbnailer_init (TumblerSpecializedThumbnailer *thumbnailer)
{
  thumbnailer->priv = TUMBLER_SPECIALIZED_THUMBNAILER_GET_PRIVATE (thumbnailer);
  thumbnailer->priv->mime_types = NULL;
}



static void
tumbler_specialized_thumbnailer_constructed (GObject *object)
{
  TumblerSpecializedThumbnailer *thumbnailer = TUMBLER_SPECIALIZED_THUMBNAILER (object);
  gchar                         *bus_path;

  g_return_if_fail (TUMBLER_SPECIALIZED_THUMBNAILER (thumbnailer));

  bus_path = g_strdup_printf ("/%s", thumbnailer->priv->name);
  bus_path = g_strdelimit (bus_path, ".", '/');

  thumbnailer->priv->proxy = dbus_g_proxy_new_for_name (thumbnailer->priv->connection,
                                                        thumbnailer->priv->name,
                                                        bus_path,
                                                        "org.freedesktop.thumbnailer.Thumbnailer");

  g_free (bus_path);

  dbus_g_proxy_add_signal (thumbnailer->priv->proxy, "Ready", 
                           G_TYPE_STRING, G_TYPE_INVALID);

  dbus_g_proxy_add_signal (thumbnailer->priv->proxy, "Error",
                           G_TYPE_STRING, G_TYPE_INT, G_TYPE_STRING, G_TYPE_INVALID);

  dbus_g_proxy_connect_signal (thumbnailer->priv->proxy, "Ready",
                               G_CALLBACK (tumbler_specialized_thumbnailer_proxy_ready),
                               thumbnailer, NULL);

  dbus_g_proxy_connect_signal (thumbnailer->priv->proxy, "Error",
                               G_CALLBACK (tumbler_specialized_thumbnailer_proxy_error),
                               thumbnailer, NULL);

  /* TODO make sure to handle the destroy signal of the proxy properly */
}



static void
tumbler_specialized_thumbnailer_finalize (GObject *object)
{
  TumblerSpecializedThumbnailer *thumbnailer = TUMBLER_SPECIALIZED_THUMBNAILER (object);

  g_free (thumbnailer->priv->name);

  g_strfreev (thumbnailer->priv->hash_keys);
  g_strfreev (thumbnailer->priv->mime_types);
  g_strfreev (thumbnailer->priv->uri_schemes);

  g_object_unref (thumbnailer->priv->proxy);

  dbus_g_connection_unref (thumbnailer->priv->connection);

  (*G_OBJECT_CLASS (tumbler_specialized_thumbnailer_parent_class)->finalize) (object);
}



static void
tumbler_specialized_thumbnailer_get_property (GObject    *object,
                                              guint       prop_id,
                                              GValue     *value,
                                              GParamSpec *pspec)
{
  TumblerSpecializedThumbnailer *thumbnailer = TUMBLER_SPECIALIZED_THUMBNAILER (object);

  switch (prop_id)
    {
    case PROP_MIME_TYPES:
      g_value_set_pointer (value, g_strdupv (thumbnailer->priv->mime_types));
      break;
    case PROP_URI_SCHEMES:
      g_value_set_pointer (value, g_strdupv (thumbnailer->priv->uri_schemes));
      break;
    case PROP_CONNECTION:
      g_value_set_pointer (value, dbus_g_connection_ref (thumbnailer->priv->connection));
      break;
    case PROP_NAME:
      g_value_set_string (value, thumbnailer->priv->name);
      break;
    case PROP_PROXY:
      g_value_set_object (value, thumbnailer->priv->proxy);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}



static void
tumbler_specialized_thumbnailer_set_property (GObject      *object,
                                              guint         prop_id,
                                              const GValue *value,
                                              GParamSpec   *pspec)
{
  TumblerSpecializedThumbnailer *thumbnailer = TUMBLER_SPECIALIZED_THUMBNAILER (object);

  switch (prop_id)
    {
    case PROP_MIME_TYPES:
      if (g_value_get_pointer (value) != NULL)
        thumbnailer->priv->mime_types = g_strdupv (g_value_get_pointer (value));
      break;
    case PROP_URI_SCHEMES:
      thumbnailer->priv->uri_schemes = g_strdupv (g_value_get_pointer (value));
      break;
    case PROP_CONNECTION:
      thumbnailer->priv->connection = dbus_g_connection_ref (g_value_get_pointer (value));
      break;
    case PROP_NAME:
      thumbnailer->priv->name = g_value_dup_string (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}



static void
tumbler_specialized_thumbnailer_create (TumblerThumbnailer *thumbnailer,
                                        const gchar        *uri,
                                        const gchar        *mime_hint)
{
  /* TODO */
}



static void
tumbler_specialized_thumbnailer_proxy_ready (DBusGProxy                    *proxy,
                                             const gchar                   *uri,
                                             TumblerSpecializedThumbnailer *thumbnailer)
{
  g_return_if_fail (DBUS_IS_G_PROXY (proxy));
  g_return_if_fail (uri != NULL);
  g_return_if_fail (TUMBLER_IS_SPECIALIZED_THUMBNAILER (thumbnailer));

  g_signal_emit_by_name (thumbnailer, "ready", uri);
}



static void
tumbler_specialized_thumbnailer_proxy_error (DBusGProxy                    *proxy,
                                             const gchar                   *uri,
                                             gint                           error_code,
                                             const gchar                   *message,
                                             TumblerSpecializedThumbnailer *thumbnailer)
{
  g_return_if_fail (DBUS_IS_G_PROXY (proxy));
  g_return_if_fail (uri != NULL);
  g_return_if_fail (message != NULL);
  g_return_if_fail (TUMBLER_IS_SPECIALIZED_THUMBNAILER (thumbnailer));

  g_signal_emit_by_name (thumbnailer, "error", uri, error_code, message);
}



TumblerThumbnailer *
tumbler_specialized_thumbnailer_new (DBusGConnection *connection,
                                     const gchar     *name,
                                     const gchar     *uri_scheme,
                                     const gchar     *mime_type)
{
  TumblerSpecializedThumbnailer *thumbnailer;
  const gchar                   *uri_schemes[2] = { uri_scheme, NULL };
  const gchar                   *mime_types[2] = { mime_type, NULL };

  g_return_val_if_fail (connection != NULL, NULL);
  g_return_val_if_fail (name != NULL, NULL);
  g_return_val_if_fail (uri_scheme != NULL, NULL);
  g_return_val_if_fail (mime_type != NULL, NULL);

  thumbnailer = g_object_new (TUMBLER_TYPE_SPECIALIZED_THUMBNAILER, 
                              "connection", connection, "name", name, 
                              "uri-schemes", uri_schemes, "mime-types", mime_types,
                              NULL);

  return TUMBLER_THUMBNAILER (thumbnailer);
}
