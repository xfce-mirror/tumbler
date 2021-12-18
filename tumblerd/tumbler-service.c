/* vi:set et ai sw=2 sts=2 ts=2: */
/*-
 * Copyright (c) 2009-2011 Jannis Pohlmann <jannis@xfce.org>
 * Copyright (c) 2015      Ali Abdallah    <ali@xfce.org>
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

#include <gio/gio.h>

#include <tumbler/tumbler.h>

#include <tumblerd/tumbler-component.h>
#include <tumblerd/tumbler-scheduler.h>
#include <tumblerd/tumbler-service.h>
#include <tumblerd/tumbler-lifo-scheduler.h>
#include <tumblerd/tumbler-group-scheduler.h>
#include <tumblerd/tumbler-utils.h>
#include <tumblerd/tumbler-service-gdbus.h>


#define THUMBNAILER_PATH    "/org/freedesktop/thumbnails/Thumbnailer1"
#define THUMBNAILER_SERVICE "org.freedesktop.thumbnails.Thumbnailer1"
#define THUMBNAILER_IFACE   "org.freedesktop.thumbnails.Thumbnailer1"




/* property identifiers */
enum
{
  PROP_0,
  PROP_CONNECTION,
  PROP_REGISTRY,
};



typedef struct _SchedulerIdleInfo SchedulerIdleInfo;



static void tumbler_service_constructed        (GObject            *object);
static void tumbler_service_finalize           (GObject            *object);
static void tumbler_service_get_property       (GObject            *object,
                                                guint               prop_id,
                                                GValue             *value,
                                                GParamSpec         *pspec);
static void tumbler_service_set_property       (GObject            *object,
                                                guint               prop_id,
                                                const GValue       *value,
                                                GParamSpec         *pspec);
static gboolean tumbler_service_queue_cb        (TumblerExportedService  *skeleton,
                                                 GDBusMethodInvocation   *invocation,
                                                 const gchar *const      *uris,
                                                 const gchar *const      *mime_hints,
                                                 const gchar             *flavor_name,
                                                 const gchar             *scheduler_name,
                                                 guint                    handle_to_dequeue,
                                                 TumblerService          *service);
static gboolean tumbler_service_dequeue_cb      (TumblerExportedService  *skeleton,
                                                 GDBusMethodInvocation   *invocation,
                                                 guint                     handle,
                                                 TumblerService          *service);
static gboolean tumbler_service_get_schedulers_cb(TumblerExportedService  *skeleton,
                                                  GDBusMethodInvocation   *invocation,
                                                  TumblerService          *service);
static gboolean tumbler_service_get_supported_cb(TumblerExportedService  *skeleton,
                                                 GDBusMethodInvocation   *invocation,
                                                 TumblerService          *service);
static gboolean tumbler_service_get_flavors_cb  (TumblerExportedService  *skeleton,
                                                 GDBusMethodInvocation   *invocation,
                                                 TumblerService          *service);
static void tumbler_service_scheduler_error    (TumblerScheduler   *scheduler,
                                                guint32             handle,
                                                const gchar *const *failed_uris,
                                                gint                error_code,
                                                const gchar        *message,
                                                const gchar        *origin,
                                                TumblerService     *service);
static void tumbler_service_scheduler_finished (TumblerScheduler   *scheduler,
                                                guint32             handle,
                                                const gchar        *origin,
                                                TumblerService     *service);
static void tumbler_service_scheduler_ready    (TumblerScheduler   *scheduler,
                                                guint32             handle,
                                                const gchar *const *uris,
                                                const gchar        *origin,
                                                TumblerService     *service);
static void tumbler_service_scheduler_started  (TumblerScheduler   *scheduler,
                                                guint32             handle,
                                                const gchar        *origin,
                                                TumblerService     *service);
static void tumbler_service_pre_unmount        (TumblerService     *service,
                                                GMount             *mount,
                                                GVolumeMonitor     *monitor);
static void scheduler_idle_info_free           (SchedulerIdleInfo  *info);



struct _TumblerServiceClass
{
  TumblerComponentClass __parent__;
};

struct _TumblerService
{
  TumblerComponent       __parent__;

  GDBusConnection        *connection;
  TumblerExportedService *skeleton;
  gboolean                dbus_interface_exported;
  
