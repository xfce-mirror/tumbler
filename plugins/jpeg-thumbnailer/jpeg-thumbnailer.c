/* vi:set et ai sw=2 sts=2 ts=2: */
/*-
 * Copyright (c) 2005-2007 Benedikt Meurer <benny@xfce.org>
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
 *
 * Based on code written by Alexander Larsson <alexl@redhat.com>
 * for libgnomeui.
 */

#include "jpeg-thumbnailer.h"

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <glib-object.h>
#include <glib/gi18n.h>
#include <jpeglib.h>

#ifdef HAVE_SYS_MMAN_H
#include <sys/mman.h>
#endif
#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif
#ifdef HAVE_MEMORY_H
#include <memory.h>
#endif
#ifdef HAVE_SETJMP_H
#include <setjmp.h>
#endif
#ifdef HAVE_STDIO_H
#include <stdio.h>
#endif
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif



static void
fatal_error_handler (j_common_ptr cinfo) G_GNUC_NORETURN;
static void
tvtj_free (guchar *pixels,
           gpointer data);
static void
jpeg_thumbnailer_create (TumblerAbstractThumbnailer *thumbnailer,
                         GCancellable *cancellable,
                         TumblerFileInfo *info);



struct _JPEGThumbnailer
{
  TumblerAbstractThumbnailer __parent__;
};



G_DEFINE_DYNAMIC_TYPE (JPEGThumbnailer,
                       jpeg_thumbnailer,
                       TUMBLER_TYPE_ABSTRACT_THUMBNAILER);



void
jpeg_thumbnailer_register (TumblerProviderPlugin *plugin)
{
  jpeg_thumbnailer_register_type (G_TYPE_MODULE (plugin));
}



static void
jpeg_thumbnailer_class_init (JPEGThumbnailerClass *klass)
{
  TumblerAbstractThumbnailerClass *abstractthumbnailer_class;

  abstractthumbnailer_class = TUMBLER_ABSTRACT_THUMBNAILER_CLASS (klass);
  abstractthumbnailer_class->create = jpeg_thumbnailer_create;
}



static void
jpeg_thumbnailer_class_finalize (JPEGThumbnailerClass *klass)
{
}



static void
jpeg_thumbnailer_init (JPEGThumbnailer *thumbnailer)
{
}



static void
tvtj_noop (void)
{
}



typedef struct
{
  struct jpeg_error_mgr mgr;
  jmp_buf setjmp_buffer;
} TvtjErrorHandler;



static void
fatal_error_handler (j_common_ptr cinfo)
{
  TvtjErrorHandler *handler = (TvtjErrorHandler *) cinfo->err;
  longjmp (handler->setjmp_buffer, 1);
}



static void
tvtj_free (guchar *pixels,
           gpointer data)
{
  g_free (pixels);
}



static gboolean
tvtj_fill_input_buffer (j_decompress_ptr cinfo)
{
  struct jpeg_source_mgr *source = cinfo->src;

  /* return a fake EOI marker so we will eventually terminate */
  if (G_LIKELY (source->bytes_in_buffer == 0))
    {
      static const JOCTET FAKE_EOI[2] = {
        (JOCTET) 0xff,
        (JOCTET) JPEG_EOI,
      };

      source->next_input_byte = FAKE_EOI;
      source->bytes_in_buffer = G_N_ELEMENTS (FAKE_EOI);
    }

  return TRUE;
}



static void
tvtj_skip_input_data (j_decompress_ptr cinfo,
                      glong num_bytes)
{
  struct jpeg_source_mgr *source = cinfo->src;

  if (G_LIKELY (num_bytes > 0))
    {
      num_bytes = MIN (num_bytes, (glong) source->bytes_in_buffer);

      source->next_input_byte += num_bytes;
      source->bytes_in_buffer -= num_bytes;
    }
}



static inline gint
tvtj_denom (gint width,
            gint height,
            gint size)
{
  if (width > size * 8 && height > size * 8)
    return 8;
  else if (width > size * 4 && height > size * 4)
    return 4;
  else if (width > size * 2 && height > size * 2)
    return 2;
  else
    return 1;
}



