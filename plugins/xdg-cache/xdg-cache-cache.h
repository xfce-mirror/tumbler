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

#ifndef __XDG_CACHE_CACHE_H__
#define __XDG_CACHE_CACHE_H__

#include <gio/gio.h>

#include <tumbler/tumbler.h>

G_BEGIN_DECLS;

#define XDG_CACHE_TYPE_CACHE            (xdg_cache_cache_get_type ())
#define XDG_CACHE_CACHE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), XDG_CACHE_TYPE_CACHE, XDGCacheCache))
#define XDG_CACHE_CACHE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), XDG_CACHE_TYPE_CACHE, XDGCacheCacheClass))
#define XDG_CACHE_IS_CACHE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), XDG_CACHE_TYPE_CACHE))
#define XDG_CACHE_IS_CACHE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), XDG_CACHE_TYPE_CACHE)
#define XDG_CACHE_CACHE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), XDG_CACHE_TYPE_CACHE, XDGCacheCacheClass))

typedef struct _XDGCacheCachePrivate XDGCacheCachePrivate;
typedef struct _XDGCacheCacheClass   XDGCacheCacheClass;
typedef struct _XDGCacheCache        XDGCacheCache;

GType  xdg_cache_cache_get_type      (void) G_GNUC_CONST;
void   xdg_cache_cache_register      (TumblerProviderPlugin *plugin);

GFile *xdg_cache_cache_get_file      (const gchar           *uri,
                                      TumblerThumbnailFlavor flavor) G_GNUC_MALLOC G_GNUC_WARN_UNUSED_RESULT;
GFile *xdg_cache_cache_get_temp_file (const gchar           *uri,
                                      TumblerThumbnailFlavor flavor) G_GNUC_MALLOC G_GNUC_WARN_UNUSED_RESULT;

G_END_DECLS;

#endif /* !__XDG_CACHE_CACHE_H__ */
