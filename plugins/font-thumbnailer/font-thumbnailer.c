/* vi:set et ai sw=2 sts=2 ts=2: */
/*-
 * Copyright (c) 2006 Benedikt Meurer <benny@xfce.org>
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

#include <ft2build.h>
#include FT_FREETYPE_H

#include <glib.h>
#include <glib/gi18n.h>
#include <glib-object.h>

#include <gdk-pixbuf/gdk-pixbuf.h>

#include <tumbler/tumbler.h>

#include <font-thumbnailer/font-thumbnailer.h>



static void font_thumbnailer_finalize (GObject                    *object);
static void font_thumbnailer_create   (TumblerAbstractThumbnailer *thumbnailer,
                                       GCancellable               *cancellable,
                                       TumblerFileInfo            *info);



struct _FontThumbnailerClass
{
  TumblerAbstractThumbnailerClass __parent__;
};

struct _FontThumbnailer
{
  TumblerAbstractThumbnailer __parent__;

  FT_Library library;
  FT_Error   library_error;
};



G_DEFINE_DYNAMIC_TYPE (FontThumbnailer, 
                       font_thumbnailer,
                       TUMBLER_TYPE_ABSTRACT_THUMBNAILER);



void
font_thumbnailer_register (TumblerProviderPlugin *plugin)
{
  font_thumbnailer_register_type (G_TYPE_MODULE (plugin));
}



static void
font_thumbnailer_class_init (FontThumbnailerClass *klass)
{
  TumblerAbstractThumbnailerClass *abstractthumbnailer_class;
  GObjectClass                    *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->finalize = font_thumbnailer_finalize;

  abstractthumbnailer_class = TUMBLER_ABSTRACT_THUMBNAILER_CLASS (klass);
  abstractthumbnailer_class->create = font_thumbnailer_create;
}



static void
font_thumbnailer_class_finalize (FontThumbnailerClass *klass)
{
}



static void
font_thumbnailer_init (FontThumbnailer *thumbnailer)
{
  /* initialize freetype and remember possible errors */
  thumbnailer->library_error = FT_Init_FreeType (&thumbnailer->library);
}


static void 
font_thumbnailer_finalize (GObject *object)
{
  FontThumbnailer *thumbnailer = FONT_THUMBNAILER (object);

  /* release the freetype library object */
  FT_Done_FreeType (thumbnailer->library);

  (*G_OBJECT_CLASS (font_thumbnailer_parent_class)->finalize) (object);
}



static const gchar *
ft_strerror (FT_Error error)
{
#undef __FTERRORS_H__
#define FT_ERRORDEF(e,v,s) case e: return s;
#define FT_ERROR_START_LIST
#define FT_ERROR_END_LIST
  switch (error)
    {
#include FT_ERRORS_H
    default:
      return "unknown";
    }
}




static FT_Error
render_glyph (GdkPixbuf *pixbuf,
              FT_Face    face,
              FT_UInt    glyph,
              gint      *pen_x,
              gint      *pen_y)
{
  FT_GlyphSlot slot = face->glyph;
  FT_Error     error;
  guchar      *pixels;
  guchar       pixel;
  gint         rowstride;
  gint         height;
  gint         width;
  gint         off_x;
  gint         off_y;
  gint         off;
  gint         i, j;

  /* load the glyph */
  error = FT_Load_Glyph (face, glyph, FT_LOAD_DEFAULT);
  if (G_UNLIKELY (error != 0))
    return error;

  /* render the glyph */
  error = FT_Render_Glyph (slot, ft_render_mode_normal);
  if (G_UNLIKELY (error != 0))
    return error;

  off_x = *pen_x + slot->bitmap_left;
  off_y = *pen_y - slot->bitmap_top;

  pixels = gdk_pixbuf_get_pixels (pixbuf);
  width = gdk_pixbuf_get_width (pixbuf);
  height = gdk_pixbuf_get_height (pixbuf);
  rowstride = gdk_pixbuf_get_rowstride (pixbuf);

  for (j = 0; (guint) j < slot->bitmap.rows; ++j)
    {
      if (j + off_y < 0 || j + off_y >= height)
        continue;

      for (i = 0; (guint) i < slot->bitmap.width; ++i)
        {
          if (i + off_x < 0 || i + off_x >= width)
            continue;

          switch (slot->bitmap.pixel_mode)
            {
            case ft_pixel_mode_mono:
              pixel = slot->bitmap.buffer[j * slot->bitmap.pitch + i / 8];
              pixel = 255 - ((pixel >> (7 - i % 8)) & 0x1) * 255;
              break;

            case ft_pixel_mode_grays:
              pixel = 255 - slot->bitmap.buffer[j * slot->bitmap.pitch + i];
              break;

            default:
              pixel = 255;
              break;
            }

          off = (j + off_y) * rowstride + 3 * (i + off_x);
          pixels[off + 0] = pixel;
          pixels[off + 1] = pixel;
          pixels[off + 2] = pixel;
        }
    }

  *pen_x += slot->advance.x >> 6;

  return 0;
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

  /* determine which axis needs to be scaled down more */
  wratio = (gdouble) source_width  / (gdouble) dest_width;
  hratio = (gdouble) source_height / (gdouble) dest_height;

  /* adjust the other axis */
  if (hratio > wratio)
    dest_width = rint (source_width / hratio);
  else
    dest_height = rint (source_height / wratio);

  /* scale the pixbuf down to the desired size */
  return gdk_pixbuf_scale_simple (source, MAX (dest_width, 1), MAX (dest_height, 1), 
                                  GDK_INTERP_BILINEAR);
}



