/* vi:set et ai sw=2 sts=2 ts=2: */
/*-
 * Copyright (c) 2009 Jannis Pohlmann <jannis@xfce.org>
 * Copyright (c) 2011 Jérôme Guelfucci <jeromeg@xfce.org>
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
#include <glib/gi18n.h>
#include <glib-object.h>

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <math.h>
#include <tumbler/tumbler.h>
#include <webkit/webkit.h>

#include <webkit-thumbnailer/webkit-thumbnailer.h>

#define LOAD_TIMEOUT 1000


static void webkit_thumbnailer_finalize (GObject                    *object);
static void webkit_thumbnailer_create   (TumblerAbstractThumbnailer *thumbnailer,
                                         GCancellable               *cancellable,
                                         TumblerFileInfo            *info);



struct _WebkitThumbnailerClass
{
  TumblerAbstractThumbnailerClass __parent__;
};

struct _WebkitThumbnailer
{
  TumblerAbstractThumbnailer __parent__;

  GdkPixbuf *tmp;
  GtkWidget *offscreen;
  GtkWidget *view;
};



G_DEFINE_DYNAMIC_TYPE (WebkitThumbnailer,
                       webkit_thumbnailer,
                       TUMBLER_TYPE_ABSTRACT_THUMBNAILER);



void
webkit_thumbnailer_register (TumblerProviderPlugin *plugin)
{
  webkit_thumbnailer_register_type (G_TYPE_MODULE (plugin));
}



static void
webkit_thumbnailer_class_init (WebkitThumbnailerClass *klass)
{
  TumblerAbstractThumbnailerClass *abstractthumbnailer_class;
  GObjectClass                    *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->finalize = webkit_thumbnailer_finalize;

  abstractthumbnailer_class = TUMBLER_ABSTRACT_THUMBNAILER_CLASS (klass);
  abstractthumbnailer_class->create = webkit_thumbnailer_create;
}



static void
webkit_thumbnailer_class_finalize (WebkitThumbnailerClass *klass)
{
}



static void
cb_view_load_finished (GtkWidget         *web_view,
                       WebKitWebFrame    *web_frame,
                       WebkitThumbnailer *thumbnailer)
{
  gtk_widget_queue_draw (web_view);
  gdk_window_process_updates (gtk_widget_get_window (thumbnailer->offscreen),
                              TRUE);

  thumbnailer->tmp =
    gtk_offscreen_window_get_pixbuf (GTK_OFFSCREEN_WINDOW (thumbnailer->offscreen));

  gtk_main_quit ();
}



static void
webkit_thumbnailer_init (WebkitThumbnailer *thumbnailer)
{
  WebKitWebSettings *settings;

  gtk_init (NULL, NULL);

  thumbnailer->offscreen = gtk_offscreen_window_new ();
  thumbnailer->view = webkit_web_view_new ();
  thumbnailer->tmp = NULL;

  /* create a new websettings and disable potential threats */
  settings = webkit_web_settings_new ();

  g_object_set (G_OBJECT(settings),
                "enable-scripts", FALSE,
                "enable-plugins", FALSE,
                "enable-html5-database", FALSE,
                "enable-html5-local-storage", FALSE,
                "enable-java-applet", FALSE,
                NULL);

  /* apply the result to the web view */
  webkit_web_view_set_settings (WEBKIT_WEB_VIEW(thumbnailer->view),
                                settings);

  /* retrieve thumbnails once the page is loaded */
  g_signal_connect (thumbnailer->view,
                    "load-finished",
                    G_CALLBACK (cb_view_load_finished),
                    thumbnailer);

  gtk_container_add (GTK_CONTAINER (thumbnailer->offscreen),
                     thumbnailer->view);

  gtk_widget_set_size_request (thumbnailer->offscreen, 1024, 600);

  gtk_widget_show_all (thumbnailer->offscreen);
}



static void
webkit_thumbnailer_finalize (GObject *object)
{
  WebkitThumbnailer *thumbnailer = WEBKIT_THUMBNAILER (object);

  gtk_widget_destroy (thumbnailer->offscreen);

  (*G_OBJECT_CLASS (webkit_thumbnailer_parent_class)->finalize) (object);
}



