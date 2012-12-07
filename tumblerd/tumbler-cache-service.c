/* vi:set et ai sw=2 sts=2 ts=2: */
/*-
 * Copyright (c) 2009-2011 Jannis Pohlmann <jannis@xfce.org>
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

#include <tumblerd/tumbler-component.h>
#include <tumblerd/tumbler-cache-service.h>
#include <tumblerd/tumbler-cache-service-dbus-bindings.h>
#include <tumblerd/tumbler-utils.h>



typedef struct _MoveRequest    MoveRequest;
typedef struct _CopyRequest    CopyRequest;
typedef struct _DeleteRequest  DeleteRequest;
typedef struct _CleanupRequest CleanupRequest;



/* Property identifiers */
enum
{
  PROP_0,
  PROP_CONNECTION,
};



static void tumbler_cache_service_constructed    (GObject      *object);
static void tumbler_cache_service_finalize       (GObject      *object);
static void tumbler_cache_service_get_property   (GObject      *object,
                                                  guint         prop_id,
                                                  GValue       *value,
                                                  GParamSpec   *pspec);
static void tumbler_cache_service_set_property   (GObject      *object,
                                                  guint         prop_id,
                                                  const GValue *value,
                                                  GParamSpec   *pspec);
static void tumbler_cache_service_move_thread    (gpointer      data,
                                                  gpointer      user_data);
static void tumbler_cache_service_copy_thread    (gpointer      data,
                                                  gpointer      user_data);
static void tumbler_cache_service_delete_thread  (gpointer      data,
                                                  gpointer      user_data);
static void tumbler_cache_service_cleanup_thread (gpointer      data,
                                                  gpointer      user_data);



struct _TumblerCacheServiceClass
{
  TumblerComponentClass __parent__;
};

struct _TumblerCacheService
{
  TumblerComponent __parent__;

  DBusGConnection *connection;

  TumblerCache    *cache;

  GThreadPool     *move_pool;
  GThreadPool     *copy_pool;
  GThreadPool     *delete_pool;
  GThreadPool     *cleanup_pool;

  TUMBLER_MUTEX    (mutex);
};

struct _MoveRequest
{
  gchar **from_uris;
  gchar **to_uris;
};

struct _CopyRequest
{
  gchar **from_uris;
  gchar **to_uris;
};

struct _DeleteRequest
{
  gchar **uris;
};

struct _CleanupRequest
{
  guint32 since;
  gchar **base_uris;
};



G_DEFINE_TYPE (TumblerCacheService, tumbler_cache_service, TUMBLER_TYPE_COMPONENT);



static void
tumbler_cache_service_class_init (TumblerCacheServiceClass *klass)
{
  GObjectClass *gobject_class;

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
  tumbler_mutex_create (service->mutex);
}



static void
tumbler_cache_service_constructed (GObject *object)
{
  TumblerCacheService *service = TUMBLER_CACHE_SERVICE (object);

  /* chain up to parent classes */
  if (G_OBJECT_CLASS (tumbler_cache_service_parent_class)->constructed != NULL)
    (G_OBJECT_CLASS (tumbler_cache_service_parent_class)->constructed) (object);

  service->cache = tumbler_cache_get_default ();

  service->move_pool = g_thread_pool_new (tumbler_cache_service_move_thread, 
                                          service, 1, FALSE, NULL);
  service->copy_pool = g_thread_pool_new (tumbler_cache_service_copy_thread, 
                                          service, 1, FALSE, NULL);
  service->delete_pool = g_thread_pool_new (tumbler_cache_service_delete_thread,
                                            service, 1, FALSE, NULL);
  service->cleanup_pool = g_thread_pool_new (tumbler_cache_service_cleanup_thread, 
                                             service, 1, FALSE, NULL);

  /* everything's fine, install the cache type D-Bus info */
  dbus_g_object_type_install_info (G_OBJECT_TYPE (service),
                                   &dbus_glib_tumbler_cache_service_object_info);

  /* register the cache instance as a handler of the cache interface */
  dbus_g_connection_register_g_object (service->connection, 
                                       "/org/freedesktop/thumbnails/Cache1",
                                       G_OBJECT (service));
}



