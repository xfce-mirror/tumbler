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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib.h>
#include <glib-object.h>

#include <tumbler/tumbler.h>

#include <xdg-cache/xdg-cache-cache.h>
#include <xdg-cache/xdg-cache-provider.h>



static void   xdg_cache_provider_cache_provider_init (TumblerCacheProviderIface *iface);
static GList *xdg_cache_provider_get_caches          (TumblerCacheProvider      *provider);



struct _XDGCacheProviderClass
{
  GObjectClass __parent__;
};

struct _XDGCacheProvider
{
  GObject __parent__;
};



G_DEFINE_DYNAMIC_TYPE_EXTENDED (XDGCacheProvider,
                                xdg_cache_provider,
                                G_TYPE_OBJECT,
                                0,
                                TUMBLER_ADD_INTERFACE (TUMBLER_TYPE_CACHE_PROVIDER,
                                                       xdg_cache_provider_cache_provider_init));



void
xdg_cache_provider_register (TumblerProviderPlugin *plugin)
{
  xdg_cache_provider_register_type (G_TYPE_MODULE (plugin));
}



static void
xdg_cache_provider_class_init (XDGCacheProviderClass *klass)
{
  GObjectClass *gobject_class;

  /* Determine the parent type class */
  xdg_cache_provider_parent_class = g_type_class_peek_parent (klass);

  gobject_class = G_OBJECT_CLASS (klass);
}



static void
xdg_cache_provider_class_finalize (XDGCacheProviderClass *klass)
{
}



static void
xdg_cache_provider_cache_provider_init (TumblerCacheProviderIface *iface)
{
  iface->get_caches = xdg_cache_provider_get_caches;
}



static void
xdg_cache_provider_init (XDGCacheProvider *provider)
{
}



static GList *
xdg_cache_provider_get_caches (TumblerCacheProvider *provider)
{
  XDGCacheCache *cache;
  GList         *caches = NULL;

  g_return_val_if_fail (XDG_CACHE_IS_PROVIDER (provider), NULL);

  cache = g_object_new (XDG_CACHE_TYPE_CACHE, NULL);
  caches = g_list_append (caches, cache);

  return caches;
}
