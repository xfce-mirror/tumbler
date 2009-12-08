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

#ifndef __JPEG_THUMBNAILER_H__
#define __JPEG_THUMBNAILER_H__

#include <glib-object.h>

G_BEGIN_DECLS;

#define TYPE_JPEG_THUMBNAILER            (jpeg_thumbnailer_get_type ())
#define JPEG_THUMBNAILER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), TYPE_JPEG_THUMBNAILER, JPEGThumbnailer))
#define JPEG_THUMBNAILER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), TYPE_JPEG_THUMBNAILER, JPEGThumbnailerClass))
#define IS_JPEG_THUMBNAILER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TYPE_JPEG_THUMBNAILER))
#define IS_JPEG_THUMBNAILER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), TYPE_JPEG_THUMBNAILER)
#define JPEG_THUMBNAILER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), TYPE_JPEG_THUMBNAILER, JPEGThumbnailerClass))

typedef struct _JPEGThumbnailerClass   JPEGThumbnailerClass;
typedef struct _JPEGThumbnailer        JPEGThumbnailer;

GType jpeg_thumbnailer_get_type (void) G_GNUC_CONST;
void  jpeg_thumbnailer_register (TumblerProviderPlugin *plugin);

G_END_DECLS;

#endif /* !__JPEG_THUMBNAILER_H__ */
