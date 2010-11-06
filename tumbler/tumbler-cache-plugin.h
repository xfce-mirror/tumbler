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

#ifndef __TUMBLER_CACHE_PLUGIN_H__
#define __TUMBLER_CACHE_PLUGIN_H__

#include <tumbler/tumbler-cache.h>

G_BEGIN_DECLS

#define TUMBLER_TYPE_CACHE_PLUGIN            (tumbler_cache_plugin_get_type ())
#define TUMBLER_CACHE_PLUGIN(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), TUMBLER_TYPE_CACHE_PLUGIN, TumblerCachePlugin))
#define TUMBLER_CACHE_PLUGIN_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), TUMBLER_TYPE_CACHE_PLUGIN, TumblerCachePluginClass))
#define TUMBLER_IS_CACHE_PLUGIN(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TUMBLER_TYPE_CACHE_PLUGIN))
#define TUMBLER_IS_CACHE_PLUGIN_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), TUMBLER_TYPE_CACHE_PLUGIN)
#define TUMBLER_CACHE_PLUGIN_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), TUMBLER_TYPE_CACHE_PLUGIN, TumblerCachePluginClass))

typedef struct _TumblerCachePluginPrivate TumblerCachePluginPrivate;
typedef struct _TumblerCachePluginClass   TumblerCachePluginClass;
typedef struct _TumblerCachePlugin        TumblerCachePlugin;

GType         tumbler_cache_plugin_get_type    (void) G_GNUC_CONST;

GTypeModule  *tumbler_cache_plugin_get_default (void);
TumblerCache *tumbler_cache_plugin_get_cache   (TumblerCachePlugin *plugin);

G_END_DECLS

#endif /* !__TUMBLER_CACHE_PLUGIN_H__ */
