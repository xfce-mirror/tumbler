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

#ifndef __TUMBLER_THUMBNAIL_INFO_H__
#define __TUMBLER_THUMBNAIL_INFO_H__

#include <glib-object.h>

#include <tumbler/tumbler-enum-types.h>

G_BEGIN_DECLS

#define TUMBLER_TYPE_THUMBNAIL_INFO           (tumbler_thumbnail_info_get_type ())
#define TUMBLER_THUMBNAIL_INFO(obj)           (G_TYPE_CHECK_INSTANCE_CAST ((obj), TUMBLER_TYPE_THUMBNAIL_INFO, TumblerThumbnailInfo))
#define TUMBLER_IS_THUMBNAIL_INFO(obj)        (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TUMBLER_TYPE_THUMBNAIL_INFO))
#define TUMBLER_THUMBNAIL_INFO_GET_IFACE(obj) (G_TYPE_INSTANCE_GET_INTERFACE ((obj), TUMBLER_TYPE_THUMBNAIL_INFO, TumblerThumbnailInfoIface))

typedef struct _TumblerThumbnailInfo      TumblerThumbnailInfo;
typedef struct _TumblerThumbnailInfoIface TumblerThumbnailInfoIface;

struct _TumblerThumbnailInfoIface
{
  GTypeInterface __parent__;

  /* signals */

  /* virtual methods */
  TumblerThumbnailFormat (*get_format) (TumblerThumbnailInfo *info);
  const gchar           *(*get_uri)    (TumblerThumbnailInfo *info);
};

GType                  tumbler_thumbnail_info_get_type           (void) G_GNUC_CONST;
TumblerThumbnailInfo  *tumbler_thumbnail_info_new                (const gchar            *uri) G_GNUC_MALLOC G_GNUC_WARN_UNUSED_RESULT;
TumblerThumbnailInfo  *tumbler_thumbnail_info_new_for_format     (const gchar            *uri,
                                                                  TumblerThumbnailFormat  format) G_GNUC_MALLOC G_GNUC_WARN_UNUSED_RESULT;
TumblerThumbnailFormat tumbler_thumbnail_info_get_format         (TumblerThumbnailInfo   *info);

TumblerThumbnailFormat tumbler_thumbnail_info_get_default_format (void);
void                   tumbler_thumbnail_info_set_default_format (TumblerThumbnailFormat format);


G_END_DECLS

#endif /* !__TUMBLER_THUMBNAIL_INFO_H__ */
