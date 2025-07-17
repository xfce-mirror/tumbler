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

#include "tumbler-marshal.h"
#include "tumbler-thumbnailer.h"
#include "tumbler.h"
#include "tumbler-visibility.h"



/* signal identifiers */
enum
{
  SIGNAL_READY,
  SIGNAL_ERROR,
  SIGNAL_UNREGISTER,
  LAST_SIGNAL,
};

static guint tumbler_thumbnailer_signals[LAST_SIGNAL];



G_DEFINE_INTERFACE (TumblerThumbnailer, tumbler_thumbnailer, G_TYPE_OBJECT)



static void
tumbler_thumbnailer_default_init (TumblerThumbnailerIface *klass)
{
  g_object_interface_install_property (klass,
                                       g_param_spec_pointer ("mime-types",
                                                             "mime-types",
                                                             "mime-types",
                                                             G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE));

  g_object_interface_install_property (klass,
                                       g_param_spec_pointer ("uri-schemes",
                                                             "uri-schemes",
                                                             "uri-schemes",
                                                             G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE));

  g_object_interface_install_property (klass,
                                       g_param_spec_pointer ("hash-keys",
                                                             "hash-keys",
                                                             "hash-keys",
                                                             G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE));

  g_object_interface_install_property (klass,
                                       g_param_spec_int ("priority",
                                                         "priority",
                                                         "priority",
                                                         0, G_MAXINT, 0,
                                                         G_PARAM_READWRITE));

  g_object_interface_install_property (klass,
                                       g_param_spec_int64 ("max-file-size",
                                                           "max-file-size",
                                                           "max-file-size",
                                                           0, G_MAXINT64, 0,
                                                           G_PARAM_READWRITE));

  g_object_interface_install_property (klass,
                                       g_param_spec_pointer ("locations",
                                                             "locations",
                                                             "locations",
                                                             G_PARAM_READWRITE));

  g_object_interface_install_property (klass,
                                       g_param_spec_pointer ("excludes",
                                                             "excludes",
                                                             "excludes",
                                                             G_PARAM_READWRITE));

  tumbler_thumbnailer_signals[SIGNAL_READY] =
    g_signal_new ("ready",
                  TUMBLER_TYPE_THUMBNAILER,
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (TumblerThumbnailerIface, ready),
                  NULL,
                  NULL,
                  g_cclosure_marshal_VOID__OBJECT,
                  G_TYPE_NONE,
                  1,
                  G_TYPE_OBJECT);

  tumbler_thumbnailer_signals[SIGNAL_ERROR] =
    g_signal_new ("error",
                  TUMBLER_TYPE_THUMBNAILER,
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (TumblerThumbnailerIface, error),
                  NULL,
                  NULL,
                  tumbler_marshal_VOID__OBJECT_UINT_INT_STRING,
                  G_TYPE_NONE,
                  4,
                  G_TYPE_OBJECT,
                  G_TYPE_UINT,
                  G_TYPE_INT,
                  G_TYPE_STRING);

  tumbler_thumbnailer_signals[SIGNAL_UNREGISTER] =
    g_signal_new ("unregister",
                  TUMBLER_TYPE_THUMBNAILER,
                  G_SIGNAL_RUN_LAST | G_SIGNAL_NO_HOOKS,
                  G_STRUCT_OFFSET (TumblerThumbnailerIface, unregister),
                  NULL,
                  NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE,
                  0);
}



void
tumbler_thumbnailer_create (TumblerThumbnailer *thumbnailer,
                            GCancellable *cancellable,
                            TumblerFileInfo *info)
{
  g_return_if_fail (TUMBLER_IS_THUMBNAILER (thumbnailer));
  g_return_if_fail (TUMBLER_IS_FILE_INFO (info));
  g_return_if_fail (TUMBLER_THUMBNAILER_GET_IFACE (thumbnailer)->create != NULL);

  (*TUMBLER_THUMBNAILER_GET_IFACE (thumbnailer)->create) (thumbnailer,
                                                          cancellable,
                                                          info);
}



gchar **
tumbler_thumbnailer_get_hash_keys (TumblerThumbnailer *thumbnailer)
{
  gchar **hash_keys;

  g_return_val_if_fail (TUMBLER_IS_THUMBNAILER (thumbnailer), NULL);

  g_object_get (thumbnailer, "hash-keys", &hash_keys, NULL);
  return hash_keys;
}



