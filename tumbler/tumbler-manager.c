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
#include <glib-object.h>
#include <glib/gi18n.h>

#include <dbus/dbus.h>
#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-lowlevel.h>

#include <tumbler/tumbler-manager.h>
#include <tumbler/tumbler-manager-dbus-bindings.h>
#include <tumbler/tumbler-specialized-thumbnailer.h>
#include <tumbler/tumbler-utils.h>



#define TUMBLER_MANAGER_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), TUMBLER_TYPE_MANAGER, TumblerManagerPrivate))



/* Property identifiers */
enum
{
  PROP_0,
  PROP_CONNECTION,
  PROP_REGISTRY,
};



static void tumbler_manager_class_init   (TumblerManagerClass *klass);
static void tumbler_manager_init         (TumblerManager      *manager);
static void tumbler_manager_constructed  (GObject                *object);
static void tumbler_manager_finalize     (GObject                *object);
static void tumbler_manager_get_property (GObject                *object,
                                          guint                   prop_id,
                                          GValue                 *value,
                                          GParamSpec             *pspec);
static void tumbler_manager_set_property (GObject                *object,
                                          guint                   prop_id,
                                          const GValue           *value,
                                          GParamSpec             *pspec);



struct _TumblerManagerClass
{
  GObjectClass __parent__;
};

struct _TumblerManager
{
  GObject __parent__;

  TumblerManagerPrivate *priv;
};

struct _TumblerManagerPrivate
{
  DBusGConnection *connection;
  TumblerRegistry *registry;

  GMutex          *mutex;
};



static GObjectClass *tumbler_manager_parent_class = NULL;



GType
tumbler_manager_get_type (void)
{
  static GType type = G_TYPE_INVALID;

  if (G_UNLIKELY (type == G_TYPE_INVALID))
    {
      type = g_type_register_static_simple (G_TYPE_OBJECT, 
                                            "TumblerManager",
                                            sizeof (TumblerManagerClass),
                                            (GClassInitFunc) tumbler_manager_class_init,
                                            sizeof (TumblerManager),
                                            (GInstanceInitFunc) tumbler_manager_init,
                                            0);
    }

  return type;
}



static void
tumbler_manager_class_init (TumblerManagerClass *klass)
{
  GObjectClass *gobject_class;

  g_type_class_add_private (klass, sizeof (TumblerManagerPrivate));

  /* Determine the parent type class */
  tumbler_manager_parent_class = g_type_class_peek_parent (klass);

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->constructed = tumbler_manager_constructed; 
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
  manager->priv = TUMBLER_MANAGER_GET_PRIVATE (manager);
  manager->priv->mutex = g_mutex_new ();
}



static void
tumbler_manager_constructed (GObject *object)
{
#if 0
  TumblerManager *manager = TUMBLER_MANAGER (object);
#endif
}



static void
tumbler_manager_finalize (GObject *object)
{
  TumblerManager *manager = TUMBLER_MANAGER (object);

  g_object_unref (manager->priv->registry);

  dbus_g_connection_unref (manager->priv->connection);

  g_mutex_free (manager->priv->mutex);

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
      g_value_set_pointer (value, manager->priv->connection);
      break;
    case PROP_REGISTRY:
      g_value_set_object (value, manager->priv->registry);
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
      manager->priv->connection = dbus_g_connection_ref (g_value_get_pointer (value));
      break;
    case PROP_REGISTRY:
      value, manager->priv->registry = g_value_dup_object (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}



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

  g_mutex_lock (manager->priv->mutex);

  /* initialize the D-Bus error */
  dbus_error_init (&dbus_error);

  /* get the native D-Bus connection */
  connection = dbus_g_connection_get_connection (manager->priv->connection);

  /* request ownership for the manager interface */
  result = dbus_bus_request_name (connection, "org.freedesktop.thumbnailer.Manager",
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

      g_mutex_unlock (manager->priv->mutex);

      /* i can't work like this! */
      return FALSE;
    }

  /* everything's fine, install the manager type D-Bus info */
  dbus_g_object_type_install_info (G_OBJECT_TYPE (manager), 
                                   &dbus_glib_tumbler_manager_object_info);

  /* register the manager instance as a handler of the manager interface */
  dbus_g_connection_register_g_object (manager->priv->connection, "/", 
                                       G_OBJECT (manager));

  g_mutex_unlock (manager->priv->mutex);

  /* this is how I roll */
  return TRUE;
}



void
tumbler_manager_register (TumblerManager        *manager, 
                          gchar                 *uri_scheme, 
                          gchar                 *mime_type, 
                          DBusGMethodInvocation *context)
{
  TumblerThumbnailer *thumbnailer;
  gchar              *sender_name;

  dbus_async_return_if_fail (TUMBLER_IS_MANAGER (manager), context);
  dbus_async_return_if_fail (uri_scheme != NULL, context);
  dbus_async_return_if_fail (mime_type != NULL, context);

  sender_name = dbus_g_method_get_sender (context);

  g_mutex_lock (manager->priv->mutex);

  thumbnailer = tumbler_specialized_thumbnailer_new_foreign (manager->priv->connection,
                                                             sender_name, uri_scheme, 
                                                             mime_type);

  tumbler_registry_add (manager->priv->registry, thumbnailer);

  g_object_unref (thumbnailer);

  g_mutex_unlock (manager->priv->mutex);

  g_free (sender_name);

  dbus_g_method_return (context);
}



void
tumbler_manager_get_supported (TumblerManager        *manager, 
                               DBusGMethodInvocation *context)
{
  GHashTable *types;
  GList      *thumbnailers;
  GList      *keys;
  GList      *lp;
  GStrv       supported_types;
  GStrv       mime_types;
  gint        n;

  dbus_async_return_if_fail (TUMBLER_IS_MANAGER (manager), context);

  /* create a hash table to collect unique MIME types */
  types = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);

  g_mutex_lock (manager->priv->mutex);

  /* get a list of all active thumbnailers */
  thumbnailers = tumbler_registry_get_thumbnailers (manager->priv->registry);

  /* iterate over all of them */
  for (lp = thumbnailers; lp != NULL; lp = lp->next)
    {
      /* determine the MIME types supported by the current thumbnailer */
      mime_types = tumbler_thumbnailer_get_mime_types (lp->data);

      /* insert all MIME types into the hash table */
      for (n = 0; mime_types != NULL && mime_types[n] != NULL; ++n)
        g_hash_table_replace (types, g_strdup (mime_types[n]), NULL);
    }

  /* relase the thumbnailer list */
  g_list_free (thumbnailers);
  
  g_mutex_unlock (manager->priv->mutex);

  /* determine all suported MIME types */
  keys = g_hash_table_get_keys (types);

  /* allocate a string array for them */
  supported_types = g_new0 (gchar *, g_list_length (keys) + 1);

  /* insert all MIME types into the array */
  for (lp = keys, n = 0; lp != NULL; lp = lp->next, ++n)
    supported_types[n] = g_strdup (lp->data);

  /* NULL-terminate the array */
  supported_types[n] = NULL;

  /* release the list of supported MIME types */
  g_list_free (keys);

  /* destroy the hash table we used */
  g_hash_table_unref (types);

  dbus_g_method_return (context, supported_types);
}
