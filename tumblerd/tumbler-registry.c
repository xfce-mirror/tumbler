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



static void         tumbler_registry_finalize                  (GObject            *object);
static void         tumbler_registry_remove_thumbnailer        (const gchar        *key,
                                                                GList             **list,
                                                                TumblerThumbnailer *thumbnailer);
static void         tumbler_registry_list_free                 (gpointer            data);
static GList       *tumbler_registry_get_thumbnailers_internal (TumblerRegistry    *registry);
static gint         tumbler_registry_compare                   (TumblerThumbnailer *a,
                                                                TumblerThumbnailer *b);
TumblerThumbnailer *tumbler_registry_lookup                    (TumblerRegistry    *registry,
                                                                const gchar        *hash_key);



struct _TumblerRegistryClass
{
  GObjectClass __parent__;
};

struct _TumblerRegistry
{
  GObject       __parent__;

  GHashTable   *thumbnailers;
  GHashTable   *preferred_thumbnailers;
  GMutex       *mutex;

  gchar       **uri_schemes;
  gchar       **mime_types;
};



G_DEFINE_TYPE (TumblerRegistry, tumbler_registry, G_TYPE_OBJECT);



static void
tumbler_registry_class_init (TumblerRegistryClass *klass)
{
  GObjectClass *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->finalize = tumbler_registry_finalize; 
}



static void
tumbler_registry_init (TumblerRegistry *registry)
{
  registry->mutex = g_mutex_new ();
  registry->thumbnailers = g_hash_table_new_full (g_str_hash, g_str_equal,
                                                  g_free, tumbler_registry_list_free);
  registry->preferred_thumbnailers = g_hash_table_new_full (g_str_hash, g_str_equal,
                                                            g_free, g_object_unref);
}



static void
tumbler_registry_finalize (GObject *object)
{
  TumblerRegistry *registry = TUMBLER_REGISTRY (object);

  /* release all thumbnailers */
  g_hash_table_unref (registry->preferred_thumbnailers);
  g_hash_table_unref (registry->thumbnailers);

  /* free the cached URI schemes and MIME types */
  g_strfreev (registry->uri_schemes);
  g_strfreev (registry->mime_types);

  /* destroy the mutex */
  g_mutex_free (registry->mutex);

  (*G_OBJECT_CLASS (tumbler_registry_parent_class)->finalize) (object);
}



#ifdef DEBUG
static void
dump_registry (TumblerRegistry *registry)
{
  TumblerThumbnailer *thumbnailer;
  GHashTableIter      iter;
  const gchar        *hash_key;
  GList             **thumbnailers;
  GList              *lp;

  g_print ("Registry:\n");

  g_print ("  Preferred Thumbnailers:\n");
  g_hash_table_iter_init (&iter, registry->preferred_thumbnailers);
  while (g_hash_table_iter_next (&iter, (gpointer) &hash_key, (gpointer) &thumbnailer))
    {
      g_print ("    %s: %s\n", hash_key,
               tumbler_specialized_thumbnailer_get_name (TUMBLER_SPECIALIZED_THUMBNAILER (thumbnailer)));
    }

  g_print ("  Registry Thumbnailers:\n");
  g_hash_table_iter_init (&iter, registry->thumbnailers);
  while (g_hash_table_iter_next (&iter, (gpointer) &hash_key, (gpointer) &thumbnailers))
    {
      for (lp = *thumbnailers; lp != NULL; lp = lp->next)
        {
          if (TUMBLER_IS_SPECIALIZED_THUMBNAILER (lp->data))
            {
              g_print ("    %s: %s\n", 
                       hash_key, tumbler_specialized_thumbnailer_get_name (lp->data));
            }
        }
    }

  g_print ("\n");
}
#endif



static void
tumbler_registry_remove_thumbnailer (const gchar        *key,
                                     GList             **list,
                                     TumblerThumbnailer *thumbnailer)
{
  GList *lp;

  for (lp = *list; lp != NULL; lp = lp->next)
    {
      if (lp->data == thumbnailer)
        {
          g_object_unref (lp->data);
          *list = g_list_delete_link (*list, lp);
          break;
        }
    }
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



static GList *
tumbler_registry_get_thumbnailers_internal (TumblerRegistry *registry)
{
  GList **list;
  GList  *thumbnailers = NULL;
  GList  *lists;
  GList  *lp;

  g_return_val_if_fail (TUMBLER_IS_REGISTRY (registry), NULL);

  /* get the thumbnailer lists */
  lists = g_hash_table_get_values (registry->thumbnailers);

  for (lp = lists; lp != NULL; lp = lp->next)
    {
      list = lp->data;

      /* add the first thumbnailer of each list to the output list */
      if (list != NULL && *list != NULL)
        thumbnailers = g_list_prepend (thumbnailers, g_object_ref ((*list)->data));
    }

  /* release the thumbnailer list */
  g_list_free (lists);

  return thumbnailers;
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

  thumbnailer = g_hash_table_lookup (registry->preferred_thumbnailers, hash_key);
  if (thumbnailer != NULL)
    return g_object_ref (thumbnailer);

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
    }

  /* connect to the unregister signal of the thumbnailer */
  g_signal_connect_swapped (thumbnailer, "unregister", 
                            G_CALLBACK (tumbler_registry_remove), registry);

  g_strfreev (hash_keys);

#ifdef DEBUG
  dump_registry (registry);
#endif

  g_mutex_unlock (registry->mutex);
}



