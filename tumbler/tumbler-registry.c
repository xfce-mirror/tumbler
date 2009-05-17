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

#include <tumbler/tumbler-registry.h>



#define TUMBLER_REGISTRY_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), TUMBLER_TYPE_REGISTRY, TumblerRegistryPrivate))



/* Property identifiers */
enum
{
  PROP_0,
};



static void tumbler_registry_class_init   (TumblerRegistryClass *klass);
static void tumbler_registry_init         (TumblerRegistry      *registry);
static void tumbler_registry_finalize     (GObject              *object);
static void tumbler_registry_get_property (GObject              *object,
                                           guint                 prop_id,
                                           GValue               *value,
                                           GParamSpec           *pspec);
static void tumbler_registry_set_property (GObject              *object,
                                           guint                 prop_id,
                                           const GValue         *value,
                                           GParamSpec           *pspec);



struct _TumblerRegistryClass
{
  GObjectClass __parent__;
};

struct _TumblerRegistry
{
  GObject __parent__;

  TumblerRegistryPrivate *priv;
};

struct _TumblerRegistryPrivate
{
  GHashTable *thumbnailers;
};



static GObjectClass *tumbler_registry_parent_class = NULL;



GType
tumbler_registry_get_type (void)
{
  static GType type = G_TYPE_INVALID;

  if (G_UNLIKELY (type == G_TYPE_INVALID))
    {
      type = g_type_register_static_simple (G_TYPE_OBJECT, 
                                            "TumblerRegistry",
                                            sizeof (TumblerRegistryClass),
                                            (GClassInitFunc) tumbler_registry_class_init,
                                            sizeof (TumblerRegistry),
                                            (GInstanceInitFunc) tumbler_registry_init,
                                            0);
    }

  return type;
}



static void
tumbler_registry_class_init (TumblerRegistryClass *klass)
{
  GObjectClass *gobject_class;

  g_type_class_add_private (klass, sizeof (TumblerRegistryPrivate));

  /* Determine the parent type class */
  tumbler_registry_parent_class = g_type_class_peek_parent (klass);

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->finalize = tumbler_registry_finalize; 
  gobject_class->get_property = tumbler_registry_get_property;
  gobject_class->set_property = tumbler_registry_set_property;
}



static void
tumbler_registry_init (TumblerRegistry *registry)
{
  registry->priv = TUMBLER_REGISTRY_GET_PRIVATE (registry);
}



static void
tumbler_registry_finalize (GObject *object)
{
#if 0
  TumblerRegistry *registry = TUMBLER_REGISTRY (object);
#endif

  (*G_OBJECT_CLASS (tumbler_registry_parent_class)->finalize) (object);
}



static void
tumbler_registry_get_property (GObject    *object,
                               guint       prop_id,
                               GValue     *value,
                               GParamSpec *pspec)
{
#if 0
  TumblerRegistry *registry = TUMBLER_REGISTRY (object);
#endif

  switch (prop_id)
    {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}



static void
tumbler_registry_set_property (GObject      *object,
                               guint         prop_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
#if 0
  TumblerRegistry *registry = TUMBLER_REGISTRY (object);
#endif

  switch (prop_id)
    {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}



TumblerRegistry *
tumbler_registry_new (void)
{
  return g_object_new (TUMBLER_TYPE_REGISTRY, NULL);
}



gboolean
tumbler_registry_load (TumblerRegistry *registry,
                       GError         **error)
{
  g_return_val_if_fail (TUMBLER_IS_REGISTRY (registry), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  return TRUE;
}
