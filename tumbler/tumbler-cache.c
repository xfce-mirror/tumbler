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



GList *
tumbler_cache_get_thumbnails (TumblerCache *cache,
                              const gchar  *uri)
{
  g_return_val_if_fail (TUMBLER_IS_CACHE (cache), NULL);
  g_return_val_if_fail (uri != NULL && *uri != '\0', NULL);
  g_return_val_if_fail (TUMBLER_CACHE_GET_IFACE (cache)->get_thumbnails != NULL, NULL);

  return (TUMBLER_CACHE_GET_IFACE (cache)->get_thumbnails) (cache, uri);
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
  g_return_if_fail (TUMBLER_CACHE_GET_IFACE (cache)->delete != NULL);

  (TUMBLER_CACHE_GET_IFACE (cache)->delete) (cache, uris);
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
