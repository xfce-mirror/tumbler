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

#include <tumblerd/tumbler-group-scheduler.h>
#include <tumblerd/tumbler-scheduler.h>



/* Property identifiers */
enum
{
  PROP_0,
  PROP_NAME,
};



typedef struct _UriError UriError;



static void tumbler_group_scheduler_iface_init        (TumblerSchedulerIface     *iface);
static void tumbler_group_scheduler_finalize          (GObject                   *object);
static void tumbler_group_scheduler_get_property      (GObject                   *object,
                                                       guint                      prop_id,
                                                       GValue                    *value,
                                                       GParamSpec                *pspec);
static void tumbler_group_scheduler_set_property      (GObject                   *object,
                                                       guint                      prop_id,
                                                       const GValue              *value,
                                                       GParamSpec                *pspec);
static void tumbler_group_scheduler_push              (TumblerScheduler          *scheduler,
                                                       TumblerSchedulerRequest   *request);
static void tumbler_group_scheduler_unqueue           (TumblerScheduler          *scheduler,
                                                       guint                      handle);
static void tumbler_group_scheduler_finish_request    (TumblerGroupScheduler     *scheduler,
                                                       TumblerSchedulerRequest   *request);
static void tumbler_group_scheduler_unqueue_request   (TumblerSchedulerRequest   *request,
                                                       gpointer                   user_data);
static void tumbler_group_scheduler_thread            (gpointer                   data,
                                                       gpointer                   user_data);
static void tumbler_group_scheduler_thumbnailer_error (TumblerThumbnailer        *thumbnailer,
                                                       const gchar               *failed_uri,
                                                       gint                       error_code,
                                                       const gchar               *message,
                                                       GList                    **uri_errors);
static void tumbler_group_scheduler_thumbnailer_ready (TumblerThumbnailer        *thumbnailer,
                                                       const gchar               *uri,
                                                       GList                    **ready_uris);



struct _TumblerGroupSchedulerClass
{
  GObjectClass __parent__;
};

struct _TumblerGroupScheduler
{
  GObject __parent__;

  GThreadPool *pool;
  GMutex      *mutex;
  GList       *requests;
  guint        group;
  gboolean     prioritized;

  gchar       *name;
};

struct _UriError
{
  guint  error_code;
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
  iface->unqueue = tumbler_group_scheduler_unqueue;
}



static void
tumbler_group_scheduler_init (TumblerGroupScheduler *scheduler)
{
  scheduler->mutex = g_mutex_new ();
  scheduler->requests = NULL;

  /* Note that unless we convert this boolean to a TLS (thread-local), that
   * we can only do this 'prioritized' flag with a thread-pool that is set to
   * exclusive: because then the one thread keeps running until the pool is 
   * freed. */

  scheduler->prioritized = FALSE;

  /* allocate a pool with one thread for all requests */
  scheduler->pool = g_thread_pool_new (tumbler_group_scheduler_thread, 
                                       scheduler, 1, TRUE, NULL);

}



static void
tumbler_group_scheduler_finalize (GObject *object)
{
  TumblerGroupScheduler *scheduler = TUMBLER_GROUP_SCHEDULER (object);

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

  (*G_OBJECT_CLASS (tumbler_group_scheduler_parent_class)->finalize) (object);
}



