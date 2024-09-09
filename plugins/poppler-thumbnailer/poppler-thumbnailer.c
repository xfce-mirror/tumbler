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
#include "config.h"
#endif

#include "poppler-thumbnailer.h"

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <glib-object.h>
#include <glib/gi18n.h>
#include <poppler.h>



static void
poppler_thumbnailer_create (TumblerAbstractThumbnailer *thumbnailer,
                            GCancellable *cancellable,
                            TumblerFileInfo *info);



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
poppler_thumbnailer_pixbuf_from_surface (cairo_surface_t *surface)
{
#if 0
  return gdk_pixbuf_get_from_surface (surface,
                                      0, 0,
                                      cairo_image_surface_get_width (surface),
                                      cairo_image_surface_get_height (surface));
#else
  GdkPixbuf *pixbuf;
  cairo_surface_t *image;
  cairo_t *cr;
  gboolean has_alpha;
  gint width, height;
  cairo_format_t surface_format;
  gint pixbuf_n_channels;
  gint pixbuf_rowstride;
  guchar *pixbuf_pixels;
  gint x, y;
  guchar *p;
  guchar tmp;

  width = cairo_image_surface_get_width (surface);
  height = cairo_image_surface_get_height (surface);

  surface_format = cairo_image_surface_get_format (surface);
  has_alpha = (surface_format == CAIRO_FORMAT_ARGB32);

  pixbuf = gdk_pixbuf_new (GDK_COLORSPACE_RGB, TRUE, 8, width, height);
  pixbuf_n_channels = gdk_pixbuf_get_n_channels (pixbuf);
  pixbuf_rowstride = gdk_pixbuf_get_rowstride (pixbuf);
  pixbuf_pixels = gdk_pixbuf_get_pixels (pixbuf);

  image = cairo_image_surface_create_for_data (pixbuf_pixels,
                                               surface_format,
                                               width, height,
                                               pixbuf_rowstride);
  cr = cairo_create (image);
  cairo_set_source_surface (cr, surface, 0, 0);

  if (has_alpha)
    cairo_mask_surface (cr, surface, 0, 0);
  else
    cairo_paint (cr);

  cairo_destroy (cr);
  cairo_surface_destroy (image);

  for (y = 0; y < height; y++)
    {
      p = pixbuf_pixels + y * pixbuf_rowstride;

      for (x = 0; x < width; x++)
        {
#if G_BYTE_ORDER == G_LITTLE_ENDIAN
          tmp = p[0];
          p[0] = p[2];
          p[2] = tmp;
          p[3] = has_alpha ? p[3] : 0xff;
#else
          tmp = p[0];
          p[0] = p[1];
          p[1] = p[2];
          p[2] = p[3];
          p[3] = has_alpha ? tmp : 0xff;
#endif
          p += pixbuf_n_channels;
        }
    }

  return pixbuf;
#endif
}



static GdkPixbuf *
poppler_thumbnailer_pixbuf_from_page (PopplerPage *page)
{
  cairo_surface_t *surface;
  cairo_t *cr;
  GdkPixbuf *pixbuf;
  gdouble width, height;

  /* get the page size */
  poppler_page_get_size (page, &width, &height);

  surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, width, height);
  cr = cairo_create (surface);

  cairo_save (cr);
  poppler_page_render (page, cr);
  cairo_restore (cr);

  cairo_set_operator (cr, CAIRO_OPERATOR_DEST_OVER);
  cairo_set_source_rgb (cr, 1.0, 1.0, 1.0);
  cairo_paint (cr);

  cairo_destroy (cr);

  pixbuf = poppler_thumbnailer_pixbuf_from_surface (surface);

  cairo_surface_destroy (surface);

  return pixbuf;
}



/* to be removed when Poppler version >= 0.82: GBytes takes care of that */
static void
poppler_thumbnailer_free (gpointer data)
{
#if !POPPLER_CHECK_VERSION(0, 82, 0)
  g_free (data);
#endif
}



