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

#include <tumbler/tumbler.h>

#include <tumblerd/tumbler-cache-service.h>
#include <tumblerd/tumbler-cache-service-dbus-bindings.h>
#include <tumblerd/tumbler-utils.h>



typedef struct _MoveRequest    MoveRequest;
typedef struct _CopyRequest    CopyRequest;
typedef struct _DeleteRequest  DeleteRequest;
typedef struct _CleanupRequest CleanupRequest;



#define TUMBLER_CACHE_SERVICE_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), TUMBLER_TYPE_CACHE_SERVICE, TumblerCacheServicePrivate))



/* Property identifiers */
enum
{
  PROP_0,
  PROP_CONNECTION,
};



static void tumbler_cache_service_class_init     (TumblerCacheServiceClass *klass);
static void tumbler_cache_service_init           (TumblerCacheService      *service);
static void tumbler_cache_service_constructed    (GObject                  *object);
static void tumbler_cache_service_finalize       (GObject                  *object);
static void tumbler_cache_service_get_property   (GObject                  *object,
                                                  guint                     prop_id,
                                                  GValue                   *value,
                                                  GParamSpec               *pspec);
static void tumbler_cache_service_set_property   (GObject                  *object,
                                                  guint                     prop_id,
                                                  const GValue             *value,
                                                  GParamSpec               *pspec);
static void tumbler_cache_service_move_thread    (gpointer                  data,
                                                  gpointer                  user_data);
static void tumbler_cache_service_copy_thread    (gpointer                  data,
                                                  gpointer                  user_data);
static void tumbler_cache_service_delete_thread  (gpointer                  data,
                                                  gpointer                  user_data);
static void tumbler_cache_service_cleanup_thread (gpointer                  data,
                                                  gpointer                  user_data);



struct _TumblerCacheServiceClass
{
  GObjectClass __parent__;
};

struct _TumblerCacheService
{
  GObject __parent__;

  TumblerCacheServicePrivate *priv;
};

struct _TumblerCacheServicePrivate
{
  DBusGConnection *connection;
  GThreadPool     *move_pool;
  GThreadPool     *copy_pool;
  GThreadPool     *delete_pool;
  GThreadPool     *cleanup_pool;
  GMutex          *mutex;
  GList           *caches;
};

struct _MoveRequest
{
  GStrv from_uris;
  GStrv to_uris;
};

struct _CopyRequest
{
  GStrv from_uris;
  GStrv to_uris;
};

struct _DeleteRequest
{
  GStrv uris;
};

struct _CleanupRequest
{
  guint32 since;
  gchar  *uri_prefix;
};



static GObjectClass *tumbler_cache_service_parent_class = NULL;



GType
tumbler_cache_service_get_type (void)
{
  static GType type = G_TYPE_INVALID;

  if (G_UNLIKELY (type == G_TYPE_INVALID))
    {
      type = g_type_register_static_simple (G_TYPE_OBJECT, 
                                            "TumblerCacheService",
                                            sizeof (TumblerCacheServiceClass),
                                            (GClassInitFunc) tumbler_cache_service_class_init,
                                            sizeof (TumblerCacheService),
                                            (GInstanceInitFunc) tumbler_cache_service_init,
                                            0);
    }

  return type;
}



static void
tumbler_cache_service_class_init (TumblerCacheServiceClass *klass)
{
  GObjectClass *gobject_class;

  g_type_class_add_private (klass, sizeof (TumblerCacheServicePrivate));

  /* Determine the parent type class */
  tumbler_cache_service_parent_class = g_type_class_peek_parent (klass);

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->constructed = tumbler_cache_service_constructed; 
  gobject_class->finalize = tumbler_cache_service_finalize; 
  gobject_class->get_property = tumbler_cache_service_get_property;
  gobject_class->set_property = tumbler_cache_service_set_property;

  g_object_class_install_property (gobject_class, PROP_CONNECTION,
                                   g_param_spec_pointer ("connection",
                                                         "connection",
                                                         "connection",
                                                         G_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT_ONLY));
}



static void
tumbler_cache_service_init (TumblerCacheService *service)
{
  service->priv = TUMBLER_CACHE_SERVICE_GET_PRIVATE (service);
  service->priv->mutex = g_mutex_new ();
}