void
tumbler_registry_remove (TumblerRegistry    *registry,
                         TumblerThumbnailer *thumbnailer)
{
  g_return_if_fail (TUMBLER_IS_REGISTRY (registry));
  g_return_if_fail (TUMBLER_IS_THUMBNAILER (thumbnailer));

  g_mutex_lock (registry->mutex);

  g_signal_handlers_disconnect_matched (thumbnailer, G_SIGNAL_MATCH_DATA, 
                                        0, 0, NULL, NULL, registry);
                                        
  /* remove the thumbnailer from all hash key lists */
  g_hash_table_foreach (registry->thumbnailers, 
                        (GHFunc) tumbler_registry_remove_thumbnailer, thumbnailer);

  g_mutex_unlock (registry->mutex);
}



GList *
tumbler_registry_get_thumbnailers (TumblerRegistry *registry)
{
  GList *thumbnailers;

  g_return_val_if_fail (TUMBLER_IS_REGISTRY (registry), NULL);

  g_mutex_lock (registry->mutex);

  thumbnailers = tumbler_registry_get_thumbnailers_internal (registry);

  g_mutex_unlock (registry->mutex);

  /* return all active thumbnailers */
  return thumbnailers;
}



TumblerThumbnailer **
tumbler_registry_get_thumbnailer_array (TumblerRegistry    *registry,
                                        TumblerFileInfo   **infos,
                                        guint               length)
{
  TumblerThumbnailer **thumbnailers = NULL;
  gchar               *hash_key;
  gchar               *scheme;
  guint                n;

  g_return_val_if_fail (TUMBLER_IS_REGISTRY (registry), NULL);
  g_return_val_if_fail (infos != NULL, NULL);

  for (length = 0; infos != NULL && infos[length] != NULL; ++length);

  /* allocate the thumbnailer array */
  thumbnailers = g_new0 (TumblerThumbnailer *, length + 1);

  /* iterate over all URIs */
  for (n = 0; n < length; ++n)
    {
      g_mutex_lock (registry->mutex);

      /* determine the URI scheme and generate a hash key */
      scheme = g_uri_parse_scheme (tumbler_file_info_get_uri (infos[n]));
      hash_key = g_strdup_printf ("%s-%s", scheme, 
                                  tumbler_file_info_get_mime_type (infos[n]));

      /* see if we can find a thumbnailer to handle this URI/MIME type pair */
      thumbnailers[n] = tumbler_registry_lookup (registry, hash_key);

      /* free strings */
      g_free (hash_key);
      g_free (scheme);

      g_mutex_unlock (registry->mutex);
    }

  /* NULL-terminate the array */
  thumbnailers[n] = NULL;

  return thumbnailers;
}



static void
free_pair (gpointer data)
{
  g_slice_free1 (2 * sizeof (const gchar *), data);
}



