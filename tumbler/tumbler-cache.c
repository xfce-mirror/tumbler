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

#include <glib-object.h>

#include <tumbler/tumbler-cache.h>
#include <tumbler/tumbler-cache-plugin.h>
#include <tumbler/tumbler-thumbnail.h>
#include <tumbler/tumbler-thumbnail-flavor.h>



GType
tumbler_cache_get_type (void)
{
  static volatile gsize g_define_type_id__volatile = 0;
  
  if (g_once_init_enter (&g_define_type_id__volatile))
    {
      GType g_define_type_id =
        g_type_register_static_simple (G_TYPE_INTERFACE,
                                       "TumblerCache",
                                       sizeof (TumblerCacheIface),
                                       NULL,
                                       0,
                                       NULL,
                                       0);

      g_type_interface_add_prerequisite (g_define_type_id, G_TYPE_OBJECT);

      g_once_init_leave (&g_define_type_id__volatile, g_define_type_id);
    }

  return g_define_type_id__volatile;
}



TumblerCache *
tumbler_cache_get_default (void)
{
  static TumblerCache *cache = NULL;
  GTypeModule         *plugin;

  if (cache == NULL)
    {
      plugin = tumbler_cache_plugin_get_default ();

      if (plugin != NULL)
        {
          cache = tumbler_cache_plugin_get_cache (TUMBLER_CACHE_PLUGIN (plugin));
          g_object_add_weak_pointer (G_OBJECT (cache), (gpointer) &cache);
          g_type_module_unuse (plugin);
        }
    }
  else
    {
      g_object_ref (cache);
    }
     
  return cache;
}



TumblerThumbnail *
tumbler_cache_get_thumbnail (TumblerCache           *cache,
                             const gchar            *uri,
                             TumblerThumbnailFlavor *flavor)
{
  g_return_val_if_fail (TUMBLER_IS_CACHE (cache), NULL);
  g_return_val_if_fail (uri != NULL && *uri != '\0', NULL);
  g_return_val_if_fail (flavor == NULL || TUMBLER_IS_THUMBNAIL_FLAVOR (flavor), NULL);
  g_return_val_if_fail (TUMBLER_CACHE_GET_IFACE (cache)->get_thumbnail != NULL, NULL);

  if (flavor == NULL)
    return NULL;

  return (TUMBLER_CACHE_GET_IFACE (cache)->get_thumbnail) (cache, uri, flavor);
}



void
tumbler_cache_cleanup (TumblerCache *cache,
                       const gchar  *uri_prefix,
                       guint64       since)
{
  g_return_if_fail (TUMBLER_IS_CACHE (cache));
  g_return_if_fail (TUMBLER_CACHE_GET_IFACE (cache)->cleanup != NULL);

  (TUMBLER_CACHE_GET_IFACE (cache)->cleanup) (cache, uri_prefix, since);
}



void
tumbler_cache_delete (TumblerCache *cache,
                      const GStrv   uris)
{
  g_return_if_fail (TUMBLER_IS_CACHE (cache));
  g_return_if_fail (uris != NULL);
  g_return_if_fail (TUMBLER_CACHE_GET_IFACE (cache)->do_delete != NULL);

  (TUMBLER_CACHE_GET_IFACE (cache)->do_delete) (cache, uris);
}



void
tumbler_cache_copy (TumblerCache *cache,
                    const GStrv   from_uris,
                    const GStrv   to_uris)
{
  g_return_if_fail (TUMBLER_IS_CACHE (cache));
  g_return_if_fail (from_uris != NULL);
  g_return_if_fail (to_uris != NULL);
  g_return_if_fail (g_strv_length (from_uris) == g_strv_length (to_uris));
  g_return_if_fail (TUMBLER_CACHE_GET_IFACE (cache)->copy != NULL);

  (TUMBLER_CACHE_GET_IFACE (cache)->copy) (cache, from_uris, to_uris);
}



void
tumbler_cache_move (TumblerCache *cache,
                    const GStrv   from_uris,
                    const GStrv   to_uris)
{
  g_return_if_fail (TUMBLER_IS_CACHE (cache));
  g_return_if_fail (from_uris != NULL);
  g_return_if_fail (to_uris != NULL);
  g_return_if_fail (g_strv_length (from_uris) == g_strv_length (to_uris));
  g_return_if_fail (TUMBLER_CACHE_GET_IFACE (cache)->move != NULL);

  (TUMBLER_CACHE_GET_IFACE (cache)->move) (cache, from_uris, to_uris);
}



gboolean
tumbler_cache_is_thumbnail (TumblerCache *cache,
                            const gchar  *uri)
{
  g_return_val_if_fail (TUMBLER_IS_CACHE (cache), FALSE);
  g_return_val_if_fail (uri != NULL, FALSE);
  g_return_val_if_fail (TUMBLER_CACHE_GET_IFACE (cache)->is_thumbnail != NULL, FALSE);

  return (TUMBLER_CACHE_GET_IFACE (cache)->is_thumbnail) (cache, uri);
}



GList *
tumbler_cache_get_flavors (TumblerCache *cache)
{
  g_return_val_if_fail (TUMBLER_IS_CACHE (cache), NULL);
  g_return_val_if_fail (TUMBLER_CACHE_GET_IFACE (cache)->get_flavors != NULL, NULL);

  return (TUMBLER_CACHE_GET_IFACE (cache)->get_flavors) (cache);
}



TumblerThumbnailFlavor *
tumbler_cache_get_flavor (TumblerCache *cache,
                          const gchar  *name)
{
  TumblerThumbnailFlavor *flavor = NULL;
  GList                  *flavors;
  GList                  *iter;

  g_return_val_if_fail (TUMBLER_IS_CACHE (cache), NULL);
  g_return_val_if_fail (name != NULL && *name != '\0', NULL);

  flavors = tumbler_cache_get_flavors (cache);

  for (iter = flavors; flavor == NULL && iter != NULL; iter = iter->next)
    if (g_strcmp0 (tumbler_thumbnail_flavor_get_name (iter->data), name) == 0)
      flavor = g_object_ref (iter->data);

  g_list_foreach (flavors, (GFunc) g_object_unref, NULL);
  g_list_free (flavors);

  return flavor;
}