static GdkPixbuf *
generate_pixbuf (GdkPixbuf *source,
                 gint       dest_width,
                 gint       dest_height)
{
  gdouble hratio;
  gdouble wratio;
  gint    source_height;
  gint    source_width;

  /* determine the source pixbuf dimensions */
  source_width  = gdk_pixbuf_get_width (source);
  source_height = gdk_pixbuf_get_height (source);

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
  return gdk_pixbuf_scale_simple (source, MAX (dest_width, 1), MAX (dest_height, 1),
                                  GDK_INTERP_BILINEAR);
}



static gboolean
cb_load_timeout (gpointer data)
{
  gtk_main_quit ();

  return FALSE;
}



static void
webkit_thumbnailer_create (TumblerAbstractThumbnailer *thumbnailer,
                           GCancellable               *cancellable,
                           TumblerFileInfo            *info)
{
  TumblerThumbnailFlavor *flavor;
  WebkitThumbnailer      *webkit_thumbnailer;
  TumblerImageData        data;
  TumblerThumbnail       *thumbnail;
  const gchar            *uri;
  GdkPixbuf              *pixbuf;
  GError                 *error = NULL;
  guint                   timeout_id;
  gint                    dest_height;
  gint                    dest_width;

  g_return_if_fail (IS_WEBKIT_THUMBNAILER (thumbnailer));
  g_return_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable));

  g_return_if_fail (TUMBLER_IS_FILE_INFO (info));

  webkit_thumbnailer = WEBKIT_THUMBNAILER (thumbnailer);

  /* do nothing if cancelled */
  if (g_cancellable_is_cancelled (cancellable))
    return;

  /* fetch required info */
  thumbnail = tumbler_file_info_get_thumbnail (info);
  g_assert (thumbnail != NULL);
  flavor = tumbler_thumbnail_get_flavor (thumbnail);
  tumbler_thumbnail_flavor_get_size (flavor, &dest_width, &dest_height);
  g_object_unref (flavor);

  uri = tumbler_file_info_get_uri (info);

  /* schedule a timeout to avoid waiting forever */
  timeout_id =
    g_timeout_add_seconds (LOAD_TIMEOUT, cb_load_timeout, NULL);

  /* load the page in the web view */
  webkit_web_view_load_uri (WEBKIT_WEB_VIEW (webkit_thumbnailer->view),
                            uri);

  /* wait until the page is loaded */
  gtk_main ();

  /* remove the timeout */
  g_source_remove (timeout_id);

  if (webkit_thumbnailer->tmp == NULL)
    {
      /* emit an error signal */
      g_signal_emit_by_name (thumbnailer, "error", uri, TUMBLER_ERROR_NO_CONTENT,
                             _("Failed to retrieve the content of this HMTL file"));

      /* clean up */
      g_object_unref (thumbnail);

      return;
    }

  /* generate a valid thumbnail */
  pixbuf = generate_pixbuf (webkit_thumbnailer->tmp, dest_width, dest_height);

  g_assert (pixbuf != NULL);

  /* compose the image data */
  data.data = gdk_pixbuf_get_pixels (pixbuf);
  data.has_alpha = gdk_pixbuf_get_has_alpha (pixbuf);
  data.bits_per_sample = gdk_pixbuf_get_bits_per_sample (pixbuf);
  data.width = gdk_pixbuf_get_width (pixbuf);
  data.height = gdk_pixbuf_get_height (pixbuf);
  data.rowstride = gdk_pixbuf_get_rowstride (pixbuf);
  data.colorspace = (TumblerColorspace) gdk_pixbuf_get_colorspace (pixbuf);

  /* save the thumbnail */
  tumbler_thumbnail_save_image_data (thumbnail, &data,
                                     tumbler_file_info_get_mtime (info),
                                     NULL, &error);

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
  g_object_unref (thumbnail);
  g_object_unref (pixbuf);
  g_object_unref (webkit_thumbnailer->tmp);
  webkit_thumbnailer->tmp = NULL;
}
