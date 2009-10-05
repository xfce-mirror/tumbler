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

#include <tumbler/tumbler.h>

#include <tumblerd/tumbler-specialized-thumbnailer.h>



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
  PROP_FOREIGN,
  PROP_MODIFIED,
};



static void tumbler_specialized_thumbnailer_iface_init      (TumblerThumbnailerIface       *iface);
static void tumbler_specialized_thumbnailer_constructed     (GObject                       *object);
static void tumbler_specialized_thumbnailer_finalize        (GObject                       *object);
static void tumbler_specialized_thumbnailer_get_property    (GObject                       *object,
                                                             guint                          prop_id,
                                                             GValue                        *value,
                                                             GParamSpec                    *pspec);
static void tumbler_specialized_thumbnailer_set_property    (GObject                       *object,
                                                             guint                          prop_id,
                                                             const GValue                  *value,
                                                             GParamSpec                    *pspec);
static void tumbler_specialized_thumbnailer_create          (TumblerThumbnailer            *thumbnailer,
                                                             const gchar                   *uri,
                                                             const gchar                   *mime_hint);
static void tumbler_specialized_thumbnailer_proxy_ready     (DBusGProxy                    *proxy,
                                                             const gchar                   *uri,
                                                             TumblerSpecializedThumbnailer *thumbnailer);
static void tumbler_specialized_thumbnailer_proxy_error     (DBusGProxy                    *proxy,
                                                             const gchar                   *uri,
                                                             gint                           error_code,
                                                             const gchar                   *message,
                                                             TumblerSpecializedThumbnailer *thumbnailer);
static void tumbler_specialized_thumbnailer_proxy_destroyed (DBusGProxy                    *proxy,
                                                             TumblerSpecializedThumbnailer *thumbnailer);



struct _TumblerSpecializedThumbnailerClass
{
  TumblerAbstractThumbnailerClass __parent__;
};

struct _TumblerSpecializedThumbnailer
{
  TumblerAbstractThumbnailer __parent__;

  DBusGConnection *connection;
  DBusGProxy      *proxy;

  gboolean         foreign;
  guint64          modified;

  gchar           *name;
};



G_DEFINE_TYPE_WITH_CODE (TumblerSpecializedThumbnailer, 
                         tumbler_specialized_thumbnailer,
                         TUMBLER_TYPE_ABSTRACT_THUMBNAILER,
                         G_IMPLEMENT_INTERFACE (TUMBLER_TYPE_THUMBNAILER,
                                                tumbler_specialized_thumbnailer_iface_init));



