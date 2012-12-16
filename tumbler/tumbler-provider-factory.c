/* vi:set et ai sw=2 sts=2 ts=2: */
/*-
 * Copyright (c) 2009 Jannis Pohlmann <jannis@xfce.org>
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

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include <glib.h>
#include <glib-object.h>

#include <tumbler/tumbler-provider-factory.h>
#include <tumbler/tumbler-provider-plugin.h>
#include <tumbler/tumbler-util.h>



typedef struct _TumblerProviderInfo TumblerProviderInfo;



static void   tumbler_provider_factory_finalize     (GObject                     *object);
static GList *tumbler_provider_factory_load_plugins (TumblerProviderFactory      *factory);



struct _TumblerProviderFactoryClass
{
  GObjectClass __parent__;
};

struct _TumblerProviderFactory
{
  GObject __parent__;

  GPtrArray *provider_infos;
};

struct _TumblerProviderInfo
{
  GObject *provider;
  GType    type;
};



static GList *tumbler_provider_plugins = NULL;



G_LOCK_DEFINE_STATIC (factory_lock);



G_DEFINE_TYPE (TumblerProviderFactory, tumbler_provider_factory, G_TYPE_OBJECT);



static void
tumbler_provider_factory_class_init (TumblerProviderFactoryClass *klass)
{
  GObjectClass *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->finalize = tumbler_provider_factory_finalize; 
}



static void
tumbler_provider_factory_init (TumblerProviderFactory *factory)
{
  factory->provider_infos = g_ptr_array_new ();
}



static void
tumbler_provider_factory_finalize (GObject *object)
{
  TumblerProviderFactory *factory = TUMBLER_PROVIDER_FACTORY (object);
  TumblerProviderInfo    *info;
  guint                   n;

  /* release all cached provider infos */
  for (n = 0; n < factory->provider_infos->len; ++n)
    {
      info = factory->provider_infos->pdata[n];

      /* free cached provider objects */
      if (info != NULL && info->provider != NULL)
        g_object_unref (info->provider);
      
      /* free cached provider info */
      g_slice_free (TumblerProviderInfo, factory->provider_infos->pdata[n]);
    }

  /* free the provider info array */
  g_ptr_array_free (factory->provider_infos, TRUE);

  (*G_OBJECT_CLASS (tumbler_provider_factory_parent_class)->finalize) (object);
}



static void
tumbler_provider_factory_add_types (TumblerProviderFactory *factory,
                                    TumblerProviderPlugin  *plugin)
{
  TumblerProviderInfo *provider_info;
  const GType         *types;
  guint                idx;
  gint                 n_types;
  gint                 n;

  /* collect all the types provided by the plugin */
  tumbler_provider_plugin_get_types (plugin, &types, &n_types);

  /* resize the provider info array */
  g_ptr_array_set_size (factory->provider_infos,
                        factory->provider_infos->len + n_types);

  for (n = 0; n < n_types; ++n)
    {
      /* allocate a new provider info structure */
      provider_info = g_slice_new0 (TumblerProviderInfo);
      provider_info->type = types[n];
      provider_info->provider = NULL;

      /* compute the idx for this info */
      idx = factory->provider_infos->len - n_types + n;

      /* insert the provider info into the array */
      factory->provider_infos->pdata[idx] = provider_info;
    }
}



static GList *
tumbler_provider_factory_load_plugins (TumblerProviderFactory *factory)
{
  TumblerProviderPlugin *plugin;
  const gchar           *basename;
  GList                 *lp;
  GList                 *plugins = NULL;
  GDir                  *dir;

  g_return_val_if_fail (TUMBLER_IS_PROVIDER_FACTORY (factory), NULL);

  /* try to open the plugin directory for reading */
  dir = g_dir_open (TUMBLER_PLUGIN_DIRECTORY, 0, NULL);
  if (dir != NULL)
    {
      /* iterate over all files in the plugin directory */
      for (basename = g_dir_read_name (dir); 
           basename != NULL; 
           basename = g_dir_read_name (dir))
        {
          /* check if this is a valid plugin file */
          if (g_str_has_suffix (basename, "." G_MODULE_SUFFIX))
            {
              /* check if we already have that module */
              for (lp = tumbler_provider_plugins; lp != NULL; lp = lp->next)
                if (g_str_equal (G_TYPE_MODULE (lp->data)->name, basename))
                  break;

              /* use or allocate a new plugin for the file */
              if (lp != NULL)
                {
                  /* reuse the existing plugin */
                  plugin = TUMBLER_PROVIDER_PLUGIN (lp->data);
                }
              else
                {
                  /* allocate a new plugin and add it to our list */
                  plugin = tumbler_provider_plugin_new (basename);
                  tumbler_provider_plugins = g_list_prepend (tumbler_provider_plugins, 
                                                             plugin);
                }

              /* try to load the plugin */
              if (g_type_module_use (G_TYPE_MODULE (plugin)))
                {
                  /* add the plugin to our list */
                  plugins = g_list_prepend (plugins, plugin);

                  /* we only add types to our cache the first time a module is loaded */
                  /*
                  if (lp == NULL)
                    {*/
                      /* add the types provided by the plugin */
                      tumbler_provider_factory_add_types (factory, plugin);
                   /* }*/
                }
            }
        }

      /* close the directory handle */
      g_dir_close (dir);
    }

  return plugins;
}



TumblerProviderFactory *
tumbler_provider_factory_get_default (void)
{
  static TumblerProviderFactory *factory = NULL;

  G_LOCK (factory_lock);

  if (factory == NULL)
    {
      factory = g_object_new (TUMBLER_TYPE_PROVIDER_FACTORY, NULL);
      g_object_add_weak_pointer (G_OBJECT (factory), (gpointer) &factory);
    }
  else
    {
      g_object_ref (factory);
    }

  G_UNLOCK (factory_lock);

  return factory;
}



GList *
tumbler_provider_factory_get_providers (TumblerProviderFactory *factory,
                                        GType                   type)
{
  TumblerProviderInfo *info;
  GList               *lp;
  GList               *plugins;
  GList               *providers = NULL;
  guint                n;
  const gchar         *type_name;
  gchar               *name;
  gboolean             disabled;
  GKeyFile            *rc;

  G_LOCK (factory_lock);

  /* load available plugins */
  plugins = tumbler_provider_factory_load_plugins (factory);

  /* rc file */
  rc = tumbler_util_get_settings ();

  /* iterate over all provider infos */
  for (n = 0; n < factory->provider_infos->len; ++n)
    {
      info = factory->provider_infos->pdata[n];

      /* check if this plugin is disabled with the assumption
       * the provider only provides 1 type */
      type_name = g_type_name (info->type);
      g_assert (g_str_has_suffix (type_name, "Provider"));
      name = g_strndup (type_name, strlen (type_name) - 8);
      disabled = g_key_file_get_boolean (rc, name, "Disabled", NULL);
      g_free (name);
      if (disabled)
        continue;

      /* check if the provider type implements the given type */
      if (G_LIKELY (g_type_is_a (info->type, type)))
        {
          /* create the provider on demand */
          if (info->provider == NULL)
            info->provider = g_object_new (info->type, NULL);

          /* add the provider to the list */
          providers = g_list_prepend (providers, g_object_ref (info->provider));
        }
    }

  /* release all plugins */
  for (lp = plugins; lp != NULL; lp = lp->next)
    g_type_module_unuse (G_TYPE_MODULE (lp->data));
  g_list_free (plugins);

  g_key_file_free (rc);

  G_UNLOCK (factory_lock);

  return providers;
}
