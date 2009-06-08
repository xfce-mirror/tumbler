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

#include <float.h>

#include <glib.h>
#include <glib/gi18n.h>
#include <glib-object.h>

#include <tumbler/tumbler.h>

#include <tumblerd/tumbler-threshold-scheduler.h>
#include <tumblerd/tumbler-scheduler.h>



#define TUMBLER_THRESHOLD_SCHEDULER_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), TUMBLER_TYPE_THRESHOLD_SCHEDULER, TumblerThresholdSchedulerPrivate))



/* Property identifiers */
enum
{
  PROP_0,
  PROP_THRESHOLD,
};



static void tumbler_threshold_scheduler_class_init        (TumblerThresholdSchedulerClass *klass);
static void tumbler_threshold_scheduler_iface_init        (TumblerSchedulerIface         *iface);
static void tumbler_threshold_scheduler_init              (TumblerThresholdScheduler      *scheduler);
static void tumbler_threshold_scheduler_finalize          (GObject                       *object);
static void tumbler_threshold_scheduler_get_property      (GObject                       *object,
                                                           guint                          prop_id,
                                                           GValue                        *value,
                                                           GParamSpec                    *pspec);
static void tumbler_threshold_scheduler_set_property      (GObject                       *object,
                                                           guint                          prop_id,
                                                           const GValue                  *value,
                                                           GParamSpec                    *pspec);
static void tumbler_threshold_scheduler_push              (TumblerScheduler              *scheduler,
                                                           TumblerSchedulerRequest       *request);
static void tumbler_threshold_scheduler_unqueue           (TumblerScheduler              *scheduler,
                                                           guint                          handle);
static void tumbler_threshold_scheduler_finish_request    (TumblerThresholdScheduler     *scheduler,
                                                           TumblerSchedulerRequest       *request);
static void tumbler_threshold_scheduler_unqueue_request   (TumblerSchedulerRequest       *request,
                                                           gpointer                       user_data);
static void tumbler_threshold_scheduler_thread            (gpointer                       data,
                                                           gpointer                       user_data);
static void tumbler_threshold_scheduler_thumbnailer_error (TumblerThumbnailer            *thumbnailer,
                                                           const gchar                   *failed_uri,
                                                           gint                           error_code,
                                                           const gchar                   *message,
                                                           TumblerSchedulerRequest       *request);
static void tumbler_threshold_scheduler_thumbnailer_ready (TumblerThumbnailer            *thumbnailer,
                                                           const gchar                   *uri,
                                                           TumblerSchedulerRequest       *request);



struct _TumblerThresholdSchedulerClass
{
  GObjectClass __parent__;
};

struct _TumblerThresholdScheduler
{
  GObject __parent__;

  TumblerThresholdSchedulerPrivate *priv;
};

struct _TumblerThresholdSchedulerPrivate
{
  GThreadPool *large_pool;
  GThreadPool *small_pool;
  GMutex      *mutex;
  GList       *requests;
  guint        threshold;
};



static GObjectClass *tumbler_threshold_scheduler_parent_class = NULL;



GType
tumbler_threshold_scheduler_get_type (void)
{
  static GType type = G_TYPE_INVALID;

  if (G_UNLIKELY (type == G_TYPE_INVALID))
    {
      static const GInterfaceInfo info =
      {
        (GInterfaceInitFunc) tumbler_threshold_scheduler_iface_init,
        NULL,
        NULL,
      };

      type = g_type_register_static_simple (G_TYPE_OBJECT, 
                                            "TumblerThresholdScheduler",
                                            sizeof (TumblerThresholdSchedulerClass),
                                            (GClassInitFunc) tumbler_threshold_scheduler_class_init,
                                            sizeof (TumblerThresholdScheduler),
                                            (GInstanceInitFunc) tumbler_threshold_scheduler_init,
                                            0);

      g_type_add_interface_static (type, TUMBLER_TYPE_SCHEDULER, &info);
    }

  return type;
}



