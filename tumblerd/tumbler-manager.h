/* vi:set et ai sw=2 sts=2 ts=2: */
/*-
 * Copyright (c) 2009-2011 Jannis Pohlmann <jannis@xfce.org>
 * Copyright (c) 2015      Ali Abdallah    <ali@xfce.org> 
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

#ifndef __TUMBLER_MANAGER_H__
#define __TUMBLER_MANAGER_H__

#include <tumblerd/tumbler-component.h>
#include <tumblerd/tumbler-lifecycle-manager.h>
#include <tumblerd/tumbler-registry.h>

G_BEGIN_DECLS;

#define TUMBLER_TYPE_MANAGER (tumbler_manager_get_type ())
G_DECLARE_FINAL_TYPE (TumblerManager, tumbler_manager, TUMBLER, MANAGER, TumblerComponent)

TumblerManager *tumbler_manager_new           (GDBusConnection         *connection,
                                               TumblerLifecycleManager *lifecycle_manager,
                                               TumblerRegistry         *registry) G_GNUC_MALLOC G_GNUC_WARN_UNUSED_RESULT;
void            tumbler_manager_load          (TumblerManager *manager);

gboolean        tumbler_manager_is_exported   (TumblerManager *manager);

G_END_DECLS;

#endif /* !__TUMBLER_MANAGER_H__ */