static GdkPixbuf *
trim_and_scale_pixbuf (GdkPixbuf *pixbuf,
                       gint       dest_width,
                       gint       dest_height)
{
  GdkPixbuf *subpixbuf;
  GdkPixbuf *scaled;
  gboolean   seen_pixel;
  guchar    *pixels;
  gint       rowstride;
  gint       height;
  gint       width;
  gint       i, j;
  gint       trim_left;
  gint       trim_right;
  gint       trim_top;
  gint       trim_bottom;
  gint       offset;

  pixels = gdk_pixbuf_get_pixels (pixbuf);
  width = gdk_pixbuf_get_width (pixbuf);
  height = gdk_pixbuf_get_height (pixbuf);
  rowstride = gdk_pixbuf_get_rowstride (pixbuf);

  for (i = 0; i < width; ++i)
    {
      seen_pixel = FALSE;
      for (j = 0; j < height; ++j)
        {
          offset = j * rowstride + 3 * i;
          seen_pixel = (pixels[offset + 0] != 0xff ||
                        pixels[offset + 1] != 0xff ||
                        pixels[offset + 2] != 0xff);
          if (seen_pixel)
            break;
        }
      
      if (seen_pixel)
        break;
    }

  trim_left = MIN (width, i);
  trim_left = MAX (trim_left - 8, 0);

  for (i = width - 1; i >= trim_left; --i)
    {
      seen_pixel = FALSE;
      for (j = 0; j < height; ++j)
        {
          offset = j * rowstride + 3 * i;
          seen_pixel = (pixels[offset + 0] != 0xff ||
                        pixels[offset+1] != 0xff ||
                        pixels[offset+2] != 0xff);
          if (seen_pixel)
            break;
        }
      
      if (seen_pixel)
        break;
    }

  trim_right = MAX (trim_left, i);
  trim_right = MIN (trim_right + 8, width - 1);

  for (j = 0; j < height; ++j)
    {
      seen_pixel = FALSE;
      for (i = 0; i < width; ++i)
        {
          offset = j * rowstride + 3 * i;
          seen_pixel = (pixels[offset + 0] != 0xff ||
                        pixels[offset + 1] != 0xff ||
                        pixels[offset + 2] != 0xff);
          if (seen_pixel)
            break;
        }
      
      if (seen_pixel)
        break;
    }

  trim_top = MIN (height, j);
  trim_top = MAX (trim_top - 8, 0);

  for (j = height - 1; j >= trim_top; --j)
    {
      seen_pixel = FALSE;
      for (i = 0; i < width; ++i)
        {
          offset = j * rowstride + 3 * i;
          seen_pixel = (pixels[offset + 0] != 0xff ||
                        pixels[offset + 1] != 0xff ||
                        pixels[offset + 2] != 0xff);
          if (seen_pixel)
            break;
        }
      
      if (seen_pixel)
        break;
    }

  trim_bottom = MAX (trim_top, j);
  trim_bottom = MIN (trim_bottom + 8, height - 1);

  /* determine the trimmed subpixbuf */
  subpixbuf = gdk_pixbuf_new_subpixbuf (pixbuf, trim_left, trim_top, 
                                        trim_right - trim_left, 
                                        trim_bottom - trim_top);

  /* check if we still need to scale down */
  if (gdk_pixbuf_get_width (subpixbuf) > dest_width 
      || gdk_pixbuf_get_height (subpixbuf) > dest_height)
    {
      scaled = scale_pixbuf (subpixbuf, dest_width, dest_height);
      g_object_unref (G_OBJECT (subpixbuf));
      subpixbuf = scaled;
    }
  
  return subpixbuf;
}



