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

#include <tumbler/tumbler-enum-types.h>
#include <tumbler/tumbler-png-thumbnail-info.h>
#include <tumbler/tumbler-thumbnail-info.h>



static void tumbler_thumbnail_info_class_init (TumblerThumbnailInfoIface *klass);



static TumblerThumbnailFormat tumbler_thumbnail_info_default_format = TUMBLER_THUMBNAIL_FORMAT_PNG;



GType
tumbler_thumbnail_info_get_type (void)
{
  static GType type = G_TYPE_INVALID;
  
  if (G_UNLIKELY (type == G_TYPE_INVALID))
    {
      type = g_type_register_static_simple (G_TYPE_INTERFACE,
                                            "TumblerThumbnailInfo",
                                            sizeof (TumblerThumbnailInfoIface),
                                            (GClassInitFunc) tumbler_thumbnail_info_class_init,
                                            0,
                                            NULL,
                                            0);

      g_type_interface_add_prerequisite (type, G_TYPE_OBJECT);
    }

  return type;
}



static void
tumbler_thumbnail_info_class_init (TumblerThumbnailInfoIface *klass)
{
  g_object_interface_install_property (klass,
                                       g_param_spec_enum ("format",
                                                          "format",
                                                          "format",
                                                          TUMBLER_TYPE_THUMBNAIL_FORMAT,
                                                          TUMBLER_THUMBNAIL_FORMAT_INVALID,
                                                          G_PARAM_CONSTRUCT_ONLY |
                                                          G_PARAM_READWRITE));

  g_object_interface_install_property (klass,
                                       g_param_spec_string ("uri",
                                                            "uri",
                                                            "uri",
                                                            NULL,
                                                            G_PARAM_CONSTRUCT_ONLY |
                                                            G_PARAM_READWRITE));
}



TumblerThumbnailInfo *
tumbler_thumbnail_info_new (const gchar *uri)
{
  g_return_val_if_fail (uri != NULL, NULL);

  return tumbler_thumbnail_info_new_for_format (uri, 
                                                tumbler_thumbnail_info_default_format);
}



TumblerThumbnailInfo *
tumbler_thumbnail_info_new_for_format (const gchar            *uri,
                                       TumblerThumbnailFormat  format)
{
  TumblerThumbnailInfo *info = NULL;

  g_return_val_if_fail (uri != NULL, NULL);
  g_return_val_if_fail (format != TUMBLER_THUMBNAIL_FORMAT_INVALID, NULL);

  switch (format)
    {
    case TUMBLER_THUMBNAIL_FORMAT_PNG:
      info = g_object_new (TUMBLER_TYPE_PNG_THUMBNAIL_INFO, "uri", uri, 
                           "format", format, NULL);
      break;
    default:
      g_assert_not_reached ();
      break;
    }

  return info;
}



TumblerThumbnailFormat
tumbler_thumbnail_info_get_default_format (void)
{
  return tumbler_thumbnail_info_default_format;
}



void
tumbler_thumbnail_info_set_default_format (TumblerThumbnailFormat format)
{
  g_return_if_fail (format != TUMBLER_THUMBNAIL_FORMAT_INVALID);
  tumbler_thumbnail_info_default_format = format;
}