static void
tumbler_cache_service_constructed (GObject *object)
{
  TumblerProviderFactory *factory;
  TumblerCacheService    *service = TUMBLER_CACHE_SERVICE (object);
  GList                  *caches;
  GList                  *lp;
  GList                  *providers;

  factory = tumbler_provider_factory_get_default ();
  providers = tumbler_provider_factory_get_providers (factory, 
                                                      TUMBLER_TYPE_CACHE_PROVIDER);
  g_object_unref (factory);

  service->priv->caches = NULL;

  for (lp = providers; lp != NULL; lp = lp->next)
    {
      caches = tumbler_cache_provider_get_caches (lp->data);
      service->priv->caches = g_list_concat (service->priv->caches, caches);
    }

  g_list_foreach (providers, (GFunc) g_object_unref, NULL);
  g_list_free (providers);

  service->priv->move_pool = g_thread_pool_new (tumbler_cache_service_move_thread, 
                                                service, 1, TRUE, NULL);
  service->priv->copy_pool = g_thread_pool_new (tumbler_cache_service_copy_thread, 
                                                service, 1, TRUE, NULL);
  service->priv->delete_pool = g_thread_pool_new (tumbler_cache_service_delete_thread,
                                                  service, 1, TRUE, NULL);
  service->priv->cleanup_pool = g_thread_pool_new (tumbler_cache_service_cleanup_thread, 
                                                   service, 1, TRUE, NULL);
}



static void
tumbler_cache_service_finalize (GObject *object)
{
  TumblerCacheService *service = TUMBLER_CACHE_SERVICE (object);

  g_thread_pool_free (service->priv->move_pool, TRUE, TRUE);
  g_thread_pool_free (service->priv->copy_pool, TRUE, TRUE);
  g_thread_pool_free (service->priv->delete_pool, TRUE, TRUE);
  g_thread_pool_free (service->priv->cleanup_pool, TRUE, TRUE);

  g_list_foreach (service->priv->caches, (GFunc) g_object_unref, NULL);
  g_list_free (service->priv->caches);

  dbus_g_connection_unref (service->priv->connection);

  g_mutex_free (service->priv->mutex);

  (*G_OBJECT_CLASS (tumbler_cache_service_parent_class)->finalize) (object);
}