static inline void
tvtj_convert_cmyk_to_rgb (j_decompress_ptr cinfo,
                          guchar *line)
{
  guchar *p;
  gint c, k, m, n, y;

  g_return_if_fail (cinfo != NULL);
  g_return_if_fail (cinfo->output_components == 4);
  g_return_if_fail (cinfo->out_color_space == JCS_CMYK);

  for (n = cinfo->output_width, p = line; n > 0; --n, p += 4)
    {
      c = p[0];
      m = p[1];
      y = p[2];
      k = p[3];

      if (cinfo->saw_Adobe_marker)
        {
          p[0] = k * c / 255;
          p[1] = k * m / 255;
          p[2] = k * y / 255;
        }
      else
        {
          p[0] = (255 - k) * (255 - c) / 255;
          p[1] = (255 - k) * (255 - m) / 255;
          p[2] = (255 - k) * (255 - y) / 255;
        }

      p[3] = 255;
    }
}



static GdkPixbuf *
tvtj_jpeg_load (const JOCTET *content,
                gsize length,
                gint size)
{
  struct jpeg_decompress_struct cinfo;
  struct jpeg_source_mgr source;
  TvtjErrorHandler handler;
  guchar *lines[1];
  guchar *buffer = NULL;
  guchar *pixels = NULL;
  guchar *p;
  gint out_num_components;
  guint n;

  /* setup JPEG error handling */
  cinfo.err = jpeg_std_error (&handler.mgr);
  handler.mgr.error_exit = fatal_error_handler;
  handler.mgr.output_message = (gpointer) tvtj_noop;
  if (setjmp (handler.setjmp_buffer))
    goto error;

  /* setup the source */
  source.bytes_in_buffer = length;
  source.next_input_byte = content;
  source.init_source = (gpointer) tvtj_noop;
  source.fill_input_buffer = tvtj_fill_input_buffer;
  source.skip_input_data = tvtj_skip_input_data;
  source.resync_to_restart = jpeg_resync_to_restart;
  source.term_source = (gpointer) tvtj_noop;

  /* setup the JPEG decompress struct */
  jpeg_create_decompress (&cinfo);
  cinfo.src = &source;

  /* read the JPEG header from the file */
  jpeg_read_header (&cinfo, TRUE);

  /* configure the JPEG decompress struct */
  cinfo.scale_num = 1;
  cinfo.scale_denom = tvtj_denom (cinfo.image_width, cinfo.image_height, size);
  cinfo.dct_method = JDCT_FASTEST;
  cinfo.do_fancy_upsampling = FALSE;

  /* calculate the output dimensions */
  jpeg_calc_output_dimensions (&cinfo);

  /* verify the JPEG color space */
  if (cinfo.out_color_space != JCS_GRAYSCALE
      && cinfo.out_color_space != JCS_CMYK
      && cinfo.out_color_space != JCS_RGB)
    {
      /* we don't support this color space */
      goto error;
    }

  /* start the decompression */
  jpeg_start_decompress (&cinfo);

  /* allocate the pixel buffer and extra space for grayscale data */
  if (G_LIKELY (cinfo.num_components != 1))
    {
      pixels = g_malloc (cinfo.output_width * cinfo.output_height * cinfo.num_components);
      buffer = NULL;
      out_num_components = cinfo.num_components;
      lines[0] = pixels;
    }
  else
    {
      pixels = g_malloc (cinfo.output_width * cinfo.output_height * 3);
      buffer = g_malloc (cinfo.output_width);
      out_num_components = 3;
      lines[0] = buffer;
    }

  /* process the JPEG data */
  for (p = pixels; cinfo.output_scanline < cinfo.output_height;)
    {
      jpeg_read_scanlines (&cinfo, lines, 1);

      /* convert the data to RGB */
      if (cinfo.num_components == 1 && buffer != NULL)
        {
          for (n = 0; n < cinfo.output_width; ++n)
            {
              p[n * 3 + 0] = buffer[n];
              p[n * 3 + 1] = buffer[n];
              p[n * 3 + 2] = buffer[n];
            }
          p += cinfo.output_width * 3;
        }
      else
        {
          if (cinfo.out_color_space == JCS_CMYK)
            tvtj_convert_cmyk_to_rgb (&cinfo, lines[0]);
          lines[0] += cinfo.output_width * cinfo.num_components;
        }
    }

  /* release the grayscale buffer */
  g_free (buffer);
  buffer = NULL;

  /* finish the JPEG decompression */
  jpeg_finish_decompress (&cinfo);
  jpeg_destroy_decompress (&cinfo);

  /* generate a pixbuf for the pixel data */
  return gdk_pixbuf_new_from_data (pixels, GDK_COLORSPACE_RGB,
                                   (cinfo.out_color_components == 4), 8,
                                   cinfo.output_width, cinfo.output_height,
                                   cinfo.output_width * out_num_components,
                                   tvtj_free, NULL);

error:
  jpeg_destroy_decompress (&cinfo);
  g_free (buffer);
  g_free (pixels);
  return NULL;
}



