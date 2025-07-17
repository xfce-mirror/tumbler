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

#include "tumbler-cache-service.h"
#include "tumbler-lifecycle-manager.h"
#include "tumbler-manager.h"
#include "tumbler-registry.h"
#include "tumbler-service.h"

#include "tumbler/tumbler.h"

#include <glib-object.h>
#include <libxfce4util/libxfce4util.h>
#include <stdlib.h>

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_PWD_H
#include <pwd.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif



static void
shutdown_tumbler (TumblerLifecycleManager *lifecycle_manager,
                  GMainLoop *main_loop)
{
  g_return_if_fail (TUMBLER_IS_LIFECYCLE_MANAGER (lifecycle_manager));
  g_return_if_fail (main_loop != NULL);

  /* exit the main loop */
  g_main_loop_quit (main_loop);
}



static void
on_dbus_name_lost (GDBusConnection *connection,
                   const gchar *name,
                   gpointer user_data)
{
  GMainLoop *main_loop;

  g_critical ("Name %s lost on the message dbus, exiting.", name);
  main_loop = (GMainLoop *) user_data;
  g_main_loop_quit (main_loop);
}

int
main (int argc,
      char **argv)
{
  TumblerLifecycleManager *lifecycle_manager;
  TumblerProviderFactory *provider_factory;
  GDBusConnection *connection;
  TumblerRegistry *registry;
  TumblerManager *manager;
  TumblerService *service;
  TumblerCacheService *cache_service;
  GMainLoop *main_loop;
  GError *error = NULL;
  GList *providers;
  GList *thumbnailers;
  GList *lp;
  GList *tp;
  gint retval = EXIT_SUCCESS;
  GKeyFile *rc;
  gint64 file_size;
  gint priority;
  const gchar *type_name;
  gchar **paths;
  GSList *locations;
  GSList *excludes;

  /* set the program name */
  g_set_prgname (G_LOG_DOMAIN);

#ifdef G_OS_UNIX
  if (nice (19) != 19)
    g_warning ("Couldn't change nice value of process.");
#endif

#if GLIB_CHECK_VERSION(2, 68, 0)
  G_GNUC_BEGIN_IGNORE_DEPRECATIONS
  /* to avoid overlap between stderr and stdout, e.g. when third party APIs write to
   * stderr, or if debugging macros writing to stderr are used in addition to g_debug() */
  g_log_writer_default_set_use_stderr (TRUE);
  G_GNUC_END_IGNORE_DEPRECATIONS
#endif

  /* initialize translations */
  xfce_textdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR, "UTF-8");

  /* set the application name. Translators: Don't translate "Tumbler". */
  g_set_application_name ("Tumbler Thumbnailing Service");

  connection = g_bus_get_sync (G_BUS_TYPE_SESSION, NULL, &error);

  if (error != NULL)
    {
      g_critical ("error getting session bus: %s", error->message);
      g_error_free (error);

      return EXIT_FAILURE;
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
          /* desktop thumbnailers are set up per desktop file */
          if (g_object_class_find_property (G_OBJECT_GET_CLASS (tp->data), "exec") == NULL)
            {
              /* set settings from rc file */
              type_name = G_OBJECT_TYPE_NAME (tp->data);
              priority = g_key_file_get_integer (rc, type_name, "Priority", NULL);
              file_size = g_key_file_get_int64 (rc, type_name, "MaxFileSize", NULL);

              paths = g_key_file_get_string_list (rc, type_name, "Locations", NULL, NULL);
              locations = tumbler_util_locations_from_strv (paths);
              g_strfreev (paths);

              paths = g_key_file_get_string_list (rc, type_name, "Excludes", NULL, NULL);
              excludes = tumbler_util_locations_from_strv (paths);
              g_strfreev (paths);

              g_object_set (tp->data, "priority", priority, "max-file-size", file_size,
                            "locations", locations, "excludes", excludes, NULL);

              /* cleanup */
              g_slist_free_full (locations, g_object_unref);
              g_slist_free_full (excludes, g_object_unref);
            }

          /* ready for usage */
          tumbler_registry_add (registry, tp->data);
        }

      /* free the thumbnailer list */
      g_list_free_full (thumbnailers, g_object_unref);
    }

  g_key_file_free (rc);

  /* release all providers and free the provider list */
  g_list_free_full (providers, g_object_unref);

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
      g_warning ("Failed to load specialized thumbnailers into the registry: %s",
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
                                TUMBLER_SERVICE_NAME_PREFIX ".Cache1",
                                G_BUS_NAME_OWNER_FLAGS_REPLACE,
                                NULL, /* We dont need to do anything on name acquired*/
                                on_dbus_name_lost,
                                main_loop,
                                NULL);

  /* Acquire the manager dbus name */
  g_bus_own_name_on_connection (connection,
                                TUMBLER_SERVICE_NAME_PREFIX ".Manager1",
                                G_BUS_NAME_OWNER_FLAGS_REPLACE,
                                NULL, /* We dont need to do anything on name acquired*/
                                on_dbus_name_lost,
                                main_loop,
                                NULL);

  /* Acquire the thumbnailer service dbus name */
  g_bus_own_name_on_connection (connection,
                                TUMBLER_SERVICE_NAME_PREFIX ".Thumbnailer1",
                                G_BUS_NAME_OWNER_FLAGS_REPLACE,
                                NULL, /* We dont need to do anything on name acquired*/
                                on_dbus_name_lost,
                                main_loop,
                                NULL);

  /* check to see if all services are successfully exported on the bus */
  if (tumbler_manager_is_exported (manager)
      && tumbler_service_is_exported (service)
      && tumbler_cache_service_is_exported (cache_service))
    {
      /* Let the manager initializes the thumbnailer
       * directory objects, directory monitors */
      tumbler_manager_load (manager);

      /* quit the main loop when the lifecycle manager asks us to shut down */
      g_signal_connect (lifecycle_manager, "shutdown",
                        G_CALLBACK (shutdown_tumbler), main_loop);

      /* start the lifecycle manager */
      tumbler_lifecycle_manager_start (lifecycle_manager);

      g_debug ("Ready to handle requests\n");

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