static GdkPixbuf *
generate_pixbuf (FT_Face                 face,
                 TumblerThumbnailFlavor *flavor,
                 FT_Error               *error)
{
  GdkPixbuf *pixbuf = NULL;
  GdkPixbuf *result = NULL;
  FT_UInt    glyph1;
  FT_UInt    glyph2;
  gint       width;
  gint       height;
  gint       pen_x;
  gint       pen_y;

  /* determine the desired size for this flavor */
  tumbler_thumbnail_flavor_get_size (flavor, &width, &height);

  /* try to set the pixel size */
  *error = FT_Set_Pixel_Sizes (face, 0, MIN (width, height));
  if (G_UNLIKELY (*error != 0))
    return NULL;

  /* determine prefered glyphs for the thumbnail (with appropriate fallbacks) */
  glyph1 = FT_Get_Char_Index (face, 'A');
  if (G_UNLIKELY (glyph1 == 0))
    glyph1 = MIN (65, face->num_glyphs - 1);
  glyph2 = FT_Get_Char_Index (face, 'a');
  if (G_UNLIKELY (glyph2 == 0))
    glyph2 = MIN (97, face->num_glyphs - 1);
  
  /* allocate the pixbuf to render the glyphs to */
  pixbuf = gdk_pixbuf_new (GDK_COLORSPACE_RGB, FALSE, 8, width * 3, (height * 3) / 2);
  gdk_pixbuf_fill (pixbuf, 0xffffffff);

  /* initial pen position */
  pen_x = width / 2;
  pen_y = height;

  /* render the first letter to the pixbuf */
  *error = render_glyph (pixbuf, face, glyph1, &pen_x, &pen_y);
  if (G_UNLIKELY (*error != 0))
    return NULL;

  /* render the second letter to the pixbuf */
  *error = render_glyph (pixbuf, face, glyph2, &pen_x, &pen_y);
  if (G_UNLIKELY (*error != 0))
    return NULL;

  /* trim the pixbuf and rescale if necessary */
  result = trim_and_scale_pixbuf (pixbuf, width, height);
  g_object_unref (pixbuf);

  return result;
}



