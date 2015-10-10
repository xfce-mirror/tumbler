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

#ifndef __TUMBLER_SERVICE_H__
#define __TUMBLER_SERVICE_H__

#include <dbus/dbus-glib.h>

#include <tumblerd/tumbler-lifecycle-manager.h>
#include <tumblerd/tumbler-registry.h>

G_BEGIN_DECLS;

#define TUMBLER_TYPE_SERVICE            (tumbler_service_get_type ())
#define TUMBLER_SERVICE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), TUMBLER_TYPE_SERVICE, TumblerService))
#define TUMBLER_SERVICE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), TUMBLER_TYPE_SERVICE, TumblerServiceClass))
#define TUMBLER_IS_SERVICE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TUMBLER_TYPE_SERVICE))
#define TUMBLER_IS_SERVICE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), TUMBLER_TYPE_SERVICE)
#define TUMBLER_SERVICE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), TUMBLER_TYPE_SERVICE, TumblerServiceClass))

typedef struct _TumblerServiceClass TumblerServiceClass;
typedef struct _TumblerService      TumblerService;

GType           tumbler_service_get_type (void) G_GNUC_CONST;

TumblerService *tumbler_service_new            (GDBusConnection         *connection,
                                                TumblerLifecycleManager *lifecycle_manager,
                                                TumblerRegistry         *registry) G_GNUC_MALLOC G_GNUC_WARN_UNUSED_RESULT;
gboolean        tumbler_service_is_exported    (TumblerService *service);

G_END_DECLS;

#endif /* !__TUMBLER_SERVICE_H__ */
