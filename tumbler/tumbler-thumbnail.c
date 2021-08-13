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

#include <tumbler/tumbler-cache.h>
#include <tumbler/tumbler-thumbnail.h>



static void tumbler_thumbnail_class_init (gpointer g_class,
                                          gpointer class_data);



GType
tumbler_thumbnail_get_type (void)
{
  static gsize g_define_type_id__static = 0;

  if (g_once_init_enter (&g_define_type_id__static))
    {
      GType g_define_type_id =
        g_type_register_static_simple (G_TYPE_INTERFACE,
                                       "TumblerThumbnail",
                                       sizeof (TumblerThumbnailIface),
                                       tumbler_thumbnail_class_init,
                                       0,
                                       NULL,
                                       0);

      g_type_interface_add_prerequisite (g_define_type_id, G_TYPE_OBJECT);

      g_once_init_leave (&g_define_type_id__static, g_define_type_id);
    }

  return g_define_type_id__static;
}



static void
tumbler_thumbnail_class_init (gpointer g_class,
                              gpointer class_data)
{
  g_object_interface_install_property (g_class,
                                       g_param_spec_object ("cache",
                                                            "cache",
                                                            "cache",
                                                            TUMBLER_TYPE_CACHE,
                                                            G_PARAM_READWRITE |
                                                            G_PARAM_CONSTRUCT_ONLY));

  g_object_interface_install_property (g_class,
                                       g_param_spec_string ("uri",
                                                            "uri",
                                                            "uri",
                                                            NULL,
                                                            G_PARAM_READWRITE |
                                                            G_PARAM_CONSTRUCT_ONLY));
  
  g_object_interface_install_property (g_class,
                                       g_param_spec_object ("flavor",
                                                            "flavor",
                                                            "flavor",
                                                            TUMBLER_TYPE_THUMBNAIL_FLAVOR,
                                                            G_PARAM_READWRITE |
                                                            G_PARAM_CONSTRUCT_ONLY));
}



gboolean
tumbler_thumbnail_load (TumblerThumbnail *thumbnail,
                        GCancellable     *cancellable,
                        GError          **error)
{
  g_return_val_if_fail (TUMBLER_IS_THUMBNAIL (thumbnail), FALSE);
  g_return_val_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);
  g_return_val_if_fail (TUMBLER_THUMBNAIL_GET_IFACE (thumbnail)->load != NULL, FALSE);

  return (TUMBLER_THUMBNAIL_GET_IFACE (thumbnail)->load) (thumbnail, cancellable, error);
}



gboolean
tumbler_thumbnail_needs_update (TumblerThumbnail *thumbnail,
                                const gchar      *uri,
                                guint64           mtime)
{
  g_return_val_if_fail (TUMBLER_IS_THUMBNAIL (thumbnail), FALSE);
  g_return_val_if_fail (uri != NULL, FALSE);
  g_return_val_if_fail (TUMBLER_THUMBNAIL_GET_IFACE (thumbnail)->needs_update != NULL, FALSE);

  return (TUMBLER_THUMBNAIL_GET_IFACE (thumbnail)->needs_update) (thumbnail, uri, mtime);
}



gboolean
tumbler_thumbnail_save_image_data (TumblerThumbnail *thumbnail,
                                   TumblerImageData *data,
                                   guint64           mtime,
                                   GCancellable     *cancellable,
                                   GError          **error)
{
  g_return_val_if_fail (TUMBLER_IS_THUMBNAIL (thumbnail), FALSE);
  g_return_val_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);
  g_return_val_if_fail (TUMBLER_THUMBNAIL_GET_IFACE (thumbnail)->save_image_data != NULL, FALSE);

  return (TUMBLER_THUMBNAIL_GET_IFACE (thumbnail)->save_image_data) (thumbnail, data, 
                                                                     mtime, cancellable, 
                                                                     error);
}



gboolean
tumbler_thumbnail_save_file (TumblerThumbnail *thumbnail,
                             GFile            *file,
                             guint64           mtime,
                             GCancellable     *cancellable,
                             GError          **error)
{
  g_return_val_if_fail (TUMBLER_IS_THUMBNAIL (thumbnail), FALSE);
  g_return_val_if_fail (G_IS_FILE (file), FALSE);
  g_return_val_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);
  g_return_val_if_fail (TUMBLER_THUMBNAIL_GET_IFACE (thumbnail)->save_file != NULL, FALSE);

  return (TUMBLER_THUMBNAIL_GET_IFACE (thumbnail)->save_file) (thumbnail, file, mtime,
                                                               cancellable, error);
}



TumblerThumbnailFlavor *
tumbler_thumbnail_get_flavor (TumblerThumbnail *thumbnail)
{
  TumblerThumbnailFlavor *flavor;

  g_return_val_if_fail (TUMBLER_IS_THUMBNAIL (thumbnail), NULL);

  g_object_get (thumbnail, "flavor", &flavor, NULL);
  return flavor;
}
