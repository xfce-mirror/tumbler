/* vi:set et ai sw=2 sts=2 ts=2: */
/*-
 * Copyright (c) 2011 Nick Schermer <nick@xfce.org>
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
 *
 * Parts are based on the gsf-office-thumbnailer in libgsf, which is
 * written by Federico Mena-Quintero <federico@novell.com>.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <math.h>

#include <glib.h>
#include <glib/gi18n.h>
#include <glib-object.h>

#include <gdk-pixbuf/gdk-pixbuf.h>

#include <gsf/gsf.h>
#include <gsf/gsf-input-memory.h>
#include <gsf/gsf-input-gio.h>
#include <gsf/gsf-infile.h>
#include <gsf/gsf-infile-msole.h>
#include <gsf/gsf-infile-zip.h>
#include <gsf/gsf-open-pkg-utils.h>
#include <gsf/gsf-clip-data.h>
#include <gsf/gsf-doc-meta-data.h>
#include <gsf/gsf-meta-names.h>
#include <gsf/gsf-msole-utils.h>

#include <tumbler/tumbler.h>

#include <odf-thumbnailer/odf-thumbnailer.h>


#define OPEN_XML_SCHEMA "http://schemas.openxmlformats.org/package/2006/relationships/metadata/thumbnail"



static void odf_thumbnailer_finalize (GObject                    *object);
static void odf_thumbnailer_create   (TumblerAbstractThumbnailer *thumbnailer,
                                       GCancellable               *cancellable,
                                       TumblerFileInfo            *info);



struct _OdfThumbnailerClass
{
  TumblerAbstractThumbnailerClass __parent__;
};

struct _OdfThumbnailer
{
  TumblerAbstractThumbnailer __parent__;
};



G_DEFINE_DYNAMIC_TYPE (OdfThumbnailer,
                       odf_thumbnailer,
                       TUMBLER_TYPE_ABSTRACT_THUMBNAILER);



void
odf_thumbnailer_register (TumblerProviderPlugin *plugin)
{
  odf_thumbnailer_register_type (G_TYPE_MODULE (plugin));
}



static void
odf_thumbnailer_class_init (OdfThumbnailerClass *klass)
{
  TumblerAbstractThumbnailerClass *abstractthumbnailer_class;
  GObjectClass                    *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->finalize = odf_thumbnailer_finalize;

  abstractthumbnailer_class = TUMBLER_ABSTRACT_THUMBNAILER_CLASS (klass);
  abstractthumbnailer_class->create = odf_thumbnailer_create;
}



static void
odf_thumbnailer_class_finalize (OdfThumbnailerClass *klass)
{
}



static void
odf_thumbnailer_init (OdfThumbnailer *thumbnailer)
{

}


static void
odf_thumbnailer_finalize (GObject *object)
{
  //OdfThumbnailer *thumbnailer = ODF_THUMBNAILER (object);

  (*G_OBJECT_CLASS (odf_thumbnailer_parent_class)->finalize) (object);
}



static GdkPixbuf *
odf_thumbnailer_create_from_data (const guchar *data,
                                  gsize         bytes)
{
  GdkPixbufLoader *loader;
  GdkPixbuf       *pixbuf = NULL;
  GError          *error = NULL;

  loader = gdk_pixbuf_loader_new ();
  if (gdk_pixbuf_loader_write (loader, data, bytes, &error))
    {
      if (gdk_pixbuf_loader_close (loader, &error))
        {
          pixbuf = gdk_pixbuf_loader_get_pixbuf (loader);
          if (pixbuf != NULL)
            g_object_ref (G_OBJECT (pixbuf));
        }
    }
  else
    {
      gdk_pixbuf_loader_close (loader, NULL);
    }
  g_object_unref (G_OBJECT (loader));

  return pixbuf;
}



static GdkPixbuf *
odf_thumbnailer_create_zip (GsfInfile *infile)
{
  GsfInput     *thumbnail;
  gsize         bytes;
  const guint8 *data;
  GdkPixbuf    *pixbuf = NULL;

  g_return_val_if_fail (GSF_IS_INFILE_ZIP (infile), NULL);

  /* openoffice and libreoffice thumbnail */
  thumbnail = gsf_infile_child_by_vname (infile, "Thumbnails", "thumbnail.png", NULL);
  if (thumbnail == NULL)
    {
      /* microsoft office-x thumbnails */
      thumbnail = gsf_open_pkg_open_rel_by_type (GSF_INPUT (infile), OPEN_XML_SCHEMA, NULL);
      if (thumbnail == NULL)
        return NULL;
    }

  /* read data and generate a pixbuf */
  bytes = gsf_input_remaining (thumbnail);
  data = gsf_input_read (thumbnail, bytes, NULL);
  if (data != NULL)
    pixbuf = odf_thumbnailer_create_from_data (data, bytes);

  g_object_unref (G_OBJECT (thumbnail));

  return pixbuf;
}