static void
tumbler_threshold_scheduler_class_init (TumblerThresholdSchedulerClass *klass)
{
  GObjectClass *gobject_class;

  g_type_class_add_private (klass, sizeof (TumblerThresholdSchedulerPrivate));

  /* Determine the parent type class */
  tumbler_threshold_scheduler_parent_class = g_type_class_peek_parent (klass);

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->finalize = tumbler_threshold_scheduler_finalize; 
  gobject_class->get_property = tumbler_threshold_scheduler_get_property;
  gobject_class->set_property = tumbler_threshold_scheduler_set_property;

  g_object_class_install_property (gobject_class,
                                   PROP_THRESHOLD,
                                   g_param_spec_uint ("threshold",
                                                      "threshold",
                                                      "threshold",
                                                      0, G_MAXUINT, 20, 
                                                      G_PARAM_READWRITE));

}



static void
tumbler_threshold_scheduler_iface_init (TumblerSchedulerIface *iface)
{
  iface->push = tumbler_threshold_scheduler_push;
  iface->unqueue = tumbler_threshold_scheduler_unqueue;
}



static void
tumbler_threshold_scheduler_init (TumblerThresholdScheduler *scheduler)
{
  scheduler->priv = TUMBLER_THRESHOLD_SCHEDULER_GET_PRIVATE (scheduler);

  scheduler->priv->mutex = g_mutex_new ();
  scheduler->priv->requests = NULL;

  /* allocate a pool with max. 2 threads for request with <= threshold URIs */
  scheduler->priv->small_pool = g_thread_pool_new (tumbler_threshold_scheduler_thread,
                                                   scheduler, 2, TRUE, NULL);

  /* make the thread a LIFO */
  g_thread_pool_set_sort_function (scheduler->priv->small_pool,
                                   tumbler_scheduler_request_compare, NULL);

  /* allocate a pool with max. 2 threads for request with > threshold URIs */
  scheduler->priv->large_pool = g_thread_pool_new (tumbler_threshold_scheduler_thread,
                                                   scheduler, 2, TRUE, NULL);

  /* make the thread a LIFO */
  g_thread_pool_set_sort_function (scheduler->priv->small_pool,
                                   tumbler_scheduler_request_compare, NULL);
}



static void
tumbler_threshold_scheduler_finalize (GObject *object)
{
  TumblerThresholdScheduler *scheduler = TUMBLER_THRESHOLD_SCHEDULER (object);

  /* destroy both thread pools */
  g_thread_pool_free (scheduler->priv->small_pool, TRUE, TRUE);
  g_thread_pool_free (scheduler->priv->large_pool, TRUE, TRUE);

  /* release all pending requests */
  g_list_foreach (scheduler->priv->requests, (GFunc) tumbler_scheduler_request_free,
                  NULL);

  /* destroy the request list */
  g_list_free (scheduler->priv->requests);

  /* destroy the mutex */
  g_mutex_free (scheduler->priv->mutex);

  (*G_OBJECT_CLASS (tumbler_threshold_scheduler_parent_class)->finalize) (object);
}



