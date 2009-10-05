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

#include <tumbler/tumbler-abstract-thumbnailer.h>
#include <tumbler/tumbler-thumbnailer.h>



#define TUMBLER_ABSTRACT_THUMBNAILER_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), TUMBLER_TYPE_ABSTRACT_THUMBNAILER, TumblerAbstractThumbnailerPrivate))



/* Property identifiers */
enum
{
  PROP_0,
  PROP_URI_SCHEMES,
  PROP_MIME_TYPES,
  PROP_HASH_KEYS,
};



static void tumbler_abstract_thumbnailer_thumbnailer_init (TumblerThumbnailerIface *iface);
static void tumbler_abstract_thumbnailer_constructed      (GObject                 *object);
static void tumbler_abstract_thumbnailer_finalize         (GObject                 *object);
static void tumbler_abstract_thumbnailer_get_property     (GObject                 *object,
                                                           guint                    prop_id,
                                                           GValue                  *value,
                                                           GParamSpec              *pspec);
static void tumbler_abstract_thumbnailer_set_property     (GObject                 *object,
                                                           guint                    prop_id,
                                                           const GValue            *value,
                                                           GParamSpec              *pspec);
static void tumbler_abstract_thumbnailer_create           (TumblerThumbnailer      *thumbnailer,
                                                           const gchar             *uri,
                                                           const gchar             *mime_hint);



struct _TumblerAbstractThumbnailerPrivate
{
  GStrv hash_keys;
  GStrv mime_types;
  GStrv uri_schemes;
};



G_DEFINE_TYPE_EXTENDED (TumblerAbstractThumbnailer,
                        tumbler_abstract_thumbnailer,
                        G_TYPE_OBJECT,
                        G_TYPE_FLAG_ABSTRACT,
                        G_IMPLEMENT_INTERFACE (TUMBLER_TYPE_THUMBNAILER,
                                               tumbler_abstract_thumbnailer_thumbnailer_init));



static void
tumbler_abstract_thumbnailer_class_init (TumblerAbstractThumbnailerClass *klass)
{
  GObjectClass *gobject_class;

  g_type_class_add_private (klass, sizeof (TumblerAbstractThumbnailerPrivate));

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->constructed = tumbler_abstract_thumbnailer_constructed; 
  gobject_class->finalize = tumbler_abstract_thumbnailer_finalize; 
  gobject_class->get_property = tumbler_abstract_thumbnailer_get_property;
  gobject_class->set_property = tumbler_abstract_thumbnailer_set_property;

  g_object_class_override_property (gobject_class, PROP_MIME_TYPES, "mime-types");
  g_object_class_override_property (gobject_class, PROP_URI_SCHEMES, "uri-schemes");
  g_object_class_override_property (gobject_class, PROP_HASH_KEYS, "hash-keys");
}



static void
tumbler_abstract_thumbnailer_thumbnailer_init (TumblerThumbnailerIface *iface)
{
  iface->create = tumbler_abstract_thumbnailer_create;
}



static void
tumbler_abstract_thumbnailer_init (TumblerAbstractThumbnailer *thumbnailer)
{
  thumbnailer->priv = TUMBLER_ABSTRACT_THUMBNAILER_GET_PRIVATE (thumbnailer);
}



