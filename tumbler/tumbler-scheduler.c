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

#include <tumbler/tumbler-scheduler.h>



static void tumbler_scheduler_class_init (TumblerSchedulerIface *klass);



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
}