static void
tumbler_cache_service_get_property (GObject    *object,
                                    guint       prop_id,
                                    GValue     *value,
                                    GParamSpec *pspec)
{
  TumblerCacheService *service = TUMBLER_CACHE_SERVICE (object);

  switch (prop_id)
    {
    case PROP_CONNECTION:
      g_value_set_pointer (value, service->priv->connection);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}



static void
tumbler_cache_service_set_property (GObject      *object,
                                    guint         prop_id,
                                    const GValue *value,
                                    GParamSpec   *pspec)
{
  TumblerCacheService *service = TUMBLER_CACHE_SERVICE (object);

  switch (prop_id)
    {
    case PROP_CONNECTION:
      service->priv->connection = dbus_g_connection_ref (g_value_get_pointer (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}



static void
tumbler_cache_service_move_thread (gpointer data,
                                   gpointer user_data)
{
  TumblerCacheService *service = TUMBLER_CACHE_SERVICE (user_data);
  MoveRequest         *request = data;
  GList               *lp;

  g_return_if_fail (TUMBLER_IS_CACHE_SERVICE (service));
  g_return_if_fail (request != NULL);

  g_mutex_lock (service->priv->mutex);

  for (lp = service->priv->caches; lp != NULL; lp = lp->next)
    tumbler_cache_move (lp->data, request->from_uris, request->to_uris);

  g_strfreev (request->from_uris);
  g_strfreev (request->to_uris);
  g_slice_free (MoveRequest, request);

  g_mutex_unlock (service->priv->mutex);
}



static void
tumbler_cache_service_copy_thread (gpointer data,
                                   gpointer user_data)
{
  TumblerCacheService *service = TUMBLER_CACHE_SERVICE (user_data);
  CopyRequest         *request = data;
  GList               *lp;

  g_return_if_fail (TUMBLER_IS_CACHE_SERVICE (service));
  g_return_if_fail (request != NULL);

  g_mutex_lock (service->priv->mutex);

  for (lp = service->priv->caches; lp != NULL; lp = lp->next)
    tumbler_cache_copy (lp->data, request->from_uris, request->to_uris);

  g_strfreev (request->from_uris);
  g_strfreev (request->to_uris);
  g_slice_free (CopyRequest, request);

  g_mutex_unlock (service->priv->mutex);
}



static void
tumbler_cache_service_delete_thread (gpointer data,
                                     gpointer user_data)
{
  TumblerCacheService *service = TUMBLER_CACHE_SERVICE (user_data);
  DeleteRequest       *request = data;
  GList               *lp;

  g_return_if_fail (TUMBLER_IS_CACHE_SERVICE (service));
  g_return_if_fail (request != NULL);

  g_mutex_lock (service->priv->mutex);

  for (lp = service->priv->caches; lp != NULL; lp = lp->next)
    tumbler_cache_delete (lp->data, request->uris);

  g_strfreev (request->uris);
  g_slice_free (DeleteRequest, request);

  g_mutex_unlock (service->priv->mutex);
}



static void
tumbler_cache_service_cleanup_thread (gpointer data,
                                      gpointer user_data)
{
  TumblerCacheService *service = TUMBLER_CACHE_SERVICE (user_data);
  CleanupRequest      *request = data;
  GList               *lp;

  g_return_if_fail (TUMBLER_IS_CACHE_SERVICE (service));
  g_return_if_fail (request != NULL);

  g_mutex_lock (service->priv->mutex);

  for (lp = service->priv->caches; lp != NULL; lp = lp->next)
    tumbler_cache_cleanup (lp->data, request->uri_prefix, request->since);

  g_free (request->uri_prefix);
  g_slice_free (CleanupRequest, request);

  g_mutex_unlock (service->priv->mutex);
}



TumblerCacheService *
tumbler_cache_service_new (DBusGConnection *connection)
{
  return g_object_new (TUMBLER_TYPE_CACHE_SERVICE, "connection", connection, NULL);
}



gboolean
tumbler_cache_service_start (TumblerCacheService *service,
                             GError             **error)
{
  DBusConnection *connection;
  DBusError       dbus_error;
  gint            result;

  g_return_val_if_fail (TUMBLER_IS_CACHE_SERVICE (service), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  g_mutex_lock (service->priv->mutex);

  /* initialize the D-Bus error */
  dbus_error_init (&dbus_error);

  /* get the native D-Bus connection */
  connection = dbus_g_connection_get_connection (service->priv->connection);

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

      g_mutex_unlock (service->priv->mutex);

      return FALSE;
    }
  
  /* everything's fine, install the cache type D-Bus info */
  dbus_g_object_type_install_info (G_OBJECT_TYPE (service),
                                   &dbus_glib_tumbler_cache_service_object_info);

  /* register the cache instance as a handler of the cache interface */
  dbus_g_connection_register_g_object (service->priv->connection, 
                                       "/org/freedesktop/thumbnails/Cache",
                                       G_OBJECT (service));

  g_mutex_unlock (service->priv->mutex);

  return TRUE;
}



void
tumbler_cache_service_move (TumblerCacheService   *service,
                            const GStrv            from_uris,
                            const GStrv            to_uris,
                            DBusGMethodInvocation *context)
{
  MoveRequest *request;

  dbus_async_return_if_fail (TUMBLER_IS_CACHE_SERVICE (service), context);
  dbus_async_return_if_fail (from_uris != NULL, context);
  dbus_async_return_if_fail (to_uris != NULL, context);
  dbus_async_return_if_fail (g_strv_length (from_uris) == g_strv_length (to_uris), context);

  request = g_slice_new0 (MoveRequest);
  request->from_uris = g_strdupv (from_uris);
  request->to_uris = g_strdupv (to_uris);

  g_thread_pool_push (service->priv->move_pool, request, NULL);

  dbus_g_method_return (context);
}



void
tumbler_cache_service_copy (TumblerCacheService   *service,
                            const GStrv            from_uris,
                            const GStrv            to_uris,
                            DBusGMethodInvocation *context)
{
  CopyRequest *request;

  dbus_async_return_if_fail (TUMBLER_IS_CACHE_SERVICE (service), context);
  dbus_async_return_if_fail (from_uris != NULL, context);
  dbus_async_return_if_fail (to_uris != NULL, context);
  dbus_async_return_if_fail (g_strv_length (from_uris) == g_strv_length (to_uris), context);

  request = g_slice_new0 (CopyRequest);
  request->from_uris = g_strdupv (from_uris);
  request->to_uris = g_strdupv (to_uris);

  g_thread_pool_push (service->priv->copy_pool, request, NULL);

  dbus_g_method_return (context);
}



void
tumbler_cache_service_delete (TumblerCacheService   *service,
                              const GStrv            uris,
                              DBusGMethodInvocation *context)
{
  DeleteRequest *request;

  dbus_async_return_if_fail (TUMBLER_IS_CACHE_SERVICE (service), context);
  dbus_async_return_if_fail (uris != NULL, context);

  request = g_slice_new0 (DeleteRequest);
  request->uris = g_strdupv (uris);

  g_thread_pool_push (service->priv->delete_pool, request, NULL);

  dbus_g_method_return (context);
}



void
tumbler_cache_service_cleanup (TumblerCacheService   *service,
                               const gchar           *uri_prefix,
                               guint32                since,
                               DBusGMethodInvocation *context)
{
  CleanupRequest *request;

  dbus_async_return_if_fail (TUMBLER_IS_CACHE_SERVICE (service), context);

  request = g_slice_new0 (CleanupRequest);
  request->uri_prefix = g_strdup (uri_prefix);
  request->since = since;

  g_thread_pool_push (service->priv->cleanup_pool, request, NULL);

  dbus_g_method_return (context);
}
