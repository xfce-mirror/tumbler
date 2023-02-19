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

#include <tumbler/tumbler.h>

#include <tumblerd/tumbler-cache-service.h>
#include <tumblerd/tumbler-cache-service-gdbus.h>
#include <tumblerd/tumbler-utils.h>

#define THUMBNAILER_CACHE_PATH    TUMBLER_SERVICE_PATH_PREFIX "/Cache1"
#define THUMBNAILER_CACHE_SERVICE TUMBLER_SERVICE_NAME_PREFIX ".Cache1"
#define THUMBNAILER_CACHE_IFACE   TUMBLER_SERVICE_NAME_PREFIX ".Cache1"

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



static gboolean tumbler_cache_service_cleanup (TumblerExportedCacheService   *skeleton,
                                               GDBusMethodInvocation         *invocation,
                                               const gchar *const            *base_uris,
                                               guint32                       since,
                                               TumblerCacheService           *service);
static gboolean tumbler_cache_service_delete  (TumblerExportedCacheService   *skeleton,
                                               GDBusMethodInvocation         *invocation,
                                               const gchar *const            *uris,
                                               TumblerCacheService           *service);

static gboolean tumbler_cache_service_copy    (TumblerExportedCacheService   *skeleton,
                                               GDBusMethodInvocation         *invocation,
                                               const gchar *const            *from_uris,
                                               const gchar *const            *to_uris,
                                               TumblerCacheService           *service);

static gboolean tumbler_cache_service_move    (TumblerExportedCacheService   *skeleton,
                                               GDBusMethodInvocation         *invocation,
                                               const gchar *const            *from_uris,
                                               const gchar *const            *to_uris,
                                               TumblerCacheService           *service);

struct _TumblerCacheService
{
  TumblerComponent            __parent__;

  GDBusConnection             *connection;
  TumblerExportedCacheService *skeleton;
  gboolean                     dbus_interface_exported;
 
  TumblerCache                *cache;

  GThreadPool                 *move_pool;
  GThreadPool                 *copy_pool;
  GThreadPool                 *delete_pool;
  GThreadPool                 *cleanup_pool;

  TUMBLER_MUTEX               (mutex);
};

struct _MoveRequest
{
  TumblerExportedCacheService  *skeleton;
  gchar                       **from_uris;
  gchar                       **to_uris;
  GDBusMethodInvocation        *invocation;
};

struct _CopyRequest
{
  TumblerExportedCacheService  *skeleton;
  gchar                       **from_uris;
  gchar                       **to_uris;
  GDBusMethodInvocation        *invocation;
};

struct _DeleteRequest
{
  TumblerExportedCacheService  *skeleton;
  gchar                       **uris;
  GDBusMethodInvocation        *invocation;
};

struct _CleanupRequest
{
  TumblerExportedCacheService  *skeleton;
  guint32                       since;
  gchar                       **base_uris;
  GDBusMethodInvocation        *invocation;  
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
                                   g_param_spec_object ("connection",
                                                         "connection",
                                                         "connection",
                                                         G_TYPE_DBUS_CONNECTION,
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
  GError *error = NULL;

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
  
  service->skeleton = tumbler_exported_cache_service_skeleton_new();
  
  /* everything's fine, install the cache type D-Bus info */
  g_dbus_interface_skeleton_export (G_DBUS_INTERFACE_SKELETON(service->skeleton),
                                    service->connection,
                                    THUMBNAILER_CACHE_PATH,
                                    &error);
  if (error != NULL)
    {
      g_critical ("error exporting thumbnail cache service on session bus: %s", error->message);
      g_error_free (error);
      service->dbus_interface_exported = FALSE; 
    }
  else
    {
      service->dbus_interface_exported = TRUE;
      
      g_signal_connect (service->skeleton, "handle-move",
                        G_CALLBACK(tumbler_cache_service_move), service);
      
      g_signal_connect (service->skeleton, "handle-copy",
                        G_CALLBACK(tumbler_cache_service_copy), service);
      
      g_signal_connect (service->skeleton, "handle-delete",
                        G_CALLBACK(tumbler_cache_service_delete), service);
      
      g_signal_connect (service->skeleton, "handle-cleanup",
                        G_CALLBACK(tumbler_cache_service_cleanup), service);
    }
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

  /* Unexport from dbus */
  if (service->dbus_interface_exported)
    g_dbus_interface_skeleton_unexport_from_connection 
      (
        G_DBUS_INTERFACE_SKELETON (service->skeleton),
        service->connection
      );

  /* release the Skeleton object */
  g_object_unref (service->skeleton);

  g_object_unref (service->connection);

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
      g_value_set_object (value, service->connection);
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
      service->connection = g_object_ref (g_value_get_object (value));
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
      g_debug ("Moving files in cache for moved source files");
      tumbler_util_dump_strvs_side_by_side (G_LOG_DOMAIN, "From URIs", "To URIs",
                                            (const gchar *const *) request->from_uris,
                                            (const gchar *const *) request->to_uris);

      tumbler_cache_move (service->cache, 
                          (const gchar *const *)request->from_uris, 
                          (const gchar *const *)request->to_uris);
    }