typedef struct
{
  const guchar *data_ptr;
  guint data_len;

  guint thumb_compression;

  struct /* thumbnail JPEG */
  {
    guint length;
    guint offset;
    guint orientation;
  } thumb_jpeg;
  struct /* thumbnail TIFF */
  {
    guint length;
    guint offset;
    guint interp;
    guint height;
    guint width;
  } thumb_tiff;

  gboolean big_endian;
} TvtjExif;



static guint
tvtj_exif_get_ushort (const TvtjExif *exif,
                      const guchar *data)
{
  if (G_UNLIKELY (exif->big_endian))
    return ((data[0] << 8) | data[1]);
  else
    return ((data[1] << 8) | data[0]);
}



static guint
tvtj_exif_get_ulong (const TvtjExif *exif,
                     const guchar *data)
{
  if (G_UNLIKELY (exif->big_endian))
    return ((data[0] << 24) | (data[1] << 16) | (data[2] << 8) | data[3]);
  else
    return ((data[3] << 24) | (data[2] << 16) | (data[1] << 8) | data[0]);
}



static void
tvtj_exif_parse_ifd (TvtjExif *exif,
                     const guchar *ifd_ptr,
                     guint ifd_len,
                     GSList *ifd_previous_list)
{
  const guchar *subifd_ptr;
  GSList ifd_list;
  guint subifd_off;
  guint value;
  guint tag;
  guint n;

  /* make sure we have a valid IFD here */
  if (G_UNLIKELY (ifd_len < 2))
    return;

  /* make sure we don't recurse into IFDs that are already being processed */
  if (g_slist_find (ifd_previous_list, ifd_ptr) != NULL)
    return;
  ifd_list.next = ifd_previous_list;
  ifd_list.data = (gpointer) ifd_ptr;

  /* determine the number of entries */
  n = tvtj_exif_get_ushort (exif, ifd_ptr);

  /* advance to the IFD content */
  ifd_ptr += 2;
  ifd_len -= 2;

  /* validate the number of entries */
  if (G_UNLIKELY (n * 12 > ifd_len))
    n = ifd_len / 12;

  /* process all IFD entries */
  for (; n > 0; ifd_ptr += 12, --n)
    {
      /* determine the tag of this entry */
      tag = tvtj_exif_get_ushort (exif, ifd_ptr);
      if (tag == 0x8769 || tag == 0xa005)
        {
          /* check if we have a valid sub IFD offset here */
          subifd_off = tvtj_exif_get_ulong (exif, ifd_ptr + 8);
          subifd_ptr = exif->data_ptr + subifd_off;
          if (G_LIKELY (subifd_off < exif->data_len))
            {
              /* process the sub IFD recursively */
              tvtj_exif_parse_ifd (exif, subifd_ptr, exif->data_len - subifd_off, &ifd_list);
            }
        }
      else if (tag == 0x0103)
        {
          /* verify that we have an ushort here (format 3) */
          if (tvtj_exif_get_ushort (exif, ifd_ptr + 2) == 3)
            {
              /* determine the thumbnail compression */
              exif->thumb_compression = tvtj_exif_get_ushort (exif, ifd_ptr + 8);
            }
        }
      else if (tag == 0x0100 || tag == 0x0101 || tag == 0x0106 || tag == 0x0111 || tag == 0x0117)
        {
          /* this can be either ushort or ulong */
          if (tvtj_exif_get_ushort (exif, ifd_ptr + 2) == 3)
            value = tvtj_exif_get_ushort (exif, ifd_ptr + 8);
          else if (tvtj_exif_get_ushort (exif, ifd_ptr + 2) == 4)
            value = tvtj_exif_get_ulong (exif, ifd_ptr + 8);
          else
            value = 0;

          /* and remember it appropriately */
          if (tag == 0x0100)
            exif->thumb_tiff.width = value;
          else if (tag == 0x0101)
            exif->thumb_tiff.height = value;
          else if (tag == 0x0106)
            exif->thumb_tiff.interp = value;
          else if (tag == 0x0111)
            exif->thumb_tiff.offset = value;
          else
            exif->thumb_tiff.length = value;
        }
      else if (tag == 0x0201 || tag == 0x0202)
        {
          /* verify that we have an ulong here (format 4) */
          if (tvtj_exif_get_ushort (exif, ifd_ptr + 2) == 4)
            {
              /* determine the value (thumbnail JPEG offset or length) */
              value = tvtj_exif_get_ulong (exif, ifd_ptr + 8);

              /* and remember it appropriately */
              if (G_LIKELY (tag == 0x201))
                exif->thumb_jpeg.offset = value;
              else
                exif->thumb_jpeg.length = value;
            }
        }
      else if (tag == 0x112)
        {
          /* verify that we have a 2-byte integer (format 3) */
          if (tvtj_exif_get_ushort (exif, ifd_ptr + 2) == 3
              && tvtj_exif_get_ulong (exif, ifd_ptr + 4) == 1)
            {
              /* determine the orientation value */
              value = tvtj_exif_get_ushort (exif, ifd_ptr + 8);
              exif->thumb_jpeg.orientation = MIN (value, 8);
            }
        }
    }

  /* check for link to next IFD */
  subifd_off = tvtj_exif_get_ulong (exif, ifd_ptr);
  if (subifd_off != 0 && subifd_off < exif->data_len)
    {
      /* parse next IFD recursively as well */
      tvtj_exif_parse_ifd (exif, exif->data_ptr + subifd_off, exif->data_len - subifd_off, &ifd_list);
    }
}



