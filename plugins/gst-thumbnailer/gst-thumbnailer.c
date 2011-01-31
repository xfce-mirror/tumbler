/* vi:set et ai sw=2 sts=2 ts=2: */
/*
 * Copyright (c) 2011 Intel Corporation
 *
 * Author: Ross Burton <ross@linux.intel.com>
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

#include <string.h>
#include <math.h>

#include <glib.h>
#include <glib-object.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <tumbler/tumbler.h>
#include <gst/gst.h>

#include "gst-thumbnailer.h"
#include "gst-helper.h"

#ifdef DEBUG
#define LOG(...) g_message (__VA_ARGS__)
#else
#define LOG(...)
#endif

static void gst_thumbnailer_create (TumblerAbstractThumbnailer *thumbnailer,
                                    GCancellable               *cancellable,
                                    TumblerFileInfo            *info);

struct _GstThumbnailerClass
{
  TumblerAbstractThumbnailerClass __parent__;
};

struct _GstThumbnailer
{
  TumblerAbstractThumbnailer __parent__;
};


G_DEFINE_DYNAMIC_TYPE (GstThumbnailer,
                       gst_thumbnailer,
                       TUMBLER_TYPE_ABSTRACT_THUMBNAILER);


void
gst_thumbnailer_register (TumblerProviderPlugin *plugin)
{
  gst_thumbnailer_register_type (G_TYPE_MODULE (plugin));
}

static void
gst_thumbnailer_class_init (GstThumbnailerClass *klass)
{
  TumblerAbstractThumbnailerClass *abstractthumbnailer_class;

  abstractthumbnailer_class = TUMBLER_ABSTRACT_THUMBNAILER_CLASS (klass);
  abstractthumbnailer_class->create = gst_thumbnailer_create;
}

static void
gst_thumbnailer_class_finalize (GstThumbnailerClass *klass)
{
}

static void
gst_thumbnailer_init (GstThumbnailer *thumbnailer)
{
}


/*
 * Determine if the image is "interesting" or not.  This implementation reduces
 * the RGB from 24 to 12 bits and examines the distribution of colours.
 *
 * This function is taken from Bickley, Copyright (c) Intel Corporation 2008.
 */
static gboolean
is_interesting (GdkPixbuf *pixbuf)
{
  int width, height, y, rowstride;
  gboolean has_alpha;
  guint32 histogram[4][4][4] = {{{0,},},};
  guchar *pixels;
  guint pxl_count = 0, count, i;

  g_assert (GDK_IS_PIXBUF (pixbuf));

  width = gdk_pixbuf_get_width (pixbuf);
  height = gdk_pixbuf_get_height (pixbuf);
  rowstride = gdk_pixbuf_get_rowstride (pixbuf);
  has_alpha = gdk_pixbuf_get_has_alpha (pixbuf);

  pixels = gdk_pixbuf_get_pixels (pixbuf);
  for (y = 0; y < height; y++)
    {
      guchar *row = pixels + (y * rowstride);
      int c;

      for (c = 0; c < width; c++)
        {
          guchar r, g, b;

          r = row[0];
          g = row[1];
          b = row[2];

          histogram[r / 64][g / 64][b / 64]++;

          if (has_alpha)
            {
              row += 4;
            }
          else
            {
              row += 3;
            }

          pxl_count++;
        }
    }

  count = 0;
  for (i = 0; i < 4; i++)
    {
      int j;
      for (j = 0; j < 4; j++)
        {
          int k;

          for (k = 0; k < 4; k++)
            {
              /* Count how many bins have more than
                 1% of the pixels in the histogram */
              if (histogram[i][j][k] > pxl_count / 100)
                count++;
            }
        }
    }

  /* Image is boring if there is only 1 bin with > 1% of pixels */
  return count > 1;
}

/*
 * Construct a pipline for a given @info, cancelling during initialisation on
 * @cancellable.  This function will either return a #GstElement that has been
 * prerolled and is in the paused state, or %NULL if the initialisation is
 * cancelled or an error occurs.
 */
static GstElement *
make_pipeline (TumblerFileInfo *info, GCancellable *cancellable)
{
  GstElement *playbin, *audio_sink, *video_sink;
  int count = 0, n_video = 0;
  GstStateChangeReturn state;

  g_assert (info);

  playbin = gst_element_factory_make ("playbin2", "playbin");
  g_assert (playbin);
  audio_sink = gst_element_factory_make ("fakesink", "audiosink");
  g_assert (audio_sink);
  video_sink = gst_element_factory_make ("fakesink", "videosink");
  g_assert (video_sink);

  g_object_set (playbin,
                "uri", tumbler_file_info_get_uri (info),
                "audio-sink", audio_sink,
                "video-sink", video_sink,
                NULL);

  g_object_set (video_sink,
                "sync", TRUE,
                NULL);

  /* Change to paused state so we're ready to seek */
  state = gst_element_set_state (playbin, GST_STATE_PAUSED);
  while (state == GST_STATE_CHANGE_ASYNC
         && count < 5
         && !g_cancellable_is_cancelled (cancellable))
    {
      state = gst_element_get_state (playbin, NULL, 0, 1 * GST_SECOND);
      count++;

      /* Spin mainloop so we can pick up the cancels */
      while (g_main_context_pending (NULL))
        g_main_context_iteration (NULL, FALSE);
    }

  if (state == GST_STATE_CHANGE_FAILURE || state == GST_STATE_CHANGE_ASYNC)
    {
      LOG ("failed to or still changing state, aborting (state change %d)", state);
      g_object_unref (playbin);
      return NULL;
    }

  g_object_get (playbin, "n-video", &n_video, NULL);
  if (n_video == 0)
    {
      LOG ("no video stream, aborting");
      g_object_unref (playbin);
      return NULL;
    }

  return playbin;
}

