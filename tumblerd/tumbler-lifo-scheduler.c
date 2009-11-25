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
                                                      guint                      handle);
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
  GMutex      *mutex;
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
  scheduler->mutex = g_mutex_new ();
  scheduler->requests = NULL;

  /* allocate a thread pool with a maximum of one thread */
  scheduler->pool = g_thread_pool_new (tumbler_lifo_scheduler_thread,
                                       scheduler, 1, TRUE, NULL);

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

  /* release all pending requests */
  g_list_foreach (scheduler->requests, (GFunc) tumbler_scheduler_request_free, NULL);

  /* destroy the request list */
  g_list_free (scheduler->requests);

  /* free the scheduler name */
  g_free (scheduler->name);

  /* destroy the mutex */
  g_mutex_free (scheduler->mutex);

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

  g_mutex_lock (lifo_scheduler->mutex);
  
  /* gain ownership over the requests (sets request->scheduler) */
  tumbler_scheduler_take_request (scheduler, request);

  /* prepend the request to the request list */
  lifo_scheduler->requests = g_list_prepend (lifo_scheduler->requests, request);

  /* enqueue the request in the pool */
  g_thread_pool_push (lifo_scheduler->pool, request, NULL);

  g_mutex_unlock (lifo_scheduler->mutex);
}



static void
tumbler_lifo_scheduler_dequeue (TumblerScheduler *scheduler,
                                guint             handle)
{
  TumblerLifoScheduler *lifo_scheduler = TUMBLER_LIFO_SCHEDULER (scheduler);

  g_return_if_fail (TUMBLER_IS_LIFO_SCHEDULER (scheduler));
  g_return_if_fail (handle != 0);

  g_mutex_lock (lifo_scheduler->mutex);

  /* dequeue all requests (usually only one) with this handle */
  g_list_foreach (lifo_scheduler->requests, 
                  (GFunc) tumbler_lifo_scheduler_dequeue_request, 
                  GUINT_TO_POINTER (handle));

  g_mutex_unlock (lifo_scheduler->mutex);
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

  g_mutex_lock (lifo_scheduler->mutex);

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

  g_mutex_unlock (lifo_scheduler->mutex);

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
  GList                   *lp;
  guint                    n;

  g_return_if_fail (TUMBLER_IS_LIFO_SCHEDULER (scheduler));
  g_return_if_fail (request != NULL);

  /* notify others that we're starting to process this request */
  g_signal_emit_by_name (request->scheduler, "started", request->handle,
                         request->origin);

  /* finish the request if it was already dequeued */
  g_mutex_lock (scheduler->mutex);
  if (request->dequeued)
    {
      tumbler_lifo_scheduler_finish_request (scheduler, request);
      g_mutex_unlock (scheduler->mutex);
      return;
    }
  g_mutex_unlock (scheduler->mutex);

  /* process URI by URI */
  for (n = 0; n < request->length; ++n)
    {
      /* finish the request if it was dequeued */
      g_mutex_lock (scheduler->mutex);
      if (request->dequeued)
        {
          tumbler_lifo_scheduler_finish_request (scheduler, request);
          g_mutex_unlock (scheduler->mutex);
          return;
        }
      g_mutex_unlock (scheduler->mutex);

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
              g_set_error (&error, TUMBLER_ERROR, TUMBLER_ERROR_NO_THUMBNAILER,
                           _("No thumbnailer available for \"%s\""), 
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
          g_mutex_lock (scheduler->mutex);
          tumbler_lifo_scheduler_finish_request (scheduler, request);
          g_mutex_unlock (scheduler->mutex);
          return;
        }

      /* We immediately forward error and ready so that clients rapidly know
       * when individual thumbnails are ready. It's a LIFO for better inter-
       * activity with the clients, so we assume this behaviour to be desired. */

      /* connect to the error signal of the thumbnailer */
      g_signal_connect (request->thumbnailers[n], "error", 
                        G_CALLBACK (tumbler_lifo_scheduler_thumbnailer_error),
                        request);

      /* connect to the ready signal of the thumbnailer */
      g_signal_connect (request->thumbnailers[n], "ready",
                        G_CALLBACK (tumbler_lifo_scheduler_thumbnailer_ready),
                        request);

      /* tell the thumbnailer to generate the thumbnail */
      tumbler_thumbnailer_create (request->thumbnailers[n], 
                                  request->cancellables[n],
                                  request->infos[n]);

      /* disconnect from all signals when we're finished */
      g_signal_handlers_disconnect_matched (request->thumbnailers[n],
                                            G_SIGNAL_MATCH_DATA,
                                            0, 0, NULL, NULL, request);
    }

  /* free list */
  g_list_free (missing_uris);

  g_mutex_lock (scheduler->mutex);

  /* notify others that we're finished processing the request */
  tumbler_lifo_scheduler_finish_request (scheduler, request);

  g_mutex_unlock (scheduler->mutex);
}



static void
tumbler_lifo_scheduler_thumbnailer_error (TumblerThumbnailer      *thumbnailer,
                                          const gchar             *failed_uri,
                                          gint                     error_code,
                                          const gchar             *message,
                                          TumblerSchedulerRequest *request)
{
  const gchar *failed_uris[] = { failed_uri, NULL };

  g_return_if_fail (TUMBLER_IS_THUMBNAILER (thumbnailer));
  g_return_if_fail (failed_uri != NULL);
  g_return_if_fail (request != NULL);
  g_return_if_fail (TUMBLER_IS_LIFO_SCHEDULER (request->scheduler));

  /* forward the error signal */
  g_signal_emit_by_name (request->scheduler, "error", request->handle, failed_uris, 
                         error_code, message, request->origin);
}



static void
tumbler_lifo_scheduler_thumbnailer_ready (TumblerThumbnailer      *thumbnailer,
                                          const gchar             *uri,
                                          TumblerSchedulerRequest *request)
{
  const gchar *uris[] = { uri, NULL };

  g_return_if_fail (TUMBLER_IS_THUMBNAILER (thumbnailer));
  g_return_if_fail (uri != NULL);
  g_return_if_fail (request != NULL);
  g_return_if_fail (TUMBLER_IS_LIFO_SCHEDULER (request->scheduler));

  /* forward the ready signal */
  g_signal_emit_by_name (request->scheduler, "ready", request->handle, uris, 
                         request->origin);
}



TumblerScheduler *
tumbler_lifo_scheduler_new (const gchar *name)
{
  return g_object_new (TUMBLER_TYPE_LIFO_SCHEDULER, "name", name, NULL);
}
