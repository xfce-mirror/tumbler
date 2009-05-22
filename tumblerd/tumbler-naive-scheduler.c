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
#include <glib-object.h>

#include <tumblerd/tumbler-naive-scheduler.h>
#include <tumblerd/tumbler-scheduler.h>



#define TUMBLER_NAIVE_SCHEDULER_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), TUMBLER_TYPE_NAIVE_SCHEDULER, TumblerNaiveSchedulerPrivate))



/* Property identifiers */
enum
{
  PROP_0,
};



static void tumbler_naive_scheduler_class_init        (TumblerNaiveSchedulerClass *klass);
static void tumbler_naive_scheduler_iface_init        (TumblerSchedulerIface      *iface);
static void tumbler_naive_scheduler_init              (TumblerNaiveScheduler      *scheduler);
static void tumbler_naive_scheduler_constructed       (GObject                    *object);
static void tumbler_naive_scheduler_finalize          (GObject                    *object);
static void tumbler_naive_scheduler_get_property      (GObject                    *object,
                                                       guint                       prop_id,
                                                       GValue                     *value,
                                                       GParamSpec                 *pspec);
static void tumbler_naive_scheduler_set_property      (GObject                    *object,
                                                       guint                       prop_id,
                                                       const GValue               *value,
                                                       GParamSpec                 *pspec);
static void tumbler_naive_scheduler_push              (TumblerScheduler           *scheduler,
                                                       TumblerSchedulerRequest    *request);
static void tumbler_naive_scheduler_thumbnailer_error (TumblerThumbnailer         *thumbnailer,
                                                       const gchar                *failed_uri,
                                                       gint                        error_code,
                                                       const gchar                *message,
                                                       TumblerSchedulerRequest    *request);
static void tumbler_naive_scheduler_thumbnailer_ready (TumblerThumbnailer         *thumbnailer,
                                                       const gchar                *uri,
                                                       TumblerSchedulerRequest    *request);



struct _TumblerNaiveSchedulerClass
{
  GObjectClass __parent__;
};

struct _TumblerNaiveScheduler
{
  GObject __parent__;

  TumblerNaiveSchedulerPrivate *priv;
};

struct _TumblerNaiveSchedulerPrivate
{
  GThreadPool *large_pool;
  GThreadPool *small_pool;
};



static GObjectClass *tumbler_naive_scheduler_parent_class = NULL;



GType
tumbler_naive_scheduler_get_type (void)
{
  static GType type = G_TYPE_INVALID;

  if (G_UNLIKELY (type == G_TYPE_INVALID))
    {
      static const GInterfaceInfo info =
      {
        (GInterfaceInitFunc) tumbler_naive_scheduler_iface_init,
        NULL,
        NULL,
      };

      type = g_type_register_static_simple (G_TYPE_OBJECT, 
                                            "TumblerNaiveScheduler",
                                            sizeof (TumblerNaiveSchedulerClass),
                                            (GClassInitFunc) tumbler_naive_scheduler_class_init,
                                            sizeof (TumblerNaiveScheduler),
                                            (GInstanceInitFunc) tumbler_naive_scheduler_init,
                                            0);

      g_type_add_interface_static (type, TUMBLER_TYPE_SCHEDULER, &info);
    }

  return type;
}



static void
tumbler_naive_scheduler_class_init (TumblerNaiveSchedulerClass *klass)
{
  GObjectClass *gobject_class;

  g_type_class_add_private (klass, sizeof (TumblerNaiveSchedulerPrivate));

  /* Determine the parent type class */
  tumbler_naive_scheduler_parent_class = g_type_class_peek_parent (klass);

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->constructed = tumbler_naive_scheduler_constructed; 
  gobject_class->finalize = tumbler_naive_scheduler_finalize; 
  gobject_class->get_property = tumbler_naive_scheduler_get_property;
  gobject_class->set_property = tumbler_naive_scheduler_set_property;
}



static void
tumbler_naive_scheduler_iface_init (TumblerSchedulerIface *iface)
{
  iface->push = tumbler_naive_scheduler_push;
}



static void
tumbler_naive_scheduler_init (TumblerNaiveScheduler *scheduler)
{
  scheduler->priv = TUMBLER_NAIVE_SCHEDULER_GET_PRIVATE (scheduler);
}



static void
tumbler_naive_scheduler_constructed (GObject *object)
{
  TumblerNaiveScheduler *scheduler = TUMBLER_NAIVE_SCHEDULER (object);
}



