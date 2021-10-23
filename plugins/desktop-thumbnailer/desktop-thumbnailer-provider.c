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
#include <glib-object.h>

#include <glib/gi18n.h>

#include <tumbler/tumbler.h>

#include <desktop-thumbnailer/desktop-thumbnailer-provider.h>
#include <desktop-thumbnailer/desktop-thumbnailer.h>



static void   desktop_thumbnailer_provider_thumbnailer_provider_init (TumblerThumbnailerProviderIface *iface);
static GList *desktop_thumbnailer_provider_get_thumbnailers          (TumblerThumbnailerProvider      *provider);



struct _DesktopThumbnailerProviderClass
{
  GObjectClass __parent__;
};

struct _DesktopThumbnailerProvider
{
  GObject __parent__;
};



G_DEFINE_DYNAMIC_TYPE_EXTENDED (DesktopThumbnailerProvider,
                                desktop_thumbnailer_provider,
                                G_TYPE_OBJECT,
                                0,
                                G_IMPLEMENT_INTERFACE_DYNAMIC (TUMBLER_TYPE_THUMBNAILER_PROVIDER,
                                                               desktop_thumbnailer_provider_thumbnailer_provider_init));



void
desktop_thumbnailer_provider_register (TumblerProviderPlugin *plugin)
{
  desktop_thumbnailer_provider_register_type (G_TYPE_MODULE (plugin));
}



static void
desktop_thumbnailer_provider_class_init (DesktopThumbnailerProviderClass *klass)
{
}



static void
desktop_thumbnailer_provider_class_finalize (DesktopThumbnailerProviderClass *klass)
{
}



static void
desktop_thumbnailer_provider_thumbnailer_provider_init (TumblerThumbnailerProviderIface *iface)
{
  iface->get_thumbnailers = desktop_thumbnailer_provider_get_thumbnailers;
}



static void
desktop_thumbnailer_provider_init (DesktopThumbnailerProvider *provider)
{
}

static DesktopThumbnailer *
desktop_thumbnailer_get_from_desktop_file (GFile *file,
                                           GStrv  uri_schemes)
{
  DesktopThumbnailer *thumbnailer;
  GKeyFile           *key_file;
  GError             *error = NULL;
  gchar              *filename;
  gchar              *exec;
  gchar             **mime_types;

  g_return_val_if_fail (G_IS_FILE (file), NULL);

  /* determine the absolute filename of the input file */
  filename = g_file_get_path (file);

  /* allocate a new key file object */
  key_file = g_key_file_new ();

  /* try to load the key file data from the input file */
  if (!g_key_file_load_from_file (key_file, filename, G_KEY_FILE_NONE, &error))
    {
      g_warning (_("Failed to load the file \"%s\": %s"), filename, error->message);
      g_clear_error (&error);

      g_key_file_free (key_file);
      g_free (filename);

      return NULL;
    }

  /* determine the Exec of the desktop thumbnailer */
  exec = g_key_file_get_string (key_file, "Thumbnailer Entry",
                                "Exec", &error);
  if (exec == NULL)
    {
      g_warning (_("Malformed file \"%s\": %s"), filename, error->message);
      g_clear_error (&error);

      g_key_file_free (key_file);
      g_free (filename);

      return NULL;
    }

  /* determine the MIME types supported by this thumbnailer */
  mime_types = g_key_file_get_string_list (key_file, "Thumbnailer Entry",
                                           "MimeType", NULL, &error);
  if (mime_types == NULL)
    {
      g_warning (_("Malformed file \"%s\": %s"), filename, error->message);
      g_clear_error (&error);

      g_free (exec);
      g_key_file_free (key_file);
      g_free (filename);

      return NULL;
    }

  thumbnailer = g_object_new (TYPE_DESKTOP_THUMBNAILER,
                              "uri-schemes", uri_schemes,
                              "mime-types", mime_types,
                              "exec", exec,
                              NULL);
  g_key_file_free (key_file);
  g_strfreev(mime_types);

  g_print("Registered thumbnailer %s\n", exec);
  g_free(exec);

  return thumbnailer;
}

static GList *
desktop_thumbnailer_get_thumbnailers_from_dir (GList *thumbnailers,
                                               GFile  *directory,
                                               GStrv   uri_schemes)
{
  const gchar *base_name;
  gchar       *dirname;
  GDir        *dir;

  /* determine the absolute path to the directory */
  dirname = g_file_get_path (directory);

  /* try to open the directory for reading */
  dir = g_dir_open (dirname, 0, NULL);
  if (dir == NULL)
    {
      g_free (dirname);
      return thumbnailers;
    }

  /* iterate over all files in the directory */
  for (base_name = g_dir_read_name (dir);
       base_name != NULL;
       base_name = g_dir_read_name (dir))
    {
      GFileType    type;
      GFile       *file;
      DesktopThumbnailer *thumbnailer = NULL;

      /* skip files that don't end with the .thumbnailer extension */
      if (!g_str_has_suffix (base_name, ".thumbnailer"))
        continue;

      file = g_file_get_child (directory, base_name);
      type = g_file_query_file_type (file, G_FILE_QUERY_INFO_NONE, NULL);

      /* try to load the file if it is regular */
      if (type == G_FILE_TYPE_REGULAR)
        thumbnailer = desktop_thumbnailer_get_from_desktop_file (file, uri_schemes);

      g_object_unref (file);

      if (thumbnailer)
        {

          thumbnailers = g_list_append (thumbnailers, thumbnailer);
        }
    }

  g_dir_close(dir);

  return thumbnailers;
}

static GList *
desktop_thumbnailer_provider_get_thumbnailers (TumblerThumbnailerProvider *provider)
{
  GHashTable         *single_path;
  const gchar *const *data_dirs;
  gchar              *dirname;
  GStrv               uri_schemes;
  GList              *iter;
  int                 n;
  GList              *thumbnailers = NULL;
  GList              *directories = NULL;

  uri_schemes = tumbler_util_get_supported_uri_schemes ();

  /* prepend $XDG_DATA_HOME/thumbnailers/ to the directory list */
  dirname = g_build_filename (g_get_user_data_dir (), "thumbnailers", NULL);
  directories = g_list_prepend (directories, g_file_new_for_path (dirname));
  g_free (dirname);

  /* build $XDG_DATA_DIRS/thumbnailers dirnames and prepend them to the list */
  data_dirs = g_get_system_data_dirs ();

  /* Create a ghash table to insert loaded directory path to avoid duplication */
  single_path = g_hash_table_new_full (g_file_hash,
                                       (GEqualFunc)g_file_equal,
                                       g_object_unref,
                                       NULL);

  for (n = 0; data_dirs[n] != NULL; ++n)
    {
      GFile *path;

      path = g_file_new_for_path(data_dirs[n]);

      if (!g_hash_table_lookup (single_path, path))
        {
          dirname = g_build_filename (data_dirs[n], "thumbnailers", NULL);
          directories = g_list_prepend (directories, g_file_new_for_path (dirname));
          g_hash_table_insert (single_path, path, path);
          g_free (dirname);
        }
      else
        {
          /* Free the path GFile object */
          g_object_unref(path);
        }
    }
  /* destroy the hash table used for loading single pathes */
  g_hash_table_destroy (single_path);

  /* reverse the directory list so that the directories with highest
   * priority come first */
  directories = g_list_reverse (directories);

  for (iter = directories; iter != NULL; iter = iter->next)
    {
      thumbnailers = desktop_thumbnailer_get_thumbnailers_from_dir (thumbnailers, iter->data, uri_schemes);
    }

  g_strfreev (uri_schemes);
  return thumbnailers;
}
