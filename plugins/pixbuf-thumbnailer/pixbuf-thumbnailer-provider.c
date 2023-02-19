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
#include <gdk-pixbuf/gdk-pixbuf.h>

#include <tumbler/tumbler.h>

#include <pixbuf-thumbnailer/pixbuf-thumbnailer-provider.h>
#include <pixbuf-thumbnailer/pixbuf-thumbnailer.h>



static void   pixbuf_thumbnailer_provider_thumbnailer_provider_init (TumblerThumbnailerProviderIface *iface);
static GList *pixbuf_thumbnailer_provider_get_thumbnailers          (TumblerThumbnailerProvider      *provider);



struct _PixbufThumbnailerProvider
{
  GObject __parent__;
};



G_DEFINE_DYNAMIC_TYPE_EXTENDED (PixbufThumbnailerProvider,
                                pixbuf_thumbnailer_provider,
                                G_TYPE_OBJECT,
                                0,
                                G_IMPLEMENT_INTERFACE_DYNAMIC (TUMBLER_TYPE_THUMBNAILER_PROVIDER,
                                                               pixbuf_thumbnailer_provider_thumbnailer_provider_init));



void
pixbuf_thumbnailer_provider_register (TumblerProviderPlugin *plugin)
{
  pixbuf_thumbnailer_provider_register_type (G_TYPE_MODULE (plugin));
}



static void
pixbuf_thumbnailer_provider_class_init (PixbufThumbnailerProviderClass *klass)
{
}



static void
pixbuf_thumbnailer_provider_class_finalize (PixbufThumbnailerProviderClass *klass)
{
}



static void
pixbuf_thumbnailer_provider_thumbnailer_provider_init (TumblerThumbnailerProviderIface *iface)
{
  iface->get_thumbnailers = pixbuf_thumbnailer_provider_get_thumbnailers;
}



static void
pixbuf_thumbnailer_provider_init (PixbufThumbnailerProvider *provider)
{
}



static GList *
pixbuf_thumbnailer_provider_get_thumbnailers (TumblerThumbnailerProvider *provider)
{
  PixbufThumbnailer *thumbnailer;
  GHashTable        *types;
  GSList            *formats;
  GSList            *fp;
  GList             *keys;
  GList             *lp;
  GList             *thumbnailers = NULL;
  GStrv              format_types;
  GStrv              mime_types;
  GStrv              uri_schemes;
  guint              n;

  /* determine which URI schemes are supported by GIO */
  uri_schemes = tumbler_util_get_supported_uri_schemes ();

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

  g_list_free (keys);

  /* create the pixbuf thumbnailer */
  thumbnailer = g_object_new (TYPE_PIXBUF_THUMBNAILER, 
                              "uri-schemes", uri_schemes, "mime-types", mime_types, 
                              NULL);

  tumbler_util_dump_strv (G_LOG_DOMAIN, "Supported URI schemes",
                          (const gchar *const *) uri_schemes);
  tumbler_util_dump_strv (G_LOG_DOMAIN, "Supported mime types",
                          (const gchar *const *) mime_types);

  /* free URI schemes and MIME types */
  g_strfreev (uri_schemes);
  g_strfreev (mime_types);

  g_hash_table_unref (types);

  /* add the thumbnailer to the list */
  thumbnailers = g_list_append (thumbnailers, thumbnailer);

  return thumbnailers;
}
