/* vi:set et ai sw=2 sts=2 ts=2: */
/*-
 * Copyright (c) 2009-2011 Jannis Pohlmann <jannis@xfce.org>
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

#include "tumbler-abstract-thumbnailer.h"
#include "tumbler-thumbnailer.h"
#include "tumbler-visibility.h"



/* Property identifiers */
enum
{
  PROP_0,
  PROP_URI_SCHEMES,
  PROP_MIME_TYPES,
  PROP_HASH_KEYS,
  PROP_PRIORITY,
  PROP_MAX_FILE_SIZE,
  PROP_LOCATIONS,
  PROP_EXCLUDES
};



static void
tumbler_abstract_thumbnailer_thumbnailer_init (TumblerThumbnailerIface *iface);
static void
tumbler_abstract_thumbnailer_constructed (GObject *object);
static void
tumbler_abstract_thumbnailer_finalize (GObject *object);
static void
tumbler_abstract_thumbnailer_get_property (GObject *object,
                                           guint prop_id,
                                           GValue *value,
                                           GParamSpec *pspec);
static void
tumbler_abstract_thumbnailer_set_property (GObject *object,
                                           guint prop_id,
                                           const GValue *value,
                                           GParamSpec *pspec);
static void
tumbler_abstract_thumbnailer_create (TumblerThumbnailer *thumbnailer,
                                     GCancellable *cancellable,
                                     TumblerFileInfo *info);



struct _TumblerAbstractThumbnailerPrivate
{
  gchar **hash_keys;
  gchar **mime_types;
  gchar **uri_schemes;
  gint priority;
  gint64 max_file_size;
  GSList *locations;
  GSList *excludes;
};



G_DEFINE_TYPE_EXTENDED (TumblerAbstractThumbnailer,
                        tumbler_abstract_thumbnailer,
                        G_TYPE_OBJECT,
                        G_TYPE_FLAG_ABSTRACT,
                        G_IMPLEMENT_INTERFACE (TUMBLER_TYPE_THUMBNAILER,
                                               tumbler_abstract_thumbnailer_thumbnailer_init)
                        G_ADD_PRIVATE (TumblerAbstractThumbnailer));



static void
tumbler_abstract_thumbnailer_class_init (TumblerAbstractThumbnailerClass *klass)
{
  GObjectClass *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->constructed = tumbler_abstract_thumbnailer_constructed;
  gobject_class->finalize = tumbler_abstract_thumbnailer_finalize;
  gobject_class->get_property = tumbler_abstract_thumbnailer_get_property;
  gobject_class->set_property = tumbler_abstract_thumbnailer_set_property;

  g_object_class_override_property (gobject_class, PROP_MIME_TYPES, "mime-types");
  g_object_class_override_property (gobject_class, PROP_URI_SCHEMES, "uri-schemes");
  g_object_class_override_property (gobject_class, PROP_HASH_KEYS, "hash-keys");
  g_object_class_override_property (gobject_class, PROP_PRIORITY, "priority");
  g_object_class_override_property (gobject_class, PROP_MAX_FILE_SIZE, "max-file-size");
  g_object_class_override_property (gobject_class, PROP_LOCATIONS, "locations");
  g_object_class_override_property (gobject_class, PROP_EXCLUDES, "excludes");
}



static void
tumbler_abstract_thumbnailer_thumbnailer_init (TumblerThumbnailerIface *iface)
{
  iface->create = tumbler_abstract_thumbnailer_create;
}



static void
tumbler_abstract_thumbnailer_init (TumblerAbstractThumbnailer *thumbnailer)
{
  thumbnailer->priv = tumbler_abstract_thumbnailer_get_instance_private (thumbnailer);
}