static GdkPixbuf *
tvtj_rotate_pixbuf (GdkPixbuf *src,
                    guint orientation)
{
  GdkPixbuf *dest = NULL;
  GdkPixbuf *temp;

  g_return_val_if_fail (GDK_IS_PIXBUF (src), NULL);

  switch (orientation)
    {
    case 1:
      dest = g_object_ref (src);
      break;

    case 2:
      dest = gdk_pixbuf_flip (src, TRUE);
      break;

    case 3:
      dest = gdk_pixbuf_rotate_simple (src, GDK_PIXBUF_ROTATE_UPSIDEDOWN);
      break;

    case 4:
      dest = gdk_pixbuf_flip (src, FALSE);
      break;

    case 5:
      temp = gdk_pixbuf_rotate_simple (src, GDK_PIXBUF_ROTATE_CLOCKWISE);
      dest = gdk_pixbuf_flip (temp, TRUE);
      g_object_unref (temp);
      break;

    case 6:
      dest = gdk_pixbuf_rotate_simple (src, GDK_PIXBUF_ROTATE_CLOCKWISE);
      break;

    case 7:
      temp = gdk_pixbuf_rotate_simple (src, GDK_PIXBUF_ROTATE_CLOCKWISE);
      dest = gdk_pixbuf_flip (temp, FALSE);
      g_object_unref (temp);
      break;

    case 8:
      dest = gdk_pixbuf_rotate_simple (src, GDK_PIXBUF_ROTATE_COUNTERCLOCKWISE);
      break;

    default:
      /* if no orientation tag was present */
      dest = g_object_ref (src);
      break;
    }

  return dest;
}