/*
 * Get the total duration in nanoseconds of the stream.
 */
static gint64
get_duration (GstElement *playbin)
{
  GstFormat format = GST_FORMAT_TIME;
  gint64 duration = 0;

  g_assert (playbin);

  gst_element_query_duration (playbin, &format, &duration);

  return duration;
}

/*
 * Generate a thumbnail for @info.
 */
static void
gst_thumbnailer_create (TumblerAbstractThumbnailer *thumbnailer,
                        GCancellable               *cancellable,
                        TumblerFileInfo            *info)
{
  /* These positions are taken from Totem */
  const double positions[] = {
    1.0 / 3.0,
    2.0 / 3.0,
    0.1,
    0.9,
    0.5
  };
  GstElement             *playbin;
  gint64                  duration;
  unsigned int            i;
  GstBuffer              *frame;
  GdkPixbuf              *shot;
  TumblerThumbnail       *thumbnail;
  TumblerThumbnailFlavor *flavour;
  TumblerImageData        data;
  GError                 *error = NULL;

  g_return_if_fail (IS_GST_THUMBNAILER (thumbnailer));
  g_return_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable));
  g_return_if_fail (TUMBLER_IS_FILE_INFO (info));

  /* Check for early cancellation */
  if (g_cancellable_is_cancelled (cancellable))
    return;

  playbin = make_pipeline (info, cancellable);
  if (playbin == NULL)
    {
      /* TODO: emit an error, but the specification won't let me. */
      return;
    }

  duration = get_duration (playbin);

  /* Now we have a pipeline that we know has video and is paused, ready for
     seeking.  Try to find an interesting frame at each of the positions in
     order. */
  for (i = 0; i < G_N_ELEMENTS (positions); i++)
    {
      /* Check if we've been cancelled */
      if (g_cancellable_is_cancelled (cancellable))
        {
          gst_element_set_state (playbin, GST_STATE_NULL);
          g_object_unref (playbin);
          return;
        }

      LOG ("trying position %f", positions[i]);

      gst_element_seek_simple (playbin,
                               GST_FORMAT_TIME,
                               GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_KEY_UNIT,
                               (gint64)(positions[i] * duration));

      if (gst_element_get_state (playbin, NULL, NULL, 1 * GST_SECOND)
          == GST_STATE_CHANGE_FAILURE)
        {
          LOG ("Could not seek");
          gst_element_set_state (playbin, GST_STATE_NULL);
          g_object_unref (playbin);
          return;
        }

      g_object_get (playbin, "frame", &frame, NULL);

      if (frame == NULL)
        {
          LOG ("No frame found!");
          gst_element_set_state (playbin, GST_STATE_NULL);
          g_object_unref (playbin);
          continue;
        }

      thumbnail = tumbler_file_info_get_thumbnail (info);
      flavour = tumbler_thumbnail_get_flavor (thumbnail);
      /* This frees the buffer for us */
      shot = gst_helper_convert_buffer_to_pixbuf (frame, cancellable, flavour);
      g_object_unref (flavour);

      /* If it's not interesting, throw it away and try again*/
      if (is_interesting (shot))
        {
          /* Got an interesting image, break out */
          LOG ("Found an interesting image");
          break;
        }

      /*
       * If we've still got positions to try, free the current uninteresting
       * shot. Otherwise we'll make do with what we have.
       */
      if (i + 1 < G_N_ELEMENTS (positions) && shot)
        {
          g_object_unref (shot);
          shot = NULL;
        }

      /* Spin mainloop so we can pick up the cancels */
      while (g_main_context_pending (NULL))
        {
          g_main_context_iteration (NULL, FALSE);
        }
    }

  gst_element_set_state (playbin, GST_STATE_NULL);
  g_object_unref (playbin);

  if (shot)
    {
      data.data = gdk_pixbuf_get_pixels (shot);
      data.has_alpha = gdk_pixbuf_get_has_alpha (shot);
      data.bits_per_sample = gdk_pixbuf_get_bits_per_sample (shot);
      data.width = gdk_pixbuf_get_width (shot);
      data.height = gdk_pixbuf_get_height (shot);
      data.rowstride = gdk_pixbuf_get_rowstride (shot);
      data.colorspace = (TumblerColorspace) gdk_pixbuf_get_colorspace (shot);

      tumbler_thumbnail_save_image_data (thumbnail, &data,
                                         tumbler_file_info_get_mtime (info),
                                         NULL, &error);

      g_object_unref (shot);

      if (error != NULL)
        {
          g_signal_emit_by_name (thumbnailer, "error",
                                 tumbler_file_info_get_uri (info),
                                 error->code, error->message);
          g_error_free (error);
        }
      else
        {
          g_signal_emit_by_name (thumbnailer, "ready",
                                 tumbler_file_info_get_uri (info));
        }
    }
}
