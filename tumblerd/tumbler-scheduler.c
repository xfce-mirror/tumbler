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

#include <tumbler/tumbler.h>

#include <tumblerd/tumbler-scheduler.h>

/* signal identifiers */
enum
{
  SIGNAL_ERROR,
  SIGNAL_FINISHED,
  SIGNAL_READY,
  SIGNAL_STARTED,
  LAST_SIGNAL,
};


static void tumbler_scheduler_class_init (TumblerSchedulerIface *klass);
static guint tumbler_scheduler_signals[LAST_SIGNAL];

GType
tumbler_scheduler_get_type (void)
{
  static volatile gsize g_define_type_id__volatile = 0;
  
  if (g_once_init_enter (&g_define_type_id__volatile))
    {
      GType g_define_type_id =
        g_type_register_static_simple (G_TYPE_INTERFACE,
                                       "TumblerScheduler",
                                       sizeof (TumblerSchedulerIface),
                                       (GClassInitFunc) tumbler_scheduler_class_init,
                                       0,
                                       NULL,
                                       0);

      g_type_interface_add_prerequisite (g_define_type_id, G_TYPE_OBJECT);

      g_once_init_leave (&g_define_type_id__volatile, g_define_type_id);
    }

  return g_define_type_id__volatile;
}


static void
tumbler_scheduler_class_init (TumblerSchedulerIface *klass)
{
  g_object_interface_install_property (klass, 
                                       g_param_spec_string ("name",
                                                            "name",
                                                            "name",
                                                            NULL,
                                                            G_PARAM_READWRITE |
                                                            G_PARAM_CONSTRUCT_ONLY));

  tumbler_scheduler_signals[SIGNAL_ERROR] =
    g_signal_new ("error",
                  TUMBLER_TYPE_SCHEDULER,
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (TumblerSchedulerIface, error),
                  NULL,
                  NULL,
                  tumbler_marshal_VOID__UINT_POINTER_INT_STRING,
                  G_TYPE_NONE,
                  4,
                  G_TYPE_UINT,
                  G_TYPE_STRV,
                  G_TYPE_INT,
                  G_TYPE_STRING);

  tumbler_scheduler_signals[SIGNAL_FINISHED] =
    g_signal_new ("finished",
                  TUMBLER_TYPE_SCHEDULER,
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (TumblerSchedulerIface, finished),
                  NULL,
                  NULL,
                  g_cclosure_marshal_VOID__UINT,
                  G_TYPE_NONE,
                  1,
                  G_TYPE_UINT);

  tumbler_scheduler_signals[SIGNAL_READY] =
    g_signal_new ("ready",
                  TUMBLER_TYPE_SCHEDULER,
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (TumblerSchedulerIface, ready),
                  NULL,
                  NULL,
                  g_cclosure_marshal_VOID__POINTER,
                  G_TYPE_NONE,
                  1,
                  G_TYPE_STRV);

  tumbler_scheduler_signals[SIGNAL_STARTED] =
    g_signal_new ("started",
                  TUMBLER_TYPE_SCHEDULER,
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (TumblerSchedulerIface, started),
                  NULL,
                  NULL,
                  g_cclosure_marshal_VOID__UINT,
                  G_TYPE_NONE,
                  1,
                  G_TYPE_UINT);
}



void
tumbler_scheduler_push (TumblerScheduler        *scheduler,
                        TumblerSchedulerRequest *request)
{
  g_return_if_fail (TUMBLER_IS_SCHEDULER (scheduler));
  g_return_if_fail (request != NULL);
  g_return_if_fail (TUMBLER_SCHEDULER_GET_IFACE (scheduler)->push != NULL);

  TUMBLER_SCHEDULER_GET_IFACE (scheduler)->push (scheduler, request);
}



void
tumbler_scheduler_unqueue (TumblerScheduler *scheduler,
                           guint             handle)
{
  g_return_if_fail (TUMBLER_IS_SCHEDULER (scheduler));
  g_return_if_fail (handle != 0);
  g_return_if_fail (TUMBLER_SCHEDULER_GET_IFACE (scheduler)->unqueue != NULL);

  TUMBLER_SCHEDULER_GET_IFACE (scheduler)->unqueue (scheduler, handle);
}



void
tumbler_scheduler_take_request (TumblerScheduler        *scheduler,
                                TumblerSchedulerRequest *request)
{
  g_return_if_fail (TUMBLER_IS_SCHEDULER (scheduler));
  g_return_if_fail (request != NULL);

  request->scheduler = g_object_ref (scheduler);
}


gchar*
tumbler_scheduler_get_name (TumblerScheduler *scheduler)
{
  const gchar *name;

  g_return_if_fail (TUMBLER_IS_SCHEDULER (scheduler));
  g_object_get (scheduler, "name", &name, NULL);

  return name;
}

void 
tumbler_scheduler_emit_uri_error (TumblerScheduler        *scheduler,
                                  TumblerSchedulerRequest *request, 
                                  const gchar             *uri,
                                  GError                  *error)
{
  const gchar *uris[] = { uri, NULL };

  g_return_if_fail (TUMBLER_IS_SCHEDULER (scheduler));
  g_return_if_fail (request != NULL);
  g_return_if_fail (uri != NULL);
  g_return_if_fail (error != NULL);

  g_signal_emit (scheduler, tumbler_scheduler_signals[SIGNAL_ERROR], 0, request->handle,
                 uris, error->code, error->message);
}



TumblerSchedulerRequest *
tumbler_scheduler_request_new (const GStrv          uris,
                               const GStrv          mime_hints,
                               TumblerThumbnailer **thumbnailers,
                               gint                 length)
{
  TumblerSchedulerRequest *request = NULL;
  static gint              handle  = 0;

  g_return_val_if_fail (uris != NULL, NULL);
  g_return_val_if_fail (mime_hints != NULL, NULL);
  g_return_val_if_fail (thumbnailers != NULL, NULL);

  request = g_new0 (TumblerSchedulerRequest, 1);
  request->unqueued = FALSE;
  request->scheduler = NULL;
  request->handle = handle++;
  request->uris = g_strdupv (uris);
  request->mime_hints = g_strdupv (mime_hints);
  request->thumbnailers = tumbler_thumbnailer_array_copy (thumbnailers, length);
  request->length = length;

  return request;
}



void
tumbler_scheduler_request_free (TumblerSchedulerRequest *request)
{
  g_return_if_fail (request != NULL);

  g_strfreev (request->uris);
  g_strfreev (request->mime_hints);

  if (G_LIKELY (request->scheduler != NULL))
    g_object_unref (request->scheduler);

  tumbler_thumbnailer_array_free (request->thumbnailers, request->length);

  g_free (request);
}



gint
tumbler_scheduler_request_compare (gconstpointer a,
                                   gconstpointer b,
                                   gpointer      user_data)
{
  const TumblerSchedulerRequest *request_a = a;
  const TumblerSchedulerRequest *request_b = b;

  return request_b->handle - request_a->handle;
}