static GdkPixbuf *
tvtj_exif_extract_thumbnail (const guchar *data,
                             guint length,
                             gint size,
                             guint *exif_orientation)
{
  TvtjExif exif;
  guint offset;
  GdkPixbuf *thumb = NULL;
  GdkPixbuf *rotated;

  /* make sure we have enough data */
  if (G_UNLIKELY (length < 6 + 8))
    return NULL;

  /* validate Exif header */
  if (memcmp (data, "Exif\0\0", 6) != 0)
    return NULL;

  /* advance to TIFF header */
  data += 6;
  length -= 6;

  /* setup Exif data struct */
  memset (&exif, 0, sizeof (exif));
  exif.data_ptr = data;
  exif.data_len = length;

  /* determine byte order */
  if (memcmp (data, "II", 2) == 0)
    exif.big_endian = FALSE;
  else if (memcmp (data, "MM", 2) == 0)
    exif.big_endian = TRUE;
  else
    return NULL;

  /* validate the TIFF header */
  if (tvtj_exif_get_ushort (&exif, data + 2) != 0x2a)
    return NULL;

  /* determine the first IFD offset */
  offset = tvtj_exif_get_ulong (&exif, data + 4);

  /* validate the offset */
  if (G_LIKELY (offset < length))
    {
      /* parse the first IFD (recursively parses the remaining...) */
      tvtj_exif_parse_ifd (&exif, data + offset, length - offset, NULL);

      /* check thumbnail compression type */
      if (G_LIKELY (exif.thumb_compression == 6)) /* JPEG */
        {
          /* check if we have a valid thumbnail JPEG */
          if (exif.thumb_jpeg.offset > 0 && exif.thumb_jpeg.length > 0
              && exif.thumb_jpeg.offset + exif.thumb_jpeg.length <= length)
            {
              /* try to load the embedded thumbnail JPEG */
              thumb = tvtj_jpeg_load (data + exif.thumb_jpeg.offset, exif.thumb_jpeg.length, size);
            }
        }
      else if (exif.thumb_compression == 1) /* Uncompressed */
        {
          /* check if we have a valid thumbnail (current only RGB interpretations) */
          if (G_LIKELY (exif.thumb_tiff.interp == 2)
              && exif.thumb_tiff.offset > 0 && exif.thumb_tiff.length > 0
              && exif.thumb_tiff.offset + exif.thumb_tiff.length <= length
              && exif.thumb_tiff.height * exif.thumb_tiff.width == exif.thumb_tiff.length)
            {
              /* plain RGB data, just what we need for a GdkPixbuf */
              thumb = gdk_pixbuf_new_from_data (g_memdup2 (data + exif.thumb_tiff.offset, exif.thumb_tiff.length),
                                                GDK_COLORSPACE_RGB, FALSE, 8, exif.thumb_tiff.width,
                                                exif.thumb_tiff.height, exif.thumb_tiff.width,
                                                tvtj_free, NULL);
            }
        }

      *exif_orientation = exif.thumb_jpeg.orientation;

      if (thumb != NULL
          && exif.thumb_jpeg.orientation > 1)
        {
          /* rotate thumbnail */
          rotated = tvtj_rotate_pixbuf (thumb, exif.thumb_jpeg.orientation);
          g_object_unref (thumb);
          thumb = rotated;
        }
    }

  return thumb;
}



static GdkPixbuf *
tvtj_jpeg_load_thumbnail (const JOCTET *content,
                          gsize length,
                          gint width,
                          gint height,
                          guint *exif_orientation)
{
  GdkPixbuf *pixbuf = NULL;
  guint marker_len;
  guint marker;
  gsize n;

  /* valid JPEG headers begin with SOI (Start Of Image) */
  if (G_LIKELY (length >= 2 && content[0] == 0xff && content[1] == 0xd8))
    {
      /* search for an EXIF marker */
      for (length -= 2, n = 2; n < length;)
        {
          /* check for valid marker start */
          if (G_UNLIKELY (content[n++] != 0xff))
            break;

          /* determine the next marker */
          marker = content[n];

          /* skip additional padding */
          if (G_UNLIKELY (marker == 0xff))
            continue;

          /* stop at SOS marker */
          if (marker == 0xda)
            break;

          /* advance */
          ++n;

          /* check if valid */
          if (G_UNLIKELY (n + 2 >= length))
            break;

          /* determine the marker length */
          marker_len = (content[n] << 8) | content[n + 1];

          /* check if we have an exif marker here */
          if (marker == 0xe1 && n + marker_len <= length)
            {
              /* try to extract the Exif thumbnail */
              pixbuf = tvtj_exif_extract_thumbnail (content + n + 2, marker_len - 2,
                                                    MIN (width, height), exif_orientation);

              /* do not use low quality embedded thumbnail */
              if (pixbuf != NULL && gdk_pixbuf_get_width (pixbuf) < width
                  && gdk_pixbuf_get_height (pixbuf) < height)
                g_clear_object (&pixbuf);
              else
                break;
            }

          /* try next one then */
          n += marker_len;
        }
    }

  return pixbuf;
}



