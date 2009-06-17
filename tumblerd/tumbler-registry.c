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

#include <tumbler/tumbler.h>

#include <tumblerd/tumbler-registry.h>
#include <tumblerd/tumbler-specialized-thumbnailer.h>



/* Property identifiers */
enum
{
  PROP_0,
};



static void         tumbler_registry_finalize     (GObject            *object);
static void         tumbler_registry_get_property (GObject            *object,
                                                   guint               prop_id,
                                                   GValue             *value,
                                                   GParamSpec         *pspec);
static void         tumbler_registry_set_property (GObject            *object,
                                                   guint               prop_id,
                                                   const GValue       *value,
                                                   GParamSpec         *pspec);
static void         tumbler_registry_remove       (const gchar        *key,
                                                   GList             **list,
                                                   TumblerThumbnailer *thumbnailer);
static void         tumbler_registry_unregister   (TumblerThumbnailer *thumbnailer,
                                                   TumblerRegistry    *registry);
static void         tumbler_registry_list_free    (gpointer            data);
static gint         tumbler_registry_compare      (TumblerThumbnailer *a,
                                                   TumblerThumbnailer *b);
TumblerThumbnailer *tumbler_registry_lookup       (TumblerRegistry    *registry,
                                                   const gchar        *hash_key);



struct _TumblerRegistryClass
{
  GObjectClass __parent__;
};

struct _TumblerRegistry
{
  GObject __parent__;

  GHashTable *thumbnailers;
  GMutex     *mutex;
};



G_DEFINE_TYPE (TumblerRegistry, tumbler_registry, G_TYPE_OBJECT);



static void
tumbler_registry_class_init (TumblerRegistryClass *klass)
{
  GObjectClass *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->finalize = tumbler_registry_finalize; 
  gobject_class->get_property = tumbler_registry_get_property;
  gobject_class->set_property = tumbler_registry_set_property;
}



static void
tumbler_registry_init (TumblerRegistry *registry)
{
  registry->mutex = g_mutex_new ();
  registry->thumbnailers = g_hash_table_new_full (g_str_hash, g_str_equal,
                                                        g_free, 
                                                        tumbler_registry_list_free);
}



