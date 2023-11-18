/* vi:set et ai sw=2 sts=2 ts=2: */
/*-
 * Copyright (c) 2009 Jannis Pohlmann <jannis@xfce.org>
 * Copyright (c) 2009 Nokia, 
 *   written by Philip Van Hoof <philip@codeminded.be>
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

#include <tumblerd/tumbler-lifo-scheduler.h>
#include <tumblerd/tumbler-scheduler.h>
#include <tumblerd/tumbler-utils.h>



/* Property identifiers */
enum
{
  PROP_0,
  PROP_NAME,
};



static void tumbler_lifo_scheduler_iface_init        (TumblerSchedulerIface     *iface);
static void tumbler_lifo_scheduler_finalize          (GObject                   *object);
static void tumbler_lifo_scheduler_get_property      (GObject                   *object,
                                                      guint                      prop_id,
                                                      GValue                    *value,
                                                      GParamSpec                *pspec);
static void tumbler_lifo_scheduler_set_property      (GObject                   *object,
                                                      guint                      prop_id,
                                                      const GValue              *value,
                                                      GParamSpec                *pspec);
static void tumbler_lifo_scheduler_push              (TumblerScheduler          *scheduler,
                                                      TumblerSchedulerRequest   *request);
static void tumbler_lifo_scheduler_dequeue           (TumblerScheduler          *scheduler,
                                                      guint32                    handle);
static void tumbler_lifo_scheduler_cancel_by_mount   (TumblerScheduler          *scheduler,
                                                      GMount                    *mount);
static void tumbler_lifo_scheduler_finish_request    (TumblerLifoScheduler *scheduler,
                                                      TumblerSchedulerRequest   *request);
static void tumbler_lifo_scheduler_dequeue_request   (TumblerSchedulerRequest   *request,
                                                      gpointer                   user_data);
static void tumbler_lifo_scheduler_thread            (gpointer                   data,
                                                      gpointer                   user_data);
static void tumbler_lifo_scheduler_thumbnailer_error (TumblerThumbnailer        *thumbnailer,
                                                      const gchar               *failed_uri,
                                                      GQuark                     error_domain,
                                                      gint                       error_code,
                                                      const gchar               *message,
                                                      TumblerSchedulerRequest   *request);
static void tumbler_lifo_scheduler_thumbnailer_ready (TumblerThumbnailer        *thumbnailer,
                                                      const gchar               *uri,
                                                      TumblerSchedulerRequest   *request);



struct _TumblerLifoSchedulerClass
{
  GObjectClass __parent__;
};

struct _TumblerLifoScheduler
{
  GObject __parent__;

  GThreadPool *pool;
  TUMBLER_MUTEX (mutex);
  GList       *requests;

  gchar       *name;
};



G_LOCK_DEFINE (plugin_access_lock);



G_DEFINE_TYPE_WITH_CODE (TumblerLifoScheduler,
                         tumbler_lifo_scheduler,
                         G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (TUMBLER_TYPE_SCHEDULER,
                                                tumbler_lifo_scheduler_iface_init));


static void
tumbler_lifo_scheduler_class_init (TumblerLifoSchedulerClass *klass)
{
  GObjectClass *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->finalize = tumbler_lifo_scheduler_finalize; 

  gobject_class->get_property = tumbler_lifo_scheduler_get_property;
  gobject_class->set_property = tumbler_lifo_scheduler_set_property;

  g_object_class_override_property (gobject_class, PROP_NAME, "name");

}

static void
tumbler_lifo_scheduler_iface_init (TumblerSchedulerIface *iface)
{
  iface->push = tumbler_lifo_scheduler_push;
  iface->dequeue = tumbler_lifo_scheduler_dequeue;
  iface->cancel_by_mount = tumbler_lifo_scheduler_cancel_by_mount;
}


static void
tumbler_lifo_scheduler_init (TumblerLifoScheduler *scheduler)
{
  tumbler_mutex_create (scheduler->mutex);
  scheduler->requests = NULL;

  /* allocate a pool with a number of threads depending on the system */
  scheduler->pool = g_thread_pool_new (tumbler_lifo_scheduler_thread,
                                       scheduler, g_get_num_processors (), TRUE, NULL);

  /* make the thread a LIFO */
  g_thread_pool_set_sort_function (scheduler->pool, 
                                   tumbler_scheduler_request_compare, NULL);
}



