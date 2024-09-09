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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "tumbler-cache.h"
#include "tumbler-error.h"
#include "tumbler-file-info.h"
#include "tumbler-thumbnail-flavor.h"

#include <glib/gi18n.h>



/* Property identifiers */
enum
{
  PROP_0,
  PROP_MTIME,
  PROP_URI,
  PROP_MIME_TYPE,
  PROP_FLAVOR,
};



static void
tumbler_file_info_finalize (GObject *object);
static void
tumbler_file_info_get_property (GObject *object,
                                guint prop_id,
                                GValue *value,
                                GParamSpec *pspec);
static void
tumbler_file_info_set_property (GObject *object,
                                guint prop_id,
                                const GValue *value,
                                GParamSpec *pspec);



struct _TumblerFileInfo
{
  GObject __parent__;

  TumblerThumbnailFlavor *flavor;
  TumblerThumbnail *thumbnail;

  gdouble mtime;
  gchar *uri;
  gchar *mime_type;
};



G_DEFINE_TYPE (TumblerFileInfo, tumbler_file_info, G_TYPE_OBJECT);



static void
tumbler_file_info_class_init (TumblerFileInfoClass *klass)
{
  GObjectClass *gobject_class;

  /* make sure to use the translations from Tumbler */
  bindtextdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
#ifdef HAVE_BIND_TEXTDOMAIN_CODESET
  bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
#endif

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->finalize = tumbler_file_info_finalize;
  gobject_class->get_property = tumbler_file_info_get_property;
  gobject_class->set_property = tumbler_file_info_set_property;

  g_object_class_install_property (gobject_class, PROP_MTIME,
                                   g_param_spec_double ("mtime",
                                                        "mtime",
                                                        "mtime",
                                                        0, G_MAXDOUBLE, 0,
                                                        G_PARAM_READABLE));

  g_object_class_install_property (gobject_class, PROP_URI,
                                   g_param_spec_string ("uri",
                                                        "uri",
                                                        "uri",
                                                        NULL,
                                                        G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (gobject_class, PROP_MIME_TYPE,
                                   g_param_spec_string ("mime-type",
                                                        "mime-type",
                                                        "mime-type",
                                                        NULL,
                                                        G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (gobject_class, PROP_FLAVOR,
                                   g_param_spec_object ("flavor",
                                                        "flavor",
                                                        "flavor",
                                                        TUMBLER_TYPE_THUMBNAIL_FLAVOR,
                                                        G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
}



static void
tumbler_file_info_init (TumblerFileInfo *info)
{
  info->mtime = 0;
  info->uri = NULL;
  info->mime_type = NULL;
  info->thumbnail = NULL;
}



static void
tumbler_file_info_finalize (GObject *object)
{
  TumblerFileInfo *info = TUMBLER_FILE_INFO (object);

  if (info->thumbnail != NULL)
    g_object_unref (info->thumbnail);

  if (info->flavor != NULL)
    g_object_unref (info->flavor);

  g_free (info->mime_type);
  g_free (info->uri);

  (*G_OBJECT_CLASS (tumbler_file_info_parent_class)->finalize) (object);
}



static void
tumbler_file_info_get_property (GObject *object,
                                guint prop_id,
                                GValue *value,
                                GParamSpec *pspec)
{
  TumblerFileInfo *info = TUMBLER_FILE_INFO (object);

  switch (prop_id)
    {
    case PROP_MTIME:
      g_value_set_double (value, info->mtime);
      break;
    case PROP_URI:
      g_value_set_string (value, info->uri);
      break;
    case PROP_MIME_TYPE:
      g_value_set_string (value, info->mime_type);
      break;
    case PROP_FLAVOR:
      g_value_set_object (value, info->flavor);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}



static void
tumbler_file_info_set_property (GObject *object,
                                guint prop_id,
                                const GValue *value,
                                GParamSpec *pspec)
{
  TumblerFileInfo *info = TUMBLER_FILE_INFO (object);

  switch (prop_id)
    {
    case PROP_MTIME:
      info->mtime = g_value_get_double (value);
      break;
    case PROP_URI:
      info->uri = g_value_dup_string (value);
      break;
    case PROP_MIME_TYPE:
      info->mime_type = g_value_dup_string (value);
      break;
    case PROP_FLAVOR:
      info->flavor = g_value_dup_object (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}



TumblerFileInfo *
tumbler_file_info_new (const gchar *uri,
                       const gchar *mime_type,
                       TumblerThumbnailFlavor *flavor)
{
  g_return_val_if_fail (uri != NULL, NULL);
  g_return_val_if_fail (mime_type != NULL, NULL);
  g_return_val_if_fail (flavor == NULL || TUMBLER_IS_THUMBNAIL_FLAVOR (flavor), NULL);

  return g_object_new (TUMBLER_TYPE_FILE_INFO, "uri", uri, "mime-type", mime_type,
                       "flavor", flavor, NULL);
}



gboolean
tumbler_file_info_load (TumblerFileInfo *info,
                        GCancellable *cancellable,
                        GError **error)
{
  TumblerCache *cache;
  GFileInfo *file_info;
  GError *err = NULL;
  GFile *file;

  g_return_val_if_fail (TUMBLER_IS_FILE_INFO (info), FALSE);
  g_return_val_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  /* create a GFile for the URI */
  file = g_file_new_for_uri (info->uri);

  /* query the modified time from the file */
  file_info = g_file_query_info (file,
                                 G_FILE_ATTRIBUTE_TIME_MODIFIED
                                 "," G_FILE_ATTRIBUTE_TIME_MODIFIED_USEC,
                                 G_FILE_QUERY_INFO_NONE, cancellable, &err);

  /* destroy the GFile */
  g_object_unref (file);

  /* abort if the modified time could not be queried */
  if (err != NULL)
    {
      g_propagate_error (error, err);
      return FALSE;
    }

  /* read and remember the modified time */
  info->mtime = g_file_info_get_attribute_uint64 (file_info, G_FILE_ATTRIBUTE_TIME_MODIFIED)
                + 1e-6 * g_file_info_get_attribute_uint32 (file_info, G_FILE_ATTRIBUTE_TIME_MODIFIED_USEC);

  /* we no longer need the file information */
  g_object_unref (file_info);

  /* make sure to clear the thumbnail before we load the info, just in
   * case someone decides to load it twice */
  if (info->thumbnail != NULL)
    {
      g_object_unref (info->thumbnail);
      info->thumbnail = NULL;
    }

  /* query the default cache implementation */
  cache = tumbler_cache_get_default ();
  if (cache != NULL)
    {
      /* check if the file itself is a thumbnail */
      if (!tumbler_cache_is_thumbnail (cache, info->uri))
        {
          /* query thumbnail infos for this URI from the current cache */
          info->thumbnail = tumbler_cache_get_thumbnail (cache, info->uri, info->flavor);

          /* try to load thumbnail info */
          tumbler_thumbnail_load (info->thumbnail, cancellable, &err);
        }
      else
        {
          /* we don't allow the generation of thumbnails for thumbnails */
          g_set_error (&err, TUMBLER_ERROR, TUMBLER_ERROR_IS_THUMBNAIL,
                       TUMBLER_ERROR_MESSAGE_NO_THUMB_OF_THUMB, info->uri);
        }

      /* release the cache */
      g_object_unref (cache);
    }

  if (err != NULL)
    {
      /* propagate errors */
      g_propagate_error (error, err);

      /* release the thumbnail info */
      if (info->thumbnail != NULL)
        {
          g_object_unref (info->thumbnail);
          info->thumbnail = NULL;
        }

      return FALSE;
    }
  else
    {
      return TRUE;
    }
}



const gchar *
tumbler_file_info_get_uri (TumblerFileInfo *info)
{
  g_return_val_if_fail (TUMBLER_IS_FILE_INFO (info), NULL);
  return info->uri;
}



const gchar *
tumbler_file_info_get_mime_type (TumblerFileInfo *info)
{
  g_return_val_if_fail (TUMBLER_IS_FILE_INFO (info), NULL);
  return info->mime_type;
}



gdouble
tumbler_file_info_get_mtime (TumblerFileInfo *info)
{
  g_return_val_if_fail (TUMBLER_IS_FILE_INFO (info), 0);
  return info->mtime;
}



gboolean
tumbler_file_info_needs_update (TumblerFileInfo *info)
{
  gboolean needs_update = FALSE;

  g_return_val_if_fail (TUMBLER_IS_FILE_INFO (info), FALSE);

  if (info->thumbnail != NULL)
    {
      /* check if the thumbnail for the URI needs an update */
      needs_update = tumbler_thumbnail_needs_update (info->thumbnail,
                                                     info->uri, info->mtime);
    }

  return needs_update;
}



TumblerThumbnail *
tumbler_file_info_get_thumbnail (TumblerFileInfo *info)
{
  g_return_val_if_fail (TUMBLER_IS_FILE_INFO (info), NULL);
  return g_object_ref (info->thumbnail);
}



TumblerFileInfo **
tumbler_file_info_array_new_with_flavor (const gchar *const *uris,
                                         const gchar *const *mime_types,
                                         TumblerThumbnailFlavor *flavor,
                                         guint *length)
{
  TumblerFileInfo **infos = NULL;
  guint num_uris;
  guint num_mime_types;
  guint n;
  guint num;

  g_return_val_if_fail (uris != NULL, NULL);

  num_uris = g_strv_length ((gchar **) uris);
  num_mime_types = g_strv_length ((gchar **) mime_types);
  num = MIN (num_uris, num_mime_types);

  if (length != NULL)
    *length = num;

  infos = g_new0 (TumblerFileInfo *, num + 1);

  for (n = 0; n < num; ++n)
    infos[n] = tumbler_file_info_new (uris[n], mime_types[n], flavor);

  infos[n] = NULL;

  return infos;
}



TumblerFileInfo **
tumbler_file_info_array_copy (TumblerFileInfo **infos,
                              guint length)
{
  TumblerFileInfo **copy;
  guint n;

  g_return_val_if_fail (infos != NULL, NULL);

  copy = g_new0 (TumblerFileInfo *, length + 1);

  for (n = 0; infos != NULL && infos[n] != NULL && n < length; ++n)
    copy[n] = g_object_ref (infos[n]);

  copy[n] = NULL;

  return copy;
}



void
tumbler_file_info_array_free (TumblerFileInfo **infos)
{
  gint n;

  for (n = 0; infos != NULL && infos[n] != NULL; ++n)
    g_object_unref (infos[n]);

  g_free (infos);
}
