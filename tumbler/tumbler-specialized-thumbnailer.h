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

#ifndef __TUMBLER_SPECIALIZED_THUMBNAILER_H__
#define __TUMBLER_SPECIALIZED_THUMBNAILER_H__

#include <dbus/dbus-glib-lowlevel.h>

#include <tumbler/tumbler-thumbnailer.h>

G_BEGIN_DECLS

#define TUMBLER_TYPE_SPECIALIZED_THUMBNAILER            (tumbler_specialized_thumbnailer_get_type ())
#define TUMBLER_SPECIALIZED_THUMBNAILER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), TUMBLER_TYPE_SPECIALIZED_THUMBNAILER, TumblerSpecializedThumbnailer))
#define TUMBLER_SPECIALIZED_THUMBNAILER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), TUMBLER_TYPE_SPECIALIZED_THUMBNAILER, TumblerSpecializedThumbnailerClass))
#define TUMBLER_IS_SPECIALIZED_THUMBNAILER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TUMBLER_TYPE_SPECIALIZED_THUMBNAILER))
#define TUMBLER_IS_SPECIALIZED_THUMBNAILER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), TUMBLER_TYPE_SPECIALIZED_THUMBNAILER)
#define TUMBLER_SPECIALIZED_THUMBNAILER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), TUMBLER_TYPE_SPECIALIZED_THUMBNAILER, TumblerSpecializedThumbnailerClass))

typedef struct _TumblerSpecializedThumbnailerPrivate TumblerSpecializedThumbnailerPrivate;
typedef struct _TumblerSpecializedThumbnailerClass   TumblerSpecializedThumbnailerClass;
typedef struct _TumblerSpecializedThumbnailer        TumblerSpecializedThumbnailer;

GType               tumbler_specialized_thumbnailer_get_type     (void) G_GNUC_CONST;

TumblerThumbnailer *tumbler_specialized_thumbnailer_new          (DBusGConnection               *connection,
                                                                  const gchar                   *name,
                                                                  const GStrv                   *uri_schemes,
                                                                  const GStrv                   *mime_types,
                                                                  guint64                        modified) G_GNUC_MALLOC G_GNUC_WARN_UNUSED_RESULT;
TumblerThumbnailer *tumbler_specialized_thumbnailer_new_foreign  (DBusGConnection               *connection,
                                                                  const gchar                   *name,
                                                                  const gchar                   *uri_scheme,
                                                                  const gchar                   *mime_type) G_GNUC_MALLOC G_GNUC_WARN_UNUSED_RESULT;
gboolean            tumbler_specialized_thumbnailer_get_foreign  (TumblerSpecializedThumbnailer *thumbnailer);
guint64             tumbler_specialized_thumbnailer_get_modified (TumblerSpecializedThumbnailer *thumbnailer);

G_END_DECLS

#endif /* !__TUMBLER_SPECIALIZED_THUMBNAILER_H__ */