static void
tumbler_abstract_thumbnailer_constructed (GObject *object)
{
  TumblerAbstractThumbnailer *thumbnailer = TUMBLER_ABSTRACT_THUMBNAILER (object);
  gchar                       *hash_key;
  guint                        num_hash_keys;
  guint                        num_mime_types;
  guint                        num_uri_schemes;
  guint                        i;
  guint                        j;

  g_return_if_fail (TUMBLER_IS_ABSTRACT_THUMBNAILER (thumbnailer));
  g_return_if_fail (thumbnailer->priv->mime_types != NULL);
  g_return_if_fail (thumbnailer->priv->uri_schemes != NULL);
  g_return_if_fail (thumbnailer->priv->hash_keys == NULL);

  /* chain up to parent classes */
  if (G_OBJECT_CLASS (tumbler_abstract_thumbnailer_parent_class)->constructed != NULL)
    (G_OBJECT_CLASS (tumbler_abstract_thumbnailer_parent_class)->constructed) (object);

  /* determine the size of both arrays */
  num_uri_schemes = g_strv_length (thumbnailer->priv->uri_schemes);
  num_mime_types = g_strv_length (thumbnailer->priv->mime_types);

  /* compute the number of hash keys to generate */
  num_hash_keys = num_uri_schemes * num_mime_types;

  /* allocate and NULL-terminate the hash key array */
  thumbnailer->priv->hash_keys = g_new0 (gchar *, num_hash_keys + 1);
  thumbnailer->priv->hash_keys[num_hash_keys] = NULL;

  /* iterate over all pairs of URIs and MIME types */
  for (i = 0; thumbnailer->priv->uri_schemes[i] != NULL; ++i)
    for (j = 0; thumbnailer->priv->mime_types[j] != NULL; ++j)
      {
        /* generate a hash key for the current pair */
        hash_key =  g_strdup_printf ("%s-%s", 
                                     thumbnailer->priv->uri_schemes[i],
                                     thumbnailer->priv->mime_types[j]);

        /* add the key to the array */
        thumbnailer->priv->hash_keys[(j*num_uri_schemes)+i] = hash_key;
      }
}



static void
tumbler_abstract_thumbnailer_finalize (GObject *object)
{
  TumblerAbstractThumbnailer *thumbnailer = TUMBLER_ABSTRACT_THUMBNAILER (object);

  g_strfreev (thumbnailer->priv->hash_keys);
  g_strfreev (thumbnailer->priv->mime_types);
  g_strfreev (thumbnailer->priv->uri_schemes);

  (*G_OBJECT_CLASS (tumbler_abstract_thumbnailer_parent_class)->finalize) (object);
}



static void
tumbler_abstract_thumbnailer_get_property (GObject    *object,
                                           guint       prop_id,
                                           GValue     *value,
                                           GParamSpec *pspec)
{
  TumblerAbstractThumbnailer *thumbnailer = TUMBLER_ABSTRACT_THUMBNAILER (object);

  switch (prop_id)
    {
    case PROP_MIME_TYPES:
      g_value_set_pointer (value, g_strdupv (thumbnailer->priv->mime_types));
      break;
    case PROP_URI_SCHEMES:
      g_value_set_pointer (value, g_strdupv (thumbnailer->priv->uri_schemes));
      break;
    case PROP_HASH_KEYS:
      g_value_set_pointer (value, g_strdupv (thumbnailer->priv->hash_keys));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}



static void
tumbler_abstract_thumbnailer_set_property (GObject      *object,
                                           guint         prop_id,
                                           const GValue *value,
                                           GParamSpec   *pspec)
{
  TumblerAbstractThumbnailer *thumbnailer = TUMBLER_ABSTRACT_THUMBNAILER (object);

  switch (prop_id)
    {
    case PROP_MIME_TYPES:
      thumbnailer->priv->mime_types = g_strdupv (g_value_get_pointer (value));
      break;
    case PROP_URI_SCHEMES:
      thumbnailer->priv->uri_schemes = g_strdupv (g_value_get_pointer (value));
      break;
    case PROP_HASH_KEYS:
      thumbnailer->priv->hash_keys = g_strdupv (g_value_get_pointer (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}



static void
tumbler_abstract_thumbnailer_create (TumblerThumbnailer *thumbnailer,
                                     const gchar        *uri,
                                     const gchar        *mime_hint)
{
  g_return_if_fail (TUMBLER_IS_ABSTRACT_THUMBNAILER (thumbnailer));
  g_return_if_fail (uri != NULL && *uri != '\0');
  g_return_if_fail (TUMBLER_ABSTRACT_THUMBNAILER_GET_CLASS (thumbnailer)->create != NULL);

  TUMBLER_ABSTRACT_THUMBNAILER_GET_CLASS (thumbnailer)->create (TUMBLER_ABSTRACT_THUMBNAILER (thumbnailer),
                                                                uri, mime_hint);
}
