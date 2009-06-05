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
#include <glib/gi18n.h>
#include <glib-object.h>

#include <dbus/dbus.h>
#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-lowlevel.h>

#include <tumblerd/tumbler-cache.h>
#include <tumblerd/tumbler-cache-dbus-bindings.h>



#define TUMBLER_CACHE_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), TUMBLER_TYPE_CACHE, TumblerCachePrivate))



/* Property identifiers */
enum
{
  PROP_0,
  PROP_CONNECTION,
};



static void tumbler_cache_class_init   (TumblerCacheClass *klass);
static void tumbler_cache_init         (TumblerCache      *cache);
static void tumbler_cache_constructed  (GObject           *object);
static void tumbler_cache_finalize     (GObject           *object);
static void tumbler_cache_get_property (GObject           *object,
                                        guint              prop_id,
                                        GValue            *value,
                                        GParamSpec        *pspec);
static void tumbler_cache_set_property (GObject           *object,
                                        guint              prop_id,
                                        const GValue      *value,
                                        GParamSpec        *pspec);



struct _TumblerCacheClass
{
  GObjectClass __parent__;
};

struct _TumblerCache
{
  GObject __parent__;

  TumblerCachePrivate *priv;
};

struct _TumblerCachePrivate
{
  DBusGConnection *connection;
  GMutex          *mutex;
};



static GObjectClass *tumbler_cache_parent_class = NULL;



GType
tumbler_cache_get_type (void)
{
  static GType type = G_TYPE_INVALID;

  if (G_UNLIKELY (type == G_TYPE_INVALID))
    {
      type = g_type_register_static_simple (G_TYPE_OBJECT, 
                                            "TumblerCache",
                                            sizeof (TumblerCacheClass),
                                            (GClassInitFunc) tumbler_cache_class_init,
                                            sizeof (TumblerCache),
                                            (GInstanceInitFunc) tumbler_cache_init,
                                            0);
    }

  return type;
}



static void
tumbler_cache_class_init (TumblerCacheClass *klass)
{
  GObjectClass *gobject_class;

  g_type_class_add_private (klass, sizeof (TumblerCachePrivate));

  /* Determine the parent type class */
  tumbler_cache_parent_class = g_type_class_peek_parent (klass);

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->constructed = tumbler_cache_constructed; 
  gobject_class->finalize = tumbler_cache_finalize; 
  gobject_class->get_property = tumbler_cache_get_property;
  gobject_class->set_property = tumbler_cache_set_property;

  g_object_class_install_property (gobject_class, PROP_CONNECTION,
                                   g_param_spec_pointer ("connection",
                                                         "connection",
                                                         "connection",
                                                         G_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT_ONLY));
}



static void
tumbler_cache_init (TumblerCache *cache)
{
  cache->priv = TUMBLER_CACHE_GET_PRIVATE (cache);
  cache->priv->mutex = g_mutex_new ();
}



static void
tumbler_cache_constructed (GObject *object)
{
#if 0
  TumblerCache *cache = TUMBLER_CACHE (object);
#endif
}



static void
tumbler_cache_finalize (GObject *object)
{
  TumblerCache *cache = TUMBLER_CACHE (object);

  dbus_g_connection_unref (cache->priv->connection);

  g_mutex_free (cache->priv->mutex);

  (*G_OBJECT_CLASS (tumbler_cache_parent_class)->finalize) (object);
}



static void
tumbler_cache_get_property (GObject    *object,
                            guint       prop_id,
                            GValue     *value,
                            GParamSpec *pspec)
{
  TumblerCache *cache = TUMBLER_CACHE (object);

  switch (prop_id)
    {
    case PROP_CONNECTION:
      g_value_set_pointer (value, cache->priv->connection);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}



static void
tumbler_cache_set_property (GObject      *object,
                            guint         prop_id,
                            const GValue *value,
                            GParamSpec   *pspec)
{
  TumblerCache *cache = TUMBLER_CACHE (object);

  switch (prop_id)
    {
    case PROP_CONNECTION:
      cache->priv->connection = g_value_get_pointer (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}



TumblerCache *
tumbler_cache_new (DBusGConnection *connection)
{
  return g_object_new (TUMBLER_TYPE_CACHE, "connection", connection, NULL);
}



gboolean
tumbler_cache_start (TumblerCache *cache,
                     GError      **error)
{
  DBusConnection *connection;
  DBusError       dbus_error;
  gint            result;

  g_return_val_if_fail (TUMBLER_IS_CACHE (cache), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  g_mutex_lock (cache->priv->mutex);

  /* initialize the D-Bus error */
  dbus_error_init (&dbus_error);

  /* get the native D-Bus connection */
  connection = dbus_g_connection_get_connection (cache->priv->connection);

  /* request ownership for the cache interface */
  result = dbus_bus_request_name (connection, "org.freedesktop.thumbnails.Cache", 
                                  DBUS_NAME_FLAG_DO_NOT_QUEUE, &dbus_error);

  /* check if that failed */
  if (result != DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER)
    {
      /* propagate the D-Bus error */
      if (dbus_error_is_set (&dbus_error))
        {
          if (error != NULL)
            dbus_set_g_error (error, &dbus_error);

          dbus_error_free (&dbus_error);
        }
      else if (error != NULL)
        {
          g_set_error (error, DBUS_GERROR, DBUS_GERROR_FAILED,
                       _("Another thumbnail cache service is already running"));
        }

      g_mutex_unlock (cache->priv->mutex);

      return FALSE;
    }
  
  /* everything's fine, install the cache type D-Bus info */
  dbus_g_object_type_install_info (G_OBJECT_TYPE (cache),
                                   &dbus_glib_tumbler_cache_object_info);

  /* register the cache instance as a handler of the cache interface */
  dbus_g_connection_register_g_object (cache->priv->connection, 
                                       "/org/freedesktop/thumbnails/Cache",
                                       G_OBJECT (cache));

  g_mutex_unlock (cache->priv->mutex);

  return TRUE;
}



void
tumbler_cache_move (TumblerCache          *cache,
                    const GStrv            from_uris,
                    const GStrv            to_uris,
                    DBusGMethodInvocation *context)
{
}



void
tumbler_cache_copy (TumblerCache          *cache,
                    const GStrv            from_uris,
                    const GStrv            to_uris,
                    DBusGMethodInvocation *context)
{
}



void
tumbler_cache_delete (TumblerCache          *cache,
                      const GStrv            uris,
                      DBusGMethodInvocation *context)
{
}



void
tumbler_cache_cleanup (TumblerCache          *cache,
                       const gchar           *uri_prefix,
                       guint32                since,
                       DBusGMethodInvocation *context)
{
}
