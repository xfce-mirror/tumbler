/* vi:set et ai sw=2 sts=2 ts=2: */
/*
 * Originally from Bickley - a meta data management framework.
 * Copyright © 2008, 2011 Intel Corporation.
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

#include "config.h"
#include <string.h>

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gst/gst.h>
#include <tumbler/tumbler.h>
#include "gst-helper.h"

static void
push_buffer (GstElement *element,
             GstBuffer  *out_buffer,
             GstPad     *pad,
             GstBuffer  *in_buffer)
{
  gst_buffer_set_caps (out_buffer, GST_BUFFER_CAPS (in_buffer));
  GST_BUFFER_SIZE (out_buffer) = GST_BUFFER_SIZE (in_buffer);
  memcpy (GST_BUFFER_DATA (out_buffer), GST_BUFFER_DATA (in_buffer),
          GST_BUFFER_SIZE (in_buffer));
}

static void
pull_buffer (GstElement *element,
             GstBuffer  *in_buffer,
             GstPad     *pad,
             GstBuffer **out_buffer)
{
  *out_buffer = gst_buffer_ref (in_buffer);
}

GdkPixbuf *
gst_helper_convert_buffer_to_pixbuf (GstBuffer    *buffer,
                                     GCancellable *cancellable,
                                     TumblerThumbnailFlavor *flavour)
{
  GstCaps *pb_caps;
  GstElement *pipeline;
  GstBuffer *out_buffer = NULL;
  GstElement *src, *sink, *colorspace, *scale, *filter;
  GstBus *bus;
  GstMessage *msg;
  gboolean ret;
  int thumb_size = 0, width, height, dw, dh, i;
  GstStructure *s;

  /* TODO: get the width and height, and handle them being different when
     scaling */
  tumbler_thumbnail_flavor_get_size (flavour, &thumb_size, NULL);

  s = gst_caps_get_structure (GST_BUFFER_CAPS (buffer), 0);
  gst_structure_get_int (s, "width", &width);
  gst_structure_get_int (s, "height", &height);

  if (width > height)
    {
      double ratio;

      ratio = (double) thumb_size / (double) width;
      dw = thumb_size;
      dh = height * ratio;
    }
  else
    {
      double ratio;

      ratio = (double) thumb_size / (double) height;
      dh = thumb_size;
      dw = width * ratio;
    }

  pb_caps = gst_caps_new_simple ("video/x-raw-rgb",
                                 "bpp", G_TYPE_INT, 24,
                                 "depth", G_TYPE_INT, 24,
                                 "width", G_TYPE_INT, dw,
                                 "height", G_TYPE_INT, dh,
                                 "pixel-aspect-ratio", GST_TYPE_FRACTION, 1, 1,
                                 NULL);

  pipeline = gst_pipeline_new ("pipeline");

  src = gst_element_factory_make ("fakesrc", "src");
  colorspace = gst_element_factory_make ("ffmpegcolorspace", "colorspace");
  scale = gst_element_factory_make ("videoscale", "scale");
  filter = gst_element_factory_make ("capsfilter", "filter");
  sink = gst_element_factory_make ("fakesink", "sink");

  gst_bin_add_many (GST_BIN (pipeline), src, colorspace, scale,
                    filter, sink, NULL);

  g_object_set (filter,
                "caps", pb_caps,
                NULL);
  g_object_set (src,
                "num-buffers", 1,
                "sizetype", 2,
                "sizemax", GST_BUFFER_SIZE (buffer),
                "signal-handoffs", TRUE,
                NULL);
  g_signal_connect (src, "handoff",
                    G_CALLBACK (push_buffer), buffer);

  g_object_set (sink,
                "signal-handoffs", TRUE,
                "preroll-queue-len", 1,
                NULL);
  g_signal_connect (sink, "handoff",
                    G_CALLBACK (pull_buffer), &out_buffer);

  ret = gst_element_link (src, colorspace);
  if (ret == FALSE)
    {
      g_warning ("Failed to link src->colorspace");
      return NULL;
    }

  ret = gst_element_link (colorspace, scale);
  if (ret == FALSE)
    {
      g_warning ("Failed to link colorspace->scale");
      return NULL;
    }

  ret = gst_element_link (scale, filter);
  if (ret == FALSE)
    {
      g_warning ("Failed to link scale->filter");
      return NULL;
    }

  ret = gst_element_link (filter, sink);
  if (ret == FALSE)
    {
      g_warning ("Failed to link filter->sink");
      return NULL;
    }

  bus = gst_element_get_bus (GST_ELEMENT (pipeline));

  gst_element_set_state (GST_ELEMENT (pipeline), GST_STATE_PLAYING);

  i = 0;
  msg = NULL;
  while (msg == NULL && i < 5)
    {
      msg = gst_bus_timed_pop_filtered (bus, GST_SECOND,
                                        GST_MESSAGE_ERROR | GST_MESSAGE_EOS);
      i++;
    }

  /* FIXME: Notify about error? */
  gst_message_unref (msg);

  gst_caps_unref (pb_caps);

  if (out_buffer)
    {
      GdkPixbuf *pixbuf;
      char *data;

      data = g_memdup (GST_BUFFER_DATA (out_buffer),
                       GST_BUFFER_SIZE (out_buffer));
      pixbuf = gdk_pixbuf_new_from_data ((guchar *) data,
                                         GDK_COLORSPACE_RGB, FALSE,
                                         8, dw, dh, GST_ROUND_UP_4 (dw * 3),
                                         (GdkPixbufDestroyNotify) g_free,
                                         NULL);

      gst_buffer_unref (buffer);
      return pixbuf;
    }

  /* FIXME: Check what buffers need freed */
  return NULL;
}