  tumbler_exported_cache_service_complete_move (request->skeleton, request->invocation);

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
      g_debug ("Copying files in cache for copied source files");
      tumbler_util_dump_strvs_side_by_side (G_LOG_DOMAIN, "From URIs", "To URIs",
                                            (const gchar *const *) request->from_uris,
                                            (const gchar *const *) request->to_uris);

      tumbler_cache_copy (service->cache, 
                          (const gchar *const *)request->from_uris, 
                          (const gchar *const *)request->to_uris);
    }

  tumbler_exported_cache_service_complete_copy (request->skeleton, request->invocation);

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
    {
      g_debug ("Removing files from cache for deleted source files");
      tumbler_util_dump_strv (G_LOG_DOMAIN, "URIs", (const gchar *const *) request->uris);

      tumbler_cache_delete (service->cache, (const gchar *const *)request->uris);
    }

  tumbler_exported_cache_service_complete_delete (request->skeleton, request->invocation);

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
      g_debug (request->since > 0 ? "Removing files older than %d from cache"
                                  : "Removing files from cache regardless of mtime%.0d",
               request->since);
      tumbler_util_dump_strv (G_LOG_DOMAIN, "URI schemes",
                              (const gchar *const *) request->base_uris);

      tumbler_cache_cleanup (service->cache, 
                             (const gchar *const *)request->base_uris, 
                             request->since);
    }

  tumbler_exported_cache_service_complete_cleanup (request->skeleton, request->invocation);

  g_strfreev (request->base_uris);
  g_slice_free (CleanupRequest, request);

  /* allow the lifecycle manager to shut down tumbler again (unless
   * other requests are still to be processed) */
  tumbler_component_decrement_use_count (TUMBLER_COMPONENT (service));

  tumbler_mutex_unlock (service->mutex);
}



TumblerCacheService *
tumbler_cache_service_new (GDBusConnection         *connection,
                           TumblerLifecycleManager *lifecycle_manager)
{
  return g_object_new (TUMBLER_TYPE_CACHE_SERVICE, 
                       "connection", connection, 
                       "lifecycle-manager", lifecycle_manager,
                       NULL);
}




gboolean
tumbler_cache_service_move (TumblerExportedCacheService   *skeleton,
                            GDBusMethodInvocation         *invocation,
                            const gchar *const            *from_uris,
                            const gchar *const            *to_uris,
                            TumblerCacheService           *service)
{
  MoveRequest *request;

  g_dbus_async_return_val_if_fail (TUMBLER_IS_CACHE_SERVICE (service), invocation, FALSE);
  g_dbus_async_return_val_if_fail (from_uris != NULL, invocation, FALSE);
  g_dbus_async_return_val_if_fail (to_uris != NULL, invocation, FALSE);
  g_dbus_async_return_val_if_fail (g_strv_length ((gchar **)from_uris) == g_strv_length ((gchar **)to_uris), invocation, FALSE);

  /* prevent the lifecycle manager to shut tumbler down before the
   * move request has been processed */
  tumbler_component_increment_use_count (TUMBLER_COMPONENT (service));

  request = g_slice_new0 (MoveRequest);
  request->from_uris = g_strdupv ((gchar **)from_uris);
  request->to_uris = g_strdupv ((gchar **)to_uris);
  request->invocation = invocation;

  g_thread_pool_push (service->move_pool, request, NULL);

  /* try to keep tumbler alive */
  tumbler_component_keep_alive (TUMBLER_COMPONENT (service), NULL);
  
  return TRUE;
}