static void
tumbler_abstract_thumbnailer_constructed (GObject *object)
{
  TumblerAbstractThumbnailer *thumbnailer = TUMBLER_ABSTRACT_THUMBNAILER (object);
  gchar *hash_key;
  guint num_hash_keys;
  guint num_mime_types;
  guint num_uri_schemes;
  guint i;
  guint j;

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
        hash_key = g_strdup_printf ("%s-%s",
                                    thumbnailer->priv->uri_schemes[i],
                                    thumbnailer->priv->mime_types[j]);

        /* add the key to the array */
        thumbnailer->priv->hash_keys[(j * num_uri_schemes) + i] = hash_key;
      }
}



static void
tumbler_abstract_thumbnailer_finalize (GObject *object)
{
  TumblerAbstractThumbnailer *thumbnailer = TUMBLER_ABSTRACT_THUMBNAILER (object);

  g_strfreev (thumbnailer->priv->hash_keys);
  g_strfreev (thumbnailer->priv->mime_types);
  g_strfreev (thumbnailer->priv->uri_schemes);

  g_slist_free_full (thumbnailer->priv->locations, g_object_unref);
  g_slist_free_full (thumbnailer->priv->excludes, g_object_unref);

  (*G_OBJECT_CLASS (tumbler_abstract_thumbnailer_parent_class)->finalize) (object);
}



static gpointer
tumbler_object_ref (gconstpointer object,
                    gpointer data)
{
  return g_object_ref ((gpointer) object);
}



static void
tumbler_abstract_thumbnailer_get_property (GObject *object,
                                           guint prop_id,
                                           GValue *value,
                                           GParamSpec *pspec)
{
  TumblerAbstractThumbnailer *thumbnailer = TUMBLER_ABSTRACT_THUMBNAILER (object);
  GSList *dup;

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

    case PROP_PRIORITY:
      g_value_set_int (value, thumbnailer->priv->priority);
      break;

    case PROP_MAX_FILE_SIZE:
      g_value_set_int64 (value, thumbnailer->priv->max_file_size);
      break;

    case PROP_LOCATIONS:
      dup = g_slist_copy_deep (thumbnailer->priv->locations, tumbler_object_ref, NULL);
      g_value_set_pointer (value, dup);
      break;

    case PROP_EXCLUDES:
      dup = g_slist_copy_deep (thumbnailer->priv->excludes, tumbler_object_ref, NULL);
      g_value_set_pointer (value, dup);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}



static void
tumbler_abstract_thumbnailer_set_property (GObject *object,
                                           guint prop_id,
                                           const GValue *value,
                                           GParamSpec *pspec)
{
  TumblerAbstractThumbnailer *thumbnailer = TUMBLER_ABSTRACT_THUMBNAILER (object);
  GSList *dup;

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

    case PROP_PRIORITY:
      thumbnailer->priv->priority = g_value_get_int (value);
      break;

    case PROP_MAX_FILE_SIZE:
      thumbnailer->priv->max_file_size = g_value_get_int64 (value);
      break;

    case PROP_LOCATIONS:
      dup = g_slist_copy_deep (g_value_get_pointer (value), tumbler_object_ref, NULL);
      thumbnailer->priv->locations = dup;
      break;

    case PROP_EXCLUDES:
      dup = g_slist_copy_deep (g_value_get_pointer (value), tumbler_object_ref, NULL);
      thumbnailer->priv->excludes = dup;
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}



static void
tumbler_abstract_thumbnailer_create (TumblerThumbnailer *thumbnailer,
                                     GCancellable *cancellable,
                                     TumblerFileInfo *info)
{
  g_return_if_fail (TUMBLER_IS_ABSTRACT_THUMBNAILER (thumbnailer));
  g_return_if_fail (TUMBLER_IS_FILE_INFO (info));
  g_return_if_fail (TUMBLER_ABSTRACT_THUMBNAILER_GET_CLASS (thumbnailer)->create != NULL);

  TUMBLER_ABSTRACT_THUMBNAILER_GET_CLASS (thumbnailer)->create (TUMBLER_ABSTRACT_THUMBNAILER (thumbnailer), cancellable, info);
}

#define __TUMBLER_ABSTRACT_THUMBNAILER_C__
#include "tumbler-visibility.c"