static void
tumbler_registry_finalize (GObject *object)
{
  TumblerRegistry *registry = TUMBLER_REGISTRY (object);

  g_hash_table_unref (registry->thumbnailers);
  g_mutex_free (registry->mutex);

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



static void
tumbler_registry_remove (const gchar        *key,
                         GList             **list,
                         TumblerThumbnailer *thumbnailer)
{
  GList *lp;

  for (lp = *list; lp != NULL; lp = lp->next)
    {
      if (lp->data == thumbnailer)
        *list = g_list_delete_link (*list, lp);
    }
}



static void
tumbler_registry_unregister (TumblerThumbnailer *thumbnailer,
                             TumblerRegistry    *registry)
{
  g_return_if_fail (TUMBLER_IS_THUMBNAILER (thumbnailer));
  g_return_if_fail (TUMBLER_IS_REGISTRY (registry));

  g_mutex_lock (registry->mutex);

  g_hash_table_foreach (registry->thumbnailers, (GHFunc) tumbler_registry_remove, 
                        thumbnailer);

  g_mutex_unlock (registry->mutex);
}



static gint
tumbler_registry_compare (TumblerThumbnailer *a,
                          TumblerThumbnailer *b)
{
  TumblerSpecializedThumbnailer *a_specialized;
  TumblerSpecializedThumbnailer *b_specialized;
  gboolean                       insert_a_before_b = FALSE;
  gboolean                       a_foreign;
  gboolean                       b_foreign;
  guint64                        a_modified;
  guint64                        b_modified;

  g_return_val_if_fail (TUMBLER_IS_THUMBNAILER (a), 0);
  g_return_val_if_fail (TUMBLER_IS_THUMBNAILER (b), 0);

  /* TODO Rewrite this based on a single get_registered() time function 
   * for all thumbnailer types */

  if (!TUMBLER_IS_SPECIALIZED_THUMBNAILER (a) || !TUMBLER_IS_SPECIALIZED_THUMBNAILER (b))
    {
      /* plugin thumbnailers are always overriden */
      insert_a_before_b = TRUE;
    }
  else if (TUMBLER_IS_SPECIALIZED_THUMBNAILER (b))
    {
      a_specialized = TUMBLER_SPECIALIZED_THUMBNAILER (a);
      b_specialized = TUMBLER_SPECIALIZED_THUMBNAILER (b);

      a_foreign = tumbler_specialized_thumbnailer_get_foreign (a_specialized);
      b_foreign = tumbler_specialized_thumbnailer_get_foreign (b_specialized);

      if (a_foreign || b_foreign)
        {
          /* both thumbnailers were registered dynamically over D-Bus but
           * whoever goes last wins */
          insert_a_before_b = TRUE;
        }
      else
        {
          b_modified = tumbler_specialized_thumbnailer_get_modified (a_specialized);
          a_modified = tumbler_specialized_thumbnailer_get_modified (a_specialized);

          if (a_modified > b_modified)
            {
              /* a and b are both specialized thumbnailers but a was installed
               * on the system last, so it wins */
              insert_a_before_b = TRUE;
            }
        }
    }
  else
    {
      /* we have no other thumbnailer types at the moment */
      g_assert_not_reached ();
    }

  return insert_a_before_b ? -1 : 1;
}



static void tumbler_registry_list_free (gpointer data)
{
  GList **list = data;

  /* make sure to release all thumbnailers */
  g_list_foreach (*list, (GFunc) g_object_unref, NULL);

  /* free the list and the pointer to it */
  g_list_free (*list);
  g_free (list);
}



TumblerThumbnailer *
tumbler_registry_lookup (TumblerRegistry *registry,
                         const gchar     *hash_key)
{
  TumblerThumbnailer *thumbnailer = NULL;
  GList             **list;
  GList              *first;

  g_return_val_if_fail (TUMBLER_IS_REGISTRY (registry), NULL);
  g_return_val_if_fail (hash_key != NULL, NULL);

  list = g_hash_table_lookup (registry->thumbnailers, hash_key);

  if (list != NULL)
    {
      first = g_list_first (*list);

      if (first != NULL)
        thumbnailer = g_object_ref (first->data);
    }

  return thumbnailer;
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

  /* TODO */

  return TRUE;
}



void
tumbler_registry_add (TumblerRegistry    *registry,
                      TumblerThumbnailer *thumbnailer)
{
  GList **list;
  GStrv   hash_keys;
  gint    n;

  g_return_if_fail (TUMBLER_IS_REGISTRY (registry));
  g_return_if_fail (TUMBLER_IS_THUMBNAILER (thumbnailer));

  g_mutex_lock (registry->mutex);

  /* determine the hash keys (all combinations of URI schemes and MIME types)
   * for this thumbnailer */
  hash_keys = tumbler_thumbnailer_get_hash_keys (thumbnailer);

  /* iterate over all of them */
  for (n = 0; hash_keys != NULL && hash_keys[n] != NULL; ++n)
    {
      /* fetch the thumbnailer list for this URI scheme/MIME type combination */
      list = g_hash_table_lookup (registry->thumbnailers, hash_keys[n]);

      if (list != NULL)
        {
          /* we already have thumbnailers for this combination. insert the new 
           * one at the right position in the list */
          *list = g_list_insert_sorted (*list, g_object_ref (thumbnailer),
                                        (GCompareFunc) tumbler_registry_compare);
        }
      else
        {
          /* allocate a new list */
          list = g_new0 (GList *, 1);

          /* insert the thumbnailer into the list */
          *list = g_list_prepend (*list, g_object_ref (thumbnailer));

          /* insert the pointer to the list in the hash table */
          g_hash_table_insert (registry->thumbnailers, g_strdup (hash_keys[n]), list);
        }

      /* connect to the unregister signal of the thumbnailer */
      g_signal_connect (thumbnailer, "unregister", 
                        G_CALLBACK (tumbler_registry_unregister), registry);
    }

  g_strfreev (hash_keys);

  g_mutex_unlock (registry->mutex);
}



GList *
tumbler_registry_get_thumbnailers (TumblerRegistry *registry)
{
  GList **list;
  GList  *thumbnailers = NULL;
  GList  *lists;
  GList  *lp;

  g_return_val_if_fail (TUMBLER_IS_REGISTRY (registry), NULL);

  g_mutex_lock (registry->mutex);

  /* get the thumbnailer lists */
  lists = g_hash_table_get_values (registry->thumbnailers);

  for (lp = lists; lp != NULL; lp = lp->next)
    {
      list = lp->data;

      /* add the first thumbnailer of each list to the output list */
      if (list != NULL && *list != NULL)
        thumbnailers = g_list_prepend (thumbnailers, (*list)->data);
    }

  /* release the thumbnailer list */
  g_list_free (lists);

  g_mutex_unlock (registry->mutex);

  /* return all active thumbnailers */
  return thumbnailers;
}



TumblerThumbnailer **
tumbler_registry_get_thumbnailer_array (TumblerRegistry *registry,
                                        const GStrv      uris,
                                        const GStrv      mime_hints,
                                        gint            *length)
{
  TumblerThumbnailer **thumbnailers = NULL;
  gchar               *hash_key;
  gchar               *scheme;
  gint                 num_mime_hints;
  gint                 num_thumbnailers;
  gint                 num_uris;
  gint                 n;

  g_return_val_if_fail (TUMBLER_IS_REGISTRY (registry), NULL);
  g_return_val_if_fail (uris != NULL, NULL);
  g_return_val_if_fail (mime_hints != NULL, NULL);

  num_uris = g_strv_length (uris);
  num_mime_hints = g_strv_length (mime_hints);
  
  /* we handle situations silently where num_uris != num_mime_hints */
  num_thumbnailers = MAX (0, MIN (num_uris, num_mime_hints));

  /* set the length return value */
  *length = num_thumbnailers;

  /* allocate the thumbnailer array */
  thumbnailers = g_new0 (TumblerThumbnailer *, num_thumbnailers + 1);

  /* iterate over all URIs */
  for (n = 0; n < num_thumbnailers; ++n)
    {
      g_mutex_lock (registry->mutex);

      /* determine the URI scheme and generate a hash key */
      scheme = g_uri_parse_scheme (uris[n]);
      hash_key = g_strdup_printf ("%s-%s", scheme, mime_hints[n]);

      /* see if we can find a thumbnailer to handle this URI/MIME type pair */
      thumbnailers[n] = tumbler_registry_lookup (registry, hash_key);

      /* if there is one, take a reference on it */
      if (thumbnailers[n] != NULL)
        g_object_ref (thumbnailers[n]);

      /* free strings */
      g_free (hash_key);
      g_free (scheme);

      g_mutex_unlock (registry->mutex);
    }

  /* NULL-terminate the array */
  thumbnailers[n] = NULL;

  return thumbnailers;
}
