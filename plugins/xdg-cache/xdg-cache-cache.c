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

#include <gdk-pixbuf/gdk-pixbuf.h>

#include <png.h>

#include <tumbler/tumbler.h>

#include <xdg-cache/xdg-cache-cache.h>
#include <xdg-cache/xdg-cache-thumbnail.h>



typedef struct _FlavorInfo FlavorInfo;



static void   xdg_cache_cache_iface_init     (TumblerCacheIface *iface);
static GList *xdg_cache_cache_get_thumbnails (TumblerCache      *cache,
                                              const gchar       *uri);
static void   xdg_cache_cache_cleanup        (TumblerCache      *cache,
                                              const gchar       *uri_prefix,
                                              guint64            since);
static void   xdg_cache_cache_delete         (TumblerCache      *cache,
                                              const GStrv        uris);
static void   xdg_cache_cache_copy           (TumblerCache      *cache,
                                              const GStrv        from_uris,
                                              const GStrv        to_uris);
static void   xdg_cache_cache_move           (TumblerCache      *cache,
                                              const GStrv        from_uris,
                                              const GStrv        to_uris);



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
  iface->cleanup = xdg_cache_cache_cleanup;
  iface->delete = xdg_cache_cache_delete;
  iface->copy = xdg_cache_cache_copy;
  iface->move = xdg_cache_cache_move;
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



static void
xdg_cache_cache_cleanup (TumblerCache *cache,
                         const gchar  *uri_prefix,
                         guint64       since)
{
  TumblerThumbnailFlavor *flavors;
  const gchar            *basename;
  guint64                 mtime;
  GFile                  *dummy_file;
  GFile                  *parent;
  gchar                  *dirname;
  gchar                  *filename;
  gchar                  *uri;
  GDir                   *dir;
  gint                    n;

  g_return_if_fail (XDG_CACHE_IS_CACHE (cache));
  
  flavors = tumbler_thumbnail_get_flavors ();

  for (n = 0; flavors[n] != TUMBLER_THUMBNAIL_FLAVOR_INVALID; ++n)
    {
      dummy_file = xdg_cache_cache_get_file ("foo", flavors[n]);
      parent = g_file_get_parent (dummy_file);
      dirname = g_file_get_path (parent);
      g_object_unref (parent);
      g_object_unref (dummy_file);

      dir = g_dir_open (dirname, 0, NULL);

      if (dir != NULL)
        {
          while ((basename = g_dir_read_name (dir)) != NULL)
            {
              filename = g_build_filename (dirname, basename, NULL);

              if (xdg_cache_cache_read_thumbnail_info (filename, &uri, &mtime, 
                                                       NULL, NULL))
                {
                  if ((uri_prefix == NULL || uri == NULL) 
                      || (g_str_has_prefix (uri, uri_prefix) && (mtime <= since)))
                    {
                      g_unlink (filename);
                    }
                }

              g_free (filename);
            }

          g_dir_close (dir);
        }

      g_free (dirname);
    }
}



static void
xdg_cache_cache_delete (TumblerCache *cache,
                        const GStrv   uris)
{
  TumblerThumbnailFlavor *flavors;
  GFile                  *file;
  gint                    n;
  gint                    i;

  g_return_if_fail (XDG_CACHE_IS_CACHE (cache));
  g_return_if_fail (uris != NULL);

  flavors = tumbler_thumbnail_get_flavors ();

  for (n = 0; flavors[n] != TUMBLER_THUMBNAIL_FLAVOR_INVALID; ++n)
    {
      for (i = 0; uris[i] != NULL; ++i)
        {
          file = xdg_cache_cache_get_file (uris[i], flavors[n]);
          g_file_delete (file, NULL, NULL);
          g_object_unref (file);
        }
    }
}



