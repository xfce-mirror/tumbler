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

#include <tumbler/tumbler-threshold-scheduler.h>



#define TUMBLER_THRESHOLD_SCHEDULER_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), TUMBLER_TYPE_THRESHOLD_SCHEDULER, TumblerThresholdSchedulerPrivate))



/* Property identifiers */
enum
{
  PROP_0,
};



static void tumbler_threshold_scheduler_class_init   (TumblerThresholdSchedulerClass *klass);
static void tumbler_threshold_scheduler_iface_init   (TumblerSchedulerIface         *iface);
static void tumbler_threshold_scheduler_init         (TumblerThresholdScheduler      *scheduler);
static void tumbler_threshold_scheduler_constructed  (GObject                       *object);
static void tumbler_threshold_scheduler_finalize     (GObject                       *object);
static void tumbler_threshold_scheduler_get_property (GObject                       *object,
                                                     guint                          prop_id,
                                                     GValue                        *value,
                                                     GParamSpec                    *pspec);
static void tumbler_threshold_scheduler_set_property (GObject                       *object,
                                                     guint                          prop_id,
                                                     const GValue                  *value,
                                                     GParamSpec                    *pspec);



struct _TumblerThresholdSchedulerClass
{
  GObjectClass __parent__;
};

struct _TumblerThresholdScheduler
{
  GObject __parent__;

  TumblerThresholdSchedulerPrivate *priv;
};

struct _TumblerThresholdSchedulerPrivate
{
  GThreadPool *large_pool;
  GThreadPool *small_pool;
};



static GObjectClass *tumbler_threshold_scheduler_parent_class = NULL;



GType
tumbler_threshold_scheduler_get_type (void)
{
  static GType type = G_TYPE_INVALID;

  if (G_UNLIKELY (type == G_TYPE_INVALID))
    {
      static const GInterfaceInfo info =
      {
        (GInterfaceInitFunc) tumbler_threshold_scheduler_iface_init,
        NULL,
        NULL,
      };

      type = g_type_register_static_simple (G_TYPE_OBJECT, 
                                            "TumblerThresholdScheduler",
                                            sizeof (TumblerThresholdSchedulerClass),
                                            (GClassInitFunc) tumbler_threshold_scheduler_class_init,
                                            sizeof (TumblerThresholdScheduler),
                                            (GInstanceInitFunc) tumbler_threshold_scheduler_init,
                                            0);

      g_type_add_interface_static (type, TUMBLER_TYPE_SCHEDULER, &info);
    }

  return type;
}



static void
tumbler_threshold_scheduler_class_init (TumblerThresholdSchedulerClass *klass)
{
  GObjectClass *gobject_class;

  g_type_class_add_private (klass, sizeof (TumblerThresholdSchedulerPrivate));

  /* Determine the parent type class */
  tumbler_threshold_scheduler_parent_class = g_type_class_peek_parent (klass);

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->constructed = tumbler_threshold_scheduler_constructed; 
  gobject_class->finalize = tumbler_threshold_scheduler_finalize; 
  gobject_class->get_property = tumbler_threshold_scheduler_get_property;
  gobject_class->set_property = tumbler_threshold_scheduler_set_property;
}



static void
tumbler_threshold_scheduler_iface_init (TumblerSchedulerIface *iface)
{
}



static void
tumbler_threshold_scheduler_init (TumblerThresholdScheduler *scheduler)
{
  scheduler->priv = TUMBLER_THRESHOLD_SCHEDULER_GET_PRIVATE (scheduler);
}



static void
tumbler_threshold_scheduler_constructed (GObject *object)
{
  TumblerThresholdScheduler *scheduler = TUMBLER_THRESHOLD_SCHEDULER (object);
}



static void
tumbler_threshold_scheduler_finalize (GObject *object)
{
  TumblerThresholdScheduler *scheduler = TUMBLER_THRESHOLD_SCHEDULER (object);

  (*G_OBJECT_CLASS (tumbler_threshold_scheduler_parent_class)->finalize) (object);
}



static void
tumbler_threshold_scheduler_get_property (GObject    *object,
                                          guint       prop_id,
                                          GValue     *value,
                                          GParamSpec *pspec)
{
  TumblerThresholdScheduler *scheduler = TUMBLER_THRESHOLD_SCHEDULER (object);

  switch (prop_id)
    {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}



static void
tumbler_threshold_scheduler_set_property (GObject      *object,
                                          guint         prop_id,
                                          const GValue *value,
                                          GParamSpec   *pspec)
{
  TumblerThresholdScheduler *scheduler = TUMBLER_THRESHOLD_SCHEDULER (object);

  switch (prop_id)
    {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}



TumblerScheduler *
tumbler_threshold_scheduler_new (void)
{
  return g_object_new (TUMBLER_TYPE_THRESHOLD_SCHEDULER, NULL);
}
