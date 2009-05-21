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

#include <tumbler/tumbler-marshal.h>
#include <tumbler/tumbler-scheduler.h>



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
  static GType type = G_TYPE_INVALID;
  
  if (G_UNLIKELY (type == G_TYPE_INVALID))
    {
      type = g_type_register_static_simple (G_TYPE_INTERFACE,
                                            "TumblerScheduler",
                                            sizeof (TumblerSchedulerIface),
                                            (GClassInitFunc) tumbler_scheduler_class_init,
                                            0,
                                            NULL,
                                            0);

      g_type_interface_add_prerequisite (type, G_TYPE_OBJECT);
    }

  return type;
}



static void
tumbler_scheduler_class_init (TumblerSchedulerIface *klass)
{
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
tumbler_scheduler_take_request (TumblerScheduler        *scheduler,
                                TumblerSchedulerRequest *request)
{
  g_return_if_fail (TUMBLER_IS_SCHEDULER (scheduler));
  g_return_if_fail (request != NULL);

  request->scheduler = g_object_ref (scheduler);
}



TumblerSchedulerRequest *
tumbler_scheduler_request_new (const GStrv          uris,
                               const GStrv          mime_hints,
                               TumblerThumbnailer **thumbnailers)
{
  TumblerSchedulerRequest *request = NULL;
  static gint              handle  = 0;

  g_return_val_if_fail (uris != NULL, NULL);
  g_return_val_if_fail (mime_hints != NULL, NULL);
  g_return_val_if_fail (thumbnailers != NULL, NULL);

  request = g_new0 (TumblerSchedulerRequest, 1);
  request->scheduler = NULL;
  request->handle = handle++;
  request->uris = g_strdupv (uris);
  request->mime_hints = g_strdupv (mime_hints);
  request->thumbnailers = tumbler_thumbnailer_array_copy (thumbnailers);

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

  tumbler_thumbnailer_array_free (request->thumbnailers);

  g_free (request);
}



guint
tumbler_scheduler_request_get_handle (TumblerSchedulerRequest *request)
{
  g_return_if_fail (request != NULL);
  return request->handle;
}