static void
xdg_cache_cache_copy (TumblerCache *cache,
                      const GStrv   from_uris,
                      const GStrv   to_uris)
{
  TumblerThumbnailFlavor *flavors;
  GFileInfo              *info;
  guint64                 mtime;
  GFile                  *dest_file;
  GFile                  *dest_source_file;
  GFile                  *from_file;
  GFile                  *temp_file;
  gchar                  *temp_path;
  gchar                  *dest_path;
  guint                   i;
  guint                   n;

  g_return_if_fail (XDG_CACHE_IS_CACHE (cache));
  g_return_if_fail (from_uris != NULL);
  g_return_if_fail (to_uris != NULL);
  g_return_if_fail (g_strv_length (from_uris) == g_strv_length (to_uris));

  flavors = tumbler_thumbnail_get_flavors ();

  for (n = 0; flavors[n] != TUMBLER_THUMBNAIL_FLAVOR_INVALID; ++n)
    {
      for (i = 0; i < g_strv_length (from_uris); ++i)
        {
          dest_source_file = g_file_new_for_uri (to_uris[i]);
          info = g_file_query_info (dest_source_file, G_FILE_ATTRIBUTE_TIME_MODIFIED,
                                    G_FILE_QUERY_INFO_NONE, NULL, NULL);
          g_object_unref (dest_source_file);

          if (info == NULL)
            continue;

          mtime = g_file_info_get_attribute_uint64 (info, 
                                                    G_FILE_ATTRIBUTE_TIME_MODIFIED);
          g_object_unref (info);

          from_file = xdg_cache_cache_get_file (from_uris[i], flavors[n]);
          temp_file = xdg_cache_cache_get_temp_file (to_uris[i], flavors[n]);

          if (g_file_copy (from_file, temp_file, G_FILE_COPY_OVERWRITE, 
                           NULL, NULL, NULL, NULL))
            {
              temp_path = g_file_get_path (temp_file);

              if (xdg_cache_cache_write_thumbnail_info (temp_path, to_uris[i], mtime,
                                                        NULL, NULL))
                {
                  dest_file = xdg_cache_cache_get_file (to_uris[i], flavors[n]);
                  dest_path = g_file_get_path (dest_file);

                  if (g_rename (temp_path, dest_path) != 0)
                    g_unlink (temp_path);

                  g_free (dest_path);
                  g_object_unref (dest_file);
                }
              else
                {
                  g_unlink (temp_path);
                }

              g_free (temp_path);
            }

          g_object_unref (temp_file);
          g_object_unref (from_file);
        }
    }
}




