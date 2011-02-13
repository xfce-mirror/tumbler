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

#ifndef __TUMBLER_CACHE_SERVICE_H__
#define __TUMBLER_CACHE_SERVICE_H__

#include <glib-object.h>

#include <dbus/dbus-glib.h>

G_BEGIN_DECLS;

#define TUMBLER_TYPE_CACHE_SERVICE            (tumbler_cache_service_get_type ())
#define TUMBLER_CACHE_SERVICE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), TUMBLER_TYPE_CACHE_SERVICE, TumblerCacheService))
#define TUMBLER_CACHE_SERVICE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), TUMBLER_TYPE_CACHE_SERVICE, TumblerCacheServiceClass))
#define TUMBLER_IS_CACHE_SERVICE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TUMBLER_TYPE_CACHE_SERVICE))
#define TUMBLER_IS_CACHE_SERVICE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), TUMBLER_TYPE_CACHE_SERVICE)
#define TUMBLER_CACHE_SERVICE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), TUMBLER_TYPE_CACHE_SERVICE, TumblerCacheServiceClass))

typedef struct _TumblerCacheServiceClass TumblerCacheServiceClass;
typedef struct _TumblerCacheService      TumblerCacheService;

GType                tumbler_cache_service_get_type   (void) G_GNUC_CONST;

TumblerCacheService *tumbler_cache_service_new     (DBusGConnection       *connection) G_GNUC_MALLOC G_GNUC_WARN_UNUSED_RESULT;
gboolean             tumbler_cache_service_start   (TumblerCacheService   *service,
                                                    GError               **error);
void                 tumbler_cache_service_move    (TumblerCacheService   *service,
                                                    const GStrv            from_uris,
                                                    const GStrv            to_uris,
                                                    DBusGMethodInvocation *context);
void                 tumbler_cache_service_copy    (TumblerCacheService   *service,
                                                    const GStrv            from_uris,
                                                    const GStrv            to_uris,
                                                    DBusGMethodInvocation *context);
void                 tumbler_cache_service_delete  (TumblerCacheService   *service,
                                                    const GStrv            uris,
                                                    DBusGMethodInvocation *context);
void                 tumbler_cache_service_cleanup (TumblerCacheService   *service,
                                                    const gchar *const    *uri_prefix,
                                                    guint32                since,
                                                    DBusGMethodInvocation *context);


G_END_DECLS;

#endif /* !__TUMBLER_CACHE_SERVICE_H__ */