static void
tumbler_naive_scheduler_finalize (GObject *object)
{
  TumblerNaiveScheduler *scheduler = TUMBLER_NAIVE_SCHEDULER (object);

  (*G_OBJECT_CLASS (tumbler_naive_scheduler_parent_class)->finalize) (object);
}



static void
tumbler_naive_scheduler_get_property (GObject    *object,
                                      guint       prop_id,
                                      GValue     *value,
                                      GParamSpec *pspec)
{
  TumblerNaiveScheduler *scheduler = TUMBLER_NAIVE_SCHEDULER (object);

  switch (prop_id)
    {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}



static void
tumbler_naive_scheduler_set_property (GObject      *object,
                                      guint         prop_id,
                                      const GValue *value,
                                      GParamSpec   *pspec)
{
  TumblerNaiveScheduler *scheduler = TUMBLER_NAIVE_SCHEDULER (object);

  switch (prop_id)
    {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}



static void
tumbler_naive_scheduler_push (TumblerScheduler        *scheduler,
                              TumblerSchedulerRequest *request)
{
  gint n;

  g_return_if_fail (TUMBLER_IS_NAIVE_SCHEDULER (scheduler));
  g_return_if_fail (request != NULL);

  /* Gain ownership over the request (sets request->scheduler) */
  tumbler_scheduler_take_request (scheduler, request);

  /* tell everybody that we've started processing the request */
  g_signal_emit_by_name (scheduler, "started", request->handle);

  /* iterate over all elements in the three arrays, assuming they all have
   * the same size */
  for (n = 0; request->uris[n] != NULL; ++n)
    {
      /* check if we have a thumbnailer for the n-th URI */
      if (request->thumbnailers[n] != NULL)
        {
          /* connect to the error signal of the thumbnailer */
          g_signal_connect (request->thumbnailers[n], "error",
                            G_CALLBACK (tumbler_naive_scheduler_thumbnailer_error),
                            request);

          /* connect to the ready signal of the thumbnailer */
          g_signal_connect (request->thumbnailers[n], "ready",
                            G_CALLBACK (tumbler_naive_scheduler_thumbnailer_ready),
                            request);

          /* tell the thumbnailer to generate the thumbnail */
          tumbler_thumbnailer_create (request->thumbnailers[n], request->uris[n], 
                                      request->mime_hints[n]);

          /* disconnect from all signals when we're finished */
          g_signal_handlers_disconnect_matched (request->thumbnailers[n],
                                                G_SIGNAL_MATCH_DATA,
                                                0, 0, NULL, NULL, request);
        }
      else
        {
          /* TODO Emit error signal: unsupported, no thumbnailer for the URI */
        }
    }

  /* tell everybody that we are done with this request */
  g_signal_emit_by_name (scheduler, "finished", request->handle);

  /* destroy the request */
  tumbler_scheduler_request_free (request);
}



static void
tumbler_naive_scheduler_thumbnailer_error (TumblerThumbnailer      *thumbnailer,
                                           const gchar             *failed_uri,
                                           gint                     error_code,
                                           const gchar             *message,
                                           TumblerSchedulerRequest *request)
{
  const gchar *failed_uris[] = { failed_uri, NULL };

  g_return_if_fail (TUMBLER_IS_THUMBNAILER (thumbnailer));
  g_return_if_fail (failed_uri != NULL);
  g_return_if_fail (request != NULL);
  g_return_if_fail (TUMBLER_IS_NAIVE_SCHEDULER (request->scheduler));

  /* forward the error signal */
  g_signal_emit_by_name (request->scheduler, "error", request->handle, failed_uris, 
                         error_code, message);
}



static void
tumbler_naive_scheduler_thumbnailer_ready (TumblerThumbnailer      *thumbnailer,
                                           const gchar             *uri,
                                           TumblerSchedulerRequest *request)
{
  const gchar *uris[] = { uri, NULL };

  g_return_if_fail (TUMBLER_IS_THUMBNAILER (thumbnailer));
  g_return_if_fail (uri != NULL);
  g_return_if_fail (request != NULL);
  g_return_if_fail (TUMBLER_IS_NAIVE_SCHEDULER (request->scheduler));

  /* forward the ready signal */
  g_signal_emit_by_name (request->scheduler, "ready", uris);
}



TumblerScheduler *
tumbler_naive_scheduler_new (void)
{
  return g_object_new (TUMBLER_TYPE_NAIVE_SCHEDULER, NULL);
}
