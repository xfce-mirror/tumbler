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

#include <tumbler/tumbler-manager.h>



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
#if 0
  TumblerManager *manager = TUMBLER_MANAGER (object);
#endif

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
  g_return_val_if_fail (TUMBLER_IS_MANAGER (manager), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  return TRUE;
}