static void
tumbler_threshold_scheduler_get_property (GObject    *object,
                                          guint       prop_id,
                                          GValue     *value,
                                          GParamSpec *pspec)
{
  TumblerThresholdScheduler *scheduler = TUMBLER_THRESHOLD_SCHEDULER (object);

  switch (prop_id)
    {
    case PROP_THRESHOLD:
      g_value_set_uint (value, scheduler->priv->threshold);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}



static void
tumbler_threshold_scheduler_set_property (GObject      *object,
                                          guint         prop_id,
                                          const GValue *value,
                                          GParamSpec   *pspec)
{
  TumblerThresholdScheduler *scheduler = TUMBLER_THRESHOLD_SCHEDULER (object);

  switch (prop_id)
    {
    case PROP_THRESHOLD:
      scheduler->priv->threshold = g_value_get_uint (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}



static void
tumbler_threshold_scheduler_push (TumblerScheduler        *scheduler,
                                  TumblerSchedulerRequest *request)
{
  TumblerThresholdScheduler *threshold_scheduler = 
    TUMBLER_THRESHOLD_SCHEDULER (scheduler);

  g_return_if_fail (TUMBLER_IS_THRESHOLD_SCHEDULER (scheduler));
  g_return_if_fail (request != NULL);

  g_mutex_lock (threshold_scheduler->priv->mutex);
  
  /* gain ownership over the requests (sets request->scheduler) */
  tumbler_scheduler_take_request (scheduler, request);

  /* prepend the request to the request list */
  threshold_scheduler->priv->requests = 
    g_list_prepend (threshold_scheduler->priv->requests, request);

  /* enqueue the request in one of the two thread pools depending on its size */
  if (g_strv_length (request->uris) > threshold_scheduler->priv->threshold)
    g_thread_pool_push (threshold_scheduler->priv->large_pool, request, NULL);
  else
    g_thread_pool_push (threshold_scheduler->priv->small_pool, request, NULL);

  g_mutex_unlock (threshold_scheduler->priv->mutex);
}



static void
tumbler_threshold_scheduler_unqueue (TumblerScheduler *scheduler,
                                     guint             handle)
{
  TumblerThresholdScheduler *threshold_scheduler = 
    TUMBLER_THRESHOLD_SCHEDULER (scheduler);

  g_return_if_fail (TUMBLER_IS_THRESHOLD_SCHEDULER (scheduler));
  g_return_if_fail (handle != 0);

  g_mutex_lock (threshold_scheduler->priv->mutex);

  g_list_foreach (threshold_scheduler->priv->requests, 
                  (GFunc) tumbler_threshold_scheduler_unqueue_request, 
                  GUINT_TO_POINTER (handle));

  g_mutex_unlock (threshold_scheduler->priv->mutex);
}



static void
tumbler_threshold_scheduler_finish_request (TumblerThresholdScheduler *scheduler,
                                            TumblerSchedulerRequest   *request)
{
  g_return_if_fail (TUMBLER_IS_THRESHOLD_SCHEDULER (scheduler));
  g_return_if_fail (request != NULL);

  g_signal_emit_by_name (scheduler, "finished", request->handle);

  scheduler->priv->requests = g_list_remove (scheduler->priv->requests,
                                             request);

  tumbler_scheduler_request_free (request);
}



static void
tumbler_threshold_scheduler_unqueue_request (TumblerSchedulerRequest *request,
                                             gpointer                 user_data)
{
  guint handle = GPOINTER_TO_UINT (user_data);

  g_return_if_fail (request != NULL);
  g_return_if_fail (handle != 0);

  if (request->handle == handle)
    request->unqueued = TRUE;
}



static void
tumbler_threshold_scheduler_thread (gpointer data,
                                    gpointer user_data)
{
  TumblerThresholdScheduler *scheduler = user_data;
  TumblerSchedulerRequest   *request = data;
  TumblerFileInfo           *info;
  const gchar              **uris;
  gboolean                   outdated;
  gboolean                   uri_needs_update;
  guint64                    mtime;
  GError                    *error = NULL;
  GList                     *cached_uris = NULL;
  GList                     *missing_uris = NULL;
  GList                     *thumbnails;
  GList                     *lp;
  gint                       n;

  g_return_if_fail (TUMBLER_IS_THRESHOLD_SCHEDULER (scheduler));
  g_return_if_fail (request != NULL);

  /* notify others that we're starting to process this request */
  g_signal_emit_by_name (request->scheduler, "started", request->handle);

  g_mutex_lock (scheduler->priv->mutex);

  /* finish the request if it was unqueued */
  if (request->unqueued)
    {
      tumbler_threshold_scheduler_finish_request (scheduler, request);
      return;
    }

  g_mutex_unlock (scheduler->priv->mutex);

  /* process URI by URI */
  for (n = 0; request->uris[n] != NULL; ++n)
    {
      g_mutex_lock (scheduler->priv->mutex);

      /* finish the request if it was unqueued */
      if (request->unqueued)
        {
          tumbler_threshold_scheduler_finish_request (scheduler, request);
          return;
        }

      g_mutex_unlock (scheduler->priv->mutex);

      info = tumbler_file_info_new (request->uris[n]);
      uri_needs_update = FALSE;

      if (request->thumbnailers[n] != NULL)
        {
          if (tumbler_file_info_load (info, NULL, &error))
            {
              mtime = tumbler_file_info_get_mtime (info);

              thumbnails = tumbler_file_info_get_thumbnails (info);

              for (lp = thumbnails; 
                   error == NULL && lp != NULL; 
                   lp = lp->next)
                {
                  if (tumbler_thumbnail_load (lp->data, NULL, &error))
                    {
                      outdated = tumbler_thumbnail_needs_update (lp->data, 
                                                                 request->uris[n],
                                                                 mtime);

                      uri_needs_update = uri_needs_update || outdated;
                    }
                }
            }
        }
      else
        {
          g_set_error (&error, TUMBLER_ERROR, TUMBLER_ERROR_NO_THUMBNAILER,
                       _("No thumbnailer available for \"%s\""), request->uris[n]);
        }

      g_object_unref (info);

      if (error == NULL)
        {
          if (uri_needs_update)
            missing_uris = g_list_prepend (missing_uris, GINT_TO_POINTER (n));
          else
            cached_uris = g_list_prepend (cached_uris, request->uris[n]);
        }
      else
        {
          tumbler_scheduler_emit_uri_error (TUMBLER_SCHEDULER (scheduler), request,
                                            request->uris[n], error);

          g_clear_error (&error);
        }
    }

  /* check if we have any cached files */
  if (cached_uris != NULL)
    {
      uris = g_new0 (const gchar *, g_list_length (cached_uris) + 1);
      for (n = 0, lp = g_list_last (cached_uris); lp != NULL; lp = lp->prev, ++n)
        uris[n] = lp->data;
      uris[n] = NULL;

      /* notify others that the cached thumbnails are ready */
      g_signal_emit_by_name (scheduler, "ready", uris);

      /* free string array and cached list */
      g_list_free (cached_uris);
      g_free (uris);
    }

  /* iterate over invalid/missing URI list */
  for (lp = g_list_last (missing_uris); lp != NULL; lp = lp->prev)
    {
      n = GPOINTER_TO_INT (lp->data);
      
      /* connect to the error signal of the thumbnailer */
      g_signal_connect (request->thumbnailers[n], "error", 
                        G_CALLBACK (tumbler_threshold_scheduler_thumbnailer_error),
                        request);

      /* connect to the ready signal of the thumbnailer */
      g_signal_connect (request->thumbnailers[n], "ready",
                        G_CALLBACK (tumbler_threshold_scheduler_thumbnailer_ready),
                        request);

      /* tell the thumbnailer to generate the thumbnail */
      tumbler_thumbnailer_create (request->thumbnailers[n], request->uris[n], 
                                  request->mime_hints[n]);

      /* TODO I suppose we have to make sure not to finish the request before
       * the thumbnailer has emitted an error or ready signal */

      /* disconnect from all signals when we're finished */
      g_signal_handlers_disconnect_matched (request->thumbnailers[n],
                                            G_SIGNAL_MATCH_DATA,
                                            0, 0, NULL, NULL, request);
    }

  g_mutex_lock (scheduler->priv->mutex);

  /* notify others that we're finished processing the request */
  tumbler_threshold_scheduler_finish_request (scheduler, request);

  g_mutex_unlock (scheduler->priv->mutex);
}



static void
tumbler_threshold_scheduler_thumbnailer_error (TumblerThumbnailer      *thumbnailer,
                                               const gchar             *failed_uri,
                                               gint                     error_code,
                                               const gchar             *message,
                                               TumblerSchedulerRequest *request)
{
  const gchar *failed_uris[] = { failed_uri, NULL };

  g_return_if_fail (TUMBLER_IS_THUMBNAILER (thumbnailer));
  g_return_if_fail (failed_uri != NULL);
  g_return_if_fail (request != NULL);
  g_return_if_fail (TUMBLER_IS_THRESHOLD_SCHEDULER (request->scheduler));

  /* forward the error signal */
  g_signal_emit_by_name (request->scheduler, "error", request->handle, failed_uris, 
                         error_code, message);
}



static void
tumbler_threshold_scheduler_thumbnailer_ready (TumblerThumbnailer      *thumbnailer,
                                               const gchar             *uri,
                                               TumblerSchedulerRequest *request)
{
  const gchar *uris[] = { uri, NULL };

  g_return_if_fail (TUMBLER_IS_THUMBNAILER (thumbnailer));
  g_return_if_fail (uri != NULL);
  g_return_if_fail (request != NULL);
  g_return_if_fail (TUMBLER_IS_THRESHOLD_SCHEDULER (request->scheduler));

  /* forward the ready signal */
  g_signal_emit_by_name (request->scheduler, "ready", uris);
}



TumblerScheduler *
tumbler_threshold_scheduler_new (void)
{
  return g_object_new (TUMBLER_TYPE_THRESHOLD_SCHEDULER, NULL);
}