static void
tumbler_group_scheduler_get_property (GObject    *object,
                                      guint       prop_id,
                                      GValue     *value,
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
tumbler_group_scheduler_set_property (GObject      *object,
                                      guint         prop_id,
                                      const GValue *value,
                                      GParamSpec   *pspec)
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
tumbler_group_scheduler_push (TumblerScheduler        *scheduler,
                              TumblerSchedulerRequest *request)
{
  TumblerGroupScheduler *group_scheduler = 
    TUMBLER_GROUP_SCHEDULER (scheduler);

  g_return_if_fail (TUMBLER_IS_GROUP_SCHEDULER (scheduler));
  g_return_if_fail (request != NULL);

  g_mutex_lock (group_scheduler->mutex);
  
  /* gain ownership over the requests (sets request->scheduler) */
  tumbler_scheduler_take_request (scheduler, request);

  /* prepend the request to the request list */
  group_scheduler->requests = g_list_prepend (group_scheduler->requests, request);

  /* enqueue the request in the pool */
  g_thread_pool_push (group_scheduler->pool, request, NULL);

  g_mutex_unlock (group_scheduler->mutex);
}



static void
tumbler_group_scheduler_unqueue (TumblerScheduler *scheduler,
                                 guint             handle)
{
  TumblerGroupScheduler *group_scheduler = 
    TUMBLER_GROUP_SCHEDULER (scheduler);

  g_return_if_fail (TUMBLER_IS_GROUP_SCHEDULER (scheduler));
  g_return_if_fail (handle != 0);

  g_mutex_lock (group_scheduler->mutex);

  /* unqueue all requests (usually only one) with this handle */
  g_list_foreach (group_scheduler->requests, 
                  (GFunc) tumbler_group_scheduler_unqueue_request, 
                  GUINT_TO_POINTER (handle));

  g_mutex_unlock (group_scheduler->mutex);
}



static void
tumbler_group_scheduler_finish_request (TumblerGroupScheduler *scheduler,
                                        TumblerSchedulerRequest   *request)
{
  g_return_if_fail (TUMBLER_IS_GROUP_SCHEDULER (scheduler));
  g_return_if_fail (request != NULL);

  /* emit a finished signal */
  g_signal_emit_by_name (scheduler, "finished", request->handle);

  /* remove the request from the request list */
  scheduler->requests = g_list_remove (scheduler->requests, request);

  /* destroy the request */
  tumbler_scheduler_request_free (request);
}



static void
tumbler_group_scheduler_unqueue_request (TumblerSchedulerRequest *request,
                                         gpointer                 user_data)
{
  guint handle = GPOINTER_TO_UINT (user_data);

  g_return_if_fail (request != NULL);
  g_return_if_fail (handle != 0);

  /* mark the request as unqueued if the handles match */
  if (request->handle == handle)
    request->unqueued = TRUE;
}



static UriError *
uri_error_new (gint         code,
               const gchar *uri,
               const gchar *message)
{
  UriError *error;

  error = g_slice_new0 (UriError);
  error->error_code = code;
  error->failed_uri = g_strdup (uri);
  error->message = g_strdup (message);

  return error;
}



static void
uri_error_free (UriError *error)
{
  g_free (error->message);
  g_free (error->failed_uri);

  g_slice_free (UriError, error);
}



static void
tumbler_group_scheduler_thread (gpointer data,
                                gpointer user_data)
{
  TumblerSchedulerRequest *request = data;
  TumblerGroupScheduler   *scheduler = user_data;
  TumblerFileInfo         *info;
  const gchar            **uris;
  const gchar            **failed_uris;
  const gchar            **success_uris;
  UriError                *uri_error;
  gboolean                 outdated;
  gboolean                 uri_needs_update;
  GString                 *message;
  guint64                  mtime;
  GError                  *error = NULL;
  GList                   *iter;
  GList                   *uri_errors;
  GList                   *ready_uris;
  GList                   *cached_uris = NULL;
  GList                   *missing_uris = NULL;
  GList                   *thumbnails;
  GList                   *lp;
  guint                    n;
  gint                     error_code;

  g_return_if_fail (TUMBLER_IS_GROUP_SCHEDULER (scheduler));
  g_return_if_fail (request != NULL);

  /* Set I/O priority for the exclusive ThreadPool's thread */
  if (!scheduler->prioritized) 
    {
      tumbler_scheduler_thread_use_lower_priority ();
      scheduler->prioritized = TRUE;
    }

  /* notify others that we're starting to process this request */
  g_signal_emit_by_name (request->scheduler, "started", request->handle);

  /* finish the request if it was unqueued */
  g_mutex_lock (scheduler->mutex);
  if (request->unqueued)
    {
      tumbler_group_scheduler_finish_request (scheduler, request);
      return;
    }
  g_mutex_unlock (scheduler->mutex);

  /* process URI by URI */
  for (n = 0; request->uris[n] != NULL; ++n)
    {
      /* finish the request if it was unqueued */
      g_mutex_lock (scheduler->mutex);
      if (request->unqueued)
        {
          tumbler_group_scheduler_finish_request (scheduler, request);
          return;
        }
      g_mutex_unlock (scheduler->mutex);

      /* create a file infor for the current URI */
      info = tumbler_file_info_new (request->uris[n]);
      uri_needs_update = FALSE;

      G_LOCK (group_access_lock);

      /* try to load thumbnail information about the URI */
      if (tumbler_file_info_load (info, NULL, &error))
        {
          /* check if we have a thumbnailer for the URI */
          if (request->thumbnailers[n] != NULL)
            {
              /* compute the last modification time of the URI */
              mtime = tumbler_file_info_get_mtime (info);

              /* get a list of all thumbnails for this URI */
              thumbnails = tumbler_file_info_get_thumbnails (info);

              /* iterate over them */
              for (lp = thumbnails; error == NULL && lp != NULL; lp = lp->next)
                {
                  /* try to load the thumbnail information */
                  if (tumbler_thumbnail_load (lp->data, NULL, &error))
                    {
                      /* check if the thumbnail needs an update */
                      outdated = tumbler_thumbnail_needs_update (lp->data, 
                                                                 request->uris[n],
                                                                 mtime);

                      /* if at least one thumbnail is out of date, we need to 
                       * regenerate thumbnails for the URI */
                      uri_needs_update = uri_needs_update || outdated;
                    }
                }
            }
          else
            {
              /* no thumbnailer for this URI, we need to emit an error */
              g_set_error (&error, TUMBLER_ERROR, TUMBLER_ERROR_NO_THUMBNAILER,
                           _("No thumbnailer available for \"%s\""), 
                           request->uris[n]);
            }
        }

      /* release the file info */
      g_object_unref (info);

      G_UNLOCK (group_access_lock);

      /* check if the URI is supported */
      if (error == NULL)
        {
          /* put it in the right list depending on its thumbnail status */
          if (uri_needs_update)
            missing_uris = g_list_prepend (missing_uris, GINT_TO_POINTER (n));
          else
            cached_uris = g_list_prepend (cached_uris, request->uris[n]);
        }
      else
        {
          /* emit an error for the URI */
          tumbler_scheduler_emit_uri_error (TUMBLER_SCHEDULER (scheduler), request,
                                            request->uris[n], error);
          g_clear_error (&error);
        }
    }

  /* check if we have any cached files */
  if (cached_uris != NULL)
    {
      /* allocate a URI array and fill it with all cached URIs */
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

  /* initialize lists for grouping failed URIs and URIs for which 
   * thumbnails are ready */
  uri_errors = NULL;
  ready_uris = NULL;

  /* iterate over invalid/missing URI list */
  for (lp = g_list_last (missing_uris); lp != NULL; lp = lp->prev)
    {
      n = GPOINTER_TO_INT (lp->data);

      /* finish the request if it was unqueued */
      g_mutex_lock (scheduler->mutex);
      if (request->unqueued)
        {
          tumbler_group_scheduler_finish_request (scheduler, request);
          return;
        }
      g_mutex_unlock (scheduler->mutex);

      /* connect to the error signal of the thumbnailer */
      g_signal_connect (request->thumbnailers[n], "error", 
                        G_CALLBACK (tumbler_group_scheduler_thumbnailer_error),
                        &uri_errors);

      /* connect to the ready signal of the thumbnailer */
      g_signal_connect (request->thumbnailers[n], "ready",
                        G_CALLBACK (tumbler_group_scheduler_thumbnailer_ready),
                        &ready_uris);

      /* tell the thumbnailer to generate the thumbnail */
      tumbler_thumbnailer_create (request->thumbnailers[n], request->uris[n], 
                                  request->mime_hints[n]);

      /* disconnect from all signals when we're finished */
      g_signal_handlers_disconnect_matched (request->thumbnailers[n],
                                            G_SIGNAL_MATCH_DATA,
                                            0, 0, NULL, NULL, request);
    }

  g_mutex_lock (scheduler->mutex);

  /* We emit all the errors and ready signals together in order to 
   * reduce the overall D-Bus traffic */

  /* check if we have any failed URIs */
  if (uri_errors != NULL)
    {
      /* allocate the failed URIs array */
      failed_uris = g_new0 (const gchar *, g_list_length (uri_errors) + 1);

      /* allocate the grouped error message */
      message = g_string_new ("");

      for (iter = uri_errors, n = 0; iter != NULL; iter = iter->next, ++n) 
        {
          uri_error = iter->data;

          /* we use the error code of the first failed URI */
          if (iter == uri_errors) 
            error_code = uri_error->error_code;

          if (uri_error->message != NULL)
            {
              /* we concatenate error messages with a newline inbetween */
              if (iter != uri_errors && iter->next != NULL)
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
                             failed_uris, error_code, message->str);

      /* free the failed URIs array. Its contents are owned by the URI errors */
      g_free (failed_uris);

      /* free the error message */
      g_string_free (message, TRUE);
    }

  /* free all URI errors and the error URI list */
  g_list_foreach (uri_errors, (GFunc) uri_error_free, NULL);
  g_list_free (uri_errors);

  /* check if we have any successfully processed URIs */
  if (ready_uris != NULL)
    {
      /* allocate a string array for successful URIs */
      success_uris = g_new0 (const gchar *, g_list_length (ready_uris) + 1);

      /* fill the array with all ready URIs */
      for (iter = ready_uris, n = 0; iter != NULL; iter = iter->next, ++n)
        success_uris[n] = iter->data;

      /* NULL-terminate the successful URI array */
      success_uris[n] = NULL;

      /* emit a grouped ready signal */
      g_signal_emit_by_name (request->scheduler, "ready", success_uris);

      /* free the success URI array. Its contents are owned by the ready URI list */
      g_free (success_uris);
    }

  /* free the ready URIs */
  g_list_foreach (ready_uris, (GFunc) g_free, NULL);
  g_list_free (ready_uris); 

  /* notify others that we're finished processing the request */
  tumbler_group_scheduler_finish_request (scheduler, request);

  g_mutex_unlock (scheduler->mutex);
}



static void
tumbler_group_scheduler_thumbnailer_error (TumblerThumbnailer *thumbnailer,
                                           const gchar        *failed_uri,
                                           gint                error_code,
                                           const gchar        *message,
                                           GList             **uri_errors)
{
  UriError *error;

  g_return_if_fail (TUMBLER_IS_THUMBNAILER (thumbnailer));
  g_return_if_fail (failed_uri != NULL);
  g_return_if_fail (error_code < 0);
  g_return_if_fail (uri_errors != NULL);

  /* allocate a new URI error */
  error = uri_error_new (error_code, failed_uri, message);

  /* add the error to the list */
  *uri_errors = g_list_prepend (*uri_errors, error);
}



static void
tumbler_group_scheduler_thumbnailer_ready (TumblerThumbnailer *thumbnailer,
                                           const gchar        *uri,
                                           GList             **ready_uris)
{
  g_return_if_fail (TUMBLER_IS_THUMBNAILER (thumbnailer));
  g_return_if_fail (uri != NULL);
  g_return_if_fail (ready_uris != NULL);

  *ready_uris = g_list_prepend (*ready_uris, g_strdup (uri));
}



TumblerScheduler *
tumbler_group_scheduler_new (const gchar *name)
{
  return g_object_new (TUMBLER_TYPE_GROUP_SCHEDULER, "name", name, NULL);
}
