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

#include <tumbler/tumbler-marshal.h>
#include <tumbler/tumbler-file-info.h>
#include <tumbler/tumbler-thumbnailer.h>



/* signal identifiers */
enum
{
  SIGNAL_READY,
  SIGNAL_ERROR,
  SIGNAL_UNREGISTER,
  LAST_SIGNAL,
};



static void  tumbler_thumbnailer_class_init (TumblerThumbnailerIface *klass);



static guint tumbler_thumbnailer_signals[LAST_SIGNAL];



GType
tumbler_thumbnailer_get_type (void)
{
  static volatile gsize g_define_type_id__volatile = 0;
  
  if (g_once_init_enter (&g_define_type_id__volatile))
    {
      GType g_define_type_id =
        g_type_register_static_simple (G_TYPE_INTERFACE,
                                       "TumblerThumbnailer",
                                       sizeof (TumblerThumbnailerIface),
                                       (GClassInitFunc) tumbler_thumbnailer_class_init,
                                       0,
                                       NULL,
                                       0);

      g_type_interface_add_prerequisite (g_define_type_id, G_TYPE_OBJECT);

      g_once_init_leave (&g_define_type_id__volatile, g_define_type_id);
    }

  return g_define_type_id__volatile;
}



static void
tumbler_thumbnailer_class_init (TumblerThumbnailerIface *klass)
{
  g_object_interface_install_property (klass,
                                       g_param_spec_pointer ("mime-types",
                                                             "mime-types",
                                                             "mime-types",
                                                             G_PARAM_CONSTRUCT_ONLY |
                                                             G_PARAM_READWRITE));

  g_object_interface_install_property (klass,
                                       g_param_spec_pointer ("uri-schemes",
                                                             "uri-schemes",
                                                             "uri-schemes",
                                                             G_PARAM_CONSTRUCT_ONLY |
                                                             G_PARAM_READWRITE));

  g_object_interface_install_property (klass,
                                       g_param_spec_pointer ("hash-keys",
                                                             "hash-keys",
                                                             "hash-keys",
                                                             G_PARAM_READABLE));

  tumbler_thumbnailer_signals[SIGNAL_READY] = 
    g_signal_new ("ready",
                  TUMBLER_TYPE_THUMBNAILER,
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (TumblerThumbnailerIface, ready),
                  NULL,
                  NULL,
                  g_cclosure_marshal_VOID__STRING,
                  G_TYPE_NONE,
                  1,
                  G_TYPE_STRING);

  tumbler_thumbnailer_signals[SIGNAL_ERROR] =
    g_signal_new ("error",
                  TUMBLER_TYPE_THUMBNAILER,
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (TumblerThumbnailerIface, error),
                  NULL,
                  NULL,
                  tumbler_marshal_VOID__STRING_INT_STRING,
                  G_TYPE_NONE,
                  3,
                  G_TYPE_STRING,
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
tumbler_thumbnailer_create (TumblerThumbnailer     *thumbnailer,
                            GCancellable           *cancellable,
                            TumblerFileInfo        *info)
{
  g_return_if_fail (TUMBLER_IS_THUMBNAILER (thumbnailer));
  g_return_if_fail (TUMBLER_IS_FILE_INFO (info));
  g_return_if_fail (TUMBLER_THUMBNAILER_GET_IFACE (thumbnailer)->create != NULL);

  return (*TUMBLER_THUMBNAILER_GET_IFACE (thumbnailer)->create) (thumbnailer, 
                                                                 cancellable,
                                                                 info);
}



GStrv
tumbler_thumbnailer_get_hash_keys (TumblerThumbnailer *thumbnailer)
{
  GStrv hash_keys;

  g_return_val_if_fail (TUMBLER_IS_THUMBNAILER (thumbnailer), NULL);

  g_object_get (thumbnailer, "hash-keys", &hash_keys, NULL);
  return hash_keys;
}



GStrv
tumbler_thumbnailer_get_mime_types (TumblerThumbnailer *thumbnailer)
{
  GStrv mime_types;

  g_return_val_if_fail (TUMBLER_IS_THUMBNAILER (thumbnailer), NULL);

  g_object_get (thumbnailer, "mime-types", &mime_types, NULL);
  return mime_types;
}



GStrv
tumbler_thumbnailer_get_uri_schemes (TumblerThumbnailer *thumbnailer)
{
  GStrv uri_schemes;

  g_return_val_if_fail (TUMBLER_IS_THUMBNAILER (thumbnailer), NULL);

  g_object_get (thumbnailer, "uri-schemes", &uri_schemes, NULL);
  return uri_schemes;
}



gboolean
tumbler_thumbnailer_supports_hash_key (TumblerThumbnailer *thumbnailer,
                                       const gchar        *hash_key)
{
  gboolean supported = FALSE;
  GStrv    hash_keys;
  guint    n;

  g_return_val_if_fail (TUMBLER_IS_THUMBNAILER (thumbnailer), FALSE);
  g_return_val_if_fail (hash_key != NULL && *hash_key != '\0', FALSE);

  hash_keys = tumbler_thumbnailer_get_hash_keys (thumbnailer);

  for (n = 0; !supported && hash_keys != NULL && hash_keys[n] != NULL; ++n)
    if (g_strcmp0 (hash_keys[n], hash_key) == 0)
      supported = TRUE;

  g_strfreev (hash_keys);

  return supported;
}



TumblerThumbnailer **
tumbler_thumbnailer_array_copy (TumblerThumbnailer **thumbnailers,
                                guint                length)
{
  TumblerThumbnailer **copy;
  guint                n;

  g_return_val_if_fail (thumbnailers != NULL, NULL);

  copy = g_new0 (TumblerThumbnailer *, length + 1);

  for (n = 0; n < length; ++n)
    if (thumbnailers[n] != NULL)
      copy[n] = g_object_ref (thumbnailers[n]);

  copy[n] = NULL;

  return copy;
}



void
tumbler_thumbnailer_array_free (TumblerThumbnailer **thumbnailers,
                                guint                length)
{
  guint n;

  for (n = 0; thumbnailers != NULL && n < length; ++n)
    if (thumbnailers[n] != NULL)
      g_object_unref (thumbnailers[n]);

  g_free (thumbnailers);
}