static void
tumbler_cache_service_finalize (GObject *object)
{
  TumblerCacheService *service = TUMBLER_CACHE_SERVICE (object);

  g_thread_pool_free (service->move_pool, TRUE, TRUE);
  g_thread_pool_free (service->copy_pool, TRUE, TRUE);
  g_thread_pool_free (service->delete_pool, TRUE, TRUE);
  g_thread_pool_free (service->cleanup_pool, TRUE, TRUE);

  if (service->cache != NULL)
    g_object_unref (service->cache);

  dbus_g_connection_unref (service->connection);

  tumbler_mutex_free (service->mutex);

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
      g_value_set_pointer (value, service->connection);
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
      service->connection = dbus_g_connection_ref (g_value_get_pointer (value));
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

  g_return_if_fail (TUMBLER_IS_CACHE_SERVICE (service));
  g_return_if_fail (request != NULL);

  tumbler_mutex_lock (service->mutex);

  if (service->cache != NULL)
    {
      tumbler_cache_move (service->cache, 
                          (const gchar *const *)request->from_uris, 
                          (const gchar *const *)request->to_uris);
    }

  g_strfreev (request->from_uris);
  g_strfreev (request->to_uris);
  g_slice_free (MoveRequest, request);

  /* allow the lifecycle manager to shut down tumbler again (unless
   * other requests are still to be processed) */
  tumbler_component_decrement_use_count (TUMBLER_COMPONENT (service));

  tumbler_mutex_unlock (service->mutex);
}



static void
tumbler_cache_service_copy_thread (gpointer data,
                                   gpointer user_data)
{
  TumblerCacheService *service = TUMBLER_CACHE_SERVICE (user_data);
  CopyRequest         *request = data;

  g_return_if_fail (TUMBLER_IS_CACHE_SERVICE (service));
  g_return_if_fail (request != NULL);

  tumbler_mutex_lock (service->mutex);

  if (service->cache != NULL)
    {
      tumbler_cache_copy (service->cache, 
                          (const gchar *const *)request->from_uris, 
                          (const gchar *const *)request->to_uris);
    }

  g_strfreev (request->from_uris);
  g_strfreev (request->to_uris);
  g_slice_free (CopyRequest, request);

  /* allow the lifecycle manager to shut down tumbler again (unless
   * other requests are still to be processed) */
  tumbler_component_decrement_use_count (TUMBLER_COMPONENT (service));

  tumbler_mutex_unlock (service->mutex);
}



static void
tumbler_cache_service_delete_thread (gpointer data,
                                     gpointer user_data)
{
  TumblerCacheService *service = TUMBLER_CACHE_SERVICE (user_data);
  DeleteRequest       *request = data;

  g_return_if_fail (TUMBLER_IS_CACHE_SERVICE (service));
  g_return_if_fail (request != NULL);

  tumbler_mutex_lock (service->mutex);

  if (service->cache != NULL)
    tumbler_cache_delete (service->cache, (const gchar *const *)request->uris);

  g_strfreev (request->uris);
  g_slice_free (DeleteRequest, request);

  /* allow the lifecycle manager to shut down tumbler again (unless
   * other requests are still to be processed) */
  tumbler_component_decrement_use_count (TUMBLER_COMPONENT (service));

  tumbler_mutex_unlock (service->mutex);
}



static void
tumbler_cache_service_cleanup_thread (gpointer data,
                                      gpointer user_data)
{
  TumblerCacheService *service = TUMBLER_CACHE_SERVICE (user_data);
  CleanupRequest      *request = data;

  g_return_if_fail (TUMBLER_IS_CACHE_SERVICE (service));
  g_return_if_fail (request != NULL);

  tumbler_mutex_lock (service->mutex);

  if (service->cache != NULL)
    {
      tumbler_cache_cleanup (service->cache, 
                             (const gchar *const *)request->base_uris, 
                             request->since);
    }

  g_strfreev (request->base_uris);
  g_slice_free (CleanupRequest, request);

  /* allow the lifecycle manager to shut down tumbler again (unless
   * other requests are still to be processed) */
  tumbler_component_decrement_use_count (TUMBLER_COMPONENT (service));

  tumbler_mutex_unlock (service->mutex);
}



TumblerCacheService *
tumbler_cache_service_new (DBusGConnection         *connection,
                           TumblerLifecycleManager *lifecycle_manager)
{
  return g_object_new (TUMBLER_TYPE_CACHE_SERVICE, 
                       "connection", connection, 
                       "lifecycle-manager", lifecycle_manager,
                       NULL);
}



