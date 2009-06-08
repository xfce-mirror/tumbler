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

#include <glib.h>
#include <glib/gi18n.h>
#include <glib-object.h>
#include <gmodule.h>

#include "tumbler/tumbler-provider-plugin.h"



#define TUMBLER_PROVIDER_PLUGIN_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), TUMBLER_TYPE_PROVIDER_PLUGIN, TumblerProviderPluginPrivate))



/* Property identifiers */
enum
{
  PROP_0,
};



static void     tumbler_provider_plugin_class_init   (TumblerProviderPluginClass *klass);
static void     tumbler_provider_plugin_init         (TumblerProviderPlugin      *plugin);
static void     tumbler_provider_plugin_finalize     (GObject                    *object);
static gboolean tumbler_provider_plugin_load         (GTypeModule                *type_module);
static void     tumbler_provider_plugin_unload       (GTypeModule                *type_module);



struct _TumblerProviderPluginClass
{
  GTypeModuleClass __parent__;
};

struct _TumblerProviderPlugin
{
  GTypeModule __parent__;

  TumblerProviderPluginPrivate *priv;
};

struct _TumblerProviderPluginPrivate
{
  GModule *library;

  void (*initialize) (TumblerProviderPlugin *plugin);
  void (*shutdown)   (void);
  void (*get_types)  (const GType          **types,
                      gint                  *n_types);
};



static GObjectClass *tumbler_provider_plugin_parent_class = NULL;



GType
tumbler_provider_plugin_get_type (void)
{
  static GType type = G_TYPE_INVALID;

  if (G_UNLIKELY (type == G_TYPE_INVALID))
    {
      type = g_type_register_static_simple (G_TYPE_TYPE_MODULE, 
                                            "TumblerProviderPlugin",
                                            sizeof (TumblerProviderPluginClass),
                                            (GClassInitFunc) tumbler_provider_plugin_class_init,
                                            sizeof (TumblerProviderPlugin),
                                            (GInstanceInitFunc) tumbler_provider_plugin_init,
                                            0);
    }

  return type;
}



static void
tumbler_provider_plugin_class_init (TumblerProviderPluginClass *klass)
{
  GTypeModuleClass *gtype_module_class;
  GObjectClass     *gobject_class;

  g_type_class_add_private (klass, sizeof (TumblerProviderPluginPrivate));

  /* Determine the parent type class */
  tumbler_provider_plugin_parent_class = g_type_class_peek_parent (klass);

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->finalize = tumbler_provider_plugin_finalize; 

  gtype_module_class = G_TYPE_MODULE_CLASS (klass);
  gtype_module_class->load = tumbler_provider_plugin_load;
  gtype_module_class->unload = tumbler_provider_plugin_unload;
}



static void
tumbler_provider_plugin_init (TumblerProviderPlugin *plugin)
{
  plugin->priv = TUMBLER_PROVIDER_PLUGIN_GET_PRIVATE (plugin);
}



static void
tumbler_provider_plugin_finalize (GObject *object)
{
  TumblerProviderPlugin *plugin = TUMBLER_PROVIDER_PLUGIN (object);

  if (plugin->priv->library != NULL)
    g_module_close (plugin->priv->library);

  (*G_OBJECT_CLASS (tumbler_provider_plugin_parent_class)->finalize) (object);
}



static gboolean
tumbler_provider_plugin_load (GTypeModule *type_module)
{
  TumblerProviderPlugin *plugin = TUMBLER_PROVIDER_PLUGIN (type_module);
  gchar                 *path;

  /* load the module using the runtime link editor */
  path = g_build_filename (TUMBLER_PLUGIN_DIRECTORY, type_module->name, NULL);
  plugin->priv->library = g_module_open (path, G_MODULE_BIND_LAZY | G_MODULE_BIND_LOCAL);
  g_free (path);

  /* check if the load operation was successful */
  if (G_UNLIKELY (plugin->priv->library != NULL))
    {
      /* verify that all required public symbols are present in the plugin */
      if (g_module_symbol (plugin->priv->library, "tumbler_plugin_initialize", 
                            (gpointer) &plugin->priv->initialize)
          && g_module_symbol (plugin->priv->library, "tumbler_plugin_shutdown",
                              (gpointer) &plugin->priv->shutdown)
          && g_module_symbol (plugin->priv->library, "tumbler_plugin_get_types",
                              (gpointer) &plugin->priv->get_types))
        {
          /* initialize the plugin */
          (*plugin->priv->initialize) (plugin);
          return TRUE;
        }
      else
        {
          g_warning (_("Plugin \"%s\" lacks required symbols."), type_module->name);
          g_module_close (plugin->priv->library);
          return FALSE;
        }
    }
  else
    {
      g_warning (_("Failed to load plugin \"%s\": %s"), type_module->name, 
                 g_module_error ());
      return FALSE;
    }
}



static void
tumbler_provider_plugin_unload (GTypeModule *type_module)
{
  TumblerProviderPlugin *plugin = TUMBLER_PROVIDER_PLUGIN (type_module);

  /* shutdown the plugin */
  (*plugin->priv->shutdown) ();

  /* unload the plugin from memory */
  g_module_close (plugin->priv->library);
  plugin->priv->library != NULL;

  /* reset plugin state */
  plugin->priv->library = NULL;
  plugin->priv->initialize = NULL;
  plugin->priv->shutdown = NULL;
  plugin->priv->get_types = NULL;
}



TumblerProviderPlugin *
tumbler_provider_plugin_new (const gchar *filename)
{
  TumblerProviderPlugin *plugin;

  g_return_val_if_fail (filename != NULL && *filename != '\0', NULL);

  plugin = g_object_new (TUMBLER_TYPE_PROVIDER_PLUGIN, NULL);
  g_type_module_set_name (G_TYPE_MODULE (plugin), filename);

  return plugin;
}



void
tumbler_provider_plugin_get_types (const TumblerProviderPlugin *plugin,
                                   const GType                **types,
                                   gint                        *n_types)
{
  g_return_if_fail (TUMBLER_IS_PROVIDER_PLUGIN (plugin));
  g_return_if_fail (plugin->priv->get_types != NULL);
  g_return_if_fail (types != NULL);
  g_return_if_fail (n_types != NULL);

  (*plugin->priv->get_types) (types, n_types);
}
