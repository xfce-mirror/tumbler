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

#include "tumbler-error.h"
#include "tumbler-provider-plugin.h"
#include "tumbler-visibility.h"

#include <glib/gi18n.h>
#include <gmodule.h>



/* Property identifiers */
enum
{
  PROP_0,
};



static void
tumbler_provider_plugin_finalize (GObject *object);
static gboolean
tumbler_provider_plugin_load (GTypeModule *type_module);
static void
tumbler_provider_plugin_unload (GTypeModule *type_module);



struct _TumblerProviderPlugin
{
  GTypeModule __parent__;

  GModule *library;

  void (*initialize) (TumblerProviderPlugin *plugin);
  void (*shutdown) (void);
  void (*get_types) (const GType **types,
                     gint *n_types);
};



G_DEFINE_TYPE (TumblerProviderPlugin, tumbler_provider_plugin, G_TYPE_TYPE_MODULE)



static void
tumbler_provider_plugin_class_init (TumblerProviderPluginClass *klass)
{
  GTypeModuleClass *gtype_module_class;
  GObjectClass *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->finalize = tumbler_provider_plugin_finalize;

  gtype_module_class = G_TYPE_MODULE_CLASS (klass);
  gtype_module_class->load = tumbler_provider_plugin_load;
  gtype_module_class->unload = tumbler_provider_plugin_unload;
}



static void
tumbler_provider_plugin_init (TumblerProviderPlugin *plugin)
{
}



static void
tumbler_provider_plugin_finalize (GObject *object)
{
  TumblerProviderPlugin *plugin = TUMBLER_PROVIDER_PLUGIN (object);

  if (plugin->library != NULL)
    g_module_close (plugin->library);

  (*G_OBJECT_CLASS (tumbler_provider_plugin_parent_class)->finalize) (object);
}



static gboolean
tumbler_provider_plugin_load (GTypeModule *type_module)
{
  TumblerProviderPlugin *plugin = TUMBLER_PROVIDER_PLUGIN (type_module);
  gchar *path;

  /* load the module using the runtime link editor */
  path = g_build_filename (TUMBLER_PLUGIN_DIRECTORY, type_module->name, NULL);
  plugin->library = g_module_open (path, G_MODULE_BIND_LAZY | G_MODULE_BIND_LOCAL);
  g_free (path);

  /* check if the load operation was successful */
  if (G_LIKELY (plugin->library != NULL))
    {
      /* verify that all required public symbols are present in the plugin */
      if (g_module_symbol (plugin->library, "tumbler_plugin_initialize",
                           (gpointer) &plugin->initialize)
          && g_module_symbol (plugin->library, "tumbler_plugin_shutdown",
                              (gpointer) &plugin->shutdown)
          && g_module_symbol (plugin->library, "tumbler_plugin_get_types",
                              (gpointer) &plugin->get_types))
        {
          /* initialize the plugin */
          (*plugin->initialize) (plugin);
          return TRUE;
        }
      else
        {
          g_warning (TUMBLER_WARNING_PLUGIN_LACKS_SYMBOLS, type_module->name);
          g_module_close (plugin->library);
          plugin->library = NULL;
          return FALSE;
        }
    }
  else
    {
      g_warning (TUMBLER_WARNING_LOAD_PLUGIN_FAILED, type_module->name, g_module_error ());
      return FALSE;
    }
}



static void
tumbler_provider_plugin_unload (GTypeModule *type_module)
{
  TumblerProviderPlugin *plugin = TUMBLER_PROVIDER_PLUGIN (type_module);

  /* shutdown the plugin */
  (*plugin->shutdown) ();

  /* unload the plugin from memory */
  g_module_close (plugin->library);
  plugin->library = NULL;

  /* reset plugin state */
  plugin->initialize = NULL;
  plugin->shutdown = NULL;
  plugin->get_types = NULL;
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
                                   const GType **types,
                                   gint *n_types)
{
  g_return_if_fail (TUMBLER_IS_PROVIDER_PLUGIN ((TumblerProviderPlugin *) plugin));
  g_return_if_fail (plugin->get_types != NULL);
  g_return_if_fail (types != NULL);
  g_return_if_fail (n_types != NULL);

  (*plugin->get_types) (types, n_types);
}

#define __TUMBLER_PROVIDER_PLUGIN_C__
#include "tumbler-visibility.c"