void
tumbler_registry_update_supported (TumblerRegistry *registry)
{
  GHashTableIter iter;
  GHashTable    *unique_pairs;
  GSList        *used_strings = NULL;
  GList         *thumbnailers;
  GList         *lp;
  const gchar  **pair;
  GString       *pair_string;
  GStrv          mime_types;
  GStrv          uri_schemes;
  gint           n;
  gint           u;

  g_return_if_fail (TUMBLER_IS_REGISTRY (registry));

  g_mutex_lock (registry->mutex);

  /* free the old cache */
  g_strfreev (registry->uri_schemes);
  registry->uri_schemes = NULL;
  g_strfreev (registry->mime_types);
  registry->mime_types = NULL;

  /* get a list of all active thumbnailers */
  thumbnailers = tumbler_registry_get_thumbnailers_internal (registry);

  g_mutex_unlock (registry->mutex);

  /* abort if there are no thumbnailers */
  if (thumbnailers == NULL)
    return;

  /* create a hash table to collect unique URI scheme / MIME type pairs */
  unique_pairs = g_hash_table_new_full (g_str_hash, g_str_equal, 
                                        (GDestroyNotify) g_free, 
                                        (GDestroyNotify) free_pair);

  /* iterate over all of them */
  for (lp = thumbnailers; lp != NULL; lp = lp->next)
    {
      /* determine the MIME types & URI schemes supported by the current thumbnailer */
      mime_types = tumbler_thumbnailer_get_mime_types (lp->data);
      uri_schemes = tumbler_thumbnailer_get_uri_schemes (lp->data);

      /* insert all MIME types & URI schemes into the hash table */
      for (n = 0; 
           mime_types != NULL && uri_schemes != NULL && mime_types[n] != NULL; 
           ++n)
        {
          /* remember the MIME type so that we can later reuse it without copying */
          used_strings = g_slist_prepend (used_strings, mime_types[n]);

          for (u = 0; uri_schemes[u] != NULL; ++u)
            {
              /* remember the URI scheme for this pair so that we can later reuse it 
               * without copying. Only remember it once (n==0) to avoid segmentation 
               * faults when freeing the list */
              if (n == 0)
                used_strings = g_slist_prepend (used_strings, uri_schemes[u]);

              /* allocate a pair with the current URI scheme and MIME type */
              pair = g_slice_alloc (2 * sizeof (const gchar *));

              /* we can now reuse the strings */
              pair[0] = uri_schemes[u];
              pair[1] = mime_types[n];

              /* combine the two to a unique pair identifier */
              pair_string = g_string_new (pair[0]);
              g_string_append_c (pair_string, '-');
              g_string_append (pair_string, pair[1]);

              /* remember the pair in the hash table */
              g_hash_table_insert (unique_pairs, pair_string->str, pair);

              /* free the pair string */
              g_string_free (pair_string, FALSE);
            }
        }

      /* free MIME types & URI schemes array. Their contents are stored in
       * used_strings and are freed later */
      g_free (mime_types);
      g_free (uri_schemes);
    }

  /* relase the thumbnailer list */
  g_list_foreach (thumbnailers, (GFunc) g_object_unref, NULL);
  g_list_free (thumbnailers);

  n = g_hash_table_size (unique_pairs);

  g_mutex_lock (registry->mutex);

  /* allocate a string array for the URI scheme / MIME type pairs */
  registry->uri_schemes = g_new0 (gchar *, n+1);
  registry->mime_types = g_new0 (gchar *, n+1);

  /* insert all unique URI scheme / MIME type pairs into string arrays */
  n = 0;
  g_hash_table_iter_init (&iter, unique_pairs);
  while (g_hash_table_iter_next (&iter, NULL, (gpointer) &pair)) 
    {
      /* fill the cache arrays with copied values */
      registry->uri_schemes[n] = g_strdup (pair[0]);
      registry->mime_types[n] = g_strdup (pair[1]);

      ++n;
    }

  /* NULL-terminate the arrays */
  registry->uri_schemes[n] = NULL;
  registry->mime_types[n] = NULL;

  g_mutex_unlock (registry->mutex);

  /* destroy the hash table we used */
  g_hash_table_unref (unique_pairs);

  /* free all strings we used but haven't freed yet */
  g_slist_foreach (used_strings, (GFunc) g_free, NULL);
  g_slist_free (used_strings);
}



void
tumbler_registry_get_supported (TumblerRegistry     *registry,
                                const gchar *const **uri_schemes,
                                const gchar *const **mime_types)
{
  g_return_if_fail (TUMBLER_IS_REGISTRY (registry));

  g_mutex_lock (registry->mutex);
  
  if (uri_schemes != NULL)
    *uri_schemes = (const gchar *const *)registry->uri_schemes;

  if (mime_types != NULL)
    *mime_types = (const gchar *const *)registry->mime_types;

  g_mutex_unlock (registry->mutex);
}



TumblerThumbnailer *
tumbler_registry_get_preferred (TumblerRegistry *registry,
                                const gchar     *hash_key)
{
  TumblerThumbnailer *thumbnailer = NULL;

  g_return_val_if_fail (TUMBLER_IS_REGISTRY (registry), NULL);
  g_return_val_if_fail (hash_key != NULL && *hash_key != '\0', NULL);

  g_mutex_lock (registry->mutex);
  thumbnailer = g_hash_table_lookup (registry->preferred_thumbnailers, hash_key);
  g_mutex_unlock (registry->mutex);

  return thumbnailer != NULL ? g_object_ref (thumbnailer) : NULL;
}



void
tumbler_registry_set_preferred (TumblerRegistry    *registry,
                                const gchar        *hash_key,
                                TumblerThumbnailer *thumbnailer)
{
  g_return_if_fail (TUMBLER_IS_REGISTRY (registry));
  g_return_if_fail (hash_key != NULL && *hash_key != '\0');
  g_return_if_fail (thumbnailer == NULL || TUMBLER_IS_THUMBNAILER (thumbnailer));

  g_mutex_lock (registry->mutex);
  
  if (thumbnailer == NULL)
    {
      g_hash_table_remove (registry->preferred_thumbnailers, hash_key);
    }
  else
    {
      g_hash_table_insert (registry->preferred_thumbnailers, 
                           g_strdup (hash_key), g_object_ref (thumbnailer));
    }

#ifdef DEBUG
  dump_registry (registry);
#endif

  g_mutex_unlock (registry->mutex);
}
