/* vi:set et ai sw=2 sts=2 ts=2: */
/*-
 * Copyright (c) 2009 Jannis Pohlmann <jannis@xfce.org>
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

#if !defined (TUMBLER_INSIDE_TUMBLER_H) && !defined (TUMBLER_COMPILATION)
#error "Only <tumbler/tumbler.h> may be included directly. This file might disappear or change contents."
#endif

#ifndef __TUMBLER_FILE_INFO_H__
#define __TUMBLER_FILE_INFO_H__

#include <gio/gio.h>

#include <tumbler/tumbler-thumbnail.h>

G_BEGIN_DECLS;

#define TUMBLER_TYPE_FILE_INFO            (tumbler_file_info_get_type ())
#define TUMBLER_FILE_INFO(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), TUMBLER_TYPE_FILE_INFO, TumblerFileInfo))
#define TUMBLER_FILE_INFO_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), TUMBLER_TYPE_FILE_INFO, TumblerFileInfoClass))
#define TUMBLER_IS_FILE_INFO(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TUMBLER_TYPE_FILE_INFO))
#define TUMBLER_IS_FILE_INFO_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), TUMBLER_TYPE_FILE_INFO)
#define TUMBLER_FILE_INFO_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), TUMBLER_TYPE_FILE_INFO, TumblerFileInfoClass))

typedef struct _TumblerFileInfoClass   TumblerFileInfoClass;
typedef struct _TumblerFileInfo        TumblerFileInfo;

GType             tumbler_file_info_get_type              (void) G_GNUC_CONST;

TumblerFileInfo  *tumbler_file_info_new                   (const gchar            *uri,
                                                           const gchar            *mime_type,
                                                           TumblerThumbnailFlavor *flavor) G_GNUC_MALLOC G_GNUC_WARN_UNUSED_RESULT;
gboolean          tumbler_file_info_load                  (TumblerFileInfo        *info,
                                                           GCancellable           *cancellable,
                                                           GError                **error);
const gchar      *tumbler_file_info_get_uri               (TumblerFileInfo        *info);
const gchar      *tumbler_file_info_get_mime_type         (TumblerFileInfo        *info);
gdouble           tumbler_file_info_get_mtime             (TumblerFileInfo        *info);
gboolean          tumbler_file_info_needs_update          (TumblerFileInfo        *info);
TumblerThumbnail *tumbler_file_info_get_thumbnail         (TumblerFileInfo        *info) G_GNUC_WARN_UNUSED_RESULT;

TumblerFileInfo **tumbler_file_info_array_new_with_flavor (const gchar *const     *uris,
                                                           const gchar *const     *mime_types,
                                                           TumblerThumbnailFlavor *flavor,
                                                           guint                  *length) G_GNUC_MALLOC G_GNUC_WARN_UNUSED_RESULT;
TumblerFileInfo **tumbler_file_info_array_copy            (TumblerFileInfo       **infos,
                                                           guint                   length) G_GNUC_MALLOC G_GNUC_WARN_UNUSED_RESULT;
void              tumbler_file_info_array_free            (TumblerFileInfo       **infos);

G_END_DECLS;

#endif /* !__TUMBLER_FILE_INFO_H__ */
