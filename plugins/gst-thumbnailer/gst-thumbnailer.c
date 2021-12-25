/* vi:set et ai sw=2 sts=2 ts=2: */
/*
 * Copyright (C) 2003,2004 Bastien Nocera <hadess@hadess.net>
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
 * Most of the code is taken from the totem-video-thumbnailer and
 * made suitable for Tumbler by Nick Schermer.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>

#include <glib.h>
#include <glib/gi18n.h>
#include <glib-object.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <tumbler/tumbler.h>

#include <gst/gst.h>
#include <gst/tag/tag.h>

#include "gst-thumbnailer.h"



#define BORING_IMAGE_VARIANCE       256.0    /* tweak this if necessary */
#define TUMBLER_GST_PLAY_FLAG_VIDEO (1 << 0) /* from GstPlayFlags */
#define TUMBLER_GST_PLAY_FLAG_AUDIO (1 << 1) /* from GstPlayFlags */



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



static GdkPixbuf *
gst_thumbnailer_buffer_to_pixbuf (GstBuffer *buffer)
{
  GstMapInfo       info;
  GdkPixbuf       *pixbuf = NULL;
  GdkPixbufLoader *loader;

  if (!gst_buffer_map (buffer, &info, GST_MAP_READ))
    return NULL;

  loader = gdk_pixbuf_loader_new ();

  if (gdk_pixbuf_loader_write (loader, info.data, info.size, NULL)
      && gdk_pixbuf_loader_close (loader, NULL))
    {
      pixbuf = gdk_pixbuf_loader_get_pixbuf (loader);
      if (pixbuf != NULL)
        g_object_ref (pixbuf);
    }

  g_object_unref (loader);

  gst_buffer_unmap (buffer, &info);

  return pixbuf;
}



static GdkPixbuf *
gst_thumbnailer_cover_from_tags (GstTagList   *tags,
                                 GCancellable *cancellable)
{
  GstSample          *cover = NULL;
  guint               i;
  GstCaps            *caps;
  const GstStructure *caps_struct;
  gint                type = GST_TAG_IMAGE_TYPE_UNDEFINED;
  GstBuffer          *buffer;
  GdkPixbuf          *pixbuf = NULL;

  for (i = 0; ; i++)
    {
      GstSample	*sample;

      if (g_cancellable_is_cancelled (cancellable))
        break;

      /* look for image in the tags */
      if (!gst_tag_list_get_sample_index (tags, GST_TAG_IMAGE, i, &sample))
        break;

      caps = gst_sample_get_caps (sample);
      caps_struct = gst_caps_get_structure (caps, 0);
      gst_structure_get_enum (caps_struct,
                              "image-type",
                              GST_TYPE_TAG_IMAGE_TYPE,
                              &type);

      if (cover != NULL)
	gst_sample_unref (cover);
      cover = sample;

      /* prefer the from cover image if specified */
     if (type == GST_TAG_IMAGE_TYPE_FRONT_COVER)
	break;
    }

  if (cover == NULL
      && !g_cancellable_is_cancelled (cancellable))
    {
      /* look for preview image */
      gst_tag_list_get_sample_index (tags, GST_TAG_PREVIEW_IMAGE, 0, &cover);
    }

  if (cover != NULL)
    {
      /* create image */
      buffer = gst_sample_get_buffer (cover);
      pixbuf = gst_thumbnailer_buffer_to_pixbuf (buffer);
      gst_sample_unref (cover);
    }

  return pixbuf;
}



static GdkPixbuf *
gst_thumbnailer_cover_by_name (GstElement   *play,
                               const gchar  *signal_name,
                               GCancellable *cancellable)
{
  GstTagList *tags = NULL;
  GdkPixbuf  *cover;

  g_signal_emit_by_name (G_OBJECT (play), signal_name, 0, &tags);

  if (tags == NULL)
    return FALSE;

  /* check the tags for a cover */
  cover = gst_thumbnailer_cover_from_tags (tags, cancellable);
  gst_tag_list_free (tags);

  return cover;
}



