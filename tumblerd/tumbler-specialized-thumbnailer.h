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

#include "tumbler/tumbler.h"

G_BEGIN_DECLS

#define TUMBLER_TYPE_SPECIALIZED_THUMBNAILER (tumbler_specialized_thumbnailer_get_type ())
G_DECLARE_FINAL_TYPE (TumblerSpecializedThumbnailer, tumbler_specialized_thumbnailer, TUMBLER, SPECIALIZED_THUMBNAILER, TumblerAbstractThumbnailer)

TumblerThumbnailer *tumbler_specialized_thumbnailer_new             (GDBusConnection               *connection,
                                                                     const gchar                   *name,
                                                                     const gchar                   *object_path,
                                                                     const gchar *const            *uri_schemes,
                                                                     const gchar *const            *mime_types,
                                                                     guint64                        modified) G_GNUC_MALLOC G_GNUC_WARN_UNUSED_RESULT;
TumblerThumbnailer *tumbler_specialized_thumbnailer_new_foreign     (GDBusConnection               *connection,
                                                                     const gchar                   *name,
                                                                     const gchar *const            *uri_scheme,
                                                                     const gchar *const            *mime_type) G_GNUC_MALLOC G_GNUC_WARN_UNUSED_RESULT;
const gchar        *tumbler_specialized_thumbnailer_get_name        (TumblerSpecializedThumbnailer *thumbnailer);
const gchar        *tumbler_specialized_thumbnailer_get_object_path (TumblerSpecializedThumbnailer *thumbnailer);
gboolean            tumbler_specialized_thumbnailer_get_foreign     (TumblerSpecializedThumbnailer *thumbnailer);
guint64             tumbler_specialized_thumbnailer_get_modified    (TumblerSpecializedThumbnailer *thumbnailer);

G_END_DECLS

#endif /* !__TUMBLER_SPECIALIZED_THUMBNAILER_H__ */
