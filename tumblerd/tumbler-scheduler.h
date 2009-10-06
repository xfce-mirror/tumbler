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

#ifndef __TUMBLER_SCHEDULER_H__
#define __TUMBLER_SCHEDULER_H__

#include <tumbler/tumbler.h>

G_BEGIN_DECLS

typedef struct _TumblerSchedulerRequest TumblerSchedulerRequest;

#define TUMBLER_TYPE_SCHEDULER           (tumbler_scheduler_get_type ())
#define TUMBLER_SCHEDULER(obj)           (G_TYPE_CHECK_INSTANCE_CAST ((obj), TUMBLER_TYPE_SCHEDULER, TumblerScheduler))
#define TUMBLER_IS_SCHEDULER(obj)        (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TUMBLER_TYPE_SCHEDULER))
#define TUMBLER_SCHEDULER_GET_IFACE(obj) (G_TYPE_INSTANCE_GET_INTERFACE ((obj), TUMBLER_TYPE_SCHEDULER, TumblerSchedulerIface))

typedef struct _TumblerScheduler      TumblerScheduler;
typedef struct _TumblerSchedulerIface TumblerSchedulerIface;

struct _TumblerSchedulerIface
{
  GTypeInterface __parent__;

  /* signals */
  void (*error)    (TumblerScheduler        *scheduler,
                    guint                    handle,
                    const GStrv              failed_uris,
                    gint                     error_code,
                    const gchar             *message);
  void (*finished) (TumblerScheduler        *scheduler,
                    guint                    handle);
  void (*ready)    (TumblerScheduler        *scheduler,
                    const GStrv              uris);
  void (*started)  (TumblerScheduler        *scheduler,
                    guint                    handle);

  /* virtual methods */
  void (*push)     (TumblerScheduler        *scheduler,
                    TumblerSchedulerRequest *request);
  void (*unqueue)  (TumblerScheduler        *scheduler,
                    guint                    handle);
};

GType                    tumbler_scheduler_get_type              (void) G_GNUC_CONST;

void                     tumbler_scheduler_push                  (TumblerScheduler        *scheduler,
                                                                  TumblerSchedulerRequest *request);
void                     tumbler_scheduler_unqueue               (TumblerScheduler        *scheduler,
                                                                  guint                    handle);
gchar*                   tumbler_scheduler_get_name              (TumblerScheduler        *scheduler);
void                     tumbler_scheduler_take_request          (TumblerScheduler        *scheduler,
                                                                  TumblerSchedulerRequest *request);
void                     tumbler_scheduler_emit_uri_error        (TumblerScheduler        *scheduler,
                                                                  TumblerSchedulerRequest *request, 
                                                                  const gchar             *uri,
                                                                  GError                  *error);
TumblerSchedulerRequest *tumbler_scheduler_request_new           (const GStrv              uris,
                                                                  const GStrv              mime_hints,
                                                                  TumblerThumbnailer     **thumbnailers,
                                                                  gint                     length);
void                     tumbler_scheduler_request_free          (TumblerSchedulerRequest *request);
gint                     tumbler_scheduler_request_compare       (gconstpointer            a,
                                                                  gconstpointer            b,
                                                                  gpointer                 user_data);

struct _TumblerSchedulerRequest
{
  TumblerThumbnailer **thumbnailers;
  TumblerScheduler    *scheduler;
  gboolean             unqueued;
  GStrv                mime_hints;
  GStrv                uris;
  guint                handle;
  gint                 length;
};

G_END_DECLS

#endif /* !__TUMBLER_SCHEDULER_H__ */
