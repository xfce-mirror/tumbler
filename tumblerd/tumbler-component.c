/* vi:set et ai sw=2 sts=2 ts=2: */
/*-
 * Copyright (c) 2011 Jannis Pohlmann <jannis@xfce.org>
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

#include <tumblerd/tumbler-component.h>
#include <tumblerd/tumbler-lifecycle-manager.h>



#define TUMBLER_COMPONENT_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), TUMBLER_TYPE_COMPONENT, TumblerComponentPrivate))



/* property identifiers */
enum
{
  PROP_0,
  PROP_LIFECYCLE_MANAGER,
};



static void tumbler_component_finalize     (GObject      *object);
static void tumbler_component_get_property (GObject      *object,
                                            guint         prop_id,
                                            GValue       *value,
                                            GParamSpec   *pspec);
static void tumbler_component_set_property (GObject      *object,
                                            guint         prop_id,
                                            const GValue *value,
                                            GParamSpec   *pspec);



struct _TumblerComponentPrivate
{
  TumblerLifecycleManager *lifecycle_manager;
};



G_DEFINE_TYPE (TumblerComponent, tumbler_component, G_TYPE_OBJECT)



static void
tumbler_component_class_init (TumblerComponentClass *klass)
{
  GObjectClass *gobject_class;

  g_type_class_add_private (klass, sizeof (TumblerComponentPrivate));

  /* Determine the parent type class */
  tumbler_component_parent_class = g_type_class_peek_parent (klass);

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->finalize = tumbler_component_finalize; 
  gobject_class->get_property = tumbler_component_get_property;
  gobject_class->set_property = tumbler_component_set_property;

  g_object_class_install_property (gobject_class,
                                   PROP_LIFECYCLE_MANAGER,
                                   g_param_spec_object ("lifecycle-manager",
                                                        "lifecycle-manager",
                                                        "lifecycle-manager",
                                                        TUMBLER_TYPE_LIFECYCLE_MANAGER,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));
}



static void
tumbler_component_init (TumblerComponent *component)
{
  component->priv = TUMBLER_COMPONENT_GET_PRIVATE (component);
}



static void
tumbler_component_finalize (GObject *object)
{
  TumblerComponent *component = TUMBLER_COMPONENT (object);

  g_object_unref (component->priv->lifecycle_manager);

  (*G_OBJECT_CLASS (tumbler_component_parent_class)->finalize) (object);
}



static void
tumbler_component_get_property (GObject    *object,
                                guint       prop_id,
                                GValue     *value,
                                GParamSpec *pspec)
{
  TumblerComponent *component = TUMBLER_COMPONENT (object);

  switch (prop_id)
    {
    case PROP_LIFECYCLE_MANAGER:
      g_value_set_object (value, component->priv->lifecycle_manager);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}



static void
tumbler_component_set_property (GObject      *object,
                                guint         prop_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
  TumblerComponent *component = TUMBLER_COMPONENT (object);

  switch (prop_id)
    {
    case PROP_LIFECYCLE_MANAGER:
      component->priv->lifecycle_manager = g_value_dup_object(value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}



gboolean
tumbler_component_keep_alive (TumblerComponent *component,
                              GError          **error)
{
  g_return_val_if_fail (TUMBLER_IS_COMPONENT (component), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  return tumbler_lifecycle_manager_keep_alive (component->priv->lifecycle_manager,
                                               error);
}



void
tumbler_component_increment_use_count (TumblerComponent *component)
{
  g_return_if_fail (TUMBLER_IS_COMPONENT (component));
  tumbler_lifecycle_manager_increment_use_count (component->priv->lifecycle_manager);
}



void
tumbler_component_decrement_use_count (TumblerComponent *component)
{
  g_return_if_fail (TUMBLER_IS_COMPONENT (component));
  tumbler_lifecycle_manager_decrement_use_count (component->priv->lifecycle_manager);
}
