/* vi:set et ai sw=2 sts=2 ts=2: */
/*-
 * Copyright (c) 2009 Jannis Pohlmann <jannis@xfce.org>
 *
 * This program is free software; you can redistribute it and/or 
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of 
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public 
 * License along with this program; if not, write to the Free 
 * Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <math.h>

#include <gdk-pixbuf/gdk-pixbuf.h>

#include <gio/gio.h>

#include <tumbler/tumbler.h>

#include <tumblerd/tumbler-builtin-thumbnailers.h>
#include <tumblerd/tumbler-builtin-thumbnailer.h>
#include <tumblerd/tumbler-thumbnailer.h>



#ifdef ENABLE_PIXBUF_THUMBNAILER

static gboolean
_tumbler_pixbuf_thumbnailer (TumblerBuiltinThumbnailer *thumbnailer,
                             const gchar               *uri,
                             const gchar               *mime_hint,
                             GError                   **error)
{
  TumblerThumbnailFlavor *flavors;
  TumblerThumbnailInfo   *info;
  GFileInputStream       *stream;
  GdkPixbuf              *pixbuf;
  GdkPixbuf              *source_pixbuf;
  gdouble                 factor;
  GError                 *err = NULL;
  GFile                  *file;
  gint                    dest_width;
  gint                    dest_height;
  gint                    height;
  gint                    width;
  gint                    size;
  gint                    n;

  g_return_val_if_fail (TUMBLER_IS_BUILTIN_THUMBNAILER (thumbnailer), FALSE);
  g_return_val_if_fail (uri != NULL, FALSE);
  g_return_val_if_fail (mime_hint != NULL, FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  /* create the thumbnail info with the URI and modified time */
  info = tumbler_thumbnail_info_new (uri);

  /* create a GFile for this URI */
  file = g_file_new_for_uri (uri);

  /* try to open the source file for reading */
  stream = g_file_read (file, NULL, &err);

  if (stream != NULL)
    {
      /* try to load the source image from the stream */
      source_pixbuf = gdk_pixbuf_new_from_stream (G_INPUT_STREAM (stream), NULL, &err);

      if (source_pixbuf != NULL)
        {
          /* determine flavors we need to (re)generate */
          flavors = tumbler_thumbnail_info_get_invalid_flavors (info, NULL);

          /* iterate over these flavors */
          for (n = 0; err == NULL && flavors[n] != TUMBLER_THUMBNAIL_FLAVOR_INVALID; ++n)
            {
              /* determine the pixel size of the current flavor */
              size = tumbler_thumbnail_info_get_flavor_size (flavors[n]);

              if (flavors[n] == TUMBLER_THUMBNAIL_FLAVOR_CROPPED)
                {
                  /* TODO unsupported */
                }
              else
                {
                  /* determine width and height of the source image */
                  width = gdk_pixbuf_get_width (source_pixbuf);
                  height = gdk_pixbuf_get_height (source_pixbuf);

                  if (width <= size && height <= size)
                    {
                      /* the image is smaller than requested, no resizing required */
                      dest_width = width;
                      dest_height = height;
                    }
                  else
                    {
                      /* the image is larger than the thumbnail should be */
                      if (width > height)
                        {
                          /* width is larger than the height, use size for the width */
                          dest_width = size;
                          
                          /* determine the new height for this width */
                          factor = (gdouble) size / (gdouble) width;
                          dest_height = MIN (round (height * factor), size);
                        }
                      else
                        {
                          /* height is larger than the width, use size for the height */
                          dest_height = size;

                          /* determine the new width for this height */
                          factor = (gdouble) size / (gdouble) height;
                          dest_width = MIN (round (width * factor), size);
                        }
                    }

                  /* scale the pixbuf if necessary */
                  pixbuf = gdk_pixbuf_scale_simple (source_pixbuf, dest_width, 
                                                    dest_height, GDK_INTERP_BILINEAR);

                  /* try to generate the thumbnail flavor */
                  if (!tumbler_thumbnail_info_generate_flavor (info, flavors[n],
                                                               pixbuf, NULL, &err))
                    {
                      /* there was an error, abort */
                      g_object_unref (pixbuf);
                      break;
                    }

                  /* destroy the scaled image */
                  g_object_unref (pixbuf);
                }
            }

          /* destroy the source image */
          g_object_unref (source_pixbuf);
        }

      /* close and destroy the input stream */
      g_object_unref (stream);
    }
  
  /* destroy the GFile */
  g_object_unref (file);

  if (err != NULL)
    {
      g_propagate_error (error, err);

      /* try to generate a fail file */
      tumbler_thumbnail_info_generate_fail (info, NULL);

      g_object_unref (info);
      return FALSE;
    }
  else
    {
      g_object_unref (info);
      return TRUE;
    }
}



TumblerThumbnailer *
tumbler_pixbuf_thumbnailer_new (void)
{
  TumblerThumbnailer *thumbnailer = NULL;
  static const gchar *uri_schemes[] = { "file", "sftp", "http", NULL, };
  GHashTable         *types;
  GSList             *formats;
  GSList             *fp;
  GList              *keys;
  GList              *lp;
  GStrv               format_types;
  GStrv               mime_types;
  gint                n;

  /* create a hash table to collect unique MIME types */
  types = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);

  /* get a list of all formats supported by GdkPixbuf */
  formats = gdk_pixbuf_get_formats ();

  /* iterate over all formats */
  for (fp = formats; fp != NULL; fp = fp->next)
    {
      /* ignore the disabled ones */
      if (!gdk_pixbuf_format_is_disabled (fp->data))
        {
          /* get a list of MIME types supported by this format */
          format_types = gdk_pixbuf_format_get_mime_types (fp->data);

          /* put them all in the unqiue MIME type hash table */
          for (n = 0; format_types != NULL && format_types[n] != NULL; ++n)
            g_hash_table_replace (types, g_strdup (format_types[n]), NULL);

          /* free the string array */
          g_strfreev (format_types);
        }
    }
  
  /* free the format list */
  g_slist_free (formats);

  /* get a list with all unique MIME types */
  keys = g_hash_table_get_keys (types);

  /* allocate a string array for them */
  mime_types = g_new0 (gchar *, g_list_length (keys) + 1);

  /* copy all MIME types into the string array */
  for (lp = keys, n = 0; lp != NULL; lp = lp->next, ++n)
    mime_types[n] = g_strdup (lp->data);

  /* NULL-terminate the array */
  mime_types[n] = NULL;

  /* free the uniqueue MIME types list */
  g_list_free (keys);

  /* destroy the hash table */
  g_hash_table_unref (types);

  /* create the pixbuf thumbnailer */
  thumbnailer = tumbler_builtin_thumbnailer_new (_tumbler_pixbuf_thumbnailer, 
                                                 (const GStrv) mime_types,
                                                 (const GStrv) uri_schemes);

  /* free the mime types array */
  g_strfreev (mime_types);

  return thumbnailer;
}

#endif /* ENABLE_PIXBUF_THUMBNAILER */
