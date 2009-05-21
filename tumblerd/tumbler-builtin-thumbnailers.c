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

#ifdef HAVE_GDK_PIXBUF
#include <gdk-pixbuf/gdk-pixbuf.h>
#endif

#include <gio/gio.h>

#include <tumbler/tumbler.h>

#include <tumblerd/tumbler-builtin-thumbnailer.h>
#include <tumblerd/tumbler-thumbnailer.h>



#ifdef HAVE_GDK_PIXBUF

static gboolean
_tumbler_pixbuf_thumbnailer (TumblerBuiltinThumbnailer *thumbnailer,
                             const gchar               *uri,
                             const gchar               *mime_hint,
                             GError                   **error)
{
  TumblerThumbnailFlavor *flavors;
  GFileOutputStream      *output_stream;
  GFileInputStream       *input_stream;
  GdkPixbuf              *pixbuf = NULL;
  GError                 *err = NULL;
  GFile                  *input_file;
  GFile                  *output_file;
  gchar                  *basename;
  gchar                  *filename;
  gint                    size;
  gint                    n;

  g_return_val_if_fail (TUMBLER_IS_BUILTIN_THUMBNAILER (thumbnailer), FALSE);
  g_return_val_if_fail (uri != NULL, FALSE);
  g_return_val_if_fail (mime_hint != NULL, FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  /* try to open the file for reading */
  input_file = g_file_new_for_uri (uri);
  input_stream = g_file_read (input_file, NULL, &err);
  g_object_unref (input_file);

  /* propagate error if opening failed */
  if (err != NULL)
    {
      g_propagate_error (error, err);
      return FALSE;
    }

  flavors = tumbler_thumbnail_get_flavors ();

  for (n = 0; flavors[n] != TUMBLER_THUMBNAIL_FLAVOR_INVALID; ++n)
    {
      /* determine the thumbnail file */
      output_file = tumbler_thumbnail_get_file (uri, flavors[n]);
      
      /* skip the file if the thumbnail already exists */
      if (g_file_query_exists (output_file, NULL))
        {
          g_object_unref (output_file);
          continue;
        }

      size = tumbler_thumbnail_flavor_get_size (flavors[n]);

      /* try to load the pixbuf from the file */
      pixbuf = gdk_pixbuf_new_from_stream_at_scale (G_INPUT_STREAM (input_stream), 
                                                    size, size, TRUE, NULL, &err);

      /* propagate error if loading failed */
      if (err != NULL)
        {
          g_propagate_error (error, err);
          g_object_unref (input_stream);
          g_object_unref (output_file);
          return FALSE;
        }

      /* try to reset the stream */
      if (!g_seekable_seek (G_SEEKABLE (input_stream), 0, G_SEEK_SET, NULL, &err))
        {
          g_propagate_error (error, err);
          g_object_unref (input_stream);
          g_object_unref (output_file);
          g_object_unref (pixbuf);
          return FALSE;
        }

      /* apply optional orientation */
      pixbuf = gdk_pixbuf_apply_embedded_orientation (pixbuf);

      /* try to create and open the file to write the thumbnail to */
      output_stream = tumbler_thumbnail_create_and_open_file (output_file, &err);
      g_object_unref (output_file);

      /* propagate error if preparing for writing failed */
      if (err != NULL)
        {
          g_propagate_error (error, err);
          g_object_unref (input_stream);
          g_object_unref (pixbuf);
          return FALSE;
        }

      /* write the pixbuf into the file */
      gdk_pixbuf_save_to_stream (pixbuf, G_OUTPUT_STREAM (output_stream), "png", NULL, &err,
                                 NULL);
      g_object_unref (output_stream);

      /* propagate error if writing failed */
      if (err != NULL)
        {
          g_propagate_error (error, err);
          g_object_unref (input_stream);
          g_object_unref (pixbuf);
          return FALSE;
        }

      /* destroy the pixbuf */
      g_object_unref (pixbuf);
    }

  return TRUE;
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

#endif /* HAVE_GDK_PIXBUF */
