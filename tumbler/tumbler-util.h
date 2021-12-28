/* vi:set et ai sw=2 sts=2 ts=2: */
/*-
 * Copyright (c) 2009-2011 Jannis Pohlmann <jannis@xfce.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifndef __TUMBLER_UTIL_H__
#define __TUMBLER_UTIL_H__

#include <glib.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include <tumbler/tumbler-file-info.h>

G_BEGIN_DECLS

gboolean tumbler_util_is_debug_logging_enabled (const gchar *log_domain);

void tumbler_util_dump_strv (const gchar *log_domain,
                             const gchar *label,
                             const gchar *const *strv);

void tumbler_util_toggle_stderr (const gchar *log_domain);

gchar **tumbler_util_get_supported_uri_schemes (void) G_GNUC_MALLOC;

GKeyFile *tumbler_util_get_settings (void) G_GNUC_MALLOC;

GList *tumbler_util_get_thumbnailer_dirs (void);

gboolean  tumbler_util_guess_is_sparse (TumblerFileInfo *info);

void tumbler_util_size_prepared (GdkPixbufLoader *loader,
                                 gint source_width,
                                 gint source_height,
                                 TumblerThumbnail *thumbnail);

GdkPixbuf *tumbler_util_scale_pixbuf (GdkPixbuf *source,
                                      gint dest_width,
                                      gint dest_height);

G_END_DECLS

#endif /* !__TUMBLER_UTIL_H__ */