static GdkPixbuf *
gst_thumbnailer_cover (GstElement   *play,
                       GCancellable *cancellable)
{
  GdkPixbuf *cover;

  cover = gst_thumbnailer_cover_by_name (play, "get-audio-tags", cancellable);
  if (cover == NULL)
    cover = gst_thumbnailer_cover_by_name (play, "get-video-tags", cancellable);

  return cover;
}



static gboolean
gst_thumbnailer_has_video (GstElement *play)
{
  guint n_video;
  g_object_get (play, "n-video", &n_video, NULL);
  return n_video > 0;
}



static void
gst_thumbnailer_destroy_pixbuf (guchar   *pixbuf,
                                gpointer  data)
{
  gst_sample_unref (GST_SAMPLE (data));
}



static gboolean
gst_thumbnailer_pixbuf_interesting (GdkPixbuf *pixbuf)
{
  gint    rowstride;
  gint    height;
  guchar *buffer;
  gint    length;
  gint    i;
  gfloat  x_bar = 0.0f;
  gfloat  variance = 0.0f;
  gfloat  temp;

  rowstride = gdk_pixbuf_get_rowstride (pixbuf);
  height = gdk_pixbuf_get_height (pixbuf);
  length = (rowstride * height);

  buffer = gdk_pixbuf_get_pixels (pixbuf);

  /* calculate the x-bar */
  for (i = 0; i < length; i++)
    x_bar += (gfloat) buffer[i];
  x_bar /= (gfloat) length;

  /* calculate the variance */
  for (i = 0; i < length; i++)
    {
      temp = ((gfloat) buffer[i] - x_bar);
      variance += temp * temp;
    }

  return (variance > BORING_IMAGE_VARIANCE);
}



static GdkPixbuf *
gst_thumbnailer_capture_frame (GstElement *play,
                               gint        width)
{
  GstCaps      *to_caps;
  GstSample    *sample = NULL;
  GdkPixbuf    *pixbuf = NULL;
  GstStructure *s;
  GstCaps      *sample_caps;
  gint          outwidth = 0, outheight = 0;
  GstMemory    *memory;
  GstMapInfo    info;

  /* desired output format (RGB24) */
  to_caps = gst_caps_new_simple ("video/x-raw",
                                 "format", G_TYPE_STRING, "RGB",
                                 "pixel-aspect-ratio", GST_TYPE_FRACTION, 1, 1,
                                 "width", G_TYPE_INT, width,
                                 NULL);

  /* get the frame */
  g_signal_emit_by_name (play, "convert-sample", to_caps, &sample);
  gst_caps_unref (to_caps);

  if (sample == NULL)
    return NULL;

  sample_caps = gst_sample_get_caps (sample);
  if (sample_caps == NULL)
    {
      /* no caps on output buffer */
      gst_sample_unref (sample);
      return NULL;
    }

  /* size of the frame */
  s = gst_caps_get_structure (sample_caps, 0);
  gst_structure_get_int (s, "width", &outwidth);
  gst_structure_get_int (s, "height", &outheight);
  if (outwidth <= 0 || outheight <= 0)
    {
      /* invalid size */
      gst_sample_unref (sample);
      return NULL;
    }

  /* get the memory block of the buffer */
  memory = gst_buffer_get_memory (gst_sample_get_buffer (sample), 0);
  if (gst_memory_map (memory, &info, GST_MAP_READ))
    {
      /* create pixmap for the data */
      pixbuf = gdk_pixbuf_new_from_data (info.data,
                                         GDK_COLORSPACE_RGB, FALSE, 8,
                                         outwidth, outheight,
                                         GST_ROUND_UP_4 (width * 3),
                                         gst_thumbnailer_destroy_pixbuf,
                                         sample);

      /* release memory */
      gst_memory_unmap (memory, &info);
    }

  gst_memory_unref (memory);

  /* release sample if pixbuf failed */
  if (pixbuf == NULL)
    gst_sample_unref (sample);

  return pixbuf;
}



