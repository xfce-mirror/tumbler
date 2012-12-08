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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdlib.h>

#include <glib.h>
#include <glib/gstdio.h>
#include <glib-object.h>
#include <gio/gio.h>

#include <gdk-pixbuf/gdk-pixbuf.h>

#include <png.h>

#include <tumbler/tumbler.h>

#include <xdg-cache/xdg-cache-cache.h>
#include <xdg-cache/xdg-cache-thumbnail.h>



static void              xdg_cache_cache_iface_init         (TumblerCacheIface      *iface);
static void              xdg_cache_cache_finalize           (GObject                *object);
static TumblerThumbnail *xdg_cache_cache_get_thumbnail      (TumblerCache           *cache,
                                                             const gchar            *uri,
                                                             TumblerThumbnailFlavor *flavor);
static void              xdg_cache_cache_cleanup            (TumblerCache           *cache,
                                                             const gchar *const     *base_uris,
                                                             guint64                 since);
static void              xdg_cache_cache_delete             (TumblerCache           *cache,
                                                             const gchar *const     *uris);
static void              xdg_cache_cache_copy               (TumblerCache           *cache,
                                                             const gchar *const     *from_uris,
                                                             const gchar *const     *to_uris);
static void              xdg_cache_cache_move               (TumblerCache           *cache,
                                                             const gchar *const     *from_uris,
                                                             const gchar *const     *to_uris);
static gboolean          xdg_cache_cache_is_thumbnail       (TumblerCache           *cache,
                                                             const gchar            *uri);
static GList            *xdg_cache_cache_get_flavors        (TumblerCache           *cache);
static const gchar      *xdg_cache_cache_get_home           (void);



struct _XDGCacheCacheClass
{
  GObjectClass __parent__;
};

struct _XDGCacheCache
{
  GObject __parent__;

  GList  *flavors;
};



G_DEFINE_DYNAMIC_TYPE_EXTENDED (XDGCacheCache,
                                xdg_cache_cache,
                                G_TYPE_OBJECT,
                                0,
                                TUMBLER_ADD_INTERFACE (TUMBLER_TYPE_CACHE,
                                                       xdg_cache_cache_iface_init));



void
xdg_cache_cache_register (TumblerCachePlugin *plugin)
{
  xdg_cache_cache_register_type (G_TYPE_MODULE (plugin));
}



static void
xdg_cache_cache_class_init (XDGCacheCacheClass *klass)
{
  GObjectClass *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->finalize = xdg_cache_cache_finalize;
}



static void
xdg_cache_cache_class_finalize (XDGCacheCacheClass *klass)
{
}



static void
xdg_cache_cache_iface_init (TumblerCacheIface *iface)
{
  iface->get_thumbnail = xdg_cache_cache_get_thumbnail;
  iface->cleanup = xdg_cache_cache_cleanup;
  iface->do_delete = xdg_cache_cache_delete;
  iface->copy = xdg_cache_cache_copy;
  iface->move = xdg_cache_cache_move;
  iface->is_thumbnail = xdg_cache_cache_is_thumbnail;
  iface->get_flavors = xdg_cache_cache_get_flavors;
}



static void
xdg_cache_cache_init (XDGCacheCache *cache)
{
  TumblerThumbnailFlavor *flavor;

  flavor = tumbler_thumbnail_flavor_new_normal ();
  cache->flavors = g_list_prepend (cache->flavors, flavor);

  flavor = tumbler_thumbnail_flavor_new_large ();
  cache->flavors = g_list_prepend (cache->flavors, flavor);
}



static void
xdg_cache_cache_finalize (GObject *object)
{
  XDGCacheCache *cache = XDG_CACHE_CACHE (object);

  g_list_foreach (cache->flavors, (GFunc) g_object_unref, NULL);
  g_list_free (cache->flavors);
}



