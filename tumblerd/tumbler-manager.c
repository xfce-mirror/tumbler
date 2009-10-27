/* vi:set et ai sw=2 sts=2 ts=2: */
/*-
 * Copyright (c) 2009 Jannis Pohlmann <jannis@xfce.org>
 *
 * This program is free software; you can redistribute it and/or 
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of 
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public 
 * License along with this program; if not, write to the Free 
 * Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif

#include <glib.h>
#include <glib-object.h>
#include <glib/gi18n.h>
#include <glib/gstdio.h>

#include <gio/gio.h>

#include <dbus/dbus.h>
#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-lowlevel.h>

#include <tumblerd/tumbler-manager.h>
#include <tumblerd/tumbler-manager-dbus-bindings.h>
#include <tumblerd/tumbler-specialized-thumbnailer.h>
#include <tumblerd/tumbler-utils.h>



/* Property identifiers */
enum
{
  PROP_0,
  PROP_CONNECTION,
  PROP_REGISTRY,
};



typedef struct _OverrideInfo    OverrideInfo;
typedef struct _ThumbnailerInfo ThumbnailerInfo;



static void             tumbler_manager_finalize          (GObject          *object);
static void             tumbler_manager_get_property      (GObject          *object,
                                                           guint             prop_id,
                                                           GValue           *value,
                                                           GParamSpec       *pspec);
static void             tumbler_manager_set_property      (GObject          *object,
                                                           guint             prop_id,
                                                           const GValue     *value,
                                                           GParamSpec       *pspec);
static void             tumbler_manager_monitor_unref     (GFileMonitor     *monitor,
                                                           TumblerManager   *manager);
static void             tumbler_manager_load_thumbnailers (TumblerManager   *manager,
                                                           GFile            *directory);
static void             tumbler_manager_directory_changed (TumblerManager   *manager,
                                                           GFile            *file,
                                                           GFile            *other_file,
                                                           GFileMonitorEvent event_type,
                                                           GFileMonitor     *monitor);

static OverrideInfo    *override_info_new                 (void);
static void             override_info_free                (gpointer          pointer);
static void             override_info_list_free           (gpointer          pointer);
static ThumbnailerInfo *thumbnailer_info_new              (void);
static void             thumbnailer_info_free             (ThumbnailerInfo  *info);
static void             thumbnailer_info_list_free        (gpointer          pointer);

#ifdef DEBUG
static void             dump_overrides                    (TumblerManager   *manager);
static void             dump_thumbnailers                 (TumblerManager   *manager);
#endif



struct _TumblerManagerClass
{
  GObjectClass __parent__;
};

struct _TumblerManager
{
  GObject __parent__;

  DBusGConnection *connection;
  TumblerRegistry *registry;

  /* Directory and monitor objects for the thumbnailer dirs in 
   * XDG_DATA_HOME and XDG_DATA_DIRS */
  GList           *directories;
  GList           *monitors;

  /* hash table for override information, mapping hash keys to lists
   * of override infos. in each of these lists the infos are sorted by 
   * the directory index, with higher priority directories (smaller 
   * directory index) coming first */
  GHashTable      *overrides;

  /* hash table for thumbnailer service information, mapping .service 
   * basenames to lists of thumbnailer infos installed into the
   * system with these basenames. in each of these lists the infos are
   * sorted by the directory index, with higher priority directories 
   * (smaller directory index) coming first */
  GHashTable      *thumbnailers;

  GMutex          *mutex;
};

struct _OverrideInfo
{
  gchar *name;
  gchar *uri_scheme;
  gchar *mime_type;
  gint   dir_index;
};

struct _ThumbnailerInfo
{
  TumblerThumbnailer *thumbnailer;
#ifdef DEBUG
  GFile              *file;
#endif
  gint                dir_index;
};



G_DEFINE_TYPE (TumblerManager, tumbler_manager, G_TYPE_OBJECT);



