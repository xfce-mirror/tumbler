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

#ifndef __JPEG_THUMBNAILER_PROVIDER_H__
#define __JPEG_THUMBNAILER_PROVIDER_H__

#include <glib-object.h>

G_BEGIN_DECLS;

#define TYPE_JPEG_THUMBNAILER_PROVIDER            (jpeg_thumbnailer_provider_get_type ())
#define JPEG_THUMBNAILER_PROVIDER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), TYPE_JPEG_THUMBNAILER_PROVIDER, JPEGThumbnailerProvider))
#define JPEG_THUMBNAILER_PROVIDER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), TYPE_JPEG_THUMBNAILER_PROVIDER, JPEGThumbnailerProviderClass))
#define IS_JPEG_THUMBNAILER_PROVIDER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TYPE_JPEG_THUMBNAILER_PROVIDER))
#define IS_JPEG_THUMBNAILER_PROVIDER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), TYPE_JPEG_THUMBNAILER_PROVIDER)
#define JPEG_THUMBNAILER_PROVIDER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), TYPE_JPEG_THUMBNAILER_PROVIDER, JPEGThumbnailerProviderClass))

typedef struct _JPEGThumbnailerProviderClass JPEGThumbnailerProviderClass;
typedef struct _JPEGThumbnailerProvider      JPEGThumbnailerProvider;

GType jpeg_thumbnailer_provider_get_type (void) G_GNUC_CONST;
void  jpeg_thumbnailer_provider_register (TumblerProviderPlugin *plugin);

G_END_DECLS;

#endif /* !__JPEG_THUMBNAILER_PROVIDER_H__ */
