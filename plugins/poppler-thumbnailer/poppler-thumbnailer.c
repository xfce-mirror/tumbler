/* vi:set et ai sw=2 sts=2 ts=2: */
/*-
 * Copyright (c) 2010 Jannis Pohlmann <jannis@xfce.org>
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
#include <glib/gi18n.h>
#include <glib-object.h>

#include <gdk-pixbuf/gdk-pixbuf.h>

#include <poppler.h>

#include <tumbler/tumbler.h>

#include <poppler-thumbnailer/poppler-thumbnailer.h>



static void poppler_thumbnailer_create (TumblerAbstractThumbnailer *thumbnailer,
                                        GCancellable               *cancellable,
                                        TumblerFileInfo            *info);



struct _PopplerThumbnailerClass
{
  TumblerAbstractThumbnailerClass __parent__;
};

struct _PopplerThumbnailer
{
  TumblerAbstractThumbnailer __parent__;
};



G_DEFINE_DYNAMIC_TYPE (PopplerThumbnailer, 
                       poppler_thumbnailer,
                       TUMBLER_TYPE_ABSTRACT_THUMBNAILER);



void
poppler_thumbnailer_register (TumblerProviderPlugin *plugin)
{
  poppler_thumbnailer_register_type (G_TYPE_MODULE (plugin));
}



static void
poppler_thumbnailer_class_init (PopplerThumbnailerClass *klass)
{
  TumblerAbstractThumbnailerClass *abstractthumbnailer_class;

  abstractthumbnailer_class = TUMBLER_ABSTRACT_THUMBNAILER_CLASS (klass);
  abstractthumbnailer_class->create = poppler_thumbnailer_create;
}



static void
poppler_thumbnailer_class_finalize (PopplerThumbnailerClass *klass)
{
}



static void
poppler_thumbnailer_init (PopplerThumbnailer *thumbnailer)
{
}



static GdkPixbuf *
generate_pixbuf (GdkPixbuf              *source,
                 TumblerThumbnailFlavor *flavor)
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

  /* determine which axis needs to be scaled down more */
  wratio = (gdouble) source_width / (gdouble) dest_width;
  hratio = (gdouble) source_height / (gdouble) dest_height;

  /* adjust the other axis */
  if (hratio > wratio)
    dest_width = rint (source_width / hratio);
  else
    dest_height = rint (source_height / wratio);
  
  /* scale the pixbuf down to the desired size */
  return gdk_pixbuf_scale_simple (source, 
                                  MAX (dest_width, 1), MAX (dest_height, 1), 
                                  GDK_INTERP_BILINEAR);
}



static void
poppler_thumbnailer_create (TumblerAbstractThumbnailer *thumbnailer,
                            GCancellable               *cancellable,
                            TumblerFileInfo            *info)
{
  TumblerThumbnailFlavor *flavor;
  TumblerImageData        data;
  TumblerThumbnail       *thumbnail;
  PopplerDocument        *document;
  PopplerPage            *page;
  const gchar            *uri;
  GdkPixbuf              *source_pixbuf;
  GdkPixbuf              *pixbuf;
  GError                 *error = NULL;
  gdouble                 page_width;
  gdouble                 page_height;
  GFile                  *file;
  gchar                  *contents = NULL;
  gsize                   length;

  g_return_if_fail (IS_POPPLER_THUMBNAILER (thumbnailer));
  g_return_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable));
  g_return_if_fail (TUMBLER_IS_FILE_INFO (info));

  /* do nothing if cancelled */
  if (g_cancellable_is_cancelled (cancellable)) 
    return;

  /* try to load the PDF/PS file based on the URI */
  uri = tumbler_file_info_get_uri (info);
  document = poppler_document_new_from_file (uri, NULL, &error);

  /* check if that failed */
  if (document == NULL)
    {
      /* make sure to free error data */
      g_clear_error (&error);

      file = g_file_new_for_uri (uri);

      /* try to load the file contents using GIO */
      if (!g_file_load_contents (file, cancellable, &contents, &length, NULL, &error))
        {
          g_signal_emit_by_name (thumbnailer, "error", uri, TUMBLER_ERROR_UNSUPPORTED, 
                                 error->message);
          g_error_free (error);
          g_object_unref (file);
          return;
        }

      /* release the file */
      g_object_unref (file);

      /* try to create a poppler document based on the file contents */
      document = poppler_document_new_from_data (contents, length, NULL, &error);
    }

  /* emit an error if both ways to load the document failed */
  if (document == NULL)
    {
      g_signal_emit_by_name (thumbnailer, "error", uri, TUMBLER_ERROR_INVALID_FORMAT, 
                             error->message);
      g_error_free (error);
      g_free (contents);
      return;
    }

  /* check if the document has content (= at least one page) */
  if (poppler_document_get_n_pages (document) <= 0)
    {
      g_signal_emit_by_name (thumbnailer, "error", uri, TUMBLER_ERROR_NO_CONTENT, 
                             _("The document is empty"));
      g_object_unref (document);
      g_free (contents);
      return;
    }

  /* get the first page of the document */
  page = poppler_document_get_page (document, 0);

  if (page == NULL)
    {
      g_signal_emit_by_name (thumbnailer, "error", uri, TUMBLER_ERROR_NO_CONTENT,
                             _("First page of the document could not be read"));
      g_object_unref (document);
      g_free (contents);
      return;
    }

  thumbnail = tumbler_file_info_get_thumbnail (info);
  g_assert (thumbnail != NULL);

  /* generate a pixbuf for the thumbnail */
  flavor = tumbler_thumbnail_get_flavor (thumbnail);

  /* try to extract the embedded thumbnail */
  source_pixbuf = poppler_page_get_thumbnail_pixbuf (page);

  if (source_pixbuf == NULL)
    {
      /* fall back to rendering the page ourselves */
      poppler_page_get_size (page, &page_width, &page_height);
      source_pixbuf = gdk_pixbuf_new (GDK_COLORSPACE_RGB, TRUE, 8, page_width, page_height);
      poppler_page_render_to_pixbuf (page, 0, 0, page_width, page_height, 1.0, 0, source_pixbuf);
    }

  /* release allocated poppler data */
  g_object_unref (page);
  g_object_unref (document);

  /* generate the final pixbuf (involves rescaling etc.) */
  pixbuf = generate_pixbuf (source_pixbuf, flavor);
  g_object_unref (flavor);

  g_assert (pixbuf != NULL);

  data.data = gdk_pixbuf_get_pixels (pixbuf);
  data.has_alpha = gdk_pixbuf_get_has_alpha (pixbuf);
  data.bits_per_sample = gdk_pixbuf_get_bits_per_sample (pixbuf);
  data.width = gdk_pixbuf_get_width (pixbuf);
  data.height = gdk_pixbuf_get_height (pixbuf);
  data.rowstride = gdk_pixbuf_get_rowstride (pixbuf);
  data.colorspace = (TumblerColorspace) gdk_pixbuf_get_colorspace (pixbuf);

  tumbler_thumbnail_save_image_data (thumbnail, &data, 
                                     tumbler_file_info_get_mtime (info), 
                                     NULL, &error);

  if (error != NULL)
    {
      g_signal_emit_by_name (thumbnailer, "error", uri, error->code, error->message);
      g_error_free (error);
    }
  else
    {
      g_signal_emit_by_name (thumbnailer, "ready", uri);
    }


  g_object_unref (thumbnail);
  g_object_unref (pixbuf);
  g_object_unref (source_pixbuf);
  g_free (contents);
}
