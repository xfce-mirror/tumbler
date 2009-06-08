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

#ifndef __XDG_CACHE_THUMBNAIL_H__
#define __XDG_CACHE_THUMBNAIL_H__

#include <glib-object.h>

#include <tumbler/tumbler.h>

G_BEGIN_DECLS;

#define XDG_CACHE_TYPE_THUMBNAIL            (xdg_cache_thumbnail_get_type ())
#define XDG_CACHE_THUMBNAIL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), XDG_CACHE_TYPE_THUMBNAIL, XDGCacheThumbnail))
#define XDG_CACHE_THUMBNAIL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), XDG_CACHE_TYPE_THUMBNAIL, XDGCacheThumbnailClass))
#define XDG_CACHE_IS_THUMBNAIL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), XDG_CACHE_TYPE_THUMBNAIL))
#define XDG_CACHE_IS_THUMBNAIL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), XDG_CACHE_TYPE_THUMBNAIL)
#define XDG_CACHE_THUMBNAIL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), XDG_CACHE_TYPE_THUMBNAIL, XDGCacheThumbnailClass))

typedef struct _XDGCacheThumbnailPrivate XDGCacheThumbnailPrivate;
typedef struct _XDGCacheThumbnailClass   XDGCacheThumbnailClass;
typedef struct _XDGCacheThumbnail        XDGCacheThumbnail;

GType xdg_cache_thumbnail_get_type (void) G_GNUC_CONST;
void  xdg_cache_thumbnail_register (TumblerProviderPlugin *plugin);

G_END_DECLS;

#endif /* !__XDG_CACHE_THUMBNAIL_H__ */