static void
tumbler_manager_class_init (TumblerManagerClass *klass)
{
  GObjectClass *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->finalize = tumbler_manager_finalize; 
  gobject_class->get_property = tumbler_manager_get_property;
  gobject_class->set_property = tumbler_manager_set_property;

  g_object_class_install_property (gobject_class, PROP_CONNECTION,
                                   g_param_spec_pointer ("connection",
                                                         "connection",
                                                         "connection",
                                                         G_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (gobject_class, PROP_REGISTRY,
                                   g_param_spec_object ("registry",
                                                        "registry",
                                                        "registry",
                                                        TUMBLER_TYPE_REGISTRY,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));
}



static void
tumbler_manager_init (TumblerManager *manager)
{
  manager->directories = NULL;
  manager->monitors = NULL;
  manager->mutex = g_mutex_new ();

  /* create the overrides info hash table */
  manager->overrides = g_hash_table_new_full (g_str_hash, g_str_equal,
                                              g_free, override_info_list_free);

  /* create the thumbnailer info hash table */
  manager->thumbnailers = g_hash_table_new_full (g_str_hash, g_str_equal,
                                                 g_free, thumbnailer_info_list_free);
}



static void
tumbler_manager_finalize (GObject *object)
{
  TumblerManager *manager = TUMBLER_MANAGER (object);

  g_mutex_lock (manager->mutex);

  /* release all thumbnailer directory monitors */
  g_list_foreach (manager->monitors, (GFunc) tumbler_manager_monitor_unref, manager);
  g_list_free (manager->monitors);

  /* release all directory objects */
  g_list_foreach (manager->directories, (GFunc) g_object_unref, NULL);
  g_list_free (manager->directories);

  /* destroy the hash tables */
  g_hash_table_unref (manager->thumbnailers);
  g_hash_table_unref (manager->overrides);

  /* release the registry */
  g_object_unref (manager->registry);

  /* release the D-Bus connection object */
  dbus_g_connection_unref (manager->connection);

  g_mutex_unlock (manager->mutex);

  /* destroy the mutex */
  g_mutex_free (manager->mutex);

  (*G_OBJECT_CLASS (tumbler_manager_parent_class)->finalize) (object);
}



static void
tumbler_manager_get_property (GObject    *object,
                              guint       prop_id,
                              GValue     *value,
                              GParamSpec *pspec)
{
  TumblerManager *manager = TUMBLER_MANAGER (object);

  switch (prop_id)
    {
    case PROP_CONNECTION:
      g_value_set_pointer (value, manager->connection);
      break;
    case PROP_REGISTRY:
      g_value_set_object (value, manager->registry);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}



static void
tumbler_manager_set_property (GObject      *object,
                              guint         prop_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  TumblerManager *manager = TUMBLER_MANAGER (object);

  switch (prop_id)
    {
    case PROP_CONNECTION:
      manager->connection = dbus_g_connection_ref (g_value_get_pointer (value));
      break;
    case PROP_REGISTRY:
      manager->registry = g_value_dup_object (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}
 
 
 
static void
tumbler_manager_monitor_unref (GFileMonitor   *monitor,
                               TumblerManager *manager)
{
  if (monitor != NULL)
    {
      /* disconnect from the changed signal */
      g_signal_handlers_disconnect_matched (monitor, G_SIGNAL_MATCH_DATA,
                                            0, 0, NULL, NULL, manager);

      /* release (destroy) the monitor */
      g_object_unref (monitor);
    }
}



/**
 * tumbler_manager_get_dir_index:
 * @manager   : a #TumblerManager.
 * @directory : a #GFile pointing to a thumbnailer directory.
 *
 * This function returns the index of the @directory in the thumbnailer
 * directory list that is managed by the @manager. A low index means 
 * higher priority (e.g. XDG_DATA_HOME/thumbnailers comes before 
 * any thumbnailer dir in XDG_DATA_DIRS in the list). An index of -1
 * means the directory is not a thumbnailer directory.
 *
 * Return value: Index of the thumbnailer @directory (lower index indicating
 *               a higher priority). -1 if the @directory is not a thumbnailer
 *               directory.
 */
static gint
tumbler_manager_get_dir_index (TumblerManager *manager,
                               GFile          *directory)
{
  GList *lp;
  gint   dir_index = -1;
  gint   n;

  /* iterate over all thumbnailer directories */
  for (lp = manager->directories, n = 0; dir_index < 0 && lp != NULL; lp = lp->next, ++n)
    {
      /* remember the directory index if the directories match */
      if (g_file_equal (lp->data, directory))
        dir_index = n;
    }

  /* return the index of the first thumbnailer directory that matches 
   * the input directory or -1 if there is no such directory */
  return n;
}



/**
 * tumbler_manager_update_preferred:
 * @manager  : a #TumblerManager.
 * @hash_key : a hash key consisting of a URI scheme and a MIME type.
 *
 * This function picks the highest priority override section for the @hash_key
 * and iterates over all thumbnailers provided by the thumbnailer service
 * files. It selects the first thumbnailer that matches the override section
 * (by comparing its name to the thumbnailer name defined in the section)
 * and calls tumbler_registry_set_preferred() to make this thumbnailer the
 * preferred thumbnailer for the URI scheme and MIME type combination.
 *
 * If there is no such thumbnailer, the preferred thumbnailer for this
 * hash key will be cleared.
 *
 * Note that we do not call tumbler_registry_update_supported() here as
 * the selected thumbnailer (if there is any that matches the override
 * section) is supposed to be known by the registry already.
 */
static void
tumbler_manager_update_preferred (TumblerManager *manager,
                                  const gchar    *hash_key)
{
  TumblerThumbnailer *thumbnailer = NULL;
  ThumbnailerInfo    *info;
  GHashTableIter      iter;
  const gchar        *current_name;
  const gchar        *name;
  GList             **overrides;
  GList             **thumbnailers;

  g_return_if_fail (TUMBLER_IS_MANAGER (manager));
  g_return_if_fail (hash_key != NULL && *hash_key != '\0');

  /* fetch all override infos for this hash key */
  overrides = g_hash_table_lookup (manager->overrides, hash_key);
  if (overrides != NULL)
    {
      /* if there is a value for the hash key it has to be a valid pointer
       * to a non-empty override info list, otherwise there is a bug */
      g_assert (overrides != NULL && *overrides != NULL);

      /* determine the name of the active (= the first) override info 
       * for this hash key */
      name = ((OverrideInfo *)(*overrides)->data)->name;
      
      /* iterate over all thumbnailer info lists we have. stop as soon as we
       * have one matching thumbnailer info that has the correct name and
       * supports the hash key */
      g_hash_table_iter_init (&iter, manager->thumbnailers);
      while (thumbnailer == NULL 
             && g_hash_table_iter_next (&iter, NULL, (gpointer) &thumbnailers))
        {
          /* each value in the thumbnailer info hash table has to be a 
           * valid pointer to a non-empty thumbnailer info list, otherwise
           * there is a bug */
          g_assert (thumbnailers != NULL && *thumbnailers != NULL);

          /* get the active (= the first) thumbnailer info in the list */
          info = (ThumbnailerInfo *)(*thumbnailers)->data;

          /* each element in the thumbnailer info list has to be a valid
           * thumbnailer info with a specialized thumbnailer object */
          g_assert (info != NULL);
          g_assert (TUMBLER_IS_SPECIALIZED_THUMBNAILER (info->thumbnailer));

          /* determine the name of the thumbnailer */
          current_name = tumbler_specialized_thumbnailer_get_name (
            TUMBLER_SPECIALIZED_THUMBNAILER (info->thumbnailer));

          /* check if the current thumbnailer matches the override info name */
          if (g_strcmp0 (name, current_name) == 0)
            {
              /* check if the thumbnailer supports the hash key at all */
              if (tumbler_thumbnailer_supports_hash_key (info->thumbnailer, hash_key))
                thumbnailer = info->thumbnailer;
            }
        }
    }

  /* update the preferred information of the registry. if no 
   * thumbnailer was found, this will reset the preferred information
   * for this hash key */
  tumbler_registry_set_preferred (manager->registry, hash_key, thumbnailer);
}



/**
 * tumbler_manager_parse_overrides: 
 * @manager : a #TumblerManager.
 * @file    : a #GFile that is supposed to be parsed.
 *
 * Parses the override sections from @file into a list of 
 * #OverrideInfo<!---->s, one for each hash key in the @file. The caller
 * is responsible to free the returned infos with override_info_list_free().
 *
 * Return value: A list of #OverrideInfo<!---->s parsed from the input @file.
 */
static GList *
tumbler_manager_parse_overrides (TumblerManager *manager,
                                 GFile          *file)
{
  GKeyFile *key_file;
  GError   *error = NULL;
  GList    *overrides = NULL;
  GFile    *directory;
  gchar   **sections;
  gchar    *filename;
  gchar    *uri_type;
  guint     n;

  g_return_val_if_fail (TUMBLER_IS_MANAGER (manager), NULL);
  g_return_val_if_fail (G_IS_FILE (file), NULL);
  
  filename = g_file_get_path (file);

  if (!g_file_test (filename, G_FILE_TEST_IS_REGULAR))
    {
      g_free (filename);
      return NULL;
    }

  /* allocate the key file */
  key_file = g_key_file_new ();

  /* try to load the key file from the overrides file */
  if (!g_key_file_load_from_file (key_file, filename, G_KEY_FILE_NONE, &error))
    {
      g_warning (_("Failed to load the file \"%s\": %s"), filename, error->message);
      g_clear_error (&error);
      g_key_file_free (key_file);
      g_free (filename);
      return NULL;
    }

  /* determine sections in the key file */
  sections = g_key_file_get_groups (key_file, NULL);

  /* iterate over all sections */
  for (n = 0; sections != NULL && sections[n] != NULL; ++n)
    {
      OverrideInfo *info = override_info_new ();

      info->name = g_key_file_get_string (key_file, sections[n], "Name", &error);
      if (info->name == NULL)
        {
          g_warning (_("Malformed section \"%s\" in file \"%s\": %s"),
                     sections[n], filename, error->message);
          g_clear_error (&error);

          override_info_free (info);
          g_free (sections[n]);

          continue;
        }

      info->uri_scheme = g_key_file_get_string (key_file, sections[n], 
                                                "UriScheme", &error);
      if (info->uri_scheme == NULL)
        {
          g_warning (_("Malformed section \"%s\" in file \"%s\": %s"),
                     sections[n], filename, error->message);
          g_clear_error (&error);

          override_info_free (info);
          g_free (sections[n]);

          continue;
        }

      info->mime_type = g_key_file_get_string (key_file, sections[n], 
                                               "MimeType", &error);
      if (info->mime_type == NULL)
        {
          g_warning (_("Malformed section \"%s\" in file \"%s\": %s"),
                     sections[n], filename, error->message);
          g_clear_error (&error);

          override_info_free (info);
          g_free (sections[n]);

          continue;
        }

      uri_type = g_strdup_printf ("%s-%s", info->uri_scheme, info->mime_type);
      if (g_strcmp0 (sections[n], uri_type) != 0)
        {
          g_warning (_("Malformed section \"%s\" in file \"%s\": "
                       "Mismatch between section name and UriScheme/MimeType"),
                     sections[n], filename);

          g_free (uri_type);
          override_info_free (info);
          g_free (sections[n]);

          continue;
        }
      g_free (uri_type);

      directory = g_file_get_parent (file);
      info->dir_index = tumbler_manager_get_dir_index (manager, directory);
      g_object_unref (directory);

      overrides = g_list_prepend (overrides, info);

      g_free (sections[n]);
    }

  g_free (sections);
  g_key_file_free (key_file);
  g_free (filename);

  return overrides;
}



/**
 * tumbler_manager_load_overrides_file:
 * @manager : a #TumblerManager.
 * @file    : an overrides GFile to load.
 *
 * This function tries to parse @file into a number of override infos, one
 * for each section in the @file. If this succeeds, it adds each info to 
 * the correct override info list in the hash table that maps hash keys to
 * infos. The infos are inserted in sorted order (where the sort key is the
 * directory index). This ensures that infos from thumbnailer directories
 * with higher priority always come first.
 */
static void
tumbler_manager_load_overrides_file (TumblerManager *manager,
                                     GFile          *file)
{
  OverrideInfo *info;
  OverrideInfo *info2;
  gboolean      first = FALSE;
  gboolean      inserted = FALSE;
  GList        *overrides;
  GList        *lp;
  GList        *op;
  GList       **list;
  GFile        *directory;
  gchar        *hash_key;
  gint          dir_index;

  g_return_if_fail (TUMBLER_MANAGER (manager));
  g_return_if_fail (G_IS_FILE (file));

  /* determine the index of the thumbnailer directory the file is located in */
  directory = g_file_get_parent (file);
  dir_index = tumbler_manager_get_dir_index (manager, directory);
  g_object_unref (directory);

  /* do nothing if the file comes from an unknown directory */
  if (dir_index < 0)
    return;

  /* try parsing the file into override infos */
  overrides = tumbler_manager_parse_overrides (manager, file);
  
  /* iterate over all override infos we parsed successfully */
  for (op = overrides; op != NULL; op = op->next)
    {
      info = op->data;

      /* generate the hash key for the current info */
      hash_key = g_strdup_printf ("%s-%s", info->uri_scheme, info->mime_type);

      /* determine which list to add the info to */
      list = g_hash_table_lookup (manager->overrides, hash_key);

      /* check if we need to create the list because it doesn't exist yet */
      if (list == NULL)
        {
          /* allocate a pointer to a list */
          list = g_slice_alloc0 (sizeof (GList **));

          /* add the info to the list */
          *list = g_list_prepend (*list, info);

          /* insert the list into the hash table */
          g_hash_table_insert (manager->overrides, hash_key, list);

          /* update the preferred information for this hash key, as we now
           * have a new info at the very beginning of the list */
          tumbler_manager_update_preferred (manager, hash_key);
        }
      else
        {
          g_free (hash_key);

          first = FALSE;
          inserted = FALSE;

          /* find the right place in the list to insert the info and insert it
           * if the list is non-empty */
          for (lp = *list; !inserted && lp != NULL; lp = lp->next)
            {
              info2 = lp->data;

              /* we should NEVER have two sections with the same hash key in the
               * same overrides file. ideally, we'd make use of the assertion below
               * but since people are doomed to make mistakes, we're simply ignoring
               * duplicate sections here and give priority to those coming earlier
               * in the list */
#if 0
              g_assert (info2->dir_index != dir_index);
#endif

              if (info2->dir_index > dir_index)
                {
                  if (lp == *list)
                    first = TRUE;

                  *list = g_list_insert_before (*list, lp, info);

                  inserted = TRUE;
                }
            }

          if (!inserted)
            *list = g_list_append (*list, info);

          if (first)
            {
              /* update the preferred information for this hash key, as we now
               * have a new info at the very beginning of the list */
              tumbler_manager_update_preferred (manager, hash_key);
            }
        }
    }

  g_list_free (overrides);
}



/**
 * tumbler_manager_load_overrides:
 * @manager : a #TumblerManager.
 *
 * Attempts to load the override infos of all overrides files in known
 * thumbnailer direcotories into the hash key -> override info list
 * hash table.
 */
static void
tumbler_manager_load_overrides (TumblerManager *manager)
{
  GFileType type;
  GFile    *file;
  GList    *lp;

  g_return_if_fail (TUMBLER_IS_MANAGER (manager));

  /* iterate over all thumbnailer directories */
  for (lp = manager->directories; lp != NULL; lp = lp->next)
    {
      /* build the filename for the overrides file in this directory */
      file = g_file_get_child (lp->data, "overrides");

      /* query the file type. we can only parse regular files */
      type = g_file_query_file_type (file, G_FILE_QUERY_INFO_NONE, NULL);

      /* if the file is regular, try to load it */
      if (type == G_FILE_TYPE_REGULAR)
        tumbler_manager_load_overrides_file (manager, file);

      /* we no longer need the file object */
      g_object_unref (file);
    }

#ifdef DEBUG
  dump_overrides (manager);
#endif
}



/**
 * tumbler_manager_unload_overrides_file:
 * @manager : a #TumblerManager.
 * @file    : the overrides #GFile to unload.
 *
 * This removes all override infos belonging to this @file from the
 * hash key -> override info list hash table. It does so by computing
 * the directory index of the @file and matching it against all override
 * infos. Override infos that have the same directory index are removed.
 *
 * Whenever the first element in one of the override info lists is removed,
 * the preferred information of the registry is updated for that particular
 * hash key.
 *
 * The function is supposed to be called when an overrides file is deleted
 * from the hard disk and we need to synchronize our internal 
 * representation.
 */
static void
tumbler_manager_unload_overrides_file (TumblerManager *manager,
                                       GFile          *file)
{
  GHashTableIter iter;
  OverrideInfo  *info;
  const gchar   *hash_key;
  gboolean       first;
  GFile         *directory;
  GList         *lp;
  GList        **overrides;
  gint           dir_index;

  g_return_if_fail (TUMBLER_IS_MANAGER (manager));
  g_return_if_fail (G_IS_FILE (file));

  /* compute the directory index for the overrides file */
  directory = g_file_get_parent (file);
  dir_index = tumbler_manager_get_dir_index (manager, directory);
  g_object_unref (directory);

  /* do nothing if the overrides file doesn't come from one of our known
   * thumbnailer directories */
  if (dir_index < 0)
    return;

  /* iterate over all (hash key, override info list) pairs from the hash table */
  g_hash_table_iter_init (&iter, manager->overrides);
  while (g_hash_table_iter_next (&iter, (gpointer) &hash_key, (gpointer) &overrides))
    {
      first = FALSE;

      /* all values in the hash table should be valid pointers to non-empty
       * override info lists, otherwise there's a bug in the code */
      g_assert (overrides != NULL);
      g_assert (*overrides != NULL);

      /* iterate over all override infos in the current list */
      for (lp = *overrides; lp != NULL; lp = lp->next)
        {
          info = (OverrideInfo *)lp->data;

          /* there should be no NULL elements in the list, otherwise... bug! */
          g_assert (info != NULL);

          /* check if the info has the same directory index as the overrides
           * file and thus, belongs to that file */
          if (info->dir_index == dir_index)
            {
              /* if this is true and the info is first in the list, we need
               * to update the preferred thumbnailer for this hash key later */
              if (lp == *overrides)
                first = TRUE;

              /* destroy the override info */
              override_info_free (info);

              /* remove the element from the list */
              *overrides = g_list_delete_link (*overrides, lp);

              /* we assume there's only one override info with the same hash key
               * coming from the same file, so we can stop searching here */
              break;
            }
        }

      /* if there is no element left after removing the matching ones, we
       * need to remove the entire list from the hash table */
      if (*overrides == NULL)
        g_hash_table_remove (manager->overrides, hash_key);

      /* if we removed an info from the list and it was the first one, we
       * need to update the preferred thumbnailer for this hash key now */
      if (first)
        tumbler_manager_update_preferred (manager, hash_key);
    }
}



/**
 * tumbler_manager_load_thumbnailer:
 * @manager : a #TumblerManager.
 * @file    : the #GFile to load thumbnailer information from.
 *
 * Attempts to load information about a permanently installed specialized
 * thumbnailer from the @file. On success, the resulting thumbnailer info
 * is added to the basename -> thumbnailer info list hash table. If it is
 * inserted as the first element of that list, the preferred thumbnailer
 * for all hash keys supported by the thumbnailer is updated.
 */
static void
tumbler_manager_load_thumbnailer (TumblerManager *manager,
                                  GFile          *file)
{
  ThumbnailerInfo *info;
  ThumbnailerInfo *info2;
  struct stat      file_stat;
  GKeyFile        *key_file;
  gboolean         first = FALSE;
  gboolean         inserted = FALSE;
  GError          *error = NULL;
  GFile           *directory;
  GList          **list;
  GList           *lp;
  GStrv            hash_keys;
  gchar           *base_name;
  gchar           *filename;
  gchar           *name;
  gchar           *object_path;
  gchar          **uri_schemes;
  gchar          **mime_types;
  guint            n;

  g_return_if_fail (TUMBLER_IS_MANAGER (manager));
  g_return_if_fail (G_IS_FILE (file));

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

      return;
    }

  /* determine the name of the specialized thumbnailer */
  name = g_key_file_get_string (key_file, "Specialized Thumbnailer", "Name", &error);
  if (name == NULL)
    {
      g_warning (_("Malformed file \"%s\": %s"), filename, error->message);
      g_clear_error (&error);

      g_key_file_free (key_file);
      g_free (filename);

      return;
    }

  /* determine the D-Bus object path of the specialized thumbnailer */
  object_path = g_key_file_get_string (key_file, "Specialized Thumbnailer", 
                                       "ObjectPath", &error);
  if (object_path == NULL)
    {
      g_warning (_("Malformed file \"%s\": %s"), filename, error->message);
      g_clear_error (&error);

      g_key_file_free (key_file);
      g_free (filename);

      return;
    }

  /* determine the MIME types supported by this thumbnailer */
  mime_types = g_key_file_get_string_list (key_file, "Specialized Thumbnailer",
                                           "MimeTypes", NULL, &error);
  if (mime_types == NULL)
    {
      g_warning (_("Malformed file \"%s\": %s"), filename, error->message);
      g_clear_error (&error);

      g_free (object_path);
      g_free (name);
      g_key_file_free (key_file);
      g_free (filename);

      return;
    }

  /* determine the URI schemes supported by this thumbnailer */
  uri_schemes = g_key_file_get_string_list (key_file, "Specialized Thumbnailer",
                                            "UriSchemes", NULL, &error);

  /* if no URI schemes are specified, we default to the "file" scheme */
  if (uri_schemes == NULL)
    {
      uri_schemes = g_new0 (gchar *, 2);
      uri_schemes[0] = g_strdup ("file");
      uri_schemes[1] = NULL;
    }

  /* determine the time the file was last modified */
  if (g_stat (filename, &file_stat) != 0)
    {
      g_warning (_("Failed to determine last modified time of \"%s\""), filename); 

      g_strfreev (uri_schemes);
      g_strfreev (mime_types);
      g_free (object_path);
      g_free (name);
      g_key_file_free (key_file);
      g_free (filename);

      return;
    }

  /* allocate a new thumbnailer info */
  info = thumbnailer_info_new ();
#ifdef DEBUG
  info->file = g_object_ref (file);
#endif

  /* compute the directory index of the input file */
  directory = g_file_get_parent (file);
  info->dir_index = tumbler_manager_get_dir_index (manager, directory);
  g_object_unref (directory);

  /* create a new specialized thumbnailer object and pass all the required
   * information on to it */
  info->thumbnailer = 
    tumbler_specialized_thumbnailer_new (manager->connection, name, object_path,
                                         (const gchar *const *)uri_schemes,
                                         (const gchar *const *)mime_types,
                                         file_stat.st_mtime);

  /* free stuff */
  g_strfreev (uri_schemes);
  g_strfreev (mime_types);
  g_free (object_path);
  g_free (name);
  g_key_file_free (key_file);
  g_free (filename);
  
  /* determine the basename of the file */
  base_name = g_file_get_basename (file);

  /* determine the list to insert the thumbnailer info in */
  list = g_hash_table_lookup (manager->thumbnailers, base_name);

  first = FALSE;

  /* check if we need to create a new list */
  if (list == NULL)
    {
      /* allocate memory for a pointer to the list */
      list = g_slice_alloc0 (sizeof (GList **));

      /* add the thumbnailer info to the new list */
      *list = g_list_prepend (*list, info);

      /* add the list to the hash table */
      g_hash_table_insert (manager->thumbnailers, base_name, list);

      /* the new thumbnailer info is first in the new list, so we need to 
       * update the preferred thumbnailer for all hash keys supported by
       * it */
      first = TRUE;
    }
  else
    {
      /* free the basename */
      g_free (base_name);

      inserted = FALSE;

      /* iterate over all infos in the current list */
      for (lp = *list; !inserted && lp != NULL; lp = lp->next)
        {
          info2 = lp->data;

          /* we should never have two thumbnailer infos coming from the
           * same file, otherwise there'd be a bug in this program */
          g_assert (info2->dir_index != info->dir_index);

          /* check if we need to insert the thumbnailer info before the
           * current item in the list */
          if (info2->dir_index > info->dir_index)
            {
              /* if the current item is the first in the list, we'll have
               * to update the preferred thumbnailer for all hash keys 
               * supported by the new info later */
              if (lp == *list)
                first = TRUE;

              /* insert the new thumbnailer info into the list */
              *list = g_list_insert_before (*list, lp, info);

              /* we're done */
              inserted = TRUE;
            }
        }

      /* if we couldn't find an info with a dir index of lower priority, we
       * have to append the new info to the list */
      if (!inserted)
        *list = g_list_append (*list, info);
    }

  /* check if we need to update the preferred thumbnailer for the hash keys
   * supported by the new thumbnailer info */
  if (first)
    {
      /* check if there is another element in the list. that one was the first
       * info in the list before, thus its thumbnailer object should've been added 
       * to the registry. we now have to remove it and replace it by the new first 
       * info */
      if ((*list)->next != NULL)
        {
          info2 = (*list)->next->data;
          tumbler_registry_remove (manager->registry, info2->thumbnailer);
        }

      /* add the new thumbnailer to the registry */
      tumbler_registry_add (manager->registry, info->thumbnailer);

      /* determine all hash keys supported by the new thumbnailer info */
      hash_keys = tumbler_thumbnailer_get_hash_keys (info->thumbnailer);

      /* update the preferred thumbnailer for each hash key */
      for (n = 0; hash_keys != NULL && hash_keys[n] != NULL; ++n)
        {
          /* TODO we could check if an update is needed here */
          tumbler_manager_update_preferred (manager, hash_keys[n]);
        }

      /* free the hash key array */
      g_strfreev (hash_keys);
    }
}



/**
 * tumbler_manager_load_thumbnailers:
 * @manager   : a #TumblerManager.
 * @directory : #GFile pointing to a thumbnailer directory.
 *
 * Attempts to load the thumbnailer .service files from all known
 * thumbnailer directories. The resulting thumbnailer infos are then
 * inserted into the basename -> thumbnailer info list hash table.
 *
 * The preferred thumbnailer for affected hash keys are updated
 * whenever needed.
 *
 * TODO: As an optimization we could add a boolean update_preferred
 * parameter to tumbler_manager_load_thumbnailer to avoid updating
 * the preferred thumbnailer repeatedly. We could then only update
 * the preferred thumbnailers of all hash keys at once.
 */
static void
tumbler_manager_load_thumbnailers (TumblerManager *manager,
                                   GFile          *directory)
{
  const gchar *base_name;
  GFileType    type;
  GFile       *file;
  gchar       *dirname;
  GDir        *dir;

  g_return_if_fail (TUMBLER_IS_MANAGER (manager));
  g_return_if_fail (G_IS_FILE (directory));

  /* determine the absolute path to the directory */
  dirname = g_file_get_path (directory);

  /* try to open the directory for reading */
  dir = g_dir_open (dirname, 0, NULL);
  if (dir == NULL)
    {
      g_free (dirname);
      return;
    }

  /* iterate over all files in the directory */
  for (base_name = g_dir_read_name (dir); 
       base_name != NULL; 
       base_name = g_dir_read_name (dir))
    {
      /* skip files that don't end with the .service extension */
      if (!g_str_has_suffix (base_name, ".service"))
        continue;

      /* create a file object for the service file and query it's type */
      file = g_file_get_child (directory, base_name);
      type = g_file_query_file_type (file, G_FILE_QUERY_INFO_NONE, NULL);

      /* try to load the file if it is regular */
      if (type == G_FILE_TYPE_REGULAR)
        tumbler_manager_load_thumbnailer (manager, file);

      /* we no longer need the file object */
      g_object_unref (file);
    }

  /* close the directory handle */
  g_dir_close (dir);

  /* free memory used for the directory path */
  g_free (dirname);
}



/**
 * tumbler_manager_load:
 * @manager : a #TumblerManager.
 *
 * This function should only be called once. It initializes the thumbnailer
 * directory objects, directory monitors, override infos and thumbnailer 
 * infos. 
 */
static void
tumbler_manager_load (TumblerManager *manager)
{
  const gchar *const *data_dirs;
  GFileMonitor       *monitor;
  GList              *directories = NULL;
  GList              *iter;
  gchar              *dirname;
  guint               n;

  g_return_if_fail (TUMBLER_MANAGER (manager));

  g_mutex_lock (manager->mutex);

  /* this function may only be called once */
  g_assert (manager->directories == NULL);
  g_assert (manager->monitors == NULL);

  g_mutex_unlock (manager->mutex);

  /* prepend $XDG_DATA_HOME/thumbnailers/ to the directory list */
  dirname = g_build_filename (g_get_user_data_dir (), "thumbnailers", NULL);
  directories = g_list_prepend (directories, g_file_new_for_path (dirname));
  g_free (dirname);

  /* determine system data dirs */
  data_dirs = g_get_system_data_dirs ();

  /* build $XDG_DATA_DIRS/thumbnailers dirnames and prepend them to the list */
  for (n = 0; data_dirs[n] != NULL; ++n)
    {
      dirname = g_build_filename (data_dirs[n], "thumbnailers", NULL);
      directories = g_list_prepend (directories, g_file_new_for_path (dirname));
      g_free (dirname);
    }

  /* reverse the directory list so that the directories with highest 
   * priority come first */
  directories = g_list_reverse (directories);

  g_mutex_lock (manager->mutex);

  /* pass the ownership of the directories list to the manager */
  manager->directories = directories;
  manager->monitors = NULL;

  /* update the thumbnailer cache */
  for (iter = manager->directories; iter != NULL; iter = iter->next)
    tumbler_manager_load_thumbnailers (manager, iter->data);

#ifdef DEBUG
  dump_thumbnailers (manager);
#endif

  /* update the overrides cache */
  tumbler_manager_load_overrides (manager);

  /* update the supported information */
  tumbler_registry_update_supported (manager->registry);

  /* monitor the directories for changes */
  for (iter = g_list_last (manager->directories); iter != NULL; iter = iter->prev)
    {
      /* allocate a monitor, connect to it and prepend it to the monitor list */
      monitor = g_file_monitor_directory (iter->data, G_FILE_MONITOR_NONE, NULL, NULL);
      g_signal_connect_swapped (monitor, "changed", 
                                G_CALLBACK (tumbler_manager_directory_changed), manager);
      manager->monitors = g_list_prepend (manager->monitors, monitor);
    }

  g_mutex_unlock (manager->mutex);
}



/**
 * tumbler_manager_thumbnailer_file_deleted:
 * @manager : a #TumblerManager.
 * @file    : the #GFile pointing to the service file that was deleted.
 *
 * This function should be called when a thumbnailer service file has
 * been deleted and our internal representation needs to be updated.
 * 
 * If the corresponding thumbnailer info is the first in its basename
 * list, the preferred thumbnailers for all affected hash keys need
 * to be updated, and the corresponding thumbnailer has to be removed
 * from the registry and replaced by the next one in the list.
 */
static void
tumbler_manager_thumbnailer_file_deleted (TumblerManager *manager,
                                          GFile          *file)
{
  ThumbnailerInfo *info;
  ThumbnailerInfo *info2;
  GFile           *directory;
  GList          **list;
  GList           *lp;
  GStrv            hash_keys;
  gchar           *base_name;
  guint            n;
  gint             dir_index;

  g_return_if_fail (TUMBLER_IS_MANAGER (manager));
  g_return_if_fail (G_IS_FILE (file));

  /* compute the directory index for the thumbnailer file */
  directory = g_file_get_parent (file);
  dir_index = tumbler_manager_get_dir_index (manager, directory);
  g_object_unref (directory);

  /* ignore the service file if it does not come from a know thumbnailer
   * directory */
  if (dir_index < 0)
    return;

  /* look up the basename thumbnailer info list for this file */
  base_name = g_file_get_basename (file);
  list = g_hash_table_lookup (manager->thumbnailers, base_name);
  g_free (base_name);

  /* ignore the service file if there is no thumbnailer info for this
   * basename */
  if (list == NULL)
    return;

  /* if we have a list it has to be non-empty, otherwise there is a bug
   * in this program */
  g_assert (*list != NULL);

  /* iterate over all thumbnailer infos in the list */
  for (lp = *list; lp != NULL; lp = lp->next)
    {
      info = lp->data;

      /* check if the current info comes from the thumbnailer file that was 
       * deleted */
      if (info->dir_index == dir_index)
        {
          /* check if the thumbnailer info is the first in the list */
          if (lp == *list)
            {
              /* remove the thumbnailer info from the list */
              *list = g_list_delete_link (*list, lp);

              /* remove the corresponding thumbnailer from the registry */
              tumbler_registry_remove (manager->registry, info->thumbnailer);

              /* if the list is non-empty after we've removed the info we need
               * to add the new first thumbnailer to the registry */
              if (*list != NULL)
                {
                  info2 = (*list)->data;

                  /* there should be no NULL element in the list */
                  g_assert (info2 != NULL);

                  /* add the new first thumbnailer to the registry */
                  tumbler_registry_add (manager->registry, info2->thumbnailer);
                }

              /* determine all hash kayes supported by the info we're about to
               * destroy */
              hash_keys = tumbler_thumbnailer_get_hash_keys (info->thumbnailer);

              /* update the preferred thumbnailer for all these hash keys */
              for (n = 0; hash_keys != NULL && hash_keys[n] != NULL; ++n)
                {
                  /* TODO we could check if an update is needed here */
                  tumbler_manager_update_preferred (manager, hash_keys[n]);
                }

              /* free the hash keys array */
              g_strfreev (hash_keys);
            }
          else
            {
              /* we're not first in the list, so we can simply remove the
               * info from the list */
              *list = g_list_delete_link (*list, lp);
            }
              
          /* the info was removed from the list and the registry was updated,
           * so destroy it now */
          thumbnailer_info_free (info);

          /* we've found the thumbnailer info for the file, so we can stop
           * searching now */
          break;
        }
    }
}



/**
 * tumbler_manager_directory_created:
 * @manager   : a #TumblerManager.
 * @directory : a #GFile pointing to the created thumbnailer directory.
 *
 * This function should be called when a new thumbnailer directory is 
 * created and the registry needs to be updated. All thumbnailers and 
 * overrides information defined in this directory are loaded and
 * the registry is updated.
 */
static void
tumbler_manager_directory_created (TumblerManager *manager,
                                   GFile          *directory,
                                   gint            dir_index)
{
  GFile *file;

  g_return_if_fail (TUMBLER_IS_MANAGER (manager));
  g_return_if_fail (G_IS_FILE (directory));
  g_return_if_fail (dir_index >= 0);

  /* try to load all thumbnailers in this directory */
  tumbler_manager_load_thumbnailers (manager, directory);

  /* try to load the overrides file in this directory */
  file = g_file_get_child (directory, "overrides");
  tumbler_manager_load_overrides_file (manager, file);
  g_object_unref (file);
}



/**
 * tumbler_manager_directory_deleted:
 * @manager   : a #TumblerManager.
 * @directory : a #GFile pointing to the deleted thumbnailer directory.
 *
 * This function should be called when a thumbnailer directory is deleted.
 * All thumbnailer and override infos from this directory are destroyed, 
 * preferred thumbnailers updated and thumbnailers removed/added from the
 * registry.
 */
static void
tumbler_manager_directory_deleted (TumblerManager *manager,
                                   GFile          *directory,
                                   gint            dir_index)
{
  ThumbnailerInfo *info;
  ThumbnailerInfo *info2;
  GHashTableIter   iter;
  GFile           *file;
  GList          **list;
  GList           *lp;
  GStrv            hash_keys;
  guint            n;

  g_return_if_fail (TUMBLER_IS_MANAGER (manager));
  g_return_if_fail (G_IS_FILE (directory));
  g_return_if_fail (dir_index >= 0);

  /* unload the overrides file in this directory */
  file = g_file_get_child (directory, "overrides");
  tumbler_manager_unload_overrides_file (manager, file);
  g_object_unref (file);

  /* iterate over all thumbnailer info lists */
  g_hash_table_iter_init (&iter, manager->thumbnailers);
  while (g_hash_table_iter_next (&iter, NULL, (gpointer) &list))
    {
      /* all lists in the hash table should be defined and non-empty */
      g_assert (list != NULL);
      g_assert (*list != NULL);

      /* iterate over all thumbnailers in the current list */
      for (lp = *list; lp != NULL; lp = lp->next)
        {
          info = lp->data;

          /* check if the thumbnailer info comes from the deleted directory */
          if (info->dir_index == dir_index)
            {
              /* check if the thumbnailer info is the first in the list. if this
               * is the case we need to remove it from the registry and replace it
               * with the second element in the list (if available) */
              if (lp == *list)
                {
                  /* remove the info from the list */
                  *list = g_list_delete_link (*list, lp);

                  /* remove the corresponding thumbnailer from the registry */
                  tumbler_registry_remove (manager->registry, info->thumbnailer);

                  /* if the list is non-empty after removing the info, we need to
                   * add the new first thumbnailer to the registry */
                  if (*list != NULL)
                    {
                      info2 = (*list)->data;

                      /* all infos in the list should be non-NULL */
                      g_assert (info2 != NULL);

                      /* add the new first thumbnailer to the registry */
                      tumbler_registry_add (manager->registry, info2->thumbnailer);
                    }

                  /* determine all hash keys supported by the removed thumbnailer info */
                  hash_keys = tumbler_thumbnailer_get_hash_keys (info->thumbnailer);

                  /* update the preferred thumbnailer for all these hash keys */
                  for (n = 0; hash_keys != NULL && hash_keys[n] != NULL; ++n)
                    {
                      /* TODO we could check if an update is needed here */
                      tumbler_manager_update_preferred (manager, hash_keys[n]);
                    }

                  /* free the hash keys array */
                  g_strfreev (hash_keys);
                }
              else
                {
                  /* the info is not the first in the list, so we can simply
                   * remove it from the list */
                  *list = g_list_delete_link (*list, lp);
                }
                  
              /* destroy the thumbnailer info */
              thumbnailer_info_free (info);
            }
        }
    }
}



/**
 * tumbler_manager_directory_changed:
 * @manager    : a #TumblerManager.
 * @file       : 
 * @other_file :
 * @event_type :
 * @monitor    :
 *
 * TODO We can probably optimize the locking a little bit.
 */
static void
tumbler_manager_directory_changed (TumblerManager   *manager,
                                   GFile            *file,
                                   GFile            *other_file,
                                   GFileMonitorEvent event_type,
                                   GFileMonitor     *monitor)
{
  GFileType type;
  gchar    *base_name;
  gint      dir_index;

  g_return_if_fail (TUMBLER_IS_MANAGER (manager));
  g_return_if_fail (G_IS_FILE (file));
  g_return_if_fail (G_IS_FILE_MONITOR (monitor));

#ifdef DEBUG
  g_print ("Directory (contents) changed\n\n");
#endif

  if (event_type == G_FILE_MONITOR_EVENT_DELETED)
    {
      base_name = g_file_get_basename (file);

      if (g_strcmp0 (base_name, "overrides") == 0)
        {
          g_mutex_lock (manager->mutex);
          tumbler_manager_unload_overrides_file (manager, file);
          tumbler_registry_update_supported (manager->registry);
#ifdef DEBUG
          dump_overrides (manager);
#endif
          g_mutex_unlock (manager->mutex);
        }
      else if (g_str_has_suffix (base_name, ".service"))
        {
          g_mutex_lock (manager->mutex);
          tumbler_manager_thumbnailer_file_deleted (manager, file);
          tumbler_registry_update_supported (manager->registry);
#ifdef DEBUG
          dump_thumbnailers (manager);
#endif
          g_mutex_unlock (manager->mutex);
        }
      else
        {
          g_mutex_lock (manager->mutex);
          dir_index = tumbler_manager_get_dir_index (manager, file);
          g_mutex_unlock (manager->mutex);

          if (dir_index >= 0)
            {
              g_mutex_lock (manager->mutex);
              tumbler_manager_directory_deleted (manager, file, dir_index);
              tumbler_registry_update_supported (manager->registry);
#ifdef DEBUG
              dump_overrides (manager);
              dump_thumbnailers (manager);
#endif
              g_mutex_unlock (manager->mutex);
            }
        }
    }
  else 
    {
      type = g_file_query_file_type (file, G_FILE_QUERY_INFO_NONE, NULL);

      if (type == G_FILE_TYPE_REGULAR)
        {
          base_name = g_file_get_basename (file);

          if (g_strcmp0 (base_name, "overrides") == 0)
            {
              if (event_type == G_FILE_MONITOR_EVENT_CREATED)
                {
                  g_mutex_lock (manager->mutex);
                  tumbler_manager_load_overrides_file (manager, file);
                  tumbler_registry_update_supported (manager->registry);
#ifdef DEBUG
                  dump_overrides (manager);
#endif
                  g_mutex_unlock (manager->mutex);
                }
            }
          else if (g_str_has_suffix (base_name, ".service"))
            {
              if (event_type == G_FILE_MONITOR_EVENT_CREATED)
                {
                  g_mutex_lock (manager->mutex);
                  tumbler_manager_load_thumbnailer (manager, file);
                  tumbler_registry_update_supported (manager->registry);
#ifdef DEBUG
                  dump_thumbnailers (manager);
#endif
                  g_mutex_unlock (manager->mutex);
                }
            }
        }
      else
        {
          g_mutex_lock (manager->mutex);
          dir_index = tumbler_manager_get_dir_index (manager, file);
          g_mutex_unlock (manager->mutex);

          if (dir_index >= 0)
            {
              g_mutex_lock (manager->mutex);
              tumbler_manager_directory_created (manager, file, dir_index);
              tumbler_registry_update_supported (manager->registry);
#ifdef DEBUG
              dump_overrides (manager);
              dump_thumbnailers (manager);
#endif
              g_mutex_unlock (manager->mutex);
            }
        }
    }
}



static OverrideInfo *
override_info_new (void)
{
  return g_slice_new0 (OverrideInfo);
}



static void
override_info_free (gpointer pointer)
{
  OverrideInfo *info = pointer;

  if (info == NULL)
    return;

  g_free (info->name);
  g_free (info->uri_scheme);
  g_free (info->mime_type);

  g_slice_free (OverrideInfo, info);
}



static void
override_info_list_free (gpointer pointer)
{
  GList **infos = pointer;
  GList  *iter;

  for (iter = *infos; iter != NULL; iter = iter->next)
    override_info_free (iter->data);

  g_list_free (*infos);
  g_slice_free1 (sizeof (GList **), infos);
}



static ThumbnailerInfo *
thumbnailer_info_new (void)
{
  return g_slice_new0 (ThumbnailerInfo);
}



static void
thumbnailer_info_free (ThumbnailerInfo *info)
{
  if (info == NULL)
    return;

#ifdef DEBUG
  g_object_unref (info->file);
#endif
  g_object_unref (info->thumbnailer);
  g_slice_free (ThumbnailerInfo, info);
}



static void
thumbnailer_info_list_free (gpointer pointer)
{
  GList **infos = pointer;
  GList  *iter;

  for (iter = *infos; iter != NULL; iter = iter->next)
    thumbnailer_info_free (iter->data);

  g_list_free (*infos);
  g_slice_free1 (sizeof (GList **), infos);
}



#ifdef DEBUG
static void
dump_overrides (TumblerManager *manager)
{
  GHashTableIter table_iter;
  const gchar   *hash_key;
  GList        **list;
  GList         *iter;

  g_print ("Overrides:\n");

  g_hash_table_iter_init (&table_iter, manager->overrides);
  while (g_hash_table_iter_next (&table_iter, (gpointer) &hash_key, (gpointer) &list))
    {
      g_print ("  %s:\n", hash_key);
      for (iter = *list; iter != NULL; iter = iter->next)
        g_print ("    %s\n", ((OverrideInfo *)iter->data)->name);
    }

  g_print ("\n");
}



static void
dump_thumbnailers (TumblerManager *manager)
{
  GHashTableIter table_iter;
  const gchar   *base_name;
  GList        **list;
  GList         *iter;

  g_print ("Thumbnailers:\n");

  g_hash_table_iter_init (&table_iter, manager->thumbnailers);
  while (g_hash_table_iter_next (&table_iter, (gpointer) &base_name, (gpointer) &list))
    {
      g_print ("  %s:\n", base_name);
      for (iter = *list; iter != NULL; iter = iter->next)
        g_print ("    %s\n", g_file_get_path (((ThumbnailerInfo *)iter->data)->file));
    }

  g_print ("\n");
}
#endif /* DEBUG */



TumblerManager *
tumbler_manager_new (DBusGConnection *connection,
                     TumblerRegistry *registry)
{
  return g_object_new (TUMBLER_TYPE_MANAGER, "connection", connection, 
                       "registry", registry, NULL);
}



gboolean
tumbler_manager_start (TumblerManager *manager,
                       GError        **error)
{
  DBusConnection *connection;
  DBusError       dbus_error;
  gint            result;

  g_return_val_if_fail (TUMBLER_IS_MANAGER (manager), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  g_mutex_lock (manager->mutex);

  /* initialize the D-Bus error */
  dbus_error_init (&dbus_error);

  /* get the native D-Bus connection */
  connection = dbus_g_connection_get_connection (manager->connection);

  /* request ownership for the manager interface */
  result = dbus_bus_request_name (connection, "org.freedesktop.thumbnails.Manager1",
                                  DBUS_NAME_FLAG_DO_NOT_QUEUE, &dbus_error);

  /* check if that failed */
  if (result != DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER)
    {
      /* propagate the D-Bus error */
      if (dbus_error_is_set (&dbus_error))
        {
          if (error != NULL)
            dbus_set_g_error (error, &dbus_error);

          dbus_error_free (&dbus_error);
        }
      else if (error != NULL)
        {
          g_set_error (error, DBUS_GERROR, DBUS_GERROR_FAILED,
                       _("Another thumbnailer manager is already running"));
        }

      g_mutex_unlock (manager->mutex);

      /* i can't work like this! */
      return FALSE;
    }

  /* everything's fine, install the manager type D-Bus info */
  dbus_g_object_type_install_info (G_OBJECT_TYPE (manager), 
                                   &dbus_glib_tumbler_manager_object_info);

  /* register the manager instance as a handler of the manager interface */
  dbus_g_connection_register_g_object (manager->connection, 
                                       "/org/freedesktop/thumbnails/Manager1", 
                                       G_OBJECT (manager));

  g_mutex_unlock (manager->mutex);

  /* load thumbnailers installed into the system permanently */
  tumbler_manager_load (manager);

  /* this is how I roll */
  return TRUE;
}



void
tumbler_manager_register (TumblerManager        *manager, 
                          const gchar *const    *uri_schemes, 
                          const gchar *const    *mime_types, 
                          DBusGMethodInvocation *context)
{
  TumblerThumbnailer *thumbnailer;
  gchar              *sender_name;

  dbus_async_return_if_fail (TUMBLER_IS_MANAGER (manager), context);
  dbus_async_return_if_fail (uri_schemes != NULL, context);
  dbus_async_return_if_fail (mime_types != NULL, context);

  sender_name = dbus_g_method_get_sender (context);

  g_mutex_lock (manager->mutex);

  thumbnailer = tumbler_specialized_thumbnailer_new_foreign (manager->connection,
                                                             sender_name, uri_schemes, 
                                                             mime_types);

  tumbler_registry_add (manager->registry, thumbnailer);

  g_object_unref (thumbnailer);

  g_mutex_unlock (manager->mutex);

  g_free (sender_name);

  dbus_g_method_return (context);
}
