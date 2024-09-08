/* vi:set et ai sw=2 sts=2 ts=2: */
/*-
 * Copyright (c) 2009-2011 Jannis Pohlmann <jannis@xfce.org>
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

#ifndef __TUMBLER_SCHEDULER_H__
#define __TUMBLER_SCHEDULER_H__

#include "tumbler/tumbler.h"

#include <glib-object.h>

G_BEGIN_DECLS

typedef struct _TumblerSchedulerRequest TumblerSchedulerRequest;

#define TUMBLER_TYPE_SCHEDULER (tumbler_scheduler_get_type ())
G_DECLARE_INTERFACE (TumblerScheduler, tumbler_scheduler, TUMBLER, SCHEDULER, GObject)

typedef struct _TumblerSchedulerInterface
{
  GTypeInterface __parent__;

  /* signals */
  void (*error)    (TumblerScheduler        *scheduler,
                    guint32                  handle,
                    const gchar *const      *failed_uris,
                    GQuark                   error_domain,
                    gint                     error_code,
                    const gchar             *message);
  void (*finished) (TumblerScheduler        *scheduler,
                    guint32                  handle);
  void (*ready)    (TumblerScheduler        *scheduler,
                    const gchar *const      *uris);
  void (*started)  (TumblerScheduler        *scheduler,
                    guint32                  handle);

  /* virtual methods */
  void (*push)            (TumblerScheduler        *scheduler,
                           TumblerSchedulerRequest *request);
  void (*dequeue)         (TumblerScheduler        *scheduler,
                           guint32                  handle);
  void (*cancel_by_mount) (TumblerScheduler        *scheduler,
                           GMount                  *mount);
} TumblerSchedulerIface;

void                     tumbler_scheduler_push                  (TumblerScheduler        *scheduler,
                                                                  TumblerSchedulerRequest *request);
void                     tumbler_scheduler_dequeue               (TumblerScheduler        *scheduler,
                                                                  guint32                  handle);
void                     tumbler_scheduler_cancel_by_mount       (TumblerScheduler        *scheduler,
                                                                  GMount                  *mount);
gchar*                   tumbler_scheduler_get_name              (TumblerScheduler        *scheduler);
void                     tumbler_scheduler_take_request          (TumblerScheduler        *scheduler,
                                                                  TumblerSchedulerRequest *request);
void                     tumbler_scheduler_emit_uri_error        (TumblerScheduler        *scheduler,
                                                                  TumblerSchedulerRequest *request, 
                                                                  const gchar             *uri,
                                                                  GError                  *error);
TumblerSchedulerRequest *tumbler_scheduler_request_new           (TumblerFileInfo        **infos,
                                                                  GList                  **thumbnailers,
                                                                  guint                    length,
                                                                  const gchar             *origin);
void                     tumbler_scheduler_request_free          (gpointer                 data);
gint                     tumbler_scheduler_request_compare       (gconstpointer            a,
                                                                  gconstpointer            b,
                                                                  gpointer                 user_data);

void                     tumbler_scheduler_thread_use_lower_priority (void);

/* debug */
void                     tumbler_scheduler_thumberr_debuglog     (TumblerThumbnailer      *thumbnailer,
                                                                  TumblerFileInfo         *failed_info,
                                                                  GQuark                   error_domain,
                                                                  gint                     error_code,
                                                                  const gchar             *message,
                                                                  TumblerSchedulerRequest *request);

struct _TumblerSchedulerRequest
{
  GList              **thumbnailers;
  TumblerScheduler    *scheduler;
  TumblerFileInfo    **infos;
  GCancellable       **cancellables;
  gboolean             dequeued;
  guint32              handle;
  gchar               *origin;
  guint                length;
  GList               *uri_errors;
  GList               *ready_uris;
};

G_END_DECLS

#endif /* !__TUMBLER_SCHEDULER_H__ */