gchar **
tumbler_thumbnailer_get_mime_types (TumblerThumbnailer *thumbnailer)
{
  gchar **mime_types;

  g_return_val_if_fail (TUMBLER_IS_THUMBNAILER (thumbnailer), NULL);

  g_object_get (thumbnailer, "mime-types", &mime_types, NULL);
  return mime_types;
}



gchar **
tumbler_thumbnailer_get_uri_schemes (TumblerThumbnailer *thumbnailer)
{
  gchar **uri_schemes;

  g_return_val_if_fail (TUMBLER_IS_THUMBNAILER (thumbnailer), NULL);

  g_object_get (thumbnailer, "uri-schemes", &uri_schemes, NULL);
  return uri_schemes;
}



gint
tumbler_thumbnailer_get_priority (TumblerThumbnailer *thumbnailer)
{
  gint priority;

  g_return_val_if_fail (TUMBLER_IS_THUMBNAILER (thumbnailer), 0);

  g_object_get (thumbnailer, "priority", &priority, NULL);
  return priority;
}



gint64
tumbler_thumbnailer_get_max_file_size (TumblerThumbnailer *thumbnailer)
{
  gint64 max_file_size;

  g_return_val_if_fail (TUMBLER_IS_THUMBNAILER (thumbnailer), 0);

  g_object_get (thumbnailer, "max-file-size", &max_file_size, NULL);
  return max_file_size;
}



gboolean
tumbler_thumbnailer_supports_location (TumblerThumbnailer *thumbnailer,
                                       GFile *file)
{
  GSList *locations, *excludes, *lp, *ep;
  gboolean supported = FALSE;
  gboolean excluded = FALSE;

  g_return_val_if_fail (TUMBLER_IS_THUMBNAILER (thumbnailer), FALSE);
  g_return_val_if_fail (G_IS_FILE (file), FALSE);

  /* check first if file is excluded */
  g_object_get (thumbnailer, "excludes", &excludes, NULL);
  if (excludes != NULL)
    {
      for (ep = excludes; !excluded && ep != NULL; ep = ep->next)
        if (g_file_has_prefix (file, G_FILE (ep->data)))
          excluded = TRUE;
    }

  /* Path is excluded */
  if (excluded)
    return FALSE;

  /* we're cool if no locations are set */
  g_object_get (thumbnailer, "locations", &locations, NULL);
  if (locations == NULL)
    return TRUE;

  /*check if the prefix is supported */
  for (lp = locations; !supported && lp != NULL; lp = lp->next)
    if (g_file_has_prefix (file, G_FILE (lp->data)))
      supported = TRUE;

  g_slist_free_full (locations, g_object_unref);

  return supported;
}



gboolean
tumbler_thumbnailer_supports_hash_key (TumblerThumbnailer *thumbnailer,
                                       const gchar *hash_key)
{
  gboolean supported = FALSE;
  gchar **hash_keys;
  guint n;

  g_return_val_if_fail (TUMBLER_IS_THUMBNAILER (thumbnailer), FALSE);
  g_return_val_if_fail (hash_key != NULL && *hash_key != '\0', FALSE);

  hash_keys = tumbler_thumbnailer_get_hash_keys (thumbnailer);

  for (n = 0; !supported && hash_keys != NULL && hash_keys[n] != NULL; ++n)
    if (g_strcmp0 (hash_keys[n], hash_key) == 0)
      supported = TRUE;

  g_strfreev (hash_keys);

  return supported;
}



GList **
tumbler_thumbnailer_array_copy (GList **thumbnailers,
                                guint length)
{
  GList **copy;
  guint n;

  g_return_val_if_fail (thumbnailers != NULL, NULL);

  copy = g_new0 (GList *, length + 1);

  for (n = 0; n < length; ++n)
    copy[n] = g_list_copy_deep (thumbnailers[n], tumbler_util_object_ref, NULL);

  copy[n] = NULL;

  return copy;
}



void
tumbler_thumbnailer_array_free (GList **thumbnailers,
                                guint length)
{
  guint n;

  for (n = 0; thumbnailers != NULL && n < length; ++n)
    g_list_free_full (thumbnailers[n], g_object_unref);

  g_free (thumbnailers);
}

#define __TUMBLER_THUMBNAILER_C__
#include "tumbler-visibility.c"