static void
poppler_thumbnailer_create (TumblerAbstractThumbnailer *thumbnailer,
                            GCancellable *cancellable,
                            TumblerFileInfo *info)
{
  TumblerThumbnailFlavor *flavor;
  TumblerImageData data;
  TumblerThumbnail *thumbnail;
  PopplerDocument *document;
  PopplerPage *page;
  const gchar *uri;
  cairo_surface_t *surface;
  GdkPixbuf *source_pixbuf;
  GdkPixbuf *pixbuf;
  GError *error = NULL;
  GFile *file;
#if POPPLER_CHECK_VERSION(0, 82, 0)
  GBytes *bytes;
#endif
  gchar *contents = NULL;
  gsize length;
  gint width, height;

  g_return_if_fail (POPPLER_IS_THUMBNAILER (thumbnailer));
  g_return_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable));
  g_return_if_fail (TUMBLER_IS_FILE_INFO (info));

  /* do nothing if cancelled */
  if (g_cancellable_is_cancelled (cancellable))
    return;

  uri = tumbler_file_info_get_uri (info);
  g_debug ("Handling URI '%s'", uri);

  /* try to load the PDF/PS file based on the URI */
  document = poppler_document_new_from_file (uri, NULL, &error);
  if (document == NULL)
    {
      /* make sure to free error data */
      g_clear_error (&error);

      file = g_file_new_for_uri (uri);

      /* try to load the file contents using GIO */
      if (!g_file_load_contents (file, cancellable, &contents, &length, NULL, &error))
        {
          g_signal_emit_by_name (thumbnailer, "error", info,
                                 error->domain, error->code, error->message);
          g_error_free (error);
          g_object_unref (file);
          return;
        }

      /* release the file */
      g_object_unref (file);

      /* try to create a poppler document based on the file contents */
#if POPPLER_CHECK_VERSION(0, 82, 0)
      bytes = g_bytes_new_take (contents, length);
      document = poppler_document_new_from_bytes (bytes, NULL, &error);
      g_bytes_unref (bytes);
#else
      document = poppler_document_new_from_data (contents, length, NULL, &error);
#endif
    }

  /* emit an error if both ways to load the document failed */
  if (document == NULL)
    {
      g_signal_emit_by_name (thumbnailer, "error", info,
                             error->domain, error->code, error->message);
      g_error_free (error);
      poppler_thumbnailer_free (contents);
      return;
    }

  /* check if the document has content (= at least one page) */
  if (poppler_document_get_n_pages (document) <= 0)
    {
      g_signal_emit_by_name (thumbnailer, "error", info,
                             TUMBLER_ERROR, TUMBLER_ERROR_NO_CONTENT,
                             _("The document is empty"));
      g_object_unref (document);
      poppler_thumbnailer_free (contents);
      return;
    }

  /* get the first page of the document */
  page = poppler_document_get_page (document, 0);

  if (page == NULL)
    {
      g_signal_emit_by_name (thumbnailer, "error", info,
                             TUMBLER_ERROR, TUMBLER_ERROR_NO_CONTENT,
                             _("First page of the document could not be read"));
      g_object_unref (document);
      poppler_thumbnailer_free (contents);
      return;
    }

  thumbnail = tumbler_file_info_get_thumbnail (info);
  g_assert (thumbnail != NULL);

  /* generate a pixbuf for the thumbnail */
  flavor = tumbler_thumbnail_get_flavor (thumbnail);

  /* try to extract the embedded thumbnail */
  surface = poppler_page_get_thumbnail (page);
  if (surface != NULL)
    {
      source_pixbuf = poppler_thumbnailer_pixbuf_from_surface (surface);
      cairo_surface_destroy (surface);
    }
  else
    {
      /* fall back to rendering the page ourselves */
      source_pixbuf = poppler_thumbnailer_pixbuf_from_page (page);
    }

  /* release allocated poppler data */
  g_object_unref (page);
  g_object_unref (document);

  /* generate the final pixbuf (involves rescaling etc.) */
  tumbler_thumbnail_flavor_get_size (flavor, &width, &height);
  pixbuf = tumbler_util_scale_pixbuf (source_pixbuf, width, height);
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
      g_signal_emit_by_name (thumbnailer, "error", info,
                             error->domain, error->code, error->message);
      g_error_free (error);
    }
  else
    {
      g_signal_emit_by_name (thumbnailer, "ready", info);
    }


  g_object_unref (thumbnail);
  g_object_unref (pixbuf);
  g_object_unref (source_pixbuf);
  poppler_thumbnailer_free (contents);
}
