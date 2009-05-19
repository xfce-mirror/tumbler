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

#include <tumbler/tumbler-builtin-thumbnailer.h>
#include <tumbler/tumbler-thumbnailer.h>



#ifdef HAVE_GDK_PIXBUF

static gboolean
_tumbler_pixbuf_thumbnailer (TumblerBuiltinThumbnailer *thumbnailer,
                             const gchar               *uri,
                             const gchar               *mime_hint,
                             GError                   **error)
{
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