static GdkPixbuf *
odf_thumbnailer_create_msole (GsfInfile *infile)
{
  GsfInput       *summary;
  GsfDocMetaData *meta_data;
  GError         *error;
  GsfDocProp     *thumbnail;
  GdkPixbuf      *pixbuf = NULL;
  GValue const   *thumb_value;
  GsfClipData    *clip_data;
  gsize           bytes;
  const guchar   *data;

  g_return_val_if_fail (GSF_IS_INFILE_MSOLE (infile), NULL);

  /* try to find summary information stream */
  summary = gsf_infile_child_by_name (infile, "\05SummaryInformation");
  if (summary == NULL)
    return NULL;

  /* read meta data from stream */
  meta_data = gsf_doc_meta_data_new ();
  error = gsf_msole_metadata_read (summary, meta_data);
  g_object_unref (G_OBJECT (summary));
  if (error != NULL)
    {
      g_error_free (error);
      return NULL;
    }

  /* try to extract thumbnail */
  thumbnail = gsf_doc_meta_data_lookup (meta_data, GSF_META_NAME_THUMBNAIL);
  if (thumbnail != NULL)
    {
      thumb_value = gsf_doc_prop_get_val (thumbnail);
      if (thumb_value != NULL)
        {
          clip_data = g_value_get_object (thumb_value);

          /* only create thumbs from data we can handle */
          if (gsf_clip_data_get_format (clip_data) != GSF_CLIP_FORMAT_WINDOWS_CLIPBOARD
              && gsf_clip_data_get_windows_clipboard_format (clip_data, NULL) != GSF_CLIP_FORMAT_WINDOWS_ERROR)
            {
              data = gsf_clip_data_peek_real_data (GSF_CLIP_DATA (clip_data), &bytes, NULL);
              if (data != NULL)
                pixbuf = odf_thumbnailer_create_from_data (data, bytes);
            }
        }
    }

  g_object_unref (G_OBJECT (meta_data));

  return pixbuf;
}



static GdkPixbuf*
scale_pixbuf (GdkPixbuf *source,
              gint       dest_width,
              gint       dest_height)
{
  gdouble wratio;
  gdouble hratio;
  gint    source_width;
  gint    source_height;

  /* determine source pixbuf dimensions */
  source_width  = gdk_pixbuf_get_width  (source);
  source_height = gdk_pixbuf_get_height (source);

  /* don't do anything if there is no need to resize */
  if (source_width <= dest_width && source_height <= dest_height)
    return g_object_ref (source);

  /* determine which axis needs to be scaled down more */
  wratio = (gdouble) source_width  / (gdouble) dest_width;
  hratio = (gdouble) source_height / (gdouble) dest_height;

  /* adjust the other axis */
  if (hratio > wratio)
    dest_width = rint (source_width / hratio);
  else
    dest_height = rint (source_height / wratio);

  /* scale the pixbuf down to the desired size */
  return gdk_pixbuf_scale_simple (source, MAX (dest_width, 1),
                                  MAX (dest_height, 1),
                                  GDK_INTERP_BILINEAR);
}



static void
odf_thumbnailer_create (TumblerAbstractThumbnailer *thumbnailer,
                        GCancellable               *cancellable,
                        TumblerFileInfo            *info)
{
  GsfInput               *input = NULL;
  TumblerThumbnail       *thumbnail;
  const gchar            *uri;
  GFile                  *file;
  GError                 *error = NULL;
  gchar                  *path;
  GsfInfile              *infile;
  GdkPixbuf              *pixbuf = NULL;
  TumblerThumbnailFlavor *flavor;
  gint                    width, height;
  TumblerImageData        data;
  GdkPixbuf              *scaled;

  g_return_if_fail (IS_ODF_THUMBNAILER (thumbnailer));
  g_return_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable));
  g_return_if_fail (TUMBLER_IS_FILE_INFO (info));

  /* do nothing if cancelled */
  if (g_cancellable_is_cancelled (cancellable))
    return;

  uri = tumbler_file_info_get_uri (info);
  file = g_file_new_for_uri (uri);

  if (g_file_is_native (file))
    {
      /* try to mmap the file */
      path = g_file_get_path (file);
      input = gsf_input_mmap_new (path, NULL);
      g_free (path);
    }

  if (input == NULL)
    {
      /* fall-back to normal file reading */
      input = gsf_input_gio_new (file, &error);
      if (G_UNLIKELY (input == NULL))
        {
          g_signal_emit_by_name (thumbnailer, "error", uri, error->code, error->message);
          g_error_free (error);
          return;
        }
    }

  /* extract the file */
  input = gsf_input_uncompress (input);

  /* try to detect file format and run thumbnail extractor */
  infile = gsf_infile_zip_new (input, NULL);
  if (infile != NULL)
    {
      pixbuf = odf_thumbnailer_create_zip (infile);
      g_object_unref (G_OBJECT (infile));
    }
  else
    {
      infile = gsf_infile_msole_new (input, NULL);
      if (infile != NULL)
        {
          pixbuf = odf_thumbnailer_create_msole (infile);
          g_object_unref (G_OBJECT (infile));
        }
    }

  if (pixbuf != NULL)
    {
      /* get thumbnail size */
      thumbnail = tumbler_file_info_get_thumbnail (info);
      flavor = tumbler_thumbnail_get_flavor (thumbnail);
      tumbler_thumbnail_flavor_get_size (flavor, &width, &height);
      g_object_unref (G_OBJECT (flavor));
      g_object_unref (G_OBJECT (thumbnail));

      /* scale thumb if required */
      scaled = scale_pixbuf (pixbuf, width, height);
      g_object_unref (G_OBJECT (pixbuf));

      data.data = gdk_pixbuf_get_pixels (scaled);
      data.has_alpha = gdk_pixbuf_get_has_alpha (scaled);
      data.bits_per_sample = gdk_pixbuf_get_bits_per_sample (scaled);
      data.width = gdk_pixbuf_get_width (scaled);
      data.height = gdk_pixbuf_get_height (scaled);
      data.rowstride = gdk_pixbuf_get_rowstride (scaled);
      data.colorspace = (TumblerColorspace) gdk_pixbuf_get_colorspace (scaled);

      tumbler_thumbnail_save_image_data (thumbnail, &data,
                                         tumbler_file_info_get_mtime (info),
                                         NULL, &error);

      g_object_unref (G_OBJECT (scaled));
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

  g_object_unref (G_OBJECT (input));
}