static void
jpeg_thumbnailer_create (TumblerAbstractThumbnailer *thumbnailer,
                         GCancellable *cancellable,
                         TumblerFileInfo *info)
{
  TumblerThumbnailFlavor *flavor;
  TumblerImageData data;
  TumblerThumbnail *thumbnail;
  struct stat statb;
  const gchar *uri;
  GdkPixbuf *pixbuf = NULL;
  GdkPixbuf *scaled;
  gboolean streaming_needed = TRUE;
  JOCTET *content;
  GError *error = NULL;
  GFile *file;
  gsize length;
  gint fd;
  gint height;
  gint width;
  gint size;

  g_return_if_fail (JPEG_IS_THUMBNAILER (thumbnailer));
  g_return_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable));
  g_return_if_fail (TUMBLER_IS_FILE_INFO (info));

  /* do nothing if cancelled */
  if (g_cancellable_is_cancelled (cancellable))
    return;

  uri = tumbler_file_info_get_uri (info);
  g_debug ("Handling URI '%s'", uri);

  /* try to open the source file for reading */
  file = g_file_new_for_uri (uri);

  thumbnail = tumbler_file_info_get_thumbnail (info);
  g_assert (thumbnail != NULL);

  flavor = tumbler_thumbnail_get_flavor (thumbnail);
  g_assert (flavor != NULL);

  tumbler_thumbnail_flavor_get_size (flavor, &width, &height);
  size = MIN (width, height);

#ifdef HAVE_MMAP
  if (g_file_peek_path (file) != NULL)
    {
      /* try to open the file at the given path */
      fd = open (g_file_peek_path (file), O_RDONLY);
      if (G_LIKELY (fd >= 0))
        {
          /* determine the status of the file */
          if (G_LIKELY (fstat (fd, &statb) == 0 && statb.st_size > 0))
            {
              /* try to mmap the file */
              content = (JOCTET *) mmap (NULL, statb.st_size, PROT_READ,
                                         MAP_SHARED, fd, 0);

              /* verify whether the mmap was successful */
              if (G_LIKELY (content != (JOCTET *) MAP_FAILED))
                {
                  guint exif_orientation = 0;

                  /* try to load the embedded thumbnail first */
                  pixbuf = tvtj_jpeg_load_thumbnail (content, statb.st_size, width, height,
                                                     &exif_orientation);
                  if (pixbuf == NULL)
                    {
                      /* fall back to loading and scaling the image itself */
                      pixbuf = tvtj_jpeg_load (content, statb.st_size, size);

                      if (G_UNLIKELY (pixbuf == NULL))
                        {
                          g_set_error (&error, TUMBLER_ERROR, TUMBLER_ERROR_INVALID_FORMAT,
                                       TUMBLER_ERROR_MESSAGE_CREATION_FAILED);
                        }
                      else if (exif_orientation > 1)
                        {
                          GdkPixbuf *rotated;
                          rotated = tvtj_rotate_pixbuf (pixbuf, exif_orientation);
                          g_object_unref (pixbuf);
                          pixbuf = rotated;
                        }
                    }

                  /* we have successfully mmapped the file. we may not have
                   * a thumbnail but trying to read the image from a stream
                   * won't help us here, so we don't need to attempt streaming
                   * as a fallback */
                  streaming_needed = FALSE;
                }

              /* unmap the file content */
              munmap ((void *) content, statb.st_size);
            }

          /* close the file */
          close (fd);
        }
    }
#endif

  if (streaming_needed)
    {
      g_file_load_contents (file, cancellable, (gchar **) &content, &length,
                            NULL, &error);

      if (error == NULL)
        {
          guint exif_orientation = 0;
          pixbuf = tvtj_jpeg_load_thumbnail (content, length, width, height, &exif_orientation);

          if (pixbuf == NULL)
            {
              pixbuf = tvtj_jpeg_load (content, length, size);
              if (G_UNLIKELY (pixbuf == NULL))
                {
                  g_set_error (&error, TUMBLER_ERROR, TUMBLER_ERROR_INVALID_FORMAT,
                               TUMBLER_ERROR_MESSAGE_CREATION_FAILED);
                }
              else if (exif_orientation > 1)
                {
                  GdkPixbuf *rotated;
                  rotated = tvtj_rotate_pixbuf (pixbuf, exif_orientation);
                  g_object_unref (pixbuf);
                  pixbuf = rotated;
                }
            }
        }
    }

  /* either we have an error now or we have a valid thumbnail pixbuf */
  g_assert (error != NULL || pixbuf != NULL);

  if (pixbuf != NULL)
    {
      scaled = tumbler_util_scale_pixbuf (pixbuf, width, height);
      g_object_unref (pixbuf);
      pixbuf = scaled;

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

      g_object_unref (pixbuf);
    }

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

  g_object_unref (flavor);
  g_object_unref (thumbnail);
  g_object_unref (file);
}
