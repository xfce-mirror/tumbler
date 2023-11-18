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

static guint tumbler_scheduler_signals[LAST_SIGNAL];



G_DEFINE_INTERFACE (TumblerScheduler, tumbler_scheduler, G_TYPE_OBJECT)



static void
tumbler_scheduler_default_init (TumblerSchedulerIface *klass)
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
                  tumbler_marshal_VOID__UINT_BOXED_UINT_INT_STRING_STRING,
                  G_TYPE_NONE,
                  6,
                  G_TYPE_UINT,
                  G_TYPE_STRV,
                  G_TYPE_UINT,
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
                  tumbler_marshal_VOID__UINT_BOXED_STRING,
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
tumbler_scheduler_dequeue (TumblerScheduler *scheduler,
                           guint32           handle)
{
  g_return_if_fail (TUMBLER_IS_SCHEDULER (scheduler));
  g_return_if_fail (handle != 0);
  g_return_if_fail (TUMBLER_SCHEDULER_GET_IFACE (scheduler)->dequeue != NULL);

  TUMBLER_SCHEDULER_GET_IFACE (scheduler)->dequeue (scheduler, handle);
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
                 uris, error->domain, error->code, error->message, request->origin);
}



TumblerSchedulerRequest *
tumbler_scheduler_request_new (TumblerFileInfo    **infos,
                               GList              **thumbnailers,
                               guint                length,
                               const gchar         *origin)
{
  TumblerSchedulerRequest *request = NULL;
  static guint32           handle  = 0;
  guint                    n;

  g_return_val_if_fail (infos != NULL, NULL);
  g_return_val_if_fail (thumbnailers != NULL, NULL);

  request = g_new0 (TumblerSchedulerRequest, 1);
  if (origin)
    request->origin = g_strdup (origin);
  request->dequeued = FALSE;
  request->scheduler = NULL;
  if (handle == 0)
    handle++;
  request->handle = handle++;
  request->infos = tumbler_file_info_array_copy (infos, length);
  request->thumbnailers = tumbler_thumbnailer_array_copy (thumbnailers, length);
  request->length = length;

  request->cancellables = g_new0 (GCancellable *, request->length + 1);

  for (n = 0; n < request->length; ++n)
    request->cancellables[n] = g_cancellable_new ();

  request->cancellables[n] = NULL;

  return request;
}



void
tumbler_scheduler_request_free (gpointer data)
{
  TumblerSchedulerRequest *request = data;
  gint                     n;

  g_return_if_fail (request != NULL);

  tumbler_thumbnailer_array_free (request->thumbnailers, request->length);

  if (G_LIKELY (request->scheduler != NULL))
    g_object_unref (request->scheduler);

  tumbler_file_info_array_free (request->infos);

  for (n = 0; request->cancellables != NULL && request->cancellables[n] != NULL; ++n)
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
#if defined (HAVE_sched_getparam) && defined (HAVE_sched_setscheduler)
  struct sched_param sp;
#endif
  int                ioprio;
  int                ioclass;

  ioprio = 7; /* priority is ignored with idle class */
  ioclass = IOPRIO_CLASS_IDLE << IOPRIO_CLASS_SHIFT;

  ioprio_set (IOPRIO_WHO_PROCESS, 0, ioprio | ioclass);

#if defined (HAVE_sched_getparam) && defined (HAVE_sched_setscheduler)
  if (sched_getparam (0, &sp) == 0) 
    sched_setscheduler (0, SCHED_IDLE, &sp);
#endif
}



void
tumbler_scheduler_thumberr_debuglog (TumblerThumbnailer      *thumbnailer,
                                     const gchar             *failed_uri,
                                     GQuark                   error_domain,
                                     gint                     error_code,
                                     const gchar             *message,
                                     TumblerSchedulerRequest *request)
{
  gchar *text;

  g_return_if_fail (TUMBLER_IS_THUMBNAILER (thumbnailer));
  g_return_if_fail (failed_uri != NULL);
  g_return_if_fail (request != NULL);
  g_return_if_fail (TUMBLER_IS_SCHEDULER (request->scheduler));

  if (error_domain == G_IO_ERROR && error_code == G_IO_ERROR_CANCELLED)
    return;

  if (error_domain == TUMBLER_ERROR)
    text = g_strdup (message);
  else
    {
      error_code = TUMBLER_ERROR_OTHER_ERROR_DOMAIN;
      text = g_strdup_printf ("(domain %s, code %d) %s",
                              g_quark_to_string (error_domain), error_code, message);
    }

  for (guint n = 0; n < request->length; n++)
    {
      if (g_strcmp0 (tumbler_file_info_get_uri (request->infos[n]), failed_uri) == 0)
        {
          g_debug ("Failed attempt for job %d, URI '%s': Code %d, message: %s",
                   request->handle, failed_uri, error_code, text);
          break;
        }
    }

  g_free (text);
}
