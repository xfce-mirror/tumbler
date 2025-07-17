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

#include "tumbler-group-scheduler.h"
#include "tumbler-utils.h"

#include "tumbler/tumbler.h"

#include <float.h>
#include <glib/gi18n.h>



/* Property identifiers */
enum
{
  PROP_0,
  PROP_NAME,
};



typedef struct _UriError UriError;



static void
tumbler_group_scheduler_iface_init (TumblerSchedulerIface *iface);
static void
tumbler_group_scheduler_finalize (GObject *object);
static void
tumbler_group_scheduler_get_property (GObject *object,
                                      guint prop_id,
                                      GValue *value,
                                      GParamSpec *pspec);
static void
tumbler_group_scheduler_set_property (GObject *object,
                                      guint prop_id,
                                      const GValue *value,
                                      GParamSpec *pspec);
static void
tumbler_group_scheduler_push (TumblerScheduler *scheduler,
                              TumblerSchedulerRequest *request);
static void
tumbler_group_scheduler_dequeue (TumblerScheduler *scheduler,
                                 guint32 handle);
static void
tumbler_group_scheduler_cancel_by_mount (TumblerScheduler *scheduler,
                                         GMount *mount);
static void
tumbler_group_scheduler_finish_request (TumblerGroupScheduler *scheduler,
                                        TumblerSchedulerRequest *request);
static void
tumbler_group_scheduler_dequeue_request (TumblerSchedulerRequest *request,
                                         gpointer user_data);
static void
tumbler_group_scheduler_thread (gpointer data,
                                gpointer user_data);
static void
tumbler_group_scheduler_thumbnailer_error (TumblerThumbnailer *thumbnailer,
                                           TumblerFileInfo *failed_info,
                                           GQuark error_domain,
                                           gint error_code,
                                           const gchar *message,
                                           TumblerSchedulerRequest *request);
static void
tumbler_group_scheduler_thumbnailer_ready (TumblerThumbnailer *thumbnailer,
                                           TumblerFileInfo *info,
                                           TumblerSchedulerRequest *request);



struct _TumblerGroupScheduler
{
  GObject __parent__;

  GThreadPool *pool;
  TUMBLER_MUTEX (mutex);
  GList *requests;
  guint group;
  gboolean prioritized;

  gchar *name;
};

struct _UriError
{
  guint error_code;
  GQuark error_domain;
  gchar *message;
  gchar *failed_uri;
};



G_LOCK_DEFINE (group_access_lock);



G_DEFINE_TYPE_WITH_CODE (TumblerGroupScheduler,
                         tumbler_group_scheduler,
                         G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (TUMBLER_TYPE_SCHEDULER,
                                                tumbler_group_scheduler_iface_init));



static void
tumbler_group_scheduler_class_init (TumblerGroupSchedulerClass *klass)
{
  GObjectClass *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->finalize = tumbler_group_scheduler_finalize;
  gobject_class->get_property = tumbler_group_scheduler_get_property;
  gobject_class->set_property = tumbler_group_scheduler_set_property;

  g_object_class_override_property (gobject_class, PROP_NAME, "name");
}


static void
tumbler_group_scheduler_iface_init (TumblerSchedulerIface *iface)
{
  iface->push = tumbler_group_scheduler_push;
  iface->dequeue = tumbler_group_scheduler_dequeue;
  iface->cancel_by_mount = tumbler_group_scheduler_cancel_by_mount;
}



static void
tumbler_group_scheduler_init (TumblerGroupScheduler *scheduler)
{
  tumbler_mutex_create (scheduler->mutex);
  scheduler->requests = NULL;

  /* Note that unless we convert this boolean to a TLS (thread-local), that
   * we can only do this 'prioritized' flag with a thread-pool that is set to
   * exclusive: because then the one thread keeps running until the pool is
   * freed. */

  scheduler->prioritized = FALSE;

  /* allocate a pool with a number of threads depending on the system */
  scheduler->pool = g_thread_pool_new (tumbler_group_scheduler_thread,
                                       scheduler, g_get_num_processors (), TRUE, NULL);
}



