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

#include <math.h>

#include <glib.h>
#include <glib-object.h>

#include <gdk-pixbuf/gdk-pixbuf.h>

#include <tumbler/tumbler.h>

#include <pixbuf-thumbnailer/pixbuf-thumbnailer.h>



static void pixbuf_thumbnailer_create (TumblerAbstractThumbnailer *thumbnailer,
                                       GCancellable               *cancellable,
                                       const gchar                *uri,
                                       const gchar                *mime_hint);



struct _PixbufThumbnailerClass
{
  TumblerAbstractThumbnailerClass __parent__;
};

struct _PixbufThumbnailer
{
  TumblerAbstractThumbnailer __parent__;
};



G_DEFINE_DYNAMIC_TYPE (PixbufThumbnailer, 
                       pixbuf_thumbnailer,
                       TUMBLER_TYPE_ABSTRACT_THUMBNAILER);



void
pixbuf_thumbnailer_register (TumblerProviderPlugin *plugin)
{
  pixbuf_thumbnailer_register_type (G_TYPE_MODULE (plugin));
}



static void
pixbuf_thumbnailer_class_init (PixbufThumbnailerClass *klass)
{
  TumblerAbstractThumbnailerClass *abstractthumbnailer_class;

  abstractthumbnailer_class = TUMBLER_ABSTRACT_THUMBNAILER_CLASS (klass);
  abstractthumbnailer_class->create = pixbuf_thumbnailer_create;
}



static void
pixbuf_thumbnailer_class_finalize (PixbufThumbnailerClass *klass)
{
}



static void
pixbuf_thumbnailer_init (PixbufThumbnailer *thumbnailer)
{
}



static GdkPixbuf *
generate_pixbuf (GdkPixbuf              *source,
                 TumblerThumbnailFlavor  flavor)
{
  gdouble    hratio;
  gdouble    wratio;
  gint       dest_width;
  gint       dest_height;
  gint       source_width;
  gint       source_height;

  /* determine the source pixbuf dimensions */
  source_width = gdk_pixbuf_get_width (source);
  source_height = gdk_pixbuf_get_height (source);

  /* determine the desired size for this flavor */
  tumbler_thumbnail_flavor_get_size (flavor, &dest_width, &dest_height);

  /* return the same pixbuf if no scaling is required */
  if (source_width <= dest_width && source_height <= dest_height)
    return g_object_ref (source);

  if (flavor == TUMBLER_THUMBNAIL_FLAVOR_CROPPED)
    {
      /* TODO unsupported */
    }
  else
    {
      /* determine which axis needs to be scaled down more */
      wratio = (gdouble) source_width / (gdouble) dest_width;
      hratio = (gdouble) source_height / (gdouble) dest_height;

      /* adjust the other axis */
      if (hratio > wratio)
        dest_width = rint (source_width / hratio);
      else
        dest_height = rint (source_height / wratio);
    }

  /* scale the pixbuf down to the desired size */
  return gdk_pixbuf_scale_simple (source, MAX (dest_width, 1), MAX (dest_height, 1), 
                                  GDK_INTERP_BILINEAR);
}



static void
pixbuf_thumbnailer_create (TumblerAbstractThumbnailer *thumbnailer,
                           GCancellable               *cancellable,
                           const gchar                *uri,
                           const gchar                *mime_hint)
{
  TumblerThumbnailFlavor *flavors;
  TumblerThumbnailFlavor  flavor;
  GFileInputStream       *stream;
  TumblerFileInfo        *info;
  GHashTable             *pixbufs;
  GdkPixbuf              *source_pixbuf;
  GdkPixbuf              *pixbuf;
  guint64                 mtime;
  GError                 *error = NULL;
  GFile                  *file;
  GList                  *lp;
  GList                  *thumbnails;
  guint                   n;

  g_return_if_fail (IS_PIXBUF_THUMBNAILER (thumbnailer));
  g_return_if_fail (uri != NULL && *uri != '\0');

  /* do nothing if cancelled */
  if (g_cancellable_is_cancelled (cancellable)) 
    return;
 
  /* create the file info for this URI */
  info = tumbler_file_info_new (uri);

  /* try to load the file information */
  if (!tumbler_file_info_load (info, NULL, &error))
    {
      g_signal_emit_by_name (thumbnailer, "error", uri, error->code, error->message);
      g_error_free (error);
      g_object_unref (info);
      return;
    }

  /* try to open the source file for reading */
  file = g_file_new_for_uri (uri);
  stream = g_file_read (file, NULL, &error);
  g_object_unref (file);

  if (stream == NULL)
    {
      g_signal_emit_by_name (thumbnailer, "error", uri, error->code, error->message);
      g_error_free (error);
      g_object_unref (info);
      return;
    }

  source_pixbuf = gdk_pixbuf_new_from_stream (G_INPUT_STREAM (stream), 
                                              cancellable, &error);
  g_object_unref (stream);

  if (source_pixbuf == NULL)
    {
      g_signal_emit_by_name (thumbnailer, "error", uri, error->code, error->message);
      g_error_free (error);
      g_object_unref (info);
      return;
    }

  flavors = tumbler_thumbnail_get_flavors ();
  pixbufs = g_hash_table_new_full (g_direct_hash, g_direct_equal, NULL, g_object_unref);

  for (n = 0; flavors[n] != TUMBLER_THUMBNAIL_FLAVOR_INVALID; ++n)
    {
      pixbuf = generate_pixbuf (source_pixbuf, flavors[n]);

      if (pixbuf != NULL)
        g_hash_table_insert (pixbufs, GINT_TO_POINTER (flavors[n]), pixbuf);
    }

  mtime = tumbler_file_info_get_mtime (info);

  thumbnails = tumbler_file_info_get_thumbnails (info);

  for (lp = thumbnails; error == NULL && lp != NULL; lp = lp->next)
    {
      if (tumbler_thumbnail_load (lp->data, NULL, &error))
        if (tumbler_thumbnail_needs_update (lp->data, uri, mtime))
          {
            flavor = tumbler_thumbnail_get_flavor (lp->data);
            pixbuf = g_hash_table_lookup (pixbufs, GINT_TO_POINTER (flavor));

            if (pixbuf != NULL)
              tumbler_thumbnail_save_pixbuf (lp->data, pixbuf, mtime, 
                                             cancellable, &error);
          }
    }

  if (error != NULL)
    {
      g_signal_emit_by_name (thumbnailer, "error", uri, error->code, error->message);
      g_error_free (error);
    }
  else
    {
      g_signal_emit_by_name (thumbnailer, "ready", uri);
    }

  g_hash_table_unref (pixbufs);
  g_object_unref (source_pixbuf);
  g_object_unref (info);
}