static void
tumbler_specialized_thumbnailer_class_init (TumblerSpecializedThumbnailerClass *klass)
{
  GObjectClass *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->constructed = tumbler_specialized_thumbnailer_constructed;
  gobject_class->finalize = tumbler_specialized_thumbnailer_finalize; 
  gobject_class->get_property = tumbler_specialized_thumbnailer_get_property;
  gobject_class->set_property = tumbler_specialized_thumbnailer_set_property;

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

  g_object_class_install_property (gobject_class,
                                   PROP_FOREIGN,
                                   g_param_spec_boolean ("foreign",
                                                         "foreign",
                                                         "foreign",
                                                         FALSE,
                                                         G_PARAM_CONSTRUCT_ONLY |
                                                         G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class,
                                   PROP_MODIFIED,
                                   g_param_spec_uint64 ("modified",
                                                        "modified",
                                                        "modified",
                                                        0, G_MAXUINT64, 0,
                                                        G_PARAM_CONSTRUCT_ONLY |
                                                        G_PARAM_READWRITE));
}



static void
tumbler_specialized_thumbnailer_iface_init (TumblerThumbnailerIface *iface)
{
  iface->create = tumbler_specialized_thumbnailer_create;
}



static void
tumbler_specialized_thumbnailer_init (TumblerSpecializedThumbnailer *thumbnailer)
{
}



static void
tumbler_specialized_thumbnailer_constructed (GObject *object)
{
  TumblerSpecializedThumbnailer *thumbnailer = TUMBLER_SPECIALIZED_THUMBNAILER (object);
  gchar                         *bus_path;

  g_return_if_fail (TUMBLER_SPECIALIZED_THUMBNAILER (thumbnailer));

  bus_path = g_strdup_printf ("/%s", thumbnailer->name);
  bus_path = g_strdelimit (bus_path, ".", '/');

  thumbnailer->proxy = dbus_g_proxy_new_for_name (thumbnailer->connection,
                                                  thumbnailer->name,
                                                  bus_path,
                                                  "org.xfce.thumbnailer.Thumbnailer");

  g_free (bus_path);

  dbus_g_proxy_add_signal (thumbnailer->proxy, "Ready", 
                           G_TYPE_STRING, G_TYPE_INVALID);

  dbus_g_proxy_add_signal (thumbnailer->proxy, "Error",
                           G_TYPE_STRING, G_TYPE_INT, G_TYPE_STRING, G_TYPE_INVALID);

  dbus_g_proxy_connect_signal (thumbnailer->proxy, "Ready",
                               G_CALLBACK (tumbler_specialized_thumbnailer_proxy_ready),
                               thumbnailer, NULL);

  dbus_g_proxy_connect_signal (thumbnailer->proxy, "Error",
                               G_CALLBACK (tumbler_specialized_thumbnailer_proxy_error),
                               thumbnailer, NULL);

  g_signal_connect (thumbnailer->proxy, "destroy",
                    G_CALLBACK (tumbler_specialized_thumbnailer_proxy_destroyed),
                    thumbnailer);
}



static void
tumbler_specialized_thumbnailer_finalize (GObject *object)
{
  TumblerSpecializedThumbnailer *thumbnailer = TUMBLER_SPECIALIZED_THUMBNAILER (object);

  g_free (thumbnailer->name);

  dbus_g_proxy_disconnect_signal (thumbnailer->proxy, "Ready",
                                  G_CALLBACK (tumbler_specialized_thumbnailer_proxy_ready),
                                  thumbnailer);

  dbus_g_proxy_disconnect_signal (thumbnailer->proxy, "Error",
                                  G_CALLBACK (tumbler_specialized_thumbnailer_proxy_error),
                                  thumbnailer);

  g_object_unref (thumbnailer->proxy);

  dbus_g_connection_unref (thumbnailer->connection);

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
    case PROP_CONNECTION:
      g_value_set_pointer (value, dbus_g_connection_ref (thumbnailer->connection));
      break;
    case PROP_NAME:
      g_value_set_string (value, thumbnailer->name);
      break;
    case PROP_PROXY:
      g_value_set_object (value, thumbnailer->proxy);
      break;
    case PROP_FOREIGN:
      g_value_set_boolean (value, thumbnailer->foreign);
      break;
    case PROP_MODIFIED:
      g_value_set_uint64 (value, thumbnailer->modified);
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
    case PROP_CONNECTION:
      thumbnailer->connection = dbus_g_connection_ref (g_value_get_pointer (value));
      break;
    case PROP_NAME:
      thumbnailer->name = g_value_dup_string (value);
      break;
    case PROP_FOREIGN:
      thumbnailer->foreign = g_value_get_boolean (value);
      break;
    case PROP_MODIFIED:
      thumbnailer->modified = g_value_get_uint64 (value);
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



static void
tumbler_specialized_thumbnailer_proxy_destroyed (DBusGProxy                    *proxy,
                                                 TumblerSpecializedThumbnailer *thumbnailer)
{
  g_return_if_fail (DBUS_IS_G_PROXY (proxy));
  g_return_if_fail (TUMBLER_IS_SPECIALIZED_THUMBNAILER (thumbnailer));

  g_signal_emit_by_name (thumbnailer, "unregister");
}



TumblerThumbnailer *
tumbler_specialized_thumbnailer_new_foreign (DBusGConnection *connection,
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
                              "connection", connection, "foreign", TRUE, "name", name, 
                              "uri-schemes", uri_schemes, "mime-types", mime_types,
                              NULL);

  return TUMBLER_THUMBNAILER (thumbnailer);
}



gboolean
tumbler_specialized_thumbnailer_get_foreign (TumblerSpecializedThumbnailer *thumbnailer)
{
  g_return_val_if_fail (TUMBLER_IS_SPECIALIZED_THUMBNAILER (thumbnailer), FALSE);
  return thumbnailer->foreign;
}



guint64
tumbler_specialized_thumbnailer_get_modified (TumblerSpecializedThumbnailer *thumbnailer)
{
  g_return_val_if_fail (TUMBLER_IS_SPECIALIZED_THUMBNAILER (thumbnailer), 0);
  return thumbnailer->modified;
}
