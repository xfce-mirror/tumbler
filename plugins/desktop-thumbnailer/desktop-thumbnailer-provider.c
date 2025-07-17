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

#include "desktop-thumbnailer-provider.h"
#include "desktop-thumbnailer.h"

#include <glib/gi18n.h>



static void
desktop_thumbnailer_provider_thumbnailer_provider_init (TumblerThumbnailerProviderIface *iface);
static GList *
desktop_thumbnailer_provider_get_thumbnailers (TumblerThumbnailerProvider *provider);



struct _DesktopThumbnailerProvider
{
  GObject __parent__;
};

typedef struct
{
  gint priority;
  GSList *locations;
  GSList *excludes;
  gint64 max_file_size;
} TumblerThumbnailerSettings;



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
                                           GStrv uri_schemes,
                                           TumblerThumbnailerSettings *settings)
{
  DesktopThumbnailer *thumbnailer;
  GKeyFile *rc;
  GSList *locations;
  GSList *excludes;
  GError *error = NULL;
  gchar **mime_types, **paths;
  const gchar *filename;
  gchar *exec = NULL;
  gint64 max_file_size;
  gint priority;

  g_return_val_if_fail (G_IS_FILE (file), NULL);

  /* determine the absolute filename of the input file */
  filename = g_file_peek_path (file);

  /* allocate a new key file object */
  rc = g_key_file_new ();

  /* try to load the mandatory key file data from the input file */
  if (!g_key_file_load_from_file (rc, filename, G_KEY_FILE_NONE, &error)
      || (exec = g_key_file_get_string (rc, "Thumbnailer Entry", "Exec", &error)) == NULL
      || (mime_types = g_key_file_get_string_list (rc, "Thumbnailer Entry", "MimeType", NULL, &error)) == NULL)
    {
      g_warning (TUMBLER_WARNING_LOAD_FILE_FAILED, filename, error->message);
      g_error_free (error);
      g_key_file_free (rc);
      g_free (exec);

      return NULL;
    }

  /* return if disabled */
  if (g_key_file_get_boolean (rc, "X-Tumbler Settings", "Disabled", &error) && error == NULL)
    return NULL;
  else if (error != NULL)
    g_clear_error (&error);

  /* try to load the settings */
  priority = g_key_file_get_integer (rc, "X-Tumbler Settings", "Priority", &error);
  if (error != NULL)
    {
      priority = settings->priority;
      g_clear_error (&error);
    }

  max_file_size = g_key_file_get_int64 (rc, "X-Tumbler Settings", "MaxFileSize", &error);
  if (error != NULL)
    {
      max_file_size = settings->max_file_size;
      g_clear_error (&error);
    }

  paths = g_key_file_get_string_list (rc, "X-Tumbler Settings", "Locations", NULL, &error);
  if (error != NULL)
    {
      locations = g_slist_copy_deep (settings->locations, tumbler_util_object_ref, NULL);
      g_clear_error (&error);
    }
  else
    {
      locations = tumbler_util_locations_from_strv (paths);
      g_strfreev (paths);
    }

  paths = g_key_file_get_string_list (rc, "X-Tumbler Settings", "Excludes", NULL, &error);
  if (error != NULL)
    {
      excludes = g_slist_copy_deep (settings->excludes, tumbler_util_object_ref, NULL);
      g_clear_error (&error);
    }
  else
    {
      excludes = tumbler_util_locations_from_strv (paths);
      g_strfreev (paths);
    }

  thumbnailer = g_object_new (DESKTOP_TYPE_THUMBNAILER, "uri-schemes", uri_schemes,
                              "mime-types", mime_types, "priority", priority,
                              "max-file-size", max_file_size, "locations", locations,
                              "excludes", excludes, "exec", exec, NULL);

  g_debug ("Registered thumbnailer '%s'", filename);
  tumbler_util_dump_strv (G_LOG_DOMAIN, "Supported mime types",
                          (const gchar *const *) mime_types);

  g_key_file_free (rc);
  g_strfreev (mime_types);
  g_free (exec);
  g_slist_free_full (locations, g_object_unref);
  g_slist_free_full (excludes, g_object_unref);

  return thumbnailer;
}

