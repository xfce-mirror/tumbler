/* vi:set et ai sw=2 sts=2 ts=2: */
/*
 * Copyright (c) 2017 Ali Abdallah <ali@xfce.org>
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

#include <gio/gio.h>
#include <glib/gstdio.h>

#include <tumbler/tumbler.h>

#include <desktop-thumbnailer/desktop-thumbnailer.h>



static void desktop_thumbnailer_create   (TumblerAbstractThumbnailer *thumbnailer,
                                          GCancellable               *cancellable,
                                          TumblerFileInfo            *info);

static void desktop_thumbnailer_get_property(GObject *object,
                                             guint prop_id,
                                             GValue *value,
                                             GParamSpec *pspec);

static void desktop_thumbnailer_set_property(GObject *object,
                                             guint prop_id,
                                             const GValue *value,
                                             GParamSpec *pspec);

static void desktop_thumbnailer_finalize(GObject *object);



struct _DesktopThumbnailerClass
{
  TumblerAbstractThumbnailerClass __parent__;
};

struct _DesktopThumbnailer
{
  TumblerAbstractThumbnailer __parent__;

  gchar *exec;
};



G_DEFINE_DYNAMIC_TYPE (DesktopThumbnailer,
                       desktop_thumbnailer,
                       TUMBLER_TYPE_ABSTRACT_THUMBNAILER);


enum
{
  PROP_0,
  PROP_EXEC
};



void
desktop_thumbnailer_register (TumblerProviderPlugin *plugin)
{
  desktop_thumbnailer_register_type (G_TYPE_MODULE (plugin));
}



static void desktop_thumbnailer_get_property(GObject *object,
                                             guint prop_id,
                                             GValue *value,
                                             GParamSpec *pspec)
{
  DesktopThumbnailer *thumbnailer = DESKTOP_THUMBNAILER(object);

  switch(prop_id)
    {
    case PROP_EXEC:
      g_value_set_string(value, thumbnailer->exec);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
      break;
    }
}



static void desktop_thumbnailer_set_property(GObject *object,
                                             guint prop_id,
                                             const GValue *value,
                                             GParamSpec *pspec)
{
  DesktopThumbnailer *thumbnailer = DESKTOP_THUMBNAILER(object);

  switch(prop_id)
    {
    case PROP_EXEC:
      thumbnailer->exec = g_strdup(g_value_get_string(value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
      break;
    }
}



static void
desktop_thumbnailer_class_init (DesktopThumbnailerClass *klass)
{
  TumblerAbstractThumbnailerClass *abstractthumbnailer_class;
  GObjectClass                    *gobject_class;

  gobject_class = G_OBJECT_CLASS(klass);

  gobject_class->get_property = desktop_thumbnailer_get_property;
  gobject_class->set_property = desktop_thumbnailer_set_property;

  g_object_class_install_property (gobject_class,
                                   PROP_EXEC,
                                   g_param_spec_string("exec",
                                                       NULL,
                                                       NULL,
                                                       NULL,
                                                       G_PARAM_CONSTRUCT_ONLY |
                                                       G_PARAM_READWRITE));

  gobject_class->finalize = desktop_thumbnailer_finalize;
  abstractthumbnailer_class = TUMBLER_ABSTRACT_THUMBNAILER_CLASS (klass);
  abstractthumbnailer_class->create = desktop_thumbnailer_create;
}



static void
desktop_thumbnailer_class_finalize (DesktopThumbnailerClass *klass)
{
}


static void
desktop_thumbnailer_finalize(GObject *object)
{
  DesktopThumbnailer *thumbnailer = DESKTOP_THUMBNAILER (object);

  g_free (thumbnailer->exec);

  G_OBJECT_CLASS (desktop_thumbnailer_parent_class)->finalize (object);
}

static void
desktop_thumbnailer_init (DesktopThumbnailer *thumbnailer)
{
}


static void
te_string_append_quoted (GString     *string,
                         const gchar *unquoted)
{
  gchar *quoted;

  quoted = g_shell_quote (unquoted);
  g_string_append (string, quoted);
  g_free (quoted);
}



static gboolean
desktop_thumbnailer_exec_parse (const gchar *exec,
                                const gchar *file_path,
                                const gchar *file_uri,
                                gint         desired_size,
                                const gchar *output_file,
                                gint        *argc,
                                gchar     ***argv,
                                GError     **error)
{
  const gchar *p;
  gboolean     result = FALSE;
  GString     *command_line = g_string_new (NULL);

  for (p = exec; *p != '\0'; ++p)
    {
      if (p[0] == '%' && p[1] != '\0')
        {
          switch (*++p)
            {
            case 'u':
              if (G_LIKELY (file_uri != NULL))
                te_string_append_quoted (command_line, file_uri);
              break;

            case 'i':
              if (G_LIKELY (file_path != NULL))
                te_string_append_quoted (command_line, file_path);
              break;

            case 's':
              g_string_append_printf(command_line, "%d", desired_size);
              break;

            case 'o':
              if (G_LIKELY (output_file != NULL))
                te_string_append_quoted (command_line, output_file);
              break;

            case '%':
              g_string_append_c (command_line, '%');
              break;
            }
        }
      else
        {
          g_string_append_c (command_line, *p);
        }
    }

  result = g_shell_parse_argv (command_line->str, argc, argv, error);

  g_string_free (command_line, TRUE);
  return result;
}



static GdkPixbuf *
desktop_thumbnailer_load_thumbnail (DesktopThumbnailer *thumbnailer,
                                    const gchar        *uri,
                                    const gchar        *path,
                                    gint                width,
                                    gint                height,
                                    GCancellable       *cancellable,
                                    GError            **error)
{
  GFileIOStream *stream;
  GFile     *tmpfile;
  gchar     *exec, *std_err;
  gchar    **cmd_argv;
  const gchar *tmpfilepath;
  gboolean   res;
  gint       cmd_argc;
  gint       size;
  gchar     *working_directory = NULL;
  GdkPixbuf *source, *pixbuf = NULL;
  gboolean verbose;

  if (path != NULL && g_str_has_prefix (path, g_get_tmp_dir ())
      && g_regex_match_simple ("/tumbler-.{6}\\.png$", path, 0, 0))
    {
      g_set_error (error, TUMBLER_ERROR, TUMBLER_ERROR_IS_THUMBNAIL,
                   TUMBLER_ERROR_MESSAGE_NO_THUMB_OF_THUMB, path);
      return NULL;
    }

  tmpfile = g_file_new_tmp ("tumbler-XXXXXX.png", &stream, error);

  if ( G_LIKELY (tmpfile) )
    {
      tmpfilepath = g_file_peek_path (tmpfile);

      size = MIN (width, height);

      g_object_get (G_OBJECT (thumbnailer), "exec", &exec, NULL);
      res = desktop_thumbnailer_exec_parse (exec,
                                            path,
                                            uri,
                                            size,
                                            tmpfilepath,
                                            &cmd_argc,
                                            &cmd_argv,
                                            error);

      if (G_LIKELY (res))
        {
          if (path != NULL)
            working_directory = g_path_get_dirname (path);

          verbose = tumbler_util_is_debug_logging_enabled (G_LOG_DOMAIN);
          res = g_spawn_sync (working_directory,
                              cmd_argv, NULL,
                              verbose ? G_SPAWN_SEARCH_PATH
                                      : G_SPAWN_SEARCH_PATH | G_SPAWN_STDERR_TO_DEV_NULL,
                              NULL, NULL,
                              NULL, verbose ? &std_err : NULL,
                              NULL,
                              error);
          if (verbose)
            {
              g_printerr ("%s", std_err);
              g_free (std_err);
            }

          if (G_LIKELY (res))
            {
              source = gdk_pixbuf_new_from_stream (g_io_stream_get_input_stream (G_IO_STREAM (stream)),
                                                   cancellable, error);
              if (source != NULL)
                {
                  pixbuf = tumbler_util_scale_pixbuf (source, width, height);
                  g_object_unref (source);
                }
            }

          g_free (working_directory);
          g_strfreev (cmd_argv);
        }
      else
          g_warning ("Malformed command line \"%s\": %s", exec, (*error)->message);

      g_file_delete (tmpfile, NULL, NULL);
      g_object_unref (tmpfile);
      g_object_unref (stream);
      g_free (exec);
    }

  return pixbuf;
}



static void
desktop_thumbnailer_create (TumblerAbstractThumbnailer *thumbnailer,
                            GCancellable               *cancellable,
                            TumblerFileInfo            *info)
{

  TumblerThumbnail           *thumbnail;
  TumblerThumbnailFlavor     *flavor;
  TumblerImageData            data;
  const gchar                *uri;
  GFile                      *file;
  gint                        height;
  gint                        width;
  GError                     *error  =  NULL;
  GdkPixbuf                  *pixbuf =  NULL;

  g_return_if_fail (IS_DESKTOP_THUMBNAILER (thumbnailer));
  g_return_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable));
  g_return_if_fail (TUMBLER_IS_FILE_INFO (info));

  /* do nothing if cancelled */
  if (g_cancellable_is_cancelled (cancellable))
    return;

  uri = tumbler_file_info_get_uri (info);
  file = g_file_new_for_uri (uri);

  g_debug ("'Exec=%s': Handling URI '%s'", DESKTOP_THUMBNAILER (thumbnailer)->exec, uri);

  thumbnail = tumbler_file_info_get_thumbnail (info);
  g_assert (thumbnail != NULL);

  flavor = tumbler_thumbnail_get_flavor (thumbnail);
  g_assert (flavor != NULL);

  tumbler_thumbnail_flavor_get_size (flavor, &width, &height);

  pixbuf = desktop_thumbnailer_load_thumbnail (DESKTOP_THUMBNAILER (thumbnailer),
                                               uri, g_file_peek_path (file),
                                               width, height, cancellable, &error);

  if (pixbuf != NULL)
    {
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
      g_signal_emit_by_name (thumbnailer, "error", uri,
                             error->domain, error->code, error->message);
      g_error_free (error);
    }
  else
    {
      g_signal_emit_by_name (thumbnailer, "ready", uri);
    }

  g_object_unref (thumbnail);
  g_object_unref (flavor);
  g_object_unref (file);
}
