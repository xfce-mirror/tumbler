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

#ifndef __TUMBLER_CACHE_H__
#define __TUMBLER_CACHE_H__

#include <glib-object.h>

G_BEGIN_DECLS

#define TUMBLER_TYPE_CACHE           (tumbler_cache_get_type ())
#define TUMBLER_CACHE(obj)           (G_TYPE_CHECK_INSTANCE_CAST ((obj), TUMBLER_TYPE_CACHE, TumblerCache))
#define TUMBLER_IS_CACHE(obj)        (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TUMBLER_TYPE_CACHE))
#define TUMBLER_CACHE_GET_IFACE(obj) (G_TYPE_INSTANCE_GET_INTERFACE ((obj), TUMBLER_TYPE_CACHE, TumblerCacheIface))

typedef struct _TumblerCache      TumblerCache;
typedef struct _TumblerCacheIface TumblerCacheIface;

struct _TumblerCacheIface
{
  GTypeInterface __parent__;

  /* signals */

  /* virtual methods */
  GList   *(*get_thumbnails) (TumblerCache *cache,
                              const gchar  *uri);
  void     (*cleanup)        (TumblerCache *cache,
                              const gchar  *uri,
                              guint64       since);
  void     (*delete)         (TumblerCache *cache,
                              const GStrv   uris);
  void     (*copy)           (TumblerCache *cache,
                              const GStrv   from_uris,
                              const GStrv   to_uris);
  void     (*move)           (TumblerCache *cache,
                              const GStrv   from_uris,
                              const GStrv   to_uris);
  gboolean (*is_thumbnail)   (TumblerCache *cache,
                              const gchar  *uri);
};

GType    tumbler_cache_get_type (void) G_GNUC_CONST;

GList   *tumbler_cache_get_thumbnails (TumblerCache *cache,
                                       const gchar  *uri) G_GNUC_MALLOC G_GNUC_WARN_UNUSED_RESULT;
void     tumbler_cache_cleanup        (TumblerCache *cache,
                                       const gchar  *uri_prefix,
                                       guint64       since);
void     tumbler_cache_delete         (TumblerCache *cache,
                                       const GStrv   uris);
void     tumbler_cache_copy           (TumblerCache *cache,
                                       const GStrv   from_uris,
                                       const GStrv   to_uris);
void     tumbler_cache_move           (TumblerCache *cache,
                                       const GStrv   from_uris,
                                       const GStrv   to_uris);
gboolean tumbler_cache_is_thumbnail   (TumblerCache *cache,
                                       const gchar  *uri);

G_END_DECLS

#endif /* !__TUMBLER_CACHE_H__ */
