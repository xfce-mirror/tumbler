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

#ifndef __TUMBLER_NAIVE_SCHEDULER_H__
#define __TUMBLER_NAIVE_SCHEDULER_H__

#include <tumblerd/tumbler-scheduler.h>

G_BEGIN_DECLS;

#define TUMBLER_TYPE_NAIVE_SCHEDULER            (tumbler_naive_scheduler_get_type ())
#define TUMBLER_NAIVE_SCHEDULER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), TUMBLER_TYPE_NAIVE_SCHEDULER, TumblerNaiveScheduler))
#define TUMBLER_NAIVE_SCHEDULER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), TUMBLER_TYPE_NAIVE_SCHEDULER, TumblerNaiveSchedulerClass))
#define TUMBLER_IS_NAIVE_SCHEDULER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TUMBLER_TYPE_NAIVE_SCHEDULER))
#define TUMBLER_IS_NAIVE_SCHEDULER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), TUMBLER_TYPE_NAIVE_SCHEDULER)
#define TUMBLER_NAIVE_SCHEDULER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), TUMBLER_TYPE_NAIVE_SCHEDULER, TumblerNaiveSchedulerClass))

typedef struct _TumblerNaiveSchedulerPrivate TumblerNaiveSchedulerPrivate;
typedef struct _TumblerNaiveSchedulerClass   TumblerNaiveSchedulerClass;
typedef struct _TumblerNaiveScheduler        TumblerNaiveScheduler;

GType             tumbler_naive_scheduler_get_type (void) G_GNUC_CONST;

TumblerScheduler *tumbler_naive_scheduler_new      (void) G_GNUC_MALLOC G_GNUC_WARN_UNUSED_RESULT;

G_END_DECLS;

#endif /* !__TUMBLER_NAIVE_SCHEDULER_H__ */