static TumblerThumbnail *
xdg_cache_cache_get_thumbnail (TumblerCache           *cache,
                               const gchar            *uri,
                               TumblerThumbnailFlavor *flavor)
{
  g_return_val_if_fail (XDG_CACHE_IS_CACHE (cache), NULL);
  g_return_val_if_fail (uri != NULL && *uri != '\0', NULL);
  g_return_val_if_fail (TUMBLER_IS_THUMBNAIL_FLAVOR (flavor), NULL);

  /* TODO check if the flavor is supported */

  return g_object_new (XDG_CACHE_TYPE_THUMBNAIL, "cache", cache,
                       "uri", uri, "flavor", flavor, NULL);
}



static void
xdg_cache_cache_cleanup (TumblerCache       *cache,
                         const gchar *const *base_uris,
                         guint64             since)
{
  XDGCacheCache *xdg_cache = XDG_CACHE_CACHE (cache);
  const gchar   *file_basename;
  guint64        mtime;
  GFile         *base_file;
  GFile         *dummy_file;
  GFile         *original_file;
  GFile         *parent;
  GList         *iter;
  gchar         *dirname;
  gchar         *filename;
  gchar         *uri;
  guint          n;
  GDir          *dir;

  g_return_if_fail (XDG_CACHE_IS_CACHE (cache));

  /* iterate over all flavors */
  for (iter = xdg_cache->flavors; iter != NULL; iter = iter->next)
    {
      /* compute the flavor directory filename */
      dummy_file = xdg_cache_cache_get_file ("foo", iter->data);
      parent = g_file_get_parent (dummy_file);
      dirname = g_file_get_path (parent);
      g_object_unref (parent);
      g_object_unref (dummy_file);

      /* attempt to open the directory for reading */
      dir = g_dir_open (dirname, 0, NULL);
      if (dir != NULL)
        {
          /* iterate over all files in the directory */
          file_basename = g_dir_read_name (dir);
          while (file_basename != NULL)
            {
              /* build the thumbnail filename */
              filename = g_build_filename (dirname, file_basename, NULL);

              /* read thumbnail information from the file */
              if (xdg_cache_cache_read_thumbnail_info (filename, &uri, &mtime, 
                                                       NULL, NULL))
                {
                  /* check if the thumbnail information is valid or the mtime
                   * is too old */
                  if (uri == NULL || mtime <= since) 
                    {
                      /* it's invalid, so let's remove the thumbnail */
                      g_unlink (filename);
                    }
                  else
                    {
                      /* create a GFile for the original URI. we need this for
                       * reliably checking the ancestor/descendant relationship */
                      original_file = g_file_new_for_uri (uri);

                      for (n = 0; base_uris != NULL && base_uris[n] != NULL; ++n)
                        {
                          /* create a GFile for the base URI */
                          base_file = g_file_new_for_uri (base_uris[n]);

                          /* delete the file if it is a descendant of the base URI */
                          if (g_file_equal (original_file, base_file)
                              || g_file_has_prefix (original_file, base_file))
                            {
                              g_unlink (filename);
                            }

                          /* releas the base file */
                          g_object_unref(base_file);
                        }

                      /* release the original file */
                      g_object_unref (original_file);
                    }
                }

              /* free the thumbnail filename */
              g_free (filename);

              /* try to determine the next filename in the directory */
              file_basename = g_dir_read_name (dir);
            }

          /* close the handle used to reading from the directory */
          g_dir_close (dir);
        }

      /* free the thumbnail flavor directory filename */
      g_free (dirname);
    }
}



static void
xdg_cache_cache_delete (TumblerCache       *cache,
                        const gchar *const *uris)
{
  XDGCacheCache *xdg_cache = XDG_CACHE_CACHE (cache);
  GList         *iter;
  GFile         *file;
  gint           n;

  g_return_if_fail (XDG_CACHE_IS_CACHE (cache));
  g_return_if_fail (uris != NULL);

  for (iter = xdg_cache->flavors; iter != NULL; iter = iter->next)
    {
      for (n = 0; uris[n] != NULL; ++n)
        {
          file = xdg_cache_cache_get_file (uris[n], iter->data);
          g_file_delete (file, NULL, NULL);
          g_object_unref (file);
        }
    }
}