  TumblerRegistry        *registry;
  TUMBLER_MUTEX          (mutex);
  GList                  *schedulers;

  GVolumeMonitor         *volume_monitor;
};

struct _SchedulerIdleInfo
{
  TumblerScheduler *scheduler;
  TumblerService   *service;
  gchar           **uris;
  gchar            *message;
  gchar            *origin;
  guint             handle;
  gint              error_code;
};




G_DEFINE_TYPE (TumblerService, tumbler_service, TUMBLER_TYPE_COMPONENT);



static void
tumbler_service_class_init (TumblerServiceClass *klass)
{
  GObjectClass *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->constructed = tumbler_service_constructed; 
  gobject_class->finalize = tumbler_service_finalize; 
  gobject_class->get_property = tumbler_service_get_property;
  gobject_class->set_property = tumbler_service_set_property;

  g_object_class_install_property (gobject_class, PROP_CONNECTION,
                                   g_param_spec_object ("connection",
                                                        "connection",
                                                        "connection",
                                                        G_TYPE_DBUS_CONNECTION,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (gobject_class, PROP_REGISTRY,
                                   g_param_spec_object ("registry",
                                                        "registry",
                                                        "registry",
                                                        TUMBLER_TYPE_REGISTRY,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));
}



static void
tumbler_service_init (TumblerService *service)
{
  tumbler_mutex_create (service->mutex);
  service->schedulers = NULL;

  service->volume_monitor = g_volume_monitor_get ();
  g_signal_connect_swapped (service->volume_monitor, "mount-pre-unmount", 
                            G_CALLBACK (tumbler_service_pre_unmount), service);
}



static void
tumbler_service_add_scheduler (TumblerService   *service, 
                               TumblerScheduler *scheduler)
{
  /* add the scheduler to the list */
  service->schedulers = g_list_append (service->schedulers, g_object_ref (scheduler));

  /* connect to the scheduler signals */
  g_signal_connect (scheduler, "error",
                    G_CALLBACK (tumbler_service_scheduler_error), service);
  g_signal_connect (scheduler, "finished", 
                    G_CALLBACK (tumbler_service_scheduler_finished), service);
  g_signal_connect (scheduler, "ready", 
                    G_CALLBACK (tumbler_service_scheduler_ready), service);
  g_signal_connect (scheduler, "started", 
                    G_CALLBACK (tumbler_service_scheduler_started), service);
}



static void
tumbler_service_remove_scheduler (TumblerScheduler *scheduler,
                                  TumblerService   *service)
{
  g_return_if_fail (TUMBLER_IS_SCHEDULER (scheduler));
  g_return_if_fail (TUMBLER_IS_SERVICE (service));

  g_signal_handlers_disconnect_matched (scheduler, G_SIGNAL_MATCH_DATA,
                                        0, 0, NULL, NULL, service);
  g_object_unref (scheduler);
}



static void
tumbler_service_constructed (GObject *object)
{
  TumblerScheduler *scheduler;
  TumblerService   *service = TUMBLER_SERVICE (object);
  GError           *error = NULL;
  
  /* chain up to parent classes */
  if (G_OBJECT_CLASS (tumbler_service_parent_class)->constructed != NULL)
    (G_OBJECT_CLASS (tumbler_service_parent_class)->constructed) (object);

  /* create the foreground scheduler */
  scheduler = tumbler_lifo_scheduler_new ("foreground");
  tumbler_service_add_scheduler (service, scheduler);
  g_object_unref (scheduler);

  /* create the background scheduler */
  scheduler = tumbler_group_scheduler_new ("background");
  tumbler_service_add_scheduler (service, scheduler);
  g_object_unref (scheduler);

  /* everything is fine, install the generic thumbnailer D-Bus info */
  service->skeleton = tumbler_exported_service_skeleton_new();
  
  g_dbus_interface_skeleton_export (G_DBUS_INTERFACE_SKELETON(service->skeleton),
                                    service->connection,
                                    THUMBNAILER_PATH,
                                    &error);
  if (error != NULL)
    {
      g_critical ("error exporting thumbnail service on session bus: %s", error->message);
      g_error_free (error);
      service->dbus_interface_exported = FALSE; 
    }
  else
    {
      service->dbus_interface_exported = TRUE;
      
      g_signal_connect (service->skeleton, "handle-queue",
                        G_CALLBACK(tumbler_service_queue_cb), service);
      
      g_signal_connect (service->skeleton, "handle-dequeue",
                        G_CALLBACK(tumbler_service_dequeue_cb), service);
      
      g_signal_connect (service->skeleton, "handle-get-supported",
                        G_CALLBACK(tumbler_service_get_supported_cb), service);
      
      g_signal_connect (service->skeleton, "handle-get-schedulers",
                        G_CALLBACK(tumbler_service_get_schedulers_cb), service);
      
      g_signal_connect (service->skeleton, "handle-get-flavors",
                        G_CALLBACK(tumbler_service_get_flavors_cb), service);
    }
}



static void
tumbler_service_finalize (GObject *object)
{
  TumblerService *service = TUMBLER_SERVICE (object);

  /* disconnect from the volume monitor */
  g_signal_handlers_disconnect_matched (service->volume_monitor, G_SIGNAL_MATCH_DATA,
                                        0, 0, NULL, NULL, service);

  /* release the volume monitor */
  g_object_unref (service->volume_monitor);

  /* release all schedulers and the scheduler list */
  g_list_foreach (service->schedulers, (GFunc) tumbler_service_remove_scheduler, service);
  g_list_free (service->schedulers);

  /* release the reference on the thumbnailer registry */
  g_object_unref (service->registry);
  
  /* Unexport from dbus */
  if (service->dbus_interface_exported)
    g_dbus_interface_skeleton_unexport_from_connection 
      (
        G_DBUS_INTERFACE_SKELETON (service->skeleton),
        service->connection
      );

  /* release the Skeleton object */
  g_object_unref (service->skeleton);
  
  /* release the D-Bus connection object */
  g_object_unref (service->connection);

  tumbler_mutex_free (service->mutex);

  (*G_OBJECT_CLASS (tumbler_service_parent_class)->finalize) (object);
}



static void
tumbler_service_get_property (GObject    *object,
                              guint       prop_id,
                              GValue     *value,
                              GParamSpec *pspec)
{
  TumblerService *service = TUMBLER_SERVICE (object);

  switch (prop_id)
    {
    case PROP_CONNECTION:
      g_value_set_object (value, service->connection);
      break;
    case PROP_REGISTRY:
      g_value_set_object (value, service->registry);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}



static void
tumbler_service_set_property (GObject      *object,
                              guint         prop_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  TumblerService *service = TUMBLER_SERVICE (object);

  switch (prop_id)
    {
    case PROP_CONNECTION:
      service->connection = g_object_ref (g_value_get_object (value));
      break;
    case PROP_REGISTRY:
      service->registry = g_value_dup_object (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}



static gboolean
tumbler_service_error_idle (gpointer user_data)
{
  SchedulerIdleInfo *info = user_data;
  GVariant          *signal_variant;

  g_return_val_if_fail (info != NULL, FALSE);
  g_return_val_if_fail (TUMBLER_IS_SCHEDULER (info->scheduler), FALSE);
  g_return_val_if_fail (info->uris != NULL && info->uris[0] != NULL && *info->uris[0] != '\0', FALSE);
  g_return_val_if_fail (info->message != NULL && *info->message != '\0', FALSE);
  g_return_val_if_fail (info->origin != NULL && *info->origin != '\0', FALSE);
  g_return_val_if_fail (TUMBLER_IS_SERVICE (info->service), FALSE);

  /* signal variant */
  signal_variant = g_variant_new ("(u^asis)",
                                  info->handle,
                                  info->uris,
                                  info->error_code,
                                  info->message);

  /* send the signal message over D-Bus */
  g_dbus_connection_emit_signal (info->service->connection,
                                 info->origin, 
                                 THUMBNAILER_PATH,
                                 THUMBNAILER_IFACE,
                                 "Error",
                                 signal_variant, 
                                 NULL);

  scheduler_idle_info_free (info);

  return FALSE;
}



static void
tumbler_service_scheduler_error (TumblerScheduler   *scheduler,
                                 guint32             handle,
                                 const gchar *const *failed_uris,
                                 gint                error_code,
                                 const gchar        *message,
                                 const gchar        *origin,
                                 TumblerService     *service)
{
  SchedulerIdleInfo *info;

  g_return_if_fail (TUMBLER_IS_SCHEDULER (scheduler));
  g_return_if_fail (failed_uris != NULL);
  g_return_if_fail (message != NULL && *message != '\0');
  g_return_if_fail (origin != NULL && *origin != '\0');
  g_return_if_fail (TUMBLER_IS_SERVICE (service));
  
  info = g_slice_new0 (SchedulerIdleInfo);

  info->scheduler = g_object_ref (scheduler);
  info->handle = handle;
  info->uris = g_strdupv ((gchar **)failed_uris);
  info->error_code = error_code;
  info->message = g_strdup (message);
  info->origin = g_strdup (origin);
  info->service = g_object_ref (service);

  g_idle_add (tumbler_service_error_idle, info);
}



static gboolean
tumbler_service_finished_idle (gpointer user_data)
{
  SchedulerIdleInfo *info = user_data;
  GVariant          *signal_variant;

  g_return_val_if_fail (info != NULL, FALSE);
  g_return_val_if_fail (info->origin != NULL && *info->origin != '\0', FALSE);
  g_return_val_if_fail (TUMBLER_IS_SERVICE (info->service), FALSE);
  
  signal_variant = g_variant_new ("(u)",
                                  info->handle);

  /* send the signal message over D-Bus */
  g_dbus_connection_emit_signal (info->service->connection,
                                 info->origin, 
                                 THUMBNAILER_PATH,
                                 THUMBNAILER_IFACE,
                                 "Finished",
                                 signal_variant, 
                                 NULL);

  /* allow the lifecycle manager to shut down the service again (unless there
   * are other requests still being processed) */
  tumbler_component_decrement_use_count (TUMBLER_COMPONENT (info->service));

  scheduler_idle_info_free (info);

  return FALSE;
}



static void
tumbler_service_scheduler_finished (TumblerScheduler *scheduler,
                                    guint32           handle,
                                    const gchar      *origin,
                                    TumblerService   *service)
{
  SchedulerIdleInfo *info;

  g_return_if_fail (TUMBLER_IS_SCHEDULER (scheduler));
  g_return_if_fail (origin != NULL && *origin != '\0');
  g_return_if_fail (TUMBLER_IS_SERVICE (service));
  
  info = g_slice_new0 (SchedulerIdleInfo);

  info->scheduler = g_object_ref (scheduler);
  info->handle = handle;
  info->origin = g_strdup (origin);
  info->service = g_object_ref (service);

  g_idle_add (tumbler_service_finished_idle, info);
}



static gboolean 
tumbler_service_ready_idle (gpointer user_data)
{
  SchedulerIdleInfo *info = user_data;
  GVariant          *signal_variant;

  g_return_val_if_fail (info != NULL, FALSE);
  g_return_val_if_fail (TUMBLER_IS_SCHEDULER (info->scheduler), FALSE);
  g_return_val_if_fail (info->uris != NULL && info->uris[0] != NULL && *info->uris[0] != '\0', FALSE);
  g_return_val_if_fail (info->origin != NULL && *info->origin != '\0', FALSE);
  g_return_val_if_fail (TUMBLER_IS_SERVICE (info->service), FALSE);

  signal_variant = g_variant_new ("(u^as)",
                                  info->handle,
                                  info->uris);
 
  /* send the signal message over D-Bus */
  g_dbus_connection_emit_signal (info->service->connection,
                                 info->origin, 
                                 THUMBNAILER_PATH,
                                 THUMBNAILER_IFACE,
                                 "Ready",
                                 signal_variant, 
                                 NULL);
  
  scheduler_idle_info_free (info);

  return FALSE;
}



static void
tumbler_service_scheduler_ready (TumblerScheduler   *scheduler,
                                 guint32             handle,
                                 const gchar *const *uris,
                                 const gchar        *origin,
                                 TumblerService     *service)
{
  SchedulerIdleInfo *info;
  
  g_return_if_fail (TUMBLER_IS_SCHEDULER (scheduler));
  g_return_if_fail (origin != NULL && *origin != '\0');
  g_return_if_fail (uris != NULL && uris[0] != NULL && *uris[0] != '\0');
  g_return_if_fail (TUMBLER_IS_SERVICE (service));
  
  info = g_slice_new0 (SchedulerIdleInfo);

  info->scheduler = g_object_ref (scheduler);
  info->handle = handle;
  info->uris = g_strdupv ((gchar **)uris);
  info->origin = g_strdup (origin);
  info->service = g_object_ref (service);

  g_idle_add (tumbler_service_ready_idle, info);
}



static gboolean
tumbler_service_started_idle (gpointer user_data)
{
  SchedulerIdleInfo *info = user_data;
  GVariant          *signal_variant;
  
  g_return_val_if_fail (info != NULL, FALSE);
  g_return_val_if_fail (TUMBLER_IS_SCHEDULER (info->scheduler), FALSE);
  g_return_val_if_fail (info->origin != NULL && *info->origin != '\0', FALSE);
  g_return_val_if_fail (TUMBLER_IS_SERVICE (info->service), FALSE);

  signal_variant = g_variant_new ("(u)", info->handle);
 
  /* send the signal message over D-Bus */
  g_dbus_connection_emit_signal (info->service->connection,
                                 info->origin, 
                                 THUMBNAILER_PATH,
                                 THUMBNAILER_IFACE,
                                 "Started",
                                 signal_variant, 
                                 NULL);
  
  scheduler_idle_info_free (info);

  return FALSE;
}



static void
tumbler_service_scheduler_started (TumblerScheduler *scheduler,
                                   guint32           handle,
                                   const gchar      *origin,
                                   TumblerService   *service)
{
  SchedulerIdleInfo *info;
  
  g_return_if_fail (TUMBLER_IS_SCHEDULER (scheduler));
  g_return_if_fail (origin != NULL && *origin != '\0');
  g_return_if_fail (TUMBLER_IS_SERVICE (service));

  info = g_slice_new0 (SchedulerIdleInfo);

  info->scheduler = g_object_ref (scheduler);
  info->handle = handle;
  info->origin = g_strdup (origin);
  info->service = g_object_ref (service);

  g_idle_add (tumbler_service_started_idle, info);
}



static void
tumbler_service_pre_unmount (TumblerService *service,
                             GMount         *mount,
                             GVolumeMonitor *volume_monitor)
{
  GList *iter;

  g_return_if_fail (TUMBLER_IS_SERVICE (service));
  g_return_if_fail (G_IS_MOUNT (mount));
  g_return_if_fail (volume_monitor == service->volume_monitor);

  tumbler_mutex_lock (service->mutex);

  /* iterate over all schedulers, cancelling URIs belonging to the mount */
  for (iter = service->schedulers; iter != NULL; iter = iter->next)
    tumbler_scheduler_cancel_by_mount (iter->data, mount);

  tumbler_mutex_unlock (service->mutex);
}



static void
scheduler_idle_info_free (SchedulerIdleInfo *info)
{
  if (info == NULL)
    return;

  g_free (info->message);
  g_free (info->origin);
  g_strfreev (info->uris);

  g_object_unref (info->scheduler);
  g_object_unref (info->service);

  g_slice_free (SchedulerIdleInfo, info);
}



TumblerService *
tumbler_service_new (GDBusConnection         *connection,
                     TumblerLifecycleManager *lifecycle_manager,
                     TumblerRegistry         *registry)
{
  return g_object_new (TUMBLER_TYPE_SERVICE, 
                       "connection", connection, 
                       "lifecycle-manager", lifecycle_manager,
                       "registry", registry, 
                       NULL);
}




static gboolean 
tumbler_service_queue_cb (TumblerExportedService  *skeleton,
                          GDBusMethodInvocation   *invocation,
                          const gchar *const      *uris,
                          const gchar *const      *mime_hints,
                          const gchar             *flavor_name,
                          const gchar             *scheduler_name,
                          guint32                  handle_to_dequeue,
                          TumblerService          *service)
{
  TumblerSchedulerRequest *scheduler_request;
  TumblerThumbnailFlavor  *flavor;
  TumblerThumbnailer     **thumbnailers;
  TumblerScheduler        *scheduler = NULL;
  TumblerFileInfo        **infos;
  TumblerCache            *cache;
  GList                   *iter;
  gchar                   *name;
  const gchar             *origin;
  guint32                  handle;
  guint                    length;

  g_dbus_async_return_val_if_fail (TUMBLER_IS_SERVICE (service), invocation, FALSE);
  g_dbus_async_return_val_if_fail (uris != NULL, invocation, FALSE);
  g_dbus_async_return_val_if_fail (mime_hints != NULL, invocation, FALSE);

  tumbler_mutex_lock (service->mutex);

  /* prevent the lifecycle manager to shut down the service as long
   * as the request is still being processed */
  tumbler_component_increment_use_count (TUMBLER_COMPONENT (service));

  /* if the scheduler is not defined, fall back to "default" */
  if (scheduler_name == NULL || *scheduler_name == '\0')
    scheduler_name = "default";

  cache = tumbler_cache_get_default ();
  flavor = tumbler_cache_get_flavor (cache, flavor_name);
  g_object_unref (cache);

  infos = tumbler_file_info_array_new_with_flavor (uris, mime_hints, flavor,
                                                   &length);

  /* get an array with one thumbnailer for each URI in the request */
  thumbnailers = tumbler_registry_get_thumbnailer_array (service->registry, infos,
                                                         length);

  origin = g_dbus_method_invocation_get_sender (invocation);

  /* allocate a scheduler request */
  scheduler_request = tumbler_scheduler_request_new (infos, thumbnailers, 
                                                     length, origin);

  /* release the file info array */
  tumbler_file_info_array_free (infos);


  /* get the request handle */
  handle = scheduler_request->handle;

  /* iterate over all schedulers */
  for (iter = service->schedulers; iter != NULL; iter = iter->next)
    {
      /* dequeue the request with the given dequeue handle, in case this 
       * scheduler is responsible for the given handle */
      if (handle_to_dequeue != 0)
        tumbler_scheduler_dequeue (TUMBLER_SCHEDULER (iter->data), handle_to_dequeue);

      /* determine the scheduler name */
      name = tumbler_scheduler_get_name (TUMBLER_SCHEDULER (iter->data));

      /* check if this is the scheduler we are looking for */
      if (g_strcmp0 (name, scheduler_name) == 0)
        scheduler = TUMBLER_SCHEDULER (iter->data);

      /* free the scheduler name */
      g_free (name);
    }

  /* default to the first scheduler in the list if we couldn't find
   * the scheduler with the desired name */
  if (scheduler == NULL && service->schedulers != NULL)
    scheduler = TUMBLER_SCHEDULER (service->schedulers->data);

  /* report unsupported flavors back to the client */
  if (flavor == NULL)
    {
      /* fake a started signal */
      tumbler_service_scheduler_started (scheduler, handle, scheduler_request->origin,
                                         service);

      /* emit an error signal */
      tumbler_service_scheduler_error (scheduler, handle, uris, 
                                       TUMBLER_ERROR_UNSUPPORTED_FLAVOR, 
                                       TUMBLER_ERROR_MESSAGE_UNSUPPORTED_FLAVOR,
                                       scheduler_request->origin,
                                       service);

      /* fake a finished signal */
      tumbler_service_scheduler_finished (scheduler, handle, scheduler_request->origin,
                                          service);

      /* release the request */
      tumbler_scheduler_request_free (scheduler_request);
    }
  else
    {
      /* let the scheduler take it from here */
      tumbler_scheduler_push (scheduler, scheduler_request);
    }
  
  /* free the thumbnailer array */
  tumbler_thumbnailer_array_free (thumbnailers, length);

  tumbler_mutex_unlock (service->mutex);
  
  tumbler_exported_service_complete_queue(skeleton, invocation, handle);
  
  /* try to keep tumbler alive */
  tumbler_component_keep_alive (TUMBLER_COMPONENT (service), NULL);
    
  return TRUE;
}



static gboolean 
tumbler_service_dequeue_cb (TumblerExportedService  *skeleton,
                            GDBusMethodInvocation   *invocation,
                            guint32                  handle,
                            TumblerService          *service)
{
  GList *iter;

  tumbler_mutex_lock (service->mutex);

  if (handle != 0) 
    {
      /* iterate over all available schedulers */
      for (iter = service->schedulers; iter != NULL; iter = iter->next)
        {
          /* dequeue the request with the given dequeue handle, in case this
           * scheduler is responsible for the given handle */
          tumbler_scheduler_dequeue (TUMBLER_SCHEDULER (iter->data), handle);
        }
    }

  tumbler_mutex_unlock (service->mutex);

  tumbler_exported_service_complete_dequeue(skeleton, invocation);
    
  /* keep tumbler alive */
  tumbler_component_keep_alive (TUMBLER_COMPONENT (service), NULL);
  
  return TRUE;
}



static gboolean 
tumbler_service_get_schedulers_cb (TumblerExportedService  *skeleton,
                                   GDBusMethodInvocation   *invocation,
                                   TumblerService          *service)
{
  gchar **supported_schedulers;
  GList  *iter;
  guint   n = 0;

  tumbler_mutex_lock (service->mutex);

  /* allocate an error for the schedulers */
  supported_schedulers = g_new0 (gchar *, g_list_length (service->schedulers) + 2);

  /* always prepend the "default" scheduler */
  supported_schedulers[n++] = g_strdup ("default");

  /* append all supported scheduler names */
  for (iter = service->schedulers; iter != NULL; iter = iter->next)
    {
      supported_schedulers[n++] = 
        tumbler_scheduler_get_name (TUMBLER_SCHEDULER (iter->data));
    }

  tumbler_mutex_unlock (service->mutex);

  /* NULL-terminate the array */
  supported_schedulers[n] = NULL;

  /* return the scheduler array to the caller */
  tumbler_exported_service_complete_get_schedulers (skeleton, 
                                                    invocation,
                                                    (const char* const*)supported_schedulers);

  /* free the array */
  g_strfreev (supported_schedulers);

  /* try to keep tumbler alive */
  tumbler_component_keep_alive (TUMBLER_COMPONENT (service), NULL);
  
  return TRUE;
}



static gboolean 
tumbler_service_get_supported_cb (TumblerExportedService  *skeleton,
                                  GDBusMethodInvocation   *invocation,
                                  TumblerService          *service)
{
  const gchar *const *mime_types;
  const gchar *const *uri_schemes;

  g_dbus_async_return_val_if_fail (TUMBLER_IS_SERVICE (service), invocation, FALSE);

  tumbler_mutex_lock (service->mutex);

  /* fetch all supported URI scheme / MIME type pairs from the registry */
  tumbler_registry_get_supported (service->registry, &uri_schemes, &mime_types);

  tumbler_mutex_unlock (service->mutex);

  /* return the arrays to the caller */
  tumbler_exported_service_complete_get_supported(skeleton, invocation, uri_schemes, mime_types);

  /* try to keep tumbler alive */
  tumbler_component_keep_alive (TUMBLER_COMPONENT (service), NULL);
  
  return TRUE;
}



static gboolean 
tumbler_service_get_flavors_cb  (TumblerExportedService  *skeleton,
                                 GDBusMethodInvocation   *invocation,
                                 TumblerService          *service)
{
  TumblerCache *cache;
  const gchar **flavor_strings;
  GList        *flavors;
  GList        *iter;
  guint         n;

  cache = tumbler_cache_get_default ();

  if (cache != NULL)
    {
      flavors = tumbler_cache_get_flavors (cache);
      flavor_strings = g_new0 (const gchar *, g_list_length (flavors) + 1);

      for (iter = flavors, n = 0; iter != NULL; iter = iter->next, ++n)
        flavor_strings[n] = tumbler_thumbnail_flavor_get_name (iter->data);
      flavor_strings[n] = NULL;
   
      tumbler_exported_service_complete_get_flavors (skeleton, invocation, flavor_strings);
      
      g_free (flavor_strings);
      g_list_free_full (flavors, g_object_unref);
      g_object_unref (cache);
    }
  else
    {
      flavor_strings = g_new0 (const gchar *, 1);
      flavor_strings[0] = NULL;
      
      tumbler_exported_service_complete_get_flavors (skeleton, invocation, flavor_strings);

      g_free (flavor_strings);
    }

  /* try to keep tumbler alive */
  tumbler_component_keep_alive (TUMBLER_COMPONENT (service), NULL);

  return TRUE;
}



gboolean tumbler_service_is_exported (TumblerService *service)
{
  g_return_val_if_fail (TUMBLER_IS_SERVICE(service), FALSE);
  return service->dbus_interface_exported;
}
