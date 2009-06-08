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

#include <glib.h>
#include <glib-object.h>

#include <tumbler/tumbler-provider-factory.h>
#include <tumbler/tumbler-provider-plugin.h>



typedef struct _TumblerProviderInfo TumblerProviderInfo;



#define TUMBLER_PROVIDER_FACTORY_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), TUMBLER_TYPE_PROVIDER_FACTORY, TumblerProviderFactoryPrivate))



/* Property identifiers */
enum
{
  PROP_0,
};



static void   tumbler_provider_factory_class_init   (TumblerProviderFactoryClass *klass);
static void   tumbler_provider_factory_init         (TumblerProviderFactory      *factory);
static void   tumbler_provider_factory_finalize     (GObject                     *object);
static GList *tumbler_provider_factory_load_plugins (TumblerProviderFactory      *factory);



struct _TumblerProviderFactoryClass
{
  GObjectClass __parent__;
};

struct _TumblerProviderFactory
{
  GObject __parent__;

  TumblerProviderFactoryPrivate *priv;
};

struct _TumblerProviderFactoryPrivate
{
  GPtrArray *provider_infos;
};

struct _TumblerProviderInfo
{
  GObject *provider;
  GType    type;
};



static GObjectClass *tumbler_provider_factory_parent_class = NULL;
static GList        *tumbler_provider_plugins = NULL;



GType
tumbler_provider_factory_get_type (void)
{
  static GType type = G_TYPE_INVALID;

  if (G_UNLIKELY (type == G_TYPE_INVALID))
    {
      type = g_type_register_static_simple (G_TYPE_OBJECT, 
                                            "TumblerProviderFactory",
                                            sizeof (TumblerProviderFactoryClass),
                                            (GClassInitFunc) tumbler_provider_factory_class_init,
                                            sizeof (TumblerProviderFactory),
                                            (GInstanceInitFunc) tumbler_provider_factory_init,
                                            0);
    }

  return type;
}



static void
tumbler_provider_factory_class_init (TumblerProviderFactoryClass *klass)
{
  GObjectClass *gobject_class;

  g_type_class_add_private (klass, sizeof (TumblerProviderFactoryPrivate));

  /* Determine the parent type class */
  tumbler_provider_factory_parent_class = g_type_class_peek_parent (klass);

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->finalize = tumbler_provider_factory_finalize; 
}



static void
tumbler_provider_factory_init (TumblerProviderFactory *factory)
{
  factory->priv = TUMBLER_PROVIDER_FACTORY_GET_PRIVATE (factory);
  factory->priv->provider_infos = g_ptr_array_new ();
}



static void
tumbler_provider_factory_finalize (GObject *object)
{
  TumblerProviderFactory *factory = TUMBLER_PROVIDER_FACTORY (object);
  guint                   n;

  /* release all cached provider infos */
  for (n = 0; n < factory->priv->provider_infos->len; ++n)
    {
      /* free cached provider objects */
      if (factory->priv->provider_infos->pdata[n] != NULL)
        g_object_unref (factory->priv->provider_infos->pdata[n]);
      
      /* free cached provider info */
      g_slice_free (TumblerProviderInfo, factory->priv->provider_infos->pdata[n]);
    }

  /* free the provider info array */
  g_ptr_array_free (factory->priv->provider_infos, TRUE);

  (*G_OBJECT_CLASS (tumbler_provider_factory_parent_class)->finalize) (object);
}



static void
tumbler_provider_factory_add_types (TumblerProviderFactory *factory,
                                    TumblerProviderPlugin  *plugin)
{
  TumblerProviderInfo *provider_info;
  const GType         *types;
  guint                index;
  gint                 n_types;
  gint                 n;

  /* collect all the types provided by the plugin */
  tumbler_provider_plugin_get_types (plugin, &types, &n_types);

  /* resize the provider info array */
  g_ptr_array_set_size (factory->priv->provider_infos,
                        factory->priv->provider_infos->len + n_types);

  for (n = 0; n < n_types; ++n)
    {
      /* allocate a new provider info structure */
      provider_info = g_slice_new0 (TumblerProviderInfo);
      provider_info->type = types[n];
      provider_info->provider = NULL;

      /* compute the index for this info */
      index = factory->priv->provider_infos->len - n_types + n;

      /* insert the provider info into the array */
      factory->priv->provider_infos->pdata[index] = provider_info;
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
                  if (lp == NULL)
                    {
                      /* add the types provided by the plugin */
                      tumbler_provider_factory_add_types (factory, plugin);
                    }
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

  if (factory == NULL)
    {
      factory = g_object_new (TUMBLER_TYPE_PROVIDER_FACTORY, NULL);
      g_object_add_weak_pointer (G_OBJECT (factory), (gpointer) &factory);
    }
  else
    {
      g_object_ref (factory);
    }

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

  /* load available plugins */
  plugins = tumbler_provider_factory_load_plugins (factory);

  /* iterate over all provider infos */
  for (n = 0; n < factory->priv->provider_infos->len; ++n)
    {
      info = factory->priv->provider_infos->pdata[n];

      /* check if the provider type implements the given type */
      if (G_LIKELY (g_type_is_a (info->type, type)))
        {
          /* create the provider on demand */
          if (info->provider == NULL)
            info->provider = g_object_new (info->type, NULL);

          /* append the provider to the list */
          providers = g_list_append (providers, g_object_ref (info->provider));
        }
    }

  /* release all plugins */
  for (lp = plugins; lp != NULL; lp = lp->next)
    g_type_module_unuse (G_TYPE_MODULE (lp->data));
  g_list_free (plugins);

  return providers;
}