gboolean
tumbler_cache_service_start (TumblerCacheService *service,
                             GError             **error)
{
  DBusConnection *connection;
  gint            result;

  g_return_val_if_fail (TUMBLER_IS_CACHE_SERVICE (service), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  tumbler_mutex_lock (service->mutex);

  /* get the native D-Bus connection */
  connection = dbus_g_connection_get_connection (service->connection);

  /* request ownership for the cache interface */
  result = dbus_bus_request_name (connection, "org.freedesktop.thumbnails.Cache1", 
                                  DBUS_NAME_FLAG_DO_NOT_QUEUE, NULL);

  /* check if that failed */
  if (result != DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER)
    {
      if (error != NULL)
        {
          g_set_error (error, DBUS_GERROR, DBUS_GERROR_FAILED,
                       _("Another thumbnail cache service is already running"));
        }

      tumbler_mutex_unlock (service->mutex);

      return FALSE;
    }
  
  tumbler_mutex_unlock (service->mutex);

  return TRUE;
}



void
tumbler_cache_service_move (TumblerCacheService   *service,
                            const gchar *const    *from_uris,
                            const gchar *const    *to_uris,
                            DBusGMethodInvocation *context)
{
  MoveRequest *request;

  dbus_async_return_if_fail (TUMBLER_IS_CACHE_SERVICE (service), context);
  dbus_async_return_if_fail (from_uris != NULL, context);
  dbus_async_return_if_fail (to_uris != NULL, context);
  dbus_async_return_if_fail (g_strv_length ((gchar **)from_uris) == g_strv_length ((gchar **)to_uris), context);

  /* prevent the lifecycle manager to shut tumbler down before the
   * move request has been processed */
  tumbler_component_increment_use_count (TUMBLER_COMPONENT (service));

  request = g_slice_new0 (MoveRequest);
  request->from_uris = g_strdupv ((gchar **)from_uris);
  request->to_uris = g_strdupv ((gchar **)to_uris);

  g_thread_pool_push (service->move_pool, request, NULL);

  dbus_g_method_return (context);

  /* try to keep tumbler alive */
  tumbler_component_keep_alive (TUMBLER_COMPONENT (service), NULL);
}



void
tumbler_cache_service_copy (TumblerCacheService   *service,
                            const gchar *const    *from_uris,
                            const gchar *const    *to_uris,
                            DBusGMethodInvocation *context)
{
  CopyRequest *request;

  dbus_async_return_if_fail (TUMBLER_IS_CACHE_SERVICE (service), context);
  dbus_async_return_if_fail (from_uris != NULL, context);
  dbus_async_return_if_fail (to_uris != NULL, context);
  dbus_async_return_if_fail (g_strv_length ((gchar **)from_uris) == g_strv_length ((gchar **)to_uris), context);

  /* prevent the lifecycle manager to shut tumbler down before the
   * copy request has been processed */
  tumbler_component_increment_use_count (TUMBLER_COMPONENT (service));

  request = g_slice_new0 (CopyRequest);
  request->from_uris = g_strdupv ((gchar **)from_uris);
  request->to_uris = g_strdupv ((gchar **)to_uris);

  g_thread_pool_push (service->copy_pool, request, NULL);

  dbus_g_method_return (context);

  /* try to keep tumbler alive */
  tumbler_component_keep_alive (TUMBLER_COMPONENT (service), NULL);
}



void
tumbler_cache_service_delete (TumblerCacheService   *service,
                              const gchar *const    *uris,
                              DBusGMethodInvocation *context)
{
  DeleteRequest *request;

  dbus_async_return_if_fail (TUMBLER_IS_CACHE_SERVICE (service), context);
  dbus_async_return_if_fail (uris != NULL, context);

  /* prevent the lifecycle manager to shut tumbler down before the
   * delete request has been processed */
  tumbler_component_increment_use_count (TUMBLER_COMPONENT (service));

  request = g_slice_new0 (DeleteRequest);
  request->uris = g_strdupv ((gchar **)uris);

  g_thread_pool_push (service->delete_pool, request, NULL);

  dbus_g_method_return (context);

  /* try to keep tumbler alive */
  tumbler_component_keep_alive (TUMBLER_COMPONENT (service), NULL);
}



void
tumbler_cache_service_cleanup (TumblerCacheService   *service,
                               const gchar *const    *base_uris,
                               guint32                since,
                               DBusGMethodInvocation *context)
{
  CleanupRequest *request;

  dbus_async_return_if_fail (TUMBLER_IS_CACHE_SERVICE (service), context);

  /* prevent the lifecycle manager to shut tumbler down before the
   * cleanup request has been processed */
  tumbler_component_increment_use_count (TUMBLER_COMPONENT (service));

  request = g_slice_new0 (CleanupRequest);
  request->base_uris = g_strdupv ((gchar **)base_uris);
  request->since = since;

  g_thread_pool_push (service->cleanup_pool, request, NULL);

  dbus_g_method_return (context);

  /* try to keep tumbler alive */
  tumbler_component_keep_alive (TUMBLER_COMPONENT (service), NULL);
}
