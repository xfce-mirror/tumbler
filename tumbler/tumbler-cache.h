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

#if !defined (_TUMBLER_INSIDE_TUMBLER_H) && !defined (TUMBLER_COMPILATION)
#error "Only <tumbler/tumbler.h> may be included directly. This file might disappear or change contents."
#endif

#ifndef __TUMBLER_CACHE_H__
#define __TUMBLER_CACHE_H__

#include <glib-object.h>
#include <tumbler/tumbler-thumbnail.h>
#include <tumbler/tumbler-thumbnail-flavor.h>

G_BEGIN_DECLS

#define TUMBLER_TYPE_CACHE (tumbler_cache_get_type ())
G_DECLARE_INTERFACE (TumblerCache, tumbler_cache, TUMBLER, CACHE, GObject)

typedef struct _TumblerCacheInterface TumblerCacheIface;

struct _TumblerCacheInterface
{
  GTypeInterface __parent__;

  /* signals */

  /* virtual methods */
  TumblerThumbnail *(*get_thumbnail) (TumblerCache           *cache,
                                      const gchar            *uri,
                                      TumblerThumbnailFlavor *flavor);
  void              (*cleanup)       (TumblerCache           *cache,
                                      const gchar *const     *base_uris,
                                      gdouble                 since);
  void              (*do_delete)     (TumblerCache           *cache,
                                      const gchar *const     *uris);
  void              (*copy)          (TumblerCache           *cache,
                                      const gchar *const     *from_uris,
                                      const gchar *const     *to_uris);
  void              (*move)          (TumblerCache           *cache,
                                      const gchar *const     *from_uris,
                                      const gchar *const     *to_uris);
  gboolean          (*is_thumbnail)  (TumblerCache           *cache,
                                      const gchar            *uri);
  GList            *(*get_flavors)   (TumblerCache           *cache);
};

TumblerCache           *tumbler_cache_get_default    (void) G_GNUC_WARN_UNUSED_RESULT;

TumblerThumbnail       *tumbler_cache_get_thumbnail  (TumblerCache           *cache,
                                                      const gchar            *uri,
                                                      TumblerThumbnailFlavor *flavor) G_GNUC_MALLOC G_GNUC_WARN_UNUSED_RESULT;
void                    tumbler_cache_cleanup        (TumblerCache           *cache,
                                                      const gchar *const     *base_uris,
                                                      gdouble                 since);
void                    tumbler_cache_delete         (TumblerCache           *cache,
                                                      const gchar *const     *uris);
void                    tumbler_cache_copy           (TumblerCache           *cache,
                                                      const gchar *const     *from_uris,
                                                      const gchar *const     *to_uris);
void                    tumbler_cache_move           (TumblerCache           *cache,
                                                      const gchar *const     *from_uris,
                                                      const gchar *const     *to_uris);
gboolean                tumbler_cache_is_thumbnail   (TumblerCache           *cache,
                                                      const gchar            *uri);
GList                  *tumbler_cache_get_flavors    (TumblerCache           *cache) G_GNUC_MALLOC G_GNUC_WARN_UNUSED_RESULT;
TumblerThumbnailFlavor *tumbler_cache_get_flavor     (TumblerCache           *cache,
                                                      const gchar            *name) G_GNUC_WARN_UNUSED_RESULT;

G_END_DECLS

#endif /* !__TUMBLER_CACHE_H__ */