static void
tumbler_group_scheduler_finalize (GObject *object)
{
  TumblerGroupScheduler *scheduler = TUMBLER_GROUP_SCHEDULER (object);

  /* destroy both thread pools */
  g_thread_pool_free (scheduler->pool, TRUE, TRUE);

  /* release all pending requests and destroy the request list */
  g_list_free_full (scheduler->requests, tumbler_scheduler_request_free);

  /* free the scheduler name */
  g_free (scheduler->name);

  /* destroy the mutex */
  tumbler_mutex_free (scheduler->mutex);

  (*G_OBJECT_CLASS (tumbler_group_scheduler_parent_class)->finalize) (object);
}



static void
tumbler_group_scheduler_get_property (GObject *object,
                                      guint prop_id,
                                      GValue *value,
                                      GParamSpec *pspec)
{
  TumblerGroupScheduler *scheduler = TUMBLER_GROUP_SCHEDULER (object);

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
tumbler_group_scheduler_set_property (GObject *object,
                                      guint prop_id,
                                      const GValue *value,
                                      GParamSpec *pspec)
{
  TumblerGroupScheduler *scheduler = TUMBLER_GROUP_SCHEDULER (object);

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
tumbler_group_scheduler_push (TumblerScheduler *scheduler,
                              TumblerSchedulerRequest *request)
{
  TumblerGroupScheduler *group_scheduler = TUMBLER_GROUP_SCHEDULER (scheduler);

  g_return_if_fail (TUMBLER_IS_GROUP_SCHEDULER (scheduler));
  g_return_if_fail (request != NULL);

  tumbler_mutex_lock (group_scheduler->mutex);

  /* gain ownership over the requests (sets request->scheduler) */
  tumbler_scheduler_take_request (scheduler, request);

  /* prepend the request to the request list */
  group_scheduler->requests = g_list_prepend (group_scheduler->requests, request);

  /* enqueue the request in the pool */
  g_thread_pool_push (group_scheduler->pool, request, NULL);

  tumbler_mutex_unlock (group_scheduler->mutex);
}



static void
tumbler_group_scheduler_dequeue (TumblerScheduler *scheduler,
                                 guint32 handle)
{
  TumblerGroupScheduler *group_scheduler = TUMBLER_GROUP_SCHEDULER (scheduler);

  g_return_if_fail (TUMBLER_IS_GROUP_SCHEDULER (scheduler));
  g_return_if_fail (handle != 0);

  tumbler_mutex_lock (group_scheduler->mutex);

  /* dequeue all requests (usually only one) with this handle */
  g_list_foreach (group_scheduler->requests,
                  (GFunc) tumbler_group_scheduler_dequeue_request,
                  GUINT_TO_POINTER (handle));

  tumbler_mutex_unlock (group_scheduler->mutex);
}



static void
tumbler_group_scheduler_cancel_by_mount (TumblerScheduler *scheduler,
                                         GMount *mount)
{
  TumblerSchedulerRequest *request;
  TumblerGroupScheduler *group_scheduler = TUMBLER_GROUP_SCHEDULER (scheduler);
  GFile *mount_point;
  GFile *file;
  GList *iter;
  guint n;

  g_return_if_fail (TUMBLER_IS_GROUP_SCHEDULER (scheduler));
  g_return_if_fail (G_IS_MOUNT (mount));

  /* determine the root mount point */
  mount_point = g_mount_get_root (mount);

  tumbler_mutex_lock (group_scheduler->mutex);

  /* iterate over all requests */
  for (iter = group_scheduler->requests; iter != NULL; iter = iter->next)
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

  tumbler_mutex_unlock (group_scheduler->mutex);

  /* release the mount point */
  g_object_unref (mount_point);
}



static void
tumbler_group_scheduler_finish_request (TumblerGroupScheduler *scheduler,
                                        TumblerSchedulerRequest *request)
{
  g_return_if_fail (TUMBLER_IS_GROUP_SCHEDULER (scheduler));
  g_return_if_fail (request != NULL);

  /* emit a  signal */
  g_signal_emit_by_name (scheduler, "finished", request->handle, request->origin);

  /* remove the request from the request list */
  scheduler->requests = g_list_remove (scheduler->requests, request);

  /* destroy the request */
  tumbler_scheduler_request_free (request);
}



static void
tumbler_group_scheduler_dequeue_request (TumblerSchedulerRequest *request,
                                         gpointer user_data)
{
  guint handle = GPOINTER_TO_UINT (user_data);
  guint n;

  g_return_if_fail (request != NULL);
  g_return_if_fail (handle != 0);

  /* mark the request as dequeued if the handles match */
  if (request->handle == handle)
    {
      request->dequeued = TRUE;

      /* try cancel all thumbnails that are part of the request */
      for (n = 0; n < request->length; ++n)
        g_cancellable_cancel (request->cancellables[n]);
    }
}



static UriError *
uri_error_new (gint code,
               GQuark domain,
               const gchar *uri,
               const gchar *message)
{
  UriError *error;

  error = g_slice_new0 (UriError);
  error->error_domain = domain;
  error->error_code = code;
  error->failed_uri = g_strdup (uri);
  error->message = g_strdup (message);

  return error;
}



static void
uri_error_free (gpointer data)
{
  UriError *error = data;

  g_free (error->message);
  g_free (error->failed_uri);

  g_slice_free (UriError, error);
}



static void
tumbler_group_scheduler_thread (gpointer data,
                                gpointer user_data)
{
  TumblerSchedulerRequest *request = data;
  TumblerGroupScheduler *scheduler = user_data;
  const gchar **uris;
  const gchar **failed_uris;
  const gchar **success_uris;
  UriError *uri_error;
  gboolean uri_needs_update;
  GString *message;
  GError *error = NULL;
  GList *iter;
  GList *cached_uris = NULL;
  GList *missing_uris = NULL;
  GList *lp, *lq;
  guint n;
  gint error_code = 0;
  GQuark error_domain = 0;

  g_return_if_fail (TUMBLER_IS_GROUP_SCHEDULER (scheduler));
  g_return_if_fail (request != NULL);

  /* Set I/O priority for the exclusive ThreadPool's thread */
  if (!scheduler->prioritized)
    {
      tumbler_scheduler_thread_use_lower_priority ();
      scheduler->prioritized = TRUE;
    }

  /* notify others that we're starting to process this request */
  g_signal_emit_by_name (request->scheduler, "started", request->handle, request->origin);

  /* finish the request if it was dequeued */
  tumbler_mutex_lock (scheduler->mutex);
  if (request->dequeued)
    {
      tumbler_group_scheduler_finish_request (scheduler, request);
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
          tumbler_group_scheduler_finish_request (scheduler, request);
          tumbler_mutex_unlock (scheduler->mutex);
          return;
        }
      tumbler_mutex_unlock (scheduler->mutex);

      /* ignore the the URI if has been cancelled already */
      if (g_cancellable_is_cancelled (request->cancellables[n]))
        continue;

      /* create a file infor for the current URI */
      uri_needs_update = FALSE;

      G_LOCK (group_access_lock);

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

      G_UNLOCK (group_access_lock);

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

  /* initialize lists for grouping failed URIs and URIs for which
   * thumbnails are ready */
  request->uri_errors = NULL;
  request->ready_uris = NULL;

  /* iterate over invalid/missing URI list */
  for (lp = g_list_last (missing_uris); lp != NULL; lp = lp->prev)
    {
      n = GPOINTER_TO_INT (lp->data);

      /* finish the request if it was dequeued */
      tumbler_mutex_lock (scheduler->mutex);
      if (request->dequeued)
        {
          tumbler_group_scheduler_finish_request (scheduler, request);
          tumbler_mutex_unlock (scheduler->mutex);
          return;
        }
      tumbler_mutex_unlock (scheduler->mutex);

      for (lq = request->thumbnailers[n]; lq != NULL; lq = lq->next)
        {
          /* forward only the error signal of the last thumbnailer */
          if (lq->next == NULL)
            g_signal_connect (lq->data, "error",
                              G_CALLBACK (tumbler_group_scheduler_thumbnailer_error), request);
          else if (tumbler_util_is_debug_logging_enabled (G_LOG_DOMAIN))
            g_signal_connect (lq->data, "error",
                              G_CALLBACK (tumbler_scheduler_thumberr_debuglog), request);

          /* connect to the ready signal of the thumbnailer */
          g_signal_connect (lq->data, "ready",
                            G_CALLBACK (tumbler_group_scheduler_thumbnailer_ready), request);

          /* tell the thumbnailer to generate the thumbnail */
          tumbler_thumbnailer_create (lq->data, request->cancellables[n], request->infos[n]);

          /* disconnect from all signals when we're finished */
          g_signal_handlers_disconnect_by_data (lq->data, request);
        }
    }

  tumbler_mutex_lock (scheduler->mutex);

  /* We emit all the errors and ready signals together in order to
   * reduce the overall D-Bus traffic */

  /* check if we have any failed URIs */
  if (request->uri_errors != NULL)
    {
      /* allocate the failed URIs array */
      failed_uris = g_new0 (const gchar *, g_list_length (request->uri_errors) + 1);

      /* allocate the grouped error message */
      message = g_string_new ("");

      for (iter = request->uri_errors, n = 0; iter != NULL; iter = iter->next, ++n)
        {
          uri_error = iter->data;

          /* we use the error code of the first failed URI */
          if (iter == request->uri_errors)
            {
              error_domain = uri_error->error_domain;
              error_code = uri_error->error_code;
            }

          if (uri_error->message != NULL)
            {
              /* we concatenate error messages with a newline inbetween */
              if (iter != request->uri_errors && iter->next != NULL)
                g_string_append_c (message, '\n');

              /* append the current error message */
              g_string_append (message, uri_error->message);
            }

          /* fill the failed_uris array with URIs */
          failed_uris[n] = uri_error->failed_uri;
        }

      /* NULL-terminate the failed URI array */
      failed_uris[n] = NULL;

      /* forward the error signal */
      g_signal_emit_by_name (request->scheduler, "error", request->handle,
                             failed_uris, error_domain, error_code, message->str,
                             request->origin);

      /* free the failed URIs array. Its contents are owned by the URI errors */
      g_free (failed_uris);

      /* free the error message */
      g_string_free (message, TRUE);
    }

  /* free all URI errors and the error URI list */
  g_list_free_full (request->uri_errors, uri_error_free);

  /* check if we have any successfully processed URIs */
  if (request->ready_uris != NULL)
    {
      /* allocate a string array for successful URIs */
      success_uris = g_new0 (const gchar *, g_list_length (request->ready_uris) + 1);

      /* fill the array with all ready URIs */
      for (iter = request->ready_uris, n = 0; iter != NULL; iter = iter->next, ++n)
        success_uris[n] = iter->data;

      /* NULL-terminate the successful URI array */
      success_uris[n] = NULL;

      /* emit a grouped ready signal */
      g_signal_emit_by_name (request->scheduler, "ready", request->handle,
                             success_uris, request->origin);

      /* free the success URI array. Its contents are owned by the ready URI list */
      g_free (success_uris);
    }

  /* free the ready URIs */
  g_list_free_full (request->ready_uris, g_free);

  /* notify others that we're finished processing the request */
  tumbler_group_scheduler_finish_request (scheduler, request);

  tumbler_mutex_unlock (scheduler->mutex);
}



static void
tumbler_group_scheduler_thumbnailer_error (TumblerThumbnailer *thumbnailer,
                                           TumblerFileInfo *failed_info,
                                           GQuark error_domain,
                                           gint error_code,
                                           const gchar *message,
                                           TumblerSchedulerRequest *request)
{
  g_return_if_fail (TUMBLER_IS_THUMBNAILER (thumbnailer));
  g_return_if_fail (TUMBLER_IS_FILE_INFO (failed_info));
  g_return_if_fail (request != NULL);

  for (guint n = 0; n < request->length; n++)
    {
      if (request->infos[n] == failed_info)
        {
          /* add the error to the list */
          UriError *error = uri_error_new (error_code, error_domain,
                                           tumbler_file_info_get_uri (failed_info),
                                           message);
          request->uri_errors = g_list_prepend (request->uri_errors, error);
          break;
        }
    }
}



static void
tumbler_group_scheduler_thumbnailer_ready (TumblerThumbnailer *thumbnailer,
                                           TumblerFileInfo *info,
                                           TumblerSchedulerRequest *request)
{
  g_return_if_fail (TUMBLER_IS_THUMBNAILER (thumbnailer));
  g_return_if_fail (TUMBLER_IS_FILE_INFO (info));
  g_return_if_fail (request != NULL);

  for (guint n = 0; n < request->length; n++)
    {
      if (request->infos[n] == info)
        {
          /* add the uri to the list */
          request->ready_uris = g_list_prepend (request->ready_uris, g_strdup (tumbler_file_info_get_uri (info)));

          /* cancel lower priority thumbnailers for this uri */
          g_cancellable_cancel (request->cancellables[n]);
          break;
        }
    }
}



TumblerScheduler *
tumbler_group_scheduler_new (const gchar *name)
{
  return g_object_new (TUMBLER_TYPE_GROUP_SCHEDULER, "name", name, NULL);
}
