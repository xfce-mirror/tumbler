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
#include <glib/gi18n.h>
#include <glib-object.h>

#include <tumbler/tumbler-error.h>
#include <tumbler/tumbler-file-info.h>
#include <tumbler/tumbler-provider-factory.h>
#include <tumbler/tumbler-cache-provider.h>
#include <tumbler/tumbler-thumbnail.h>



/* Property identifiers */
enum
{
  PROP_0,
  PROP_MTIME,
  PROP_URI,
};



static void tumbler_file_info_finalize     (GObject              *object);
static void tumbler_file_info_get_property (GObject              *object,
                                            guint                 prop_id,
                                            GValue               *value,
                                            GParamSpec           *pspec);
static void tumbler_file_info_set_property (GObject              *object,
                                            guint                 prop_id,
                                            const GValue         *value,
                                            GParamSpec           *pspec);



struct _TumblerFileInfoClass
{
  GObjectClass __parent__;
};

struct _TumblerFileInfo
{
  GObject __parent__;

  guint64 mtime; 
  GList  *thumbnails;
  gchar  *uri;
};




G_DEFINE_TYPE (TumblerFileInfo, tumbler_file_info, G_TYPE_OBJECT);



static void
tumbler_file_info_class_init (TumblerFileInfoClass *klass)
{
  GObjectClass *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->finalize = tumbler_file_info_finalize; 
  gobject_class->get_property = tumbler_file_info_get_property;
  gobject_class->set_property = tumbler_file_info_set_property;

  g_object_class_install_property (gobject_class, PROP_MTIME,
                                   g_param_spec_uint64 ("mtime",
                                                        "mtime",
                                                        "mtime",
                                                        0, G_MAXUINT64, 0,
                                                        G_PARAM_READABLE));

  g_object_class_install_property (gobject_class, PROP_URI,
                                   g_param_spec_string ("uri",
                                                        "uri",
                                                        "uri",
                                                        NULL,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));
}



static void
tumbler_file_info_init (TumblerFileInfo *info)
{
  info->mtime = 0;
  info->uri = NULL;
  info->thumbnails = NULL;
}



static void
tumbler_file_info_finalize (GObject *object)
{
  TumblerFileInfo *info = TUMBLER_FILE_INFO (object);

  g_list_foreach (info->thumbnails, (GFunc) g_object_unref, NULL);
  g_list_free (info->thumbnails);

  g_free (info->uri);

  (*G_OBJECT_CLASS (tumbler_file_info_parent_class)->finalize) (object);
}



static void
tumbler_file_info_get_property (GObject    *object,
                                guint       prop_id,
                                GValue     *value,
                                GParamSpec *pspec)
{
  TumblerFileInfo *info = TUMBLER_FILE_INFO (object);

  switch (prop_id)
    {
    case PROP_MTIME:
      g_value_set_uint64 (value, info->mtime);
      break;
    case PROP_URI:
      g_value_set_string (value, info->uri);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}



static void
tumbler_file_info_set_property (GObject      *object,
                                guint         prop_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
  TumblerFileInfo *info = TUMBLER_FILE_INFO (object);

  switch (prop_id)
    {
    case PROP_MTIME:
      info->mtime = g_value_get_uint64 (value);
      break;
    case PROP_URI:
      info->uri = g_value_dup_string (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}



TumblerFileInfo *
tumbler_file_info_new (const gchar *uri)
{
  g_return_val_if_fail (uri != NULL, NULL);
  return g_object_new (TUMBLER_TYPE_FILE_INFO, "uri", uri, NULL);
}



gboolean
tumbler_file_info_load (TumblerFileInfo *info,
                        GCancellable    *cancellable,
                        GError         **error)
{
  TumblerProviderFactory *provider_factory;
  TumblerCache           *cache;
  GFileInfo              *file_info;
  GError                 *err = NULL;
  GFile                  *file;
  GList                  *cp;
  GList                  *lp;
  GList                  *thumbnails;
  GList                  *tp;

  g_return_val_if_fail (TUMBLER_IS_FILE_INFO (info), FALSE);
  g_return_val_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  /* create a GFile for the URI */
  file = g_file_new_for_uri (info->uri);

  /* query the modified time from the file */
  file_info = g_file_query_info (file, G_FILE_ATTRIBUTE_TIME_MODIFIED, 
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
  info->mtime = g_file_info_get_attribute_uint64 (file_info,
                                                        G_FILE_ATTRIBUTE_TIME_MODIFIED);

  /* we no longer need the file information */
  g_object_unref (file_info);

  /* make sure to clear the thumbnails list before we load the info, just in
   * case someone decides to load the info twice */
  g_list_foreach (info->thumbnails, (GFunc) g_object_unref, NULL);
  g_list_free (info->thumbnails);
  info->thumbnails = NULL;

  /* query the default cache implementation */
  cache = tumbler_cache_get_default ();
  if (cache != NULL)
    {
      /* check if the file itself is a thumbnail */
      if (!tumbler_cache_is_thumbnail (cache, info->uri))
        {
          /* query thumbnail infos for this URI from the current cache */
          thumbnails = tumbler_cache_get_thumbnails (cache, info->uri);

          /* try to load thumbnail infos. the loop will terminate if 
           * one of them fails */
          for (tp = thumbnails; err == NULL && tp != NULL; tp = tp->next)
            tumbler_thumbnail_load (tp->data, cancellable, &err);

          /* add all queried thumbnails to the list */
          info->thumbnails = g_list_concat (info->thumbnails, 
                                                  thumbnails);
        }
      else
        {
          /* we don't allow the generation of thumbnails for thumbnails */
          g_set_error (&err, TUMBLER_ERROR, TUMBLER_ERROR_IS_THUMBNAIL,
                       _("The file \"%s\" is a thumbnail itself"), info->uri);
        }

      /* release the cache */
      g_object_unref (cache);
    }

  if (err != NULL)
    {
      /* propagate errors */
      g_propagate_error (error, err);

      /* release thumbnails as we assume not to have any on errors */
      g_list_foreach (info->thumbnails, (GFunc) g_object_unref, NULL);
      g_list_free (info->thumbnails);
      info->thumbnails = NULL;

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



guint64
tumbler_file_info_get_mtime (TumblerFileInfo *info)
{
  g_return_val_if_fail (TUMBLER_IS_FILE_INFO (info), 0);
  return info->mtime;
}



gboolean
tumbler_file_info_needs_update (TumblerFileInfo *info)
{
  gboolean needs_update = FALSE;
  GList   *lp;

  g_return_val_if_fail (TUMBLER_IS_FILE_INFO (info), FALSE);

  /* iterate over all thumbnails and check if at least one of them needs an update */
  for (lp = info->thumbnails; !needs_update && lp != NULL; lp = lp->next)
    {
      needs_update = needs_update || tumbler_thumbnail_needs_update (lp->data, 
                                                                     info->uri,
                                                                     info->mtime);
    }

  return needs_update;
}



GList *
tumbler_file_info_get_thumbnails (TumblerFileInfo *info)
{
  g_return_val_if_fail (TUMBLER_IS_FILE_INFO (info), NULL);
  return info->thumbnails;
}