static void
tumbler_lifo_scheduler_finalize (GObject *object)
{
  TumblerLifoScheduler *scheduler = TUMBLER_LIFO_SCHEDULER (object);

  /* destroy both thread pools */
  g_thread_pool_free (scheduler->pool, TRUE, TRUE);

  /* release all pending requests and destroy the request list */
  g_list_free_full (scheduler->requests, tumbler_scheduler_request_free);

  /* free the scheduler name */
  g_free (scheduler->name);

  /* destroy the mutex */
  tumbler_mutex_free (scheduler->mutex);

  (*G_OBJECT_CLASS (tumbler_lifo_scheduler_parent_class)->finalize) (object);
}



static void
tumbler_lifo_scheduler_get_property (GObject    *object,
                                     guint       prop_id,
                                     GValue     *value,
                                     GParamSpec *pspec)
{
  TumblerLifoScheduler *scheduler = TUMBLER_LIFO_SCHEDULER (object);

  switch (prop_id)
    {
    case PROP_NAME:
      g_value_set_string (value, scheduler->name);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}



static void
tumbler_lifo_scheduler_set_property (GObject      *object,
                                     guint         prop_id,
                                     const GValue *value,
                                     GParamSpec   *pspec)
{
  TumblerLifoScheduler *scheduler = TUMBLER_LIFO_SCHEDULER (object);

  switch (prop_id)
    {
    case PROP_NAME:
      scheduler->name = g_value_dup_string (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}



static void
tumbler_lifo_scheduler_push (TumblerScheduler        *scheduler,
                             TumblerSchedulerRequest *request)
{
  TumblerLifoScheduler *lifo_scheduler = TUMBLER_LIFO_SCHEDULER (scheduler);

  g_return_if_fail (TUMBLER_IS_LIFO_SCHEDULER (scheduler));
  g_return_if_fail (request != NULL);

  tumbler_mutex_lock (lifo_scheduler->mutex);
  
  /* gain ownership over the requests (sets request->scheduler) */
  tumbler_scheduler_take_request (scheduler, request);

  /* prepend the request to the request list */
  lifo_scheduler->requests = g_list_prepend (lifo_scheduler->requests, request);

  /* enqueue the request in the pool */
  g_thread_pool_push (lifo_scheduler->pool, request, NULL);

  tumbler_mutex_unlock (lifo_scheduler->mutex);
}



static void
tumbler_lifo_scheduler_dequeue (TumblerScheduler *scheduler,
                                guint32           handle)
{
  TumblerLifoScheduler *lifo_scheduler = TUMBLER_LIFO_SCHEDULER (scheduler);

  g_return_if_fail (TUMBLER_IS_LIFO_SCHEDULER (scheduler));
  g_return_if_fail (handle != 0);

  tumbler_mutex_lock (lifo_scheduler->mutex);

  /* dequeue all requests (usually only one) with this handle */
  g_list_foreach (lifo_scheduler->requests, 
                  (GFunc) tumbler_lifo_scheduler_dequeue_request, 
                  GUINT_TO_POINTER (handle));

  tumbler_mutex_unlock (lifo_scheduler->mutex);
}



static void
tumbler_lifo_scheduler_cancel_by_mount (TumblerScheduler *scheduler,
                                        GMount           *mount)
{
  TumblerSchedulerRequest *request;
  TumblerLifoScheduler    *lifo_scheduler = TUMBLER_LIFO_SCHEDULER (scheduler);
  GFile                   *mount_point;
  GFile                   *file;
  GList                   *iter;
  guint                    n;

  g_return_if_fail (TUMBLER_IS_LIFO_SCHEDULER (scheduler));
  g_return_if_fail (G_IS_MOUNT (mount));

  /* determine the root mount point */
  mount_point = g_mount_get_root (mount);

  tumbler_mutex_lock (lifo_scheduler->mutex);

  /* iterate over all requests */
  for (iter = lifo_scheduler->requests; iter != NULL; iter = iter->next)
    {
      request = iter->data;

      /* iterate over all request URIs */
      for (n = 0; n < request->length; ++n)
        {
          /* determine the enclosing mount for the file */
          file = g_file_new_for_uri (tumbler_file_info_get_uri (request->infos[n]));

          /* cancel the URI if it lies of the mount point */
          if (g_file_has_prefix (file, mount_point))
            g_cancellable_cancel (request->cancellables[n]);

          /* release the file object */
          g_object_unref (file);
        }
    }

  tumbler_mutex_unlock (lifo_scheduler->mutex);

  /* release the mount point */
  g_object_unref (mount_point);
}



static void
tumbler_lifo_scheduler_finish_request (TumblerLifoScheduler    *scheduler,
                                       TumblerSchedulerRequest *request)
{
  g_return_if_fail (TUMBLER_IS_LIFO_SCHEDULER (scheduler));
  g_return_if_fail (request != NULL);

  /* emit a finished signal for this request */
  g_signal_emit_by_name (scheduler, "finished", request->handle, 
                         request->origin);

  /* remove the request from the list */
  scheduler->requests = g_list_remove (scheduler->requests, request);

  /* destroy the request since we no longer need it */
  tumbler_scheduler_request_free (request);
}



static void
tumbler_lifo_scheduler_dequeue_request (TumblerSchedulerRequest *request,
                                        gpointer                 user_data)
{
  guint handle = GPOINTER_TO_UINT (user_data);
  guint n;

  g_return_if_fail (request != NULL);
  g_return_if_fail (handle != 0);

  /* mark the request as dequeued if the handles match */
  if (request->handle == handle) 
    {
      request->dequeued = TRUE;

      /* cancel all thumbnails that are part of the request */
      for (n = 0; n < request->length; ++n)
        g_cancellable_cancel (request->cancellables[n]);
  }
}



static void
tumbler_lifo_scheduler_thread (gpointer data,
                               gpointer user_data)
{
  TumblerSchedulerRequest *request = data;
  TumblerLifoScheduler    *scheduler = user_data;
  const gchar            **uris;
  gboolean                 uri_needs_update;
  GError                  *error = NULL;
  GList                   *cached_uris = NULL;
  GList                   *missing_uris = NULL;
  GList                   *lp, *lq;
  guint                    n;

  g_return_if_fail (TUMBLER_IS_LIFO_SCHEDULER (scheduler));
  g_return_if_fail (request != NULL);

  /* notify others that we're starting to process this request */
  g_signal_emit_by_name (request->scheduler, "started", request->handle,
                         request->origin);

  /* finish the request if it was already dequeued */
  tumbler_mutex_lock (scheduler->mutex);
  if (request->dequeued)
    {
      tumbler_lifo_scheduler_finish_request (scheduler, request);
      tumbler_mutex_unlock (scheduler->mutex);
      return;
    }
  tumbler_mutex_unlock (scheduler->mutex);

  /* process URI by URI */
  for (n = 0; n < request->length; ++n)
    {
      /* finish the request if it was dequeued */
      tumbler_mutex_lock (scheduler->mutex);
      if (request->dequeued)
        {
          tumbler_lifo_scheduler_finish_request (scheduler, request);
          tumbler_mutex_unlock (scheduler->mutex);
          return;
        }
      tumbler_mutex_unlock (scheduler->mutex);

      /* ignore the the URI if has been cancelled already */
      if (g_cancellable_is_cancelled (request->cancellables[n]))
        continue;

      /* create a file info for the current URI */
      uri_needs_update = FALSE;

      G_LOCK (plugin_access_lock);

      /* try to load thumbnail information about the URI */
      if (tumbler_file_info_load (request->infos[n], NULL, &error))
        {
          /* check if we have a thumbnailer for the URI */
          if (request->thumbnailers[n] != NULL)
            {
              /* check if the thumbnail needs an update */
              uri_needs_update = tumbler_file_info_needs_update (request->infos[n]);
            }
          else
            {
              /* no thumbnailer for this URI, we need to emit an error */
              g_set_error (&error, TUMBLER_ERROR, TUMBLER_ERROR_UNSUPPORTED,
                           TUMBLER_ERROR_MESSAGE_NO_THUMBNAILER,
                           tumbler_file_info_get_uri (request->infos[n]));
            }
        }

      G_UNLOCK (plugin_access_lock);

      /* check if the URI is supported */
      if (error == NULL)
        {
          /* put it in the right list depending on its thumbnail status */
          if (uri_needs_update)
            missing_uris = g_list_prepend (missing_uris, GINT_TO_POINTER (n));
          else
            cached_uris = g_list_prepend (cached_uris, request->infos[n]);
        }
      else
        {
          /* emit an error for the URI */
          tumbler_scheduler_emit_uri_error (TUMBLER_SCHEDULER (scheduler), request,
                                            tumbler_file_info_get_uri (request->infos[n]),
                                            error);
          g_clear_error (&error);
        }
    }

  /* check if we have any cached files */
  if (cached_uris != NULL)
    {
      /* allocate a URI array and fill it with all cached URIs */
      uris = g_new0 (const gchar *, g_list_length (cached_uris) + 1);
      for (n = 0, lp = g_list_last (cached_uris); lp != NULL; lp = lp->prev, ++n)
        uris[n] = tumbler_file_info_get_uri (lp->data);
      uris[n] = NULL;

      /* notify others that the cached thumbnails are ready */
      g_signal_emit_by_name (scheduler, "ready", request->handle, uris, request->origin);

      /* free string array and cached list */
      g_list_free (cached_uris);
      g_free (uris);
    }

  /* iterate over invalid/missing URI list */
  for (lp = g_list_last (missing_uris); lp != NULL; lp = lp->prev)
    {
      n = GPOINTER_TO_INT (lp->data);

      /* finish the request if it was dequeued */
      if (request->dequeued)
        {
          tumbler_mutex_lock (scheduler->mutex);
          tumbler_lifo_scheduler_finish_request (scheduler, request);
          tumbler_mutex_unlock (scheduler->mutex);
          g_list_free (missing_uris);
          return;
        }

      for (lq = request->thumbnailers[n]; lq != NULL; lq = lq->next)
        {
          /* We immediately forward error and ready so that clients rapidly know
           * when individual thumbnails are ready. It's a LIFO for better inter-
           * activity with the clients, so we assume this behaviour to be desired. */

          /* forward only the error signal of the last thumbnailer */
          if (lq->next == NULL)
            g_signal_connect (lq->data, "error",
                              G_CALLBACK (tumbler_lifo_scheduler_thumbnailer_error), request);
          else if (tumbler_util_is_debug_logging_enabled (G_LOG_DOMAIN))
            g_signal_connect (lq->data, "error",
                              G_CALLBACK (tumbler_scheduler_thumberr_debuglog), request);

          /* connect to the ready signal of the thumbnailer */
          g_signal_connect (lq->data, "ready",
                            G_CALLBACK (tumbler_lifo_scheduler_thumbnailer_ready), request);

          /* tell the thumbnailer to generate the thumbnail */
          tumbler_thumbnailer_create (lq->data, request->cancellables[n], request->infos[n]);

          /* disconnect from all signals when we're finished */
          g_signal_handlers_disconnect_by_data (lq->data, request);
        }
    }

  /* free list */
  g_list_free (missing_uris);

  tumbler_mutex_lock (scheduler->mutex);

  /* notify others that we're finished processing the request */
  tumbler_lifo_scheduler_finish_request (scheduler, request);

  tumbler_mutex_unlock (scheduler->mutex);
}



static void
tumbler_lifo_scheduler_thumbnailer_error (TumblerThumbnailer      *thumbnailer,
                                          const gchar             *failed_uri,
                                          GQuark                   error_domain,
                                          gint                     error_code,
                                          const gchar             *message,
                                          TumblerSchedulerRequest *request)
{
  g_return_if_fail (TUMBLER_IS_THUMBNAILER (thumbnailer));
  g_return_if_fail (failed_uri != NULL);
  g_return_if_fail (request != NULL);
  g_return_if_fail (TUMBLER_IS_LIFO_SCHEDULER (request->scheduler));

  /* forward the error signal */
  for (guint n = 0; n < request->length; n++)
    {
      if (g_strcmp0 (tumbler_file_info_get_uri (request->infos[n]), failed_uri) == 0)
        {
          const gchar *failed_uris[] = { failed_uri, NULL };
          g_signal_emit_by_name (request->scheduler, "error", request->handle, failed_uris,
                                 error_domain, error_code, message, request->origin);
          break;
        }
    }
}



static void
tumbler_lifo_scheduler_thumbnailer_ready (TumblerThumbnailer      *thumbnailer,
                                          const gchar             *uri,
                                          TumblerSchedulerRequest *request)
{
  g_return_if_fail (TUMBLER_IS_THUMBNAILER (thumbnailer));
  g_return_if_fail (uri != NULL);
  g_return_if_fail (request != NULL);
  g_return_if_fail (TUMBLER_IS_LIFO_SCHEDULER (request->scheduler));

  /* forward the ready signal */
  for (guint n = 0; n < request->length; n++)
    {
      if (g_strcmp0 (tumbler_file_info_get_uri (request->infos[n]), uri) == 0)
        {
          const gchar *uris[] = { uri, NULL };
          g_signal_emit_by_name (request->scheduler, "ready", request->handle, uris, request->origin);

          /* cancel lower priority thumbnailers for this uri */
          g_cancellable_cancel (request->cancellables[n]);
          break;
        }
    }
}



TumblerScheduler *
tumbler_lifo_scheduler_new (const gchar *name)
{
  return g_object_new (TUMBLER_TYPE_LIFO_SCHEDULER, "name", name, NULL);
}
