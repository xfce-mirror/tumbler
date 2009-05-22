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

#ifndef __TUMBLER_PNG_THUMBNAIL_INFO_H__
#define __TUMBLER_PNG_THUMBNAIL_INFO_H__

#include <glib-object.h>

G_BEGIN_DECLS;

#define TUMBLER_TYPE_PNG_THUMBNAIL_INFO            (tumbler_png_thumbnail_info_get_type ())
#define TUMBLER_PNG_THUMBNAIL_INFO(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), TUMBLER_TYPE_PNG_THUMBNAIL_INFO, TumblerPNGThumbnailInfo))
#define TUMBLER_PNG_THUMBNAIL_INFO_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), TUMBLER_TYPE_PNG_THUMBNAIL_INFO, TumblerPNGThumbnailInfoClass))
#define TUMBLER_IS_PNG_THUMBNAIL_INFO(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TUMBLER_TYPE_PNG_THUMBNAIL_INFO))
#define TUMBLER_IS_PNG_THUMBNAIL_INFO_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), TUMBLER_TYPE_PNG_THUMBNAIL_INFO)
#define TUMBLER_PNG_THUMBNAIL_INFO_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), TUMBLER_TYPE_PNG_THUMBNAIL_INFO, TumblerPNGThumbnailInfoClass))

typedef struct _TumblerPNGThumbnailInfoPrivate TumblerPNGThumbnailInfoPrivate;
typedef struct _TumblerPNGThumbnailInfoClass   TumblerPNGThumbnailInfoClass;
typedef struct _TumblerPNGThumbnailInfo        TumblerPNGThumbnailInfo;

GType tumbler_png_thumbnail_info_get_type (void) G_GNUC_CONST;

G_END_DECLS;

#endif /* !__TUMBLER_PNG_THUMBNAIL_INFO_H__ */
