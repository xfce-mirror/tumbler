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

#include <tumblerd/tumbler-lifo-scheduler.h>
#include <tumblerd/tumbler-scheduler.h>



/* Property identifiers */
enum
{
  PROP_0,
  PROP_KIND,
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
static void tumbler_lifo_scheduler_unqueue           (TumblerScheduler          *scheduler,
                                                           guint                      handle);
static void tumbler_lifo_scheduler_finish_request    (TumblerLifoScheduler *scheduler,
                                                           TumblerSchedulerRequest   *request);
static void tumbler_lifo_scheduler_unqueue_request   (TumblerSchedulerRequest   *request,
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
  guint        lifo;
  gchar       *kind;
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

  g_object_class_install_property (gobject_class,
                                   PROP_KIND,
                                   g_param_spec_string ("kind",
                                                        "kind",
                                                        "kind",
                                                        NULL,
                                                        G_PARAM_CONSTRUCT_ONLY |
                                                        G_PARAM_READWRITE));
}

static const gchar*
tumbler_lifo_scheduler_get_kind (TumblerScheduler *scheduler)
{
  TumblerLifoScheduler *s = TUMBLER_LIFO_SCHEDULER (scheduler);
  return s->kind;
}

static void
tumbler_lifo_scheduler_iface_init (TumblerSchedulerIface *iface)
{
  iface->push = tumbler_lifo_scheduler_push;
  iface->unqueue = tumbler_lifo_scheduler_unqueue;
  iface->get_kind = tumbler_lifo_scheduler_get_kind;
}



static void
tumbler_lifo_scheduler_init (TumblerLifoScheduler *scheduler)
{
  scheduler->mutex = g_mutex_new ();
  scheduler->requests = NULL;

  /* allocate a pool with max. 2 threads for request with <= lifo URIs */
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
    case PROP_KIND:
      g_value_set_string (value, scheduler->kind);
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
    case PROP_KIND:
      scheduler->kind = g_value_dup_string (value);
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
  TumblerLifoScheduler *lifo_scheduler = 
    TUMBLER_LIFO_SCHEDULER (scheduler);

  g_return_if_fail (TUMBLER_IS_LIFO_SCHEDULER (scheduler));
  g_return_if_fail (request != NULL);

  g_mutex_lock (lifo_scheduler->mutex);
  
  /* gain ownership over the requests (sets request->scheduler) */
  tumbler_scheduler_take_request (scheduler, request);

  /* prepend the request to the request list */
  lifo_scheduler->requests = 
    g_list_prepend (lifo_scheduler->requests, request);

  /* enqueue the request in the pool */
  g_thread_pool_push (lifo_scheduler->pool, request, NULL);

  g_mutex_unlock (lifo_scheduler->mutex);
}



static void
tumbler_lifo_scheduler_unqueue (TumblerScheduler *scheduler,
                                guint             handle)
{
  TumblerLifoScheduler *lifo_scheduler = 
    TUMBLER_LIFO_SCHEDULER (scheduler);

  g_return_if_fail (TUMBLER_IS_LIFO_SCHEDULER (scheduler));
  g_return_if_fail (handle != 0);

  g_mutex_lock (lifo_scheduler->mutex);

  g_list_foreach (lifo_scheduler->requests, 
                  (GFunc) tumbler_lifo_scheduler_unqueue_request, 
                  GUINT_TO_POINTER (handle));

  g_mutex_unlock (lifo_scheduler->mutex);
}



static void
tumbler_lifo_scheduler_finish_request (TumblerLifoScheduler *scheduler,
                                       TumblerSchedulerRequest   *request)
{
  g_return_if_fail (TUMBLER_IS_LIFO_SCHEDULER (scheduler));
  g_return_if_fail (request != NULL);

  g_signal_emit_by_name (scheduler, "finished", request->handle);

  scheduler->requests = g_list_remove (scheduler->requests,
                                       request);

  tumbler_scheduler_request_free (request);
}



static void
tumbler_lifo_scheduler_unqueue_request (TumblerSchedulerRequest *request,
                                        gpointer                 user_data)
{
  guint handle = GPOINTER_TO_UINT (user_data);

  g_return_if_fail (request != NULL);
  g_return_if_fail (handle != 0);

  if (request->handle == handle)
    request->unqueued = TRUE;
}



static void
tumbler_lifo_scheduler_thread (gpointer data,
                               gpointer user_data)
{
  TumblerLifoScheduler *scheduler = user_data;
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

  g_return_if_fail (TUMBLER_IS_LIFO_SCHEDULER (scheduler));
  g_return_if_fail (request != NULL);

  /* notify others that we're starting to process this request */
  g_signal_emit_by_name (request->scheduler, "started", request->handle);


  /* finish the request if it was unqueued */
  if (request->unqueued)
    {
      g_mutex_lock (scheduler->mutex);
      tumbler_lifo_scheduler_finish_request (scheduler, request);
      g_mutex_unlock (scheduler->mutex);
      return;
    }

  /* process URI by URI */
  for (n = 0; request->uris[n] != NULL; ++n)
    {
      /* finish the request if it was unqueued */
      if (request->unqueued)
        {
          g_mutex_lock (scheduler->mutex);
          tumbler_lifo_scheduler_finish_request (scheduler, request);
          g_mutex_unlock (scheduler->mutex);
          return;
        }

      info = tumbler_file_info_new (request->uris[n]);
      uri_needs_update = FALSE;

      G_LOCK (plugin_access_lock);

      if (tumbler_file_info_load (info, NULL, &error))
        {
          if (request->thumbnailers[n] != NULL)
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
          else
            {
              g_set_error (&error, TUMBLER_ERROR, TUMBLER_ERROR_NO_THUMBNAILER,
                           _("No thumbnailer available for \"%s\""), 
                           request->uris[n]);
            }
        }

      g_object_unref (info);

      G_UNLOCK (plugin_access_lock);

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

      /* finish the request if it was unqueued */
      if (request->unqueued)
        {
          g_mutex_lock (scheduler->mutex);
          tumbler_lifo_scheduler_finish_request (scheduler, request);
          g_mutex_unlock (scheduler->mutex);
          return;
        }

      /* We immediately forward error and ready so that clients rapidly know
       * when individual thumbnails are ready. It's a LIFO for better inter-
       * activity with the clients, so we assume this behaviour to be wanted. */

      /* connect to the error signal of the thumbnailer */
      g_signal_connect (request->thumbnailers[n], "error", 
                        G_CALLBACK (tumbler_lifo_scheduler_thumbnailer_error),
                        request);

      /* connect to the ready signal of the thumbnailer */
      g_signal_connect (request->thumbnailers[n], "ready",
                        G_CALLBACK (tumbler_lifo_scheduler_thumbnailer_ready),
                        request);

      /* tell the thumbnailer to generate the thumbnail */
      tumbler_thumbnailer_create (request->thumbnailers[n], request->uris[n], 
                                  request->mime_hints[n]);

      /* disconnect from all signals when we're finished */
      g_signal_handlers_disconnect_matched (request->thumbnailers[n],
                                            G_SIGNAL_MATCH_DATA,
                                            0, 0, NULL, NULL, request);
    }

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
                         error_code, message);
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
  g_signal_emit_by_name (request->scheduler, "ready", uris);
}



TumblerScheduler *
tumbler_lifo_scheduler_new (const gchar *kind)
{
  return g_object_new (TUMBLER_TYPE_LIFO_SCHEDULER, "kind", kind, NULL);
}