static GdkPixbuf *
gst_thumbnailer_capture_interesting_frame (GstElement   *play,
                                           gint64        duration,
                                           gint          width,
                                           GCancellable *cancellable)
{
  GdkPixbuf     *pixbuf = NULL;
  guint          n;
  const gdouble  offsets[] = { 1.0 / 3.0, 2.0 / 3.0, 0.1, 0.9, 0.5 };
  gint64         seek_time;

  /* video has no duration, capture 1st frame */
  if (duration == -1)
    {
      if (!g_cancellable_is_cancelled (cancellable))
        return gst_thumbnailer_capture_frame (play, width);
      else
        return NULL;
    }

  for (n = 0; n < G_N_ELEMENTS (offsets); n++)
    {
      /* check if we should abort */
      if (g_cancellable_is_cancelled (cancellable))
        break;

      /* seek to offset */
      seek_time = offsets[n] * duration;
      gst_element_seek (play, 1.0,
                        GST_FORMAT_TIME, GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_KEY_UNIT,
                        GST_SEEK_TYPE_SET, seek_time * GST_MSECOND,
                        GST_SEEK_TYPE_NONE, GST_CLOCK_TIME_NONE);

      /* wait for the seek to complete */
      gst_element_get_state (play, NULL, NULL, GST_CLOCK_TIME_NONE);

      /* check if we should abort */
      if (g_cancellable_is_cancelled (cancellable))
        break;

      /* get the frame */
      pixbuf = gst_thumbnailer_capture_frame (play, width);
      if (pixbuf == NULL)
        continue;

      /* check if image is interesting or end of loop */
      if (n + 1 == G_N_ELEMENTS (offsets)
          || gst_thumbnailer_pixbuf_interesting (pixbuf))
        break;

      /* continue looking for something better */
      g_object_unref (pixbuf);
      pixbuf = NULL;
    }

  return pixbuf;
}



static GstBusSyncReply
gst_thumbnailer_error_handler (GstBus     *bus,
                               GstMessage *message,
                               gpointer    user_data)
{
  GCancellable *cancellable = user_data;

  switch (GST_MESSAGE_TYPE (message))
    {
      case GST_MESSAGE_ERROR:
      case GST_MESSAGE_EOS:
        /* stop */
        g_cancellable_cancel (cancellable);
        return GST_BUS_DROP;

      default:
        return GST_BUS_PASS;
    }
}



static gboolean
gst_thumbnailer_play_start (GstElement   *play,
                            GCancellable *cancellable)
{
  GstBus     *bus;
  gboolean    terminate = FALSE;
  GstMessage *message;
  gboolean    async_received = FALSE;

  /* pause to prepare for seeking */
  gst_element_set_state (play, GST_STATE_PAUSED);

  bus = gst_element_get_bus (play);

  while (!terminate
         && !g_cancellable_is_cancelled (cancellable))
    {
      message = gst_bus_timed_pop_filtered (bus,
                                            GST_CLOCK_TIME_NONE,
                                            GST_MESSAGE_ASYNC_DONE | GST_MESSAGE_ERROR);

      switch (GST_MESSAGE_TYPE (message))
        {
        case GST_MESSAGE_ASYNC_DONE:
          if (GST_MESSAGE_SRC (message) == GST_OBJECT (play))
            {
              async_received = TRUE;
              terminate = TRUE;
            }
          break;

        case GST_MESSAGE_ERROR:
          terminate = TRUE;
          break;

        default:
          break;
        }

      gst_message_unref (message);
    }

  /* setup the error handler */
  if (async_received)
    gst_bus_set_sync_handler (bus, gst_thumbnailer_error_handler, cancellable, NULL);

  gst_object_unref (bus);

  return async_received;
}