static void
xdg_cache_cache_copy (TumblerCache       *cache,
                      const gchar *const *from_uris,
                      const gchar *const *to_uris)
{
  XDGCacheCache *xdg_cache = XDG_CACHE_CACHE (cache);
  GFileInfo     *info;
  guint64        mtime;
  GFile         *dest_file;
  GFile         *dest_source_file;
  GFile         *from_file;
  GFile         *temp_file;
  GList         *iter;
  gchar         *temp_path;
  gchar         *dest_path;
  guint          n;

  g_return_if_fail (XDG_CACHE_IS_CACHE (cache));
  g_return_if_fail (from_uris != NULL);
  g_return_if_fail (to_uris != NULL);

  for (iter = xdg_cache->flavors; iter != NULL; iter = iter->next)
    {
      for (n = 0; n < g_strv_length ((gchar **)from_uris); ++n)
        {
          dest_source_file = g_file_new_for_uri (to_uris[n]);
          info = g_file_query_info (dest_source_file, G_FILE_ATTRIBUTE_TIME_MODIFIED,
                                    G_FILE_QUERY_INFO_NONE, NULL, NULL);
          g_object_unref (dest_source_file);

          if (info == NULL)
            continue;

          mtime = g_file_info_get_attribute_uint64 (info, 
                                                    G_FILE_ATTRIBUTE_TIME_MODIFIED);
          g_object_unref (info);

          from_file = xdg_cache_cache_get_file (from_uris[n], iter->data);
          temp_file = xdg_cache_cache_get_temp_file (to_uris[n], iter->data);

          if (g_file_copy (from_file, temp_file, G_FILE_COPY_OVERWRITE, 
                           NULL, NULL, NULL, NULL))
            {
              temp_path = g_file_get_path (temp_file);

              if (xdg_cache_cache_write_thumbnail_info (temp_path, to_uris[n], mtime,
                                                        NULL, NULL))
                {
                  dest_file = xdg_cache_cache_get_file (to_uris[n], iter->data);
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
xdg_cache_cache_move (TumblerCache       *cache,
                      const gchar *const *from_uris,
                      const gchar *const *to_uris)
{
  XDGCacheCache *xdg_cache = XDG_CACHE_CACHE (cache);
  GFileInfo     *info;
  guint64        mtime;
  GFile         *dest_file;
  GFile         *dest_source_file;
  GFile         *from_file;
  GFile         *temp_file;
  GList         *iter;
  gchar         *from_path;
  gchar         *temp_path;
  gchar         *dest_path;
  guint          n;

  g_return_if_fail (XDG_CACHE_IS_CACHE (cache));
  g_return_if_fail (from_uris != NULL);
  g_return_if_fail (to_uris != NULL);

  for (iter = xdg_cache->flavors; iter != NULL; iter = iter->next)
    {
      for (n = 0; n < g_strv_length ((gchar **)from_uris); ++n)
        {
          dest_source_file = g_file_new_for_uri (to_uris[n]);
          info = g_file_query_info (dest_source_file, G_FILE_ATTRIBUTE_TIME_MODIFIED,
                                    G_FILE_QUERY_INFO_NONE, NULL, NULL);
          g_object_unref (dest_source_file);

          if (info == NULL)
            continue;

          mtime = g_file_info_get_attribute_uint64 (info, 
                                                    G_FILE_ATTRIBUTE_TIME_MODIFIED);
          g_object_unref (info);

          from_file = xdg_cache_cache_get_file (from_uris[n], iter->data);
          temp_file = xdg_cache_cache_get_temp_file (to_uris[n], iter->data);

          if (g_file_move (from_file, temp_file, G_FILE_COPY_OVERWRITE, 
                           NULL, NULL, NULL, NULL))
            {
              temp_path = g_file_get_path (temp_file);

              if (xdg_cache_cache_write_thumbnail_info (temp_path, to_uris[n], mtime,
                                                        NULL, NULL))
                {
                  dest_file = xdg_cache_cache_get_file (to_uris[n], iter->data);
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



static gboolean
xdg_cache_cache_is_thumbnail (TumblerCache *cache,
                              const gchar  *uri)
{
  XDGCacheCache *xdg_cache = XDG_CACHE_CACHE (cache);
  const gchar   *home;
  const gchar   *dirname;
  gboolean       is_thumbnail = FALSE;
  GList         *iter;
  GFile         *flavor_dir;
  GFile         *file;
  gchar         *path;

  g_return_val_if_fail (XDG_CACHE_IS_CACHE (cache), FALSE);
  g_return_val_if_fail (uri != NULL, FALSE);

  for (iter = xdg_cache->flavors; !is_thumbnail && iter != NULL; iter = iter->next)
    {
      home = xdg_cache_cache_get_home ();
      dirname = tumbler_thumbnail_flavor_get_name (iter->data);
      path = g_build_filename (home, ".thumbnails", dirname, NULL);

      flavor_dir = g_file_new_for_path (path);
      file = g_file_new_for_uri (uri);

      if (g_file_has_prefix (file, flavor_dir))
        is_thumbnail = TRUE;

      g_object_unref (file);
      g_object_unref (flavor_dir);

      g_free (path);
    }

  return is_thumbnail;
}



static GList *
xdg_cache_cache_get_flavors (TumblerCache *cache)
{
  XDGCacheCache *xdg_cache = XDG_CACHE_CACHE (cache);
  GList         *flavors = NULL;
  GList         *iter;

  g_return_val_if_fail (XDG_CACHE_IS_CACHE (cache), NULL);

  for (iter = g_list_last (xdg_cache->flavors); iter != NULL; iter = iter->prev)
    flavors = g_list_prepend (flavors, g_object_ref (iter->data));

  return flavors;
}



static const gchar *
xdg_cache_cache_get_home (void)
{
  return g_getenv ("HOME") != NULL ? g_getenv ("HOME") : g_get_home_dir ();
}



GFile *
xdg_cache_cache_get_file (const gchar            *uri,
                          TumblerThumbnailFlavor *flavor)
{
  const gchar *home;
  const gchar *dirname;
  GFile       *file;
  gchar       *filename;
  gchar       *md5_hash;
  gchar       *path;

  g_return_val_if_fail (uri != NULL && *uri != '\0', NULL);
  g_return_val_if_fail (TUMBLER_IS_THUMBNAIL_FLAVOR (flavor), NULL);

  home = xdg_cache_cache_get_home ();
  dirname = tumbler_thumbnail_flavor_get_name (flavor);

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
xdg_cache_cache_get_temp_file (const gchar            *uri,
                               TumblerThumbnailFlavor *flavor)
{
  const gchar *home;
  const gchar *dirname;
  GTimeVal     current_time = { 0, 0 };
  GFile       *file;
  gchar       *filename;
  gchar       *md5_hash;
  gchar       *path;

  g_return_val_if_fail (uri != NULL && *uri != '\0', NULL);
  g_return_val_if_fail (TUMBLER_IS_THUMBNAIL_FLAVOR (flavor), NULL);

  home = xdg_cache_cache_get_home ();
  dirname = tumbler_thumbnail_flavor_get_name (flavor);

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
#ifdef PNG_SETJMP_SUPPORTED
              if (setjmp (png_jmpbuf (png_ptr)))
                {
                  /* finalize the PNG reader */
                  png_destroy_read_struct (&png_ptr, &info_ptr, NULL);

                  /* close the PNG file handle */
                  fclose (png);

                  return FALSE;
                }
#endif

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

                      if (strcmp ("Thumb::URI", text_ptr[i].key) == 0)
                        {
                          /* remember the Thumb::URI value */
                          *uri = g_strdup (text_ptr[i].text);
                          has_uri = TRUE;
                        }
                      else if (strcmp ("Thumb::MTime", text_ptr[i].key) == 0)
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
                                      const gchar  *uri,
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
          mtime_str = g_strdup_printf ("%" G_GUINT64_FORMAT, mtime);

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
