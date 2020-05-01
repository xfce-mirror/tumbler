/* vi:set et ai sw=2 sts=2 ts=2: */
/*-
 * Copyright (c) 2009-2012 Jannis Pohlmann <jannis@xfce.org>
 * Copyright (c) 2015      Ali Abdallah    <ali@xfce.org>
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
#ifdef HAVE_PWD_H
#include <pwd.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <stdlib.h>

#include <glib.h>
#include <glib/gi18n.h>
#include <glib-object.h>

#include <tumbler/tumbler.h>

#include <tumblerd/tumbler-cache-service.h>
#include <tumblerd/tumbler-lifecycle-manager.h>
#include <tumblerd/tumbler-manager.h>
#include <tumblerd/tumbler-registry.h>
#include <tumblerd/tumbler-service.h>



static void
shutdown_tumbler (TumblerLifecycleManager *lifecycle_manager,
                  GMainLoop               *main_loop)
{
  g_return_if_fail (TUMBLER_IS_LIFECYCLE_MANAGER (lifecycle_manager));
  g_return_if_fail (main_loop != NULL);

  /* exit the main loop */
  g_main_loop_quit (main_loop);
}



static inline gboolean
xfce_is_valid_tilde_prefix (const gchar *p)
{
  if (g_ascii_isspace (*p) /* thunar ~/music */
      || *p == '=' /* terminal --working-directory=~/ */
      || *p == '\'' || *p == '"') /* terminal --working-directory '~my music' */
    return TRUE;

  return FALSE;
}


/* from libxfce4util */
static gchar *
xfce_expand_variables (const gchar *command,
                       gchar      **envp)
{
  GString        *buf;
  const gchar    *start;
  gchar          *variable;
  const gchar    *p;
  const gchar    *value;
  gchar         **ep;
  guint           len;
#ifdef HAVE_GETPWNAM
  struct passwd  *pw;
  gchar          *username;
#endif

  if (G_UNLIKELY (command == NULL))
    return NULL;

  buf = g_string_sized_new (strlen (command));

  for (p = command; *p != '\0'; ++p)
    {
      continue_without_increase:

      if (*p == '~'
          && (p == command
              || xfce_is_valid_tilde_prefix (p - 1)))
        {
          /* walk to the end of the string or to a directory separator */
          for (start = ++p; *p != '\0' && *p != G_DIR_SEPARATOR; ++p);

          if (G_LIKELY (start == p))
            {
              /* add the current user directory */
              buf = g_string_append (buf, g_get_home_dir ());
            }
          else
            {
#ifdef HAVE_GETPWNAM
              username = g_strndup (start, p - start);
              pw = getpwnam (username);
              g_free (username);

              /* add the users' home directory if found, fallback to the
               * not-expanded string */
              if (pw != NULL && pw->pw_dir != NULL)
                buf = g_string_append (buf, pw->pw_dir);
              else
#endif
                buf = g_string_append_len (buf, start - 1, p - start + 1);
            }

          /* we are either at the end of the string or *p is a separator,
           * so continue to add it to the result buffer */
        }
      else if (*p == '$')
        {
          /* walk to the end of a valid variable name */
          for (start = ++p; *p != '\0' && (g_ascii_isalnum (*p) || *p == '_'); ++p);

          if (start < p)
            {
              value = NULL;
              len = p - start;

              /* lookup the variable in the environment supplied by the user */
              if (envp != NULL)
                {
                  /* format is NAME=VALUE */
                  for (ep = envp; *ep != NULL; ++ep)
                    if (strncmp (*ep, start, len) == 0
                        && (*ep)[len] == '=')
                      {
                        value = (*ep) + len + 1;
                        break;
                      }
                }

              /* fallback to the environment */
              if (value == NULL)
                {
                  variable = g_strndup (start, len);
                  value = g_getenv (variable);
                  g_free (variable);
                }

              if (G_LIKELY (value != NULL))
                {
                  buf = g_string_append (buf, value);
                }
              else
                {
                  /* the variable name was valid, but no value was
                   * found, insert nothing and continue */
                }

              /* *p is at the start of the charater after the variable,
               * so continue scanning without advancing the string offset
               * so two variables are replaced properly */
              goto continue_without_increase;
            }
          else
            {
              /* invalid variable format, add the
               * $ character and continue */
              --p;
            }
        }

      buf = g_string_append_c (buf, *p);
    }

  return g_string_free (buf, FALSE);
}