static GstElement *
gst_thumbnailer_play_init (TumblerFileInfo *info)
{
  GstElement *play;
  GstElement *audio_sink;
  GstElement *video_sink;

  /* prepare play factory */
  play = gst_element_factory_make ("playbin", "play");
  audio_sink = gst_element_factory_make ("fakesink", "audio-fake-sink");
  video_sink = gst_element_factory_make ("fakesink", "video-fake-sink");
  g_object_set (video_sink, "sync", TRUE, NULL);

  g_object_set (play,
                "uri", tumbler_file_info_get_uri (info),
                "audio-sink", audio_sink,
                "video-sink", video_sink,
                "flags", TUMBLER_GST_PLAY_FLAG_VIDEO | TUMBLER_GST_PLAY_FLAG_AUDIO,
                NULL);

  return play;
}



static void
gst_thumbnailer_create (TumblerAbstractThumbnailer *thumbnailer,
                        GCancellable               *cancellable,
                        TumblerFileInfo            *info)
{
  GstElement             *play;
  GdkPixbuf              *pixbuf = NULL;
  gint64                  duration;
  TumblerImageData        data;
  GError                 *error = NULL;
  TumblerThumbnail       *thumbnail;
  gint                    width, height;
  TumblerThumbnailFlavor *flavor;
  GdkPixbuf              *scaled;
  const gchar            *uri;

  /* check for early cancellation */
  if (g_cancellable_is_cancelled (cancellable))
    return;

  uri = tumbler_file_info_get_uri (info);
  g_debug ("Handling URI '%s'", uri);

  /* Check if is a sparse video file */
  if (tumbler_util_guess_is_sparse (info))
  {
    g_debug ("Video file '%s' is probably sparse, skipping", uri);

    /* there was an error, emit error signal */
    g_set_error (&error, TUMBLER_ERROR, TUMBLER_ERROR_NO_CONTENT,
                 TUMBLER_ERROR_MESSAGE_CREATION_FAILED);
    g_signal_emit_by_name (thumbnailer, "error", uri,
                           error->domain, error->code, error->message);
    g_error_free (error);

    return;
  }

  /* get size of dest thumb */
  thumbnail = tumbler_file_info_get_thumbnail (info);
  flavor = tumbler_thumbnail_get_flavor (thumbnail);
  tumbler_thumbnail_flavor_get_size (flavor, &width, &height);

  /* prepare factory */
  play = gst_thumbnailer_play_init (info);

  if (gst_thumbnailer_play_start (play, cancellable))
    {
      /* check for covers in the file */
      pixbuf = gst_thumbnailer_cover (play, cancellable);

      /* extract cover from video stream */
      if (pixbuf == NULL
          && gst_thumbnailer_has_video (play))
        {
          /* get the length of the video track */
          if (gst_element_query_duration (play, GST_FORMAT_TIME, &duration)
              && duration != -1)
            duration /= GST_MSECOND;
          else
            duration = -1;

          pixbuf = gst_thumbnailer_capture_interesting_frame (play, duration, width, cancellable);
        }
    }

  /* stop factory */
  gst_element_set_state (play, GST_STATE_NULL);
  g_object_unref (play);

  if (G_LIKELY (pixbuf != NULL))
    {
      /* scale to correct size if required */
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

      if (error != NULL)
        {
          g_signal_emit_by_name (thumbnailer, "error", uri,
                                 error->domain, error->code, error->message);
          g_error_free (error);
        }
      else
        {
          g_signal_emit_by_name (thumbnailer, "ready", uri);
        }
    }
  else
    {
      /* there was an error, emit error signal */
      if (! g_cancellable_set_error_if_cancelled (cancellable, &error))
        g_set_error (&error, TUMBLER_ERROR, TUMBLER_ERROR_NO_CONTENT,
                     TUMBLER_ERROR_MESSAGE_CREATION_FAILED);

      g_signal_emit_by_name (thumbnailer, "error", uri,
                             error->domain, error->code, error->message);
      g_error_free (error);
    }


  g_object_unref (thumbnail);
}