static void
xdg_cache_cache_move (TumblerCache *cache,
                      const GStrv   from_uris,
                      const GStrv   to_uris)
{
  TumblerThumbnailFlavor *flavors;
  GFileInfo              *info;
  guint64                 mtime;
  GFile                  *dest_file;
  GFile                  *dest_source_file;
  GFile                  *from_file;
  GFile                  *temp_file;
  gchar                  *from_path;
  gchar                  *temp_path;
  gchar                  *dest_path;
  guint                   i;
  guint                   n;

  g_return_if_fail (XDG_CACHE_IS_CACHE (cache));
  g_return_if_fail (from_uris != NULL);
  g_return_if_fail (to_uris != NULL);
  g_return_if_fail (g_strv_length (from_uris) == g_strv_length (to_uris));

  flavors = tumbler_thumbnail_get_flavors ();

  for (n = 0; flavors[n] != TUMBLER_THUMBNAIL_FLAVOR_INVALID; ++n)
    {
      for (i = 0; i < g_strv_length (from_uris); ++i)
        {
          dest_source_file = g_file_new_for_uri (to_uris[i]);
          info = g_file_query_info (dest_source_file, G_FILE_ATTRIBUTE_TIME_MODIFIED,
                                    G_FILE_QUERY_INFO_NONE, NULL, NULL);
          g_object_unref (dest_source_file);

          if (info == NULL)
            continue;

          mtime = g_file_info_get_attribute_uint64 (info, 
                                                    G_FILE_ATTRIBUTE_TIME_MODIFIED);
          g_object_unref (info);

          from_file = xdg_cache_cache_get_file (from_uris[i], flavors[n]);
          temp_file = xdg_cache_cache_get_temp_file (to_uris[i], flavors[n]);

          if (g_file_move (from_file, temp_file, G_FILE_COPY_OVERWRITE, 
                           NULL, NULL, NULL, NULL))
            {
              temp_path = g_file_get_path (temp_file);

              if (xdg_cache_cache_write_thumbnail_info (temp_path, to_uris[i], mtime,
                                                        NULL, NULL))
                {
                  dest_file = xdg_cache_cache_get_file (to_uris[i], flavors[n]);
                  dest_path = g_file_get_path (dest_file);

                  if (g_rename (temp_path, dest_path) != 0)
                    g_unlink (temp_path);

                  g_free (dest_path);
                  g_object_unref (dest_file);
                }
              else
                {
                  g_unlink (temp_path);
                }

              g_free (temp_path);
            }

          from_path = g_file_get_path (from_file);
          g_unlink (from_path);
          g_free (from_path);

          g_object_unref (temp_file);
          g_object_unref (from_file);
        }
    }
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



gboolean
xdg_cache_cache_read_thumbnail_info (const gchar  *filename,
                                     gchar       **uri,
                                     guint64      *mtime,
                                     GCancellable *cancellable,
                                     GError      **error)
{
  png_structp png_ptr;
  png_infop   info_ptr;
  png_textp   text_ptr;
  gboolean    has_uri = FALSE;
  gboolean    has_mtime = FALSE;
  FILE       *png;
  gint        num_text;
  gint        i;

  g_return_val_if_fail (filename != NULL, FALSE);
  g_return_val_if_fail (uri != NULL, FALSE);
  g_return_val_if_fail (mtime != NULL, FALSE);
  g_return_val_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  *uri = NULL;
  *mtime = 0;

  if (g_cancellable_set_error_if_cancelled (cancellable, error))
    return FALSE;

  if ((png = g_fopen (filename, "r")) != NULL)
    {
      /* initialize the PNG reader */
      png_ptr = png_create_read_struct (PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

      if (png_ptr)
        {
          /* initialize the info structure */
          info_ptr = png_create_info_struct (png_ptr);

          if (info_ptr)
            {
              /* initialize reading from the file and read the file info */
              png_init_io (png_ptr, png);
              png_read_info (png_ptr, info_ptr);

              /* check if there is embedded text information */
              if (png_get_text (png_ptr, info_ptr, &text_ptr, &num_text) > 0)
                {
                  /* iterate over all text keys */
                  for (i = 0; !(has_uri && has_mtime) && i < num_text; ++i)
                    {
                      if (!text_ptr[i].key)
                        continue;
                      else if (g_utf8_collate ("Thumb::URI", text_ptr[i].key) == 0)
                        {
                          /* remember the Thumb::URI value */
                          *uri = g_strdup (text_ptr[i].text);
                          has_uri = TRUE;
                        }
                      else if (g_utf8_collate ("Thumb::MTime", text_ptr[i].key) == 0)
                        {
                          /* remember the Thumb::MTime value */
                          if (text_ptr[i].text != NULL)
                            {
                              *mtime = atol (text_ptr[i].text);
                              has_mtime = TRUE;
                            }
                        }
                    }
                }
            }

          /* finalize the PNG reader */
          png_destroy_read_struct (&png_ptr, &info_ptr, NULL);
        }

      /* close the PNG file handle */
      fclose (png);
    }

  return TRUE;
}



gboolean
xdg_cache_cache_write_thumbnail_info (const gchar  *filename,
                                      gchar        *uri,
                                      guint64       mtime,
                                      GCancellable *cancellable,
                                      GError      **error)
{
  GdkPixbuf *pixbuf;
  GError    *err = NULL;
  gchar     *mtime_str;

  g_return_val_if_fail (filename != NULL, FALSE);
  g_return_val_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  if (g_cancellable_set_error_if_cancelled (cancellable, error))
    return FALSE;

  pixbuf = gdk_pixbuf_new_from_file (filename, &err);

  if (pixbuf != NULL)
    {
      if (!g_cancellable_set_error_if_cancelled (cancellable, &err))
        {
          mtime_str = g_strdup_printf ("%lld", mtime);

          gdk_pixbuf_save (pixbuf, filename, "png", &err,
                           "tEXt::Thumb::URI", uri,
                           "tEXt::Thumb::MTime", mtime_str,
                           NULL);

          g_free (mtime_str);
        }

      g_object_unref (pixbuf);
    }

  if (err != NULL)
    {
      g_propagate_error (error, err);
      return FALSE;
    }
  else
    {
      return TRUE;
    }
}
