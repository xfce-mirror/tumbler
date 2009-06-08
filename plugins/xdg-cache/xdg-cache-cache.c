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
#include <gio/gio.h>

#include <tumbler/tumbler.h>

#include <xdg-cache/xdg-cache-cache.h>
#include <xdg-cache/xdg-cache-thumbnail.h>



typedef struct _FlavorInfo FlavorInfo;



static void   xdg_cache_cache_iface_init     (TumblerCacheIface *iface);
static GList *xdg_cache_cache_get_thumbnails (TumblerCache      *cache,
                                              const gchar       *uri);



struct _XDGCacheCacheClass
{
  GObjectClass __parent__;
};

struct _XDGCacheCache
{
  GObject __parent__;
};

struct _FlavorInfo
{
  TumblerThumbnailFlavor flavor;
  const gchar           *dirname;
};



G_DEFINE_DYNAMIC_TYPE_EXTENDED (XDGCacheCache,
                                xdg_cache_cache,
                                G_TYPE_OBJECT,
                                0,
                                TUMBLER_ADD_INTERFACE (TUMBLER_TYPE_CACHE,
                                                       xdg_cache_cache_iface_init));



static const FlavorInfo flavor_infos[] = 
{
  { TUMBLER_THUMBNAIL_FLAVOR_NORMAL,  "normal"  },
  { TUMBLER_THUMBNAIL_FLAVOR_LARGE,   "large"   },
  { TUMBLER_THUMBNAIL_FLAVOR_CROPPED, "cropped" },
};



void
xdg_cache_cache_register (TumblerProviderPlugin *plugin)
{
  xdg_cache_cache_register_type (G_TYPE_MODULE (plugin));
}



static void
xdg_cache_cache_class_init (XDGCacheCacheClass *klass)
{
  /* Determine the parent type class */
  xdg_cache_cache_parent_class = g_type_class_peek_parent (klass);
}



static void
xdg_cache_cache_class_finalize (XDGCacheCacheClass *klass)
{
}



static void
xdg_cache_cache_iface_init (TumblerCacheIface *iface)
{
  iface->get_thumbnails = xdg_cache_cache_get_thumbnails;
}



static void
xdg_cache_cache_init (XDGCacheCache *cache)
{
}



static GList *
xdg_cache_cache_get_thumbnails (TumblerCache *cache,
                                const gchar  *uri)
{
  TumblerThumbnailFlavor *flavors;
  TumblerThumbnail       *thumbnail;
  GList                  *thumbnails = NULL;
  gint                    n;

  g_return_val_if_fail (XDG_CACHE_IS_CACHE (cache), NULL);
  g_return_val_if_fail (uri != NULL && *uri != '\0', NULL);

  flavors = tumbler_thumbnail_get_flavors ();

  for (n = 0; flavors[n] != TUMBLER_THUMBNAIL_FLAVOR_INVALID; ++n)
    {
      thumbnail = g_object_new (XDG_CACHE_TYPE_THUMBNAIL, "cache", cache,
                                "uri", uri, "flavor", flavors[n], NULL);

      thumbnails = g_list_append (thumbnails, thumbnail);
    }

  return thumbnails;
}



static const gchar *
xdg_cache_cache_get_flavor_dirname (TumblerThumbnailFlavor flavor)
{
  guint n;

  for (n = 0; n < G_N_ELEMENTS (flavor_infos); ++n)
    if (flavor_infos[n].flavor == flavor)
      return flavor_infos[n].dirname;

  g_assert_not_reached ();

  return NULL;
}



static const gchar *
xdg_cache_cache_get_home (void)
{
  return g_getenv ("HOME") != NULL ? g_getenv ("HOME") : g_get_home_dir ();
}



GFile *
xdg_cache_cache_get_file (const gchar           *uri,
                          TumblerThumbnailFlavor flavor)
{
  const gchar *home;
  const gchar *dirname;
  GFile       *file;
  gchar       *filename;
  gchar       *md5_hash;
  gchar       *path;

  g_return_val_if_fail (uri != NULL && *uri != '\0', NULL);
  g_return_val_if_fail (flavor != TUMBLER_THUMBNAIL_FLAVOR_INVALID, NULL);

  home = xdg_cache_cache_get_home ();
  dirname = xdg_cache_cache_get_flavor_dirname (flavor);

  md5_hash = g_compute_checksum_for_string (G_CHECKSUM_MD5, uri, -1);
  filename = g_strdup_printf ("%s.png", md5_hash);
  path = g_build_filename (home, ".thumbnails", dirname, filename, NULL);

  file = g_file_new_for_path (path);

  g_free (path);
  g_free (filename);
  g_free (md5_hash);

  return file;
}



GFile *
xdg_cache_cache_get_temp_file (const gchar           *uri,
                               TumblerThumbnailFlavor flavor)
{
  const gchar *home;
  const gchar *dirname;
  GTimeVal     current_time = { 0, 0 };
  GFile       *file;
  gchar       *filename;
  gchar       *md5_hash;
  gchar       *path;

  g_return_val_if_fail (uri != NULL && *uri != '\0', NULL);
  g_return_val_if_fail (flavor != TUMBLER_THUMBNAIL_FLAVOR_INVALID, NULL);

  home = xdg_cache_cache_get_home ();
  dirname = xdg_cache_cache_get_flavor_dirname (flavor);

  g_get_current_time (&current_time);

  md5_hash = g_compute_checksum_for_string (G_CHECKSUM_MD5, uri, -1);
  filename = g_strdup_printf ("%s-%ld-%ld.png", md5_hash, 
                              current_time.tv_sec, current_time.tv_usec);
  path = g_build_filename (home, ".thumbnails", dirname, filename, NULL);

  file = g_file_new_for_path (path);

  g_free (path);
  g_free (filename);
  g_free (md5_hash);

  return file;
}