gboolean
tumbler_cache_service_copy (TumblerExportedCacheService   *skeleton,
                            GDBusMethodInvocation         *invocation,
                            const gchar *const            *from_uris,
                            const gchar *const            *to_uris,
                            TumblerCacheService           *service)
{
  CopyRequest *request;
  
  g_dbus_async_return_val_if_fail (TUMBLER_IS_CACHE_SERVICE (service), invocation, FALSE);
  g_dbus_async_return_val_if_fail (from_uris != NULL, invocation, FALSE);
  g_dbus_async_return_val_if_fail (to_uris != NULL, invocation, FALSE);
  g_dbus_async_return_val_if_fail (g_strv_length ((gchar **)from_uris) == g_strv_length ((gchar **)to_uris), invocation, FALSE);
  
  /* prevent the lifecycle manager to shut tumbler down before the
   * copy request has been processed */
  tumbler_component_increment_use_count (TUMBLER_COMPONENT (service));

  request = g_slice_new0 (CopyRequest);
  request->from_uris = g_strdupv ((gchar **)from_uris);
  request->to_uris = g_strdupv ((gchar **)to_uris);
  request->invocation = invocation;

  g_thread_pool_push (service->copy_pool, request, NULL);

  /* try to keep tumbler alive */
  tumbler_component_keep_alive (TUMBLER_COMPONENT (service), NULL);
  
  return TRUE;
}



gboolean
tumbler_cache_service_delete (TumblerExportedCacheService   *skeleton,
                              GDBusMethodInvocation         *invocation,
                              const gchar *const            *uris,
                              TumblerCacheService           *service)
{
  DeleteRequest *request;
  
  g_dbus_async_return_val_if_fail (TUMBLER_IS_CACHE_SERVICE (service), invocation, FALSE);
  g_dbus_async_return_val_if_fail (uris != NULL, invocation, FALSE);

  /* prevent the lifecycle manager to shut tumbler down before the
   * delete request has been processed */
  tumbler_component_increment_use_count (TUMBLER_COMPONENT (service));

  request = g_slice_new0 (DeleteRequest);
  request->uris = g_strdupv ((gchar **)uris);
  request->invocation = invocation;

  g_thread_pool_push (service->delete_pool, request, NULL);

  /* try to keep tumbler alive */
  tumbler_component_keep_alive (TUMBLER_COMPONENT (service), NULL);
  
  return TRUE;
}



gboolean
tumbler_cache_service_cleanup (TumblerExportedCacheService   *skeleton,
                               GDBusMethodInvocation         *invocation,
                               const gchar *const            *base_uris,
                               guint32                       since,
                               TumblerCacheService           *service)
{
  CleanupRequest *request;

  g_dbus_async_return_val_if_fail (TUMBLER_IS_CACHE_SERVICE (service), invocation, FALSE);
  
  /* prevent the lifecycle manager to shut tumbler down before the
   * cleanup request has been processed */
  tumbler_component_increment_use_count (TUMBLER_COMPONENT (service));

  request = g_slice_new0 (CleanupRequest);
  request->base_uris = g_strdupv ((gchar **)base_uris);
  request->since = since;
  request->invocation = invocation;

  g_thread_pool_push (service->cleanup_pool, request, NULL);

  /* try to keep tumbler alive */
  tumbler_component_keep_alive (TUMBLER_COMPONENT (service), NULL);
  
  return TRUE;
}

gboolean tumbler_cache_service_is_exported (TumblerCacheService *service)
{
  g_return_val_if_fail (TUMBLER_IS_CACHE_SERVICE(service), FALSE);
  return service->dbus_interface_exported;
}
