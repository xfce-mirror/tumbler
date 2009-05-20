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

#include <tumbler/tumbler-thumbnailer.h>

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

  /* virtual methods */
  guint (*push) (TumblerScheduler        *scheduler,
                 TumblerSchedulerRequest *request);
};

GType                    tumbler_scheduler_get_type     (void) G_GNUC_CONST;

guint                    tumbler_scheduler_push         (TumblerScheduler        *scheduler,
                                                         TumblerSchedulerRequest *request);

TumblerSchedulerRequest *tumbler_scheduler_request_new  (const GStrv          uris,
                                                         const GStrv          mime_hints,
                                                         TumblerThumbnailer **thumbnailers);
void                     tumbler_scheduler_request_free (TumblerSchedulerRequest *request);

struct _TumblerSchedulerRequest
{
  TumblerThumbnailer **thumbnailers;
  GStrv                mime_hints;
  GStrv                uris;
};

G_END_DECLS

#endif /* !__TUMBLER_SCHEDULER_H__ */
