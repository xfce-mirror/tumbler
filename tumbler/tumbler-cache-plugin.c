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
#include <glib/gi18n.h>
#include <glib-object.h>
#include <gmodule.h>

#include <tumbler/tumbler-cache.h>
#include <tumbler/tumbler-cache-plugin.h>
#include <tumbler/tumbler-error.h>



static void     tumbler_cache_plugin_constructed (GObject           *object);
static void     tumbler_cache_plugin_dispose     (GObject           *object);
static void     tumbler_cache_plugin_finalize    (GObject           *object);
static gboolean tumbler_cache_plugin_load        (GTypeModule       *type_module);
static void     tumbler_cache_plugin_unload      (GTypeModule       *type_module);



struct _TumblerCachePluginClass
{
  GTypeModuleClass __parent__;
};

struct _TumblerCachePlugin
{
  GTypeModule __parent__;

  GModule      *library;

  void          (*initialize) (TumblerCachePlugin *plugin);
  void          (*shutdown)   (void);
  TumblerCache *(*get_cache)  (void);
};



G_DEFINE_TYPE (TumblerCachePlugin, tumbler_cache_plugin, G_TYPE_TYPE_MODULE);



static void
tumbler_cache_plugin_class_init (TumblerCachePluginClass *klass)
{
  GTypeModuleClass *gtype_module_class;
  GObjectClass     *gobject_class;

  /* Determine the parent type class */
  tumbler_cache_plugin_parent_class = g_type_class_peek_parent (klass);

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->constructed = tumbler_cache_plugin_constructed; 
  gobject_class->dispose = tumbler_cache_plugin_dispose; 
  gobject_class->finalize = tumbler_cache_plugin_finalize; 

  gtype_module_class = G_TYPE_MODULE_CLASS (klass);
  gtype_module_class->load = tumbler_cache_plugin_load;
  gtype_module_class->unload = tumbler_cache_plugin_unload;
}



static void
tumbler_cache_plugin_init (TumblerCachePlugin *plugin)
{
}



static void
tumbler_cache_plugin_constructed (GObject *object)
{
}



static void
tumbler_cache_plugin_dispose (GObject *object)
{
  (*G_OBJECT_CLASS (tumbler_cache_plugin_parent_class)->dispose) (object);
}



static void
tumbler_cache_plugin_finalize (GObject *object)
{
  (*G_OBJECT_CLASS (tumbler_cache_plugin_parent_class)->finalize) (object);
}



static gboolean
tumbler_cache_plugin_load (GTypeModule *type_module)
{
  TumblerCachePlugin *plugin = TUMBLER_CACHE_PLUGIN (type_module);
  gchar              *path;

  /* load the module using the runtime link eeditor */
  path = g_build_filename (TUMBLER_PLUGIN_DIRECTORY, G_DIR_SEPARATOR_S, 
                           "cache", G_DIR_SEPARATOR_S, type_module->name, NULL);
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
          && g_module_symbol (plugin->library, "tumbler_plugin_get_cache",
                              (gpointer) &plugin->get_cache))
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
tumbler_cache_plugin_unload (GTypeModule *type_module)
{
  TumblerCachePlugin *plugin = TUMBLER_CACHE_PLUGIN (type_module);

  /* shutdown the plugin */
  (*plugin->shutdown) ();

  /* unload the plugin from memory */
  g_module_close (plugin->library);
  plugin->library = NULL;

  /* reset plugin state */
  plugin->initialize = NULL;
  plugin->shutdown = NULL;
  plugin->get_cache = NULL;
}



GTypeModule *
tumbler_cache_plugin_get_default (void)
{
  static TumblerCachePlugin *plugin = NULL;

  if (plugin == NULL)
    {
      plugin = g_object_new (TUMBLER_TYPE_CACHE_PLUGIN, NULL);
      g_type_module_set_name (G_TYPE_MODULE (plugin), 
                              "tumbler-cache-plugin." G_MODULE_SUFFIX);
      g_object_add_weak_pointer (G_OBJECT (plugin), (gpointer) &plugin);

      if (!g_type_module_use (G_TYPE_MODULE (plugin)))
        return NULL;
    }

  return G_TYPE_MODULE (plugin);
}



TumblerCache *
tumbler_cache_plugin_get_cache (TumblerCachePlugin *plugin)
{
  g_return_val_if_fail (TUMBLER_IS_CACHE_PLUGIN (plugin), NULL);
  return (*plugin->get_cache) ();
}
