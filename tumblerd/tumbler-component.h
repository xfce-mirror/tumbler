/* vi:set et ai sw=2 sts=2 ts=2: */
/*-
 * Copyright (c) 2011 Jannis Pohlmann <jannis@xfce.org>
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

#ifndef __TUMBLER_COMPONENT_H__
#define __TUMBLER_COMPONENT_H__

#include <glib-object.h>

G_BEGIN_DECLS

#define TUMBLER_TYPE_COMPONENT (tumbler_component_get_type ())
G_DECLARE_DERIVABLE_TYPE (TumblerComponent, tumbler_component, TUMBLER, COMPONENT, GObject)

gboolean tumbler_component_keep_alive          (TumblerComponent *component,
                                                GError          **error);
void     tumbler_component_increment_use_count (TumblerComponent *component);
void     tumbler_component_decrement_use_count (TumblerComponent *component);

struct _TumblerComponentClass
{
  GObjectClass __parent__;
};

G_END_DECLS

#endif /* !__TUMBLER_COMPONENT_H__ */
