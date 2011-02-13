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

#ifndef __TUMBLER_LIFECYCLE_MANAGER_H__
#define __TUMBLER_LIFECYCLE_MANAGER_H__

#include <glib-object.h>

G_BEGIN_DECLS

#define TUMBLER_TYPE_LIFECYCLE_MANAGER            (tumbler_lifecycle_manager_get_type ())
#define TUMBLER_LIFECYCLE_MANAGER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), TUMBLER_TYPE_LIFECYCLE_MANAGER, TumblerLifecycleManager))
#define TUMBLER_LIFECYCLE_MANAGER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), TUMBLER_TYPE_LIFECYCLE_MANAGER, TumblerLifecycleManagerClass))
#define TUMBLER_IS_LIFECYCLE_MANAGER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TUMBLER_TYPE_LIFECYCLE_MANAGER))
#define TUMBLER_IS_LIFECYCLE_MANAGER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), TUMBLER_TYPE_LIFECYCLE_MANAGER)
#define TUMBLER_LIFECYCLE_MANAGER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), TUMBLER_TYPE_LIFECYCLE_MANAGER, TumblerLifecycleManagerClass))

typedef struct _TumblerLifecycleManagerClass   TumblerLifecycleManagerClass;
typedef struct _TumblerLifecycleManager        TumblerLifecycleManager;

GType                    tumbler_lifecycle_manager_get_type            (void) G_GNUC_CONST;

TumblerLifecycleManager *tumbler_lifecycle_manager_new                 (void) G_GNUC_MALLOC G_GNUC_WARN_UNUSED_RESULT;
void                     tumbler_lifecycle_manager_start               (TumblerLifecycleManager *manager);
gboolean                 tumbler_lifecycle_manager_keep_alive          (TumblerLifecycleManager *manager,
                                                                        GError                 **error);
void                     tumbler_lifecycle_manager_increment_use_count (TumblerLifecycleManager *manager);
void                     tumbler_lifecycle_manager_decrement_use_count (TumblerLifecycleManager *manager);

G_END_DECLS

#endif /* !__TUMBLER_LIFECYCLE_MANAGER_H__ */
