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

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_SCHED_H
#include <sched.h>
#endif
#ifdef HAVE_LINUX_SCHED_H 
#include <linux/sched.h>
#endif
#ifdef HAVE_SYSCALL_H
#include <syscall.h> 
#endif

#include <tumbler/tumbler.h>

#include <tumblerd/tumbler-scheduler.h>


#define IOPRIO_CLASS_SHIFT 13

#ifndef SCHED_IDLE
#define SCHED_IDLE 5
#endif



enum
{
  IOPRIO_CLASS_NONE,
  IOPRIO_CLASS_RT,
  IOPRIO_CLASS_BE,
  IOPRIO_CLASS_IDLE,
};



enum 
{
  IOPRIO_WHO_PROCESS = 1,
  IOPRIO_WHO_PGRP,
  IOPRIO_WHO_USER,
};

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
                  tumbler_marshal_VOID__UINT_POINTER_INT_STRING_STRING,
                  G_TYPE_NONE,
                  5,
                  G_TYPE_UINT,
                  G_TYPE_STRV,
                  G_TYPE_INT,
                  G_TYPE_STRING,
                  G_TYPE_STRING);

  tumbler_scheduler_signals[SIGNAL_FINISHED] =
    g_signal_new ("finished",
                  TUMBLER_TYPE_SCHEDULER,
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (TumblerSchedulerIface, finished),
                  NULL,
                  NULL,
                  tumbler_marshal_VOID__UINT_STRING,
                  G_TYPE_NONE,
                  2,
                  G_TYPE_UINT,
                  G_TYPE_STRING);

  tumbler_scheduler_signals[SIGNAL_READY] =
    g_signal_new ("ready",
                  TUMBLER_TYPE_SCHEDULER,
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (TumblerSchedulerIface, ready),
                  NULL,
                  NULL,
                  tumbler_marshal_VOID__UINT_POINTER_STRING,
                  G_TYPE_NONE,
                  3,
                  G_TYPE_UINT,
                  G_TYPE_STRV,
                  G_TYPE_STRING);

  tumbler_scheduler_signals[SIGNAL_STARTED] =
    g_signal_new ("started",
                  TUMBLER_TYPE_SCHEDULER,
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (TumblerSchedulerIface, started),
                  NULL,
                  NULL,
                  tumbler_marshal_VOID__UINT_STRING,
                  G_TYPE_NONE,
                  2,
                  G_TYPE_UINT,
                  G_TYPE_STRING);
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
tumbler_scheduler_cancel_by_mount (TumblerScheduler *scheduler,
                                   GMount           *mount)
{
  g_return_if_fail (TUMBLER_IS_SCHEDULER (scheduler));
  g_return_if_fail (G_IS_MOUNT (mount));
  g_return_if_fail (TUMBLER_SCHEDULER_GET_IFACE (scheduler)->cancel_by_mount != NULL);

  TUMBLER_SCHEDULER_GET_IFACE (scheduler)->cancel_by_mount (scheduler, mount);
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
  gchar *name;

  g_return_val_if_fail (TUMBLER_IS_SCHEDULER (scheduler), NULL);
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
                 uris, error->code, error->message, request->origin);
}



TumblerSchedulerRequest *
tumbler_scheduler_request_new (const GStrv          uris,
                               const GStrv          mime_hints,
                               TumblerThumbnailer **thumbnailers,
                               gint                 length,
                               const gchar         *origin)
{
  TumblerSchedulerRequest *request = NULL;
  static gint              handle  = 0;
  gint                     n;

  g_return_val_if_fail (uris != NULL, NULL);
  g_return_val_if_fail (mime_hints != NULL, NULL);
  g_return_val_if_fail (thumbnailers != NULL, NULL);

  request = g_new0 (TumblerSchedulerRequest, 1);
  if (origin)
    request->origin = g_strdup (origin);
  request->unqueued = FALSE;
  request->scheduler = NULL;
  request->handle = handle++;
  request->uris = g_strdupv (uris);
  request->mime_hints = g_strdupv (mime_hints);
  request->thumbnailers = tumbler_thumbnailer_array_copy (thumbnailers, length);
  request->length = length;
  request->cancellables = g_new0 (GCancellable *, request->length + 1);

  for (n = 0; n < request->length; ++n)
    request->cancellables[n] = g_cancellable_new ();
  request->cancellables[n] = NULL;

  return request;
}



void
tumbler_scheduler_request_free (TumblerSchedulerRequest *request)
{
  gint n;

  g_return_if_fail (request != NULL);

  g_strfreev (request->uris);
  g_strfreev (request->mime_hints);

  if (G_LIKELY (request->scheduler != NULL))
    g_object_unref (request->scheduler);

  tumbler_thumbnailer_array_free (request->thumbnailers, request->length);

  for (n = 0; n < request->length; ++n)
    g_object_unref (request->cancellables[n]);

  g_free (request->cancellables);
  g_free (request->origin);

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

static int
ioprio_set (int which, int who, int ioprio_val)
{
#if defined (__NR_ioprio_set) && defined (HAVE_UNISTD_H)
  return syscall (__NR_ioprio_set, which, who, ioprio_val);
#else
  return 0;
#endif
}


void
tumbler_scheduler_thread_use_lower_priority (void)
{
#ifdef HAVE_SCHED_H
  struct sched_param sp;
#endif
  int                ioprio;
  int                ioclass;

  ioprio = 7; /* priority is ignored with idle class */
  ioclass = IOPRIO_CLASS_IDLE << IOPRIO_CLASS_SHIFT;

  ioprio_set (IOPRIO_WHO_PROCESS, 0, ioprio | ioclass);

#ifdef HAVE_SCHED_H
  if (sched_getparam (0, &sp) == 0) 
    sched_setscheduler (0, SCHED_IDLE, &sp);
#endif
}