static void
font_thumbnailer_create (TumblerAbstractThumbnailer *thumbnailer,
                         GCancellable               *cancellable,
                         TumblerFileInfo            *info)
{
  TumblerThumbnailFlavor *flavor;
  TumblerImageData        data;
  TumblerThumbnail       *thumbnail;
  FontThumbnailer        *font_thumbnailer = FONT_THUMBNAILER (thumbnailer);
  const gchar            *uri;
  GdkPixbuf              *pixbuf;
  FT_Error                ft_error;
  FT_Face                 face;
  guint64                 mtime;
  GError                 *error = NULL;
  GFile                  *file;
  gchar                  *error_msg;
  gchar                  *font_data;
  gsize                   length;
  gint                    n;

  g_return_if_fail (IS_FONT_THUMBNAILER (thumbnailer));
  g_return_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable));
  g_return_if_fail (TUMBLER_IS_FILE_INFO (info));

  /* do nothing if cancelled */
  if (g_cancellable_is_cancelled (cancellable)) 
    return;

  uri = tumbler_file_info_get_uri (info);

  /* check if we have a valid freetype library object */
  if (font_thumbnailer->library_error != 0)
    {
      /* there was an error in the freetype initialization, abort */
      error_msg = g_strdup_printf (_("Could not initialize freetype: %s"),
                                   ft_strerror (font_thumbnailer->library_error));
      g_signal_emit_by_name (thumbnailer, "error", uri, 0, error_msg);
      g_free (error_msg);
      return;
    }

  /* try to read the file into memory */
  file = g_file_new_for_uri (tumbler_file_info_get_uri (info));
  if (!g_file_load_contents (file, cancellable, &font_data, &length, NULL, &error))
    {
      /* there was an error, emit error signal */
      error_msg = g_strdup_printf (_("Could not load file contents: %s"), 
                                   error->message);
      g_signal_emit_by_name (thumbnailer, "error", uri, 0, error_msg);
      g_free (error_msg);

      /* clean up */
      g_error_free (error);
      g_object_unref (file);

      return;
    }
  g_object_unref (file);

  /* try to open the font file */
  ft_error = FT_New_Memory_Face (font_thumbnailer->library, (const FT_Byte *)font_data, 
                                 length, 0, &face);
  if (G_UNLIKELY (ft_error != 0))
    {
      /* the font file could not be loaded, emit an error signal */
      error_msg = g_strdup_printf (_("Could not open font file: %s"),
                                   ft_strerror (ft_error));
      g_signal_emit_by_name (thumbnailer, "error", uri, 0, error_msg);
      g_free (error_msg);

      /* clean up */
      g_free (font_data);

      return;
    }

  /* try to set the character map */
  for (n = 0; n < face->num_charmaps; ++n)
    {
      /* check for a desired character map */
      if (face->charmaps[n]->encoding == ft_encoding_latin_1 
          || face->charmaps[n]->encoding == ft_encoding_unicode
          || face->charmaps[n]->encoding == ft_encoding_apple_roman)
        {
          /* try to set the character map */
          ft_error = FT_Set_Charmap (face, face->charmaps[n]);
          if (G_UNLIKELY (error != 0))
            {
              /* emit an error signal */
              error_msg = g_strdup_printf (_("Could not set the character map: %s"),
                                           ft_strerror (ft_error));
              g_signal_emit_by_name (thumbnailer, "error", uri, 0, error_msg);
              g_free (error_msg);

              /* clean up */
              g_free (font_data);
              FT_Done_Face (face);

              return;
            }
        }
    }

  thumbnail = tumbler_file_info_get_thumbnail (info);

  g_assert (thumbnail != NULL);

  /* generate a thumbnail for the flavor */
  flavor = tumbler_thumbnail_get_flavor (thumbnail);
  pixbuf = generate_pixbuf (face, flavor, &ft_error);
  g_object_unref (flavor);

  /* abort if there was an error */
  if (G_UNLIKELY (ft_error != 0))
    {
      /* emit an error signal */
      error_msg = g_strdup_printf (_("Could not render glyphs: %s"),
                                   ft_strerror (ft_error));
      g_signal_emit_by_name (thumbnailer, "error", uri, 0, error_msg);
      g_free (error_msg);

      /* clean up */
      g_free (font_data);
      FT_Done_Face (face);

      return;
    }

  /* release the font face */
  FT_Done_Face (face);

  /* determine when the URI was last modified */
  mtime = tumbler_file_info_get_mtime (info);

  /* compose the image data */
  data.data = gdk_pixbuf_get_pixels (pixbuf);
  data.has_alpha = gdk_pixbuf_get_has_alpha (pixbuf);
  data.bits_per_sample = gdk_pixbuf_get_bits_per_sample (pixbuf);
  data.width = gdk_pixbuf_get_width (pixbuf);
  data.height = gdk_pixbuf_get_height (pixbuf);
  data.rowstride = gdk_pixbuf_get_rowstride (pixbuf);
  data.colorspace = (TumblerColorspace) gdk_pixbuf_get_colorspace (pixbuf);

  /* save the thumbnail */
  tumbler_thumbnail_save_image_data (thumbnail, &data, mtime, NULL, &error);

  /* check if there was an error */
  if (error != NULL)
    {
      /* emit an error signal */
      g_signal_emit_by_name (thumbnailer, "error", uri, error->code, error->message);
      g_error_free (error);
    }
  else
    {
      /* otherwise, the thumbnail is now ready */
      g_signal_emit_by_name (thumbnailer, "ready", uri);
    }

  /* clean up */
  g_object_unref (pixbuf);
  g_object_unref (thumbnail);
  g_free (font_data);
}