static GSList *
locations_from_strv (gchar **array)
{
  GSList *locations = NULL;
  guint   n;
  gchar  *path;

  if (array == NULL)
    return NULL;

  for (n = 0; array[n] != NULL; n++)
    {
      path = xfce_expand_variables (array[n], NULL);
      locations = g_slist_prepend (locations, g_file_new_for_commandline_arg (path));
      g_free (path);
    }

  return locations;
}

static void
on_dbus_name_lost (GDBusConnection *connection,
                   const gchar     *name,
                   gpointer         user_data)
{
  GMainLoop *main_loop;

  g_critical (_("Name %s lost on the message dbus, exiting."),name);
  main_loop = (GMainLoop*)user_data;
  g_main_loop_quit(main_loop);
}

int
main (int    argc,
      char **argv)
{
  TumblerLifecycleManager *lifecycle_manager;
  TumblerProviderFactory  *provider_factory;
  GDBusConnection         *connection;
  TumblerRegistry         *registry;
  TumblerManager          *manager;
  TumblerService          *service;
  TumblerCacheService     *cache_service;
  GMainLoop               *main_loop;
  GError                  *error = NULL;
  GList                   *providers;
  GList                   *thumbnailers;
  GList                   *lp;
  GList                   *tp;
  gint                     retval = EXIT_SUCCESS;
  GKeyFile                *rc;
  gint64                   file_size;
  gint                     priority;
  const gchar             *type_name;
  gchar                  **paths;
  GSList                  *locations;
  GSList                  *excludes;

  /* set the program name */
  g_set_prgname (G_LOG_DOMAIN);

#ifdef G_OS_UNIX
  if (nice (19) != 19)
    g_warning (_("Couldn't change nice value of process."));
#endif

#ifdef DEBUG
  /* if something doesn't work, fix your code instead! */
  g_log_set_always_fatal (G_LOG_LEVEL_CRITICAL);
#endif

  /* set the application name. Translators: Don't translate "Tumbler". */
  g_set_application_name (_("Tumbler Thumbnailing Service"));

#if !GLIB_CHECK_VERSION (2, 36, 0)
  /* initialize the GLib type system */
  g_type_init ();
#endif

#if !GLIB_CHECK_VERSION (2, 32, 0)
  /* initialize threading system */
  if (!g_thread_supported ())
    g_thread_init (NULL);
#endif


  connection = g_bus_get_sync (G_BUS_TYPE_SESSION, NULL, &error);
  
  if (error != NULL)
    {
      g_critical ("error getting session bus: %s", error->message);
      g_error_free (error);
      
      return  EXIT_FAILURE;
    }
  
  /* create the lifecycle manager */
  lifecycle_manager = tumbler_lifecycle_manager_new ();

  /* create the thumbnailer registry */
  registry = tumbler_registry_new ();

  /* take a reference on the provider factory */
  provider_factory = tumbler_provider_factory_get_default ();

  /* query all thumbnailer providers from the factory */
  providers = tumbler_provider_factory_get_providers (provider_factory,
                                                      TUMBLER_TYPE_THUMBNAILER_PROVIDER);

  /* settings */
  rc = tumbler_util_get_settings ();

  /* iterate over all providers */
  for (lp = providers; lp != NULL; lp = lp->next)
    {
      /* query the list of thumbnailers provided by this provider */
      thumbnailers = tumbler_thumbnailer_provider_get_thumbnailers (lp->data);

      /* add all thumbnailers to the registry */
      for (tp = thumbnailers; tp != NULL; tp = tp->next)
        {
          /* set settings from rc file */
          type_name = G_OBJECT_TYPE_NAME (tp->data);
          priority = g_key_file_get_integer (rc, type_name, "Priority", NULL);
          file_size = g_key_file_get_int64 (rc, type_name, "MaxFileSize", NULL);

          paths = g_key_file_get_string_list (rc, type_name, "Locations", NULL, NULL);
          locations = locations_from_strv (paths);
          g_strfreev (paths);

          paths = g_key_file_get_string_list (rc, type_name, "Excludes", NULL, NULL);
          excludes = locations_from_strv (paths);
          g_strfreev (paths);

          g_object_set (G_OBJECT (tp->data),
                        "priority", priority,
                        "max-file-size", file_size,
                        "locations", locations,
                        "excludes", excludes,
                        NULL);

          /* ready for usage */
          tumbler_registry_add (registry, tp->data);

          /* cleanup */
          g_object_unref (tp->data);
          g_slist_foreach (locations, (GFunc) g_object_unref, NULL);
          g_slist_free (locations);
          g_slist_foreach (excludes, (GFunc) g_object_unref, NULL);
          g_slist_free (excludes);
        }

      /* free the thumbnailer list */
      g_list_free (thumbnailers);
    }

  g_key_file_free (rc);

  /* release all providers and free the provider list */
  g_list_foreach (providers, (GFunc) g_object_unref, NULL);
  g_list_free (providers);

  /* drop the reference on the provider factory */
  g_object_unref (provider_factory);

  /* update the URI schemes / MIME types supported information */
  tumbler_registry_update_supported (registry);

  /* create the thumbnail cache service */
  cache_service = tumbler_cache_service_new (connection, lifecycle_manager);

  /* create the thumbnailer manager service */
  manager = tumbler_manager_new (connection, lifecycle_manager, registry);

  /* create the generic thumbnailer service */
  service = tumbler_service_new (connection, lifecycle_manager, registry);

  /* try to load specialized thumbnailers and exit if that fails */
  if (!tumbler_registry_load (registry, &error))
    {
      g_warning (_("Failed to load specialized thumbnailers into the registry: %s"),
                 error->message);
      g_error_free (error);

      /* something failed */
      retval = EXIT_FAILURE;
      goto exit_tumbler;
    }

  /* create a new main loop */
  main_loop = g_main_loop_new (NULL, FALSE);
      
  
  /* Acquire the cache service dbus name */
  g_bus_own_name_on_connection (connection,
                                "org.freedesktop.thumbnails.Cache1",
                                G_BUS_NAME_OWNER_FLAGS_REPLACE,
                                NULL, /* We dont need to do anything on name acquired*/
                                on_dbus_name_lost,
                                main_loop,
                                NULL);

  /* Acquire the manager dbus name */
  g_bus_own_name_on_connection (connection,
                                "org.freedesktop.thumbnails.Manager1",
                                G_BUS_NAME_OWNER_FLAGS_REPLACE,
                                NULL, /* We dont need to do anything on name acquired*/
                                on_dbus_name_lost,
                                main_loop,
                                NULL);

  /* Acquire the thumbnailer service dbus name */
  g_bus_own_name_on_connection (connection,
                                "org.freedesktop.thumbnails.Thumbnailer1",
                                G_BUS_NAME_OWNER_FLAGS_REPLACE,
                                NULL, /* We dont need to do anything on name acquired*/
                                on_dbus_name_lost,
                                main_loop,
                                NULL);
  
  /* check to see if all services are successfully exported on the bus */
  if (tumbler_manager_is_exported(manager) &&
      tumbler_service_is_exported(service) &&
      tumbler_cache_service_is_exported(cache_service))
    {
      /* Let the manager initializes the thumbnailer
       * directory objects, directory monitors */
      tumbler_manager_load (manager);
        
      /* quit the main loop when the lifecycle manager asks us to shut down */
      g_signal_connect (lifecycle_manager, "shutdown", 
                        G_CALLBACK (shutdown_tumbler), main_loop);

      /* start the lifecycle manager */
      tumbler_lifecycle_manager_start (lifecycle_manager);

      /* enter the main loop, thereby making the tumbler service available */
      g_main_loop_run (main_loop);
    }
  
  exit_tumbler:

  /* shut our services down and release all objects */
  g_object_unref (service);
  g_object_unref (manager);
  g_object_unref (cache_service);
  g_object_unref (registry);
  g_object_unref (lifecycle_manager);

  /* free the dbus session bus connection */
  g_object_unref (connection);

  /* we're done, all fine */
  return retval;
}