static GList *
desktop_thumbnailer_get_thumbnailers_from_dir (GList *thumbnailers,
                                               GFile *directory,
                                               GStrv uri_schemes,
                                               TumblerThumbnailerSettings *settings,
                                               GHashTable **single_name)
{
  const gchar *base_name;
  gchar *name;
  GDir *dir;

  /* try to open the directory for reading */
  dir = g_dir_open (g_file_peek_path (directory), 0, NULL);
  if (dir == NULL)
    return thumbnailers;

  /* iterate over all files in the directory */
  for (base_name = g_dir_read_name (dir);
       base_name != NULL;
       base_name = g_dir_read_name (dir))
    {
      GFileType type;
      GFile *file;
      DesktopThumbnailer *thumbnailer = NULL;

      /* skip files that don't end with the .thumbnailer extension or already added */
      if (!g_str_has_suffix (base_name, ".thumbnailer")
          || g_hash_table_lookup (*single_name, base_name))
        continue;

      file = g_file_get_child (directory, base_name);
      type = g_file_query_file_type (file, G_FILE_QUERY_INFO_NONE, NULL);

      /* try to load the file if it is regular */
      if (type == G_FILE_TYPE_REGULAR)
        thumbnailer = desktop_thumbnailer_get_from_desktop_file (file, uri_schemes, settings);

      g_object_unref (file);

      if (thumbnailer)
        {
          thumbnailers = g_list_prepend (thumbnailers, thumbnailer);
          name = g_strdup (base_name);
          g_hash_table_insert (*single_name, name, name);
        }
    }

  g_dir_close (dir);

  return thumbnailers;
}

static GList *
desktop_thumbnailer_provider_get_thumbnailers (TumblerThumbnailerProvider *provider)
{
  TumblerThumbnailerSettings *settings;
  GHashTable *single_name;
  GKeyFile *rc;
  GList *directories, *iter, *thumbnailers = NULL;
  GStrv uri_schemes, paths;
  const gchar *type = "DesktopThumbnailer";

  uri_schemes = tumbler_util_get_supported_uri_schemes ();
  directories = tumbler_util_get_thumbnailer_dirs ();

  tumbler_util_dump_strv (G_LOG_DOMAIN, "Supported URI schemes",
                          (const gchar *const *) uri_schemes);

  /* get settings from rc file */
  rc = tumbler_util_get_settings ();
  settings = g_new (TumblerThumbnailerSettings, 1);

  settings->priority = g_key_file_get_integer (rc, type, "Priority", NULL);
  settings->max_file_size = g_key_file_get_int64 (rc, type, "MaxFileSize", NULL);

  paths = g_key_file_get_string_list (rc, type, "Locations", NULL, NULL);
  settings->locations = tumbler_util_locations_from_strv (paths);
  g_strfreev (paths);

  paths = g_key_file_get_string_list (rc, type, "Excludes", NULL, NULL);
  settings->excludes = tumbler_util_locations_from_strv (paths);
  g_strfreev (paths);

  /* use a ghash table to avoid duplication and allow for thumbnailer override */
  single_name = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);

  /* thumbnailers end up in reverse order here, since they are prepended, but the list must
   * not be reversed: this will happen during sorted insertion in tumbler_registry_add() */
  for (iter = directories; iter != NULL; iter = iter->next)
    thumbnailers = desktop_thumbnailer_get_thumbnailers_from_dir (thumbnailers, iter->data,
                                                                  uri_schemes, settings,
                                                                  &single_name);

  g_strfreev (uri_schemes);
  g_list_free_full (directories, g_object_unref);
  g_hash_table_destroy (single_name);
  g_key_file_free (rc);
  g_slist_free_full (settings->locations, g_object_unref);
  g_slist_free_full (settings->excludes, g_object_unref);
  g_free (settings);

  return thumbnailers;
}
