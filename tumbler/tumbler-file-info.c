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



#define TUMBLER_FILE_INFO_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), TUMBLER_TYPE_FILE_INFO, TumblerFileInfoPrivate))



/* Property identifiers */
enum
{
  PROP_0,
  PROP_MTIME,
  PROP_URI,
};



static void tumbler_file_info_class_init   (TumblerFileInfoClass *klass);
static void tumbler_file_info_init         (TumblerFileInfo      *info);
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

  TumblerFileInfoPrivate *priv;
};

struct _TumblerFileInfoPrivate
{
  guint64 mtime; 
  GList  *thumbnails;
  gchar  *uri;
};



static GObjectClass *tumbler_file_info_parent_class = NULL;



GType
tumbler_file_info_get_type (void)
{
  static GType type = G_TYPE_INVALID;

  if (G_UNLIKELY (type == G_TYPE_INVALID))
    {
      type = g_type_register_static_simple (G_TYPE_OBJECT, 
                                            "TumblerFileInfo",
                                            sizeof (TumblerFileInfoClass),
                                            (GClassInitFunc) tumbler_file_info_class_init,
                                            sizeof (TumblerFileInfo),
                                            (GInstanceInitFunc) tumbler_file_info_init,
                                            0);
    }

  return type;
}



static void
tumbler_file_info_class_init (TumblerFileInfoClass *klass)
{
  GObjectClass *gobject_class;

  g_type_class_add_private (klass, sizeof (TumblerFileInfoPrivate));

  /* Determine the parent type class */
  tumbler_file_info_parent_class = g_type_class_peek_parent (klass);

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
  info->priv = TUMBLER_FILE_INFO_GET_PRIVATE (info);
  info->priv->mtime = 0;
  info->priv->uri = NULL;
  info->priv->thumbnails = NULL;
}



static void
tumbler_file_info_finalize (GObject *object)
{
  TumblerFileInfo *info = TUMBLER_FILE_INFO (object);

  g_list_foreach (info->priv->thumbnails, (GFunc) g_object_unref, NULL);
  g_list_free (info->priv->thumbnails);

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
      g_value_set_uint64 (value, info->priv->mtime);
      break;
    case PROP_URI:
      g_value_set_string (value, info->priv->uri);
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
      info->priv->mtime = g_value_get_uint64 (value);
      break;
    case PROP_URI:
      info->priv->uri = g_value_dup_string (value);
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
  GFileInfo *file_info;
  GError    *err = NULL;
  GFile     *file;
  GList     *caches;
  GList     *cp;
  GList     *lp;
  GList     *providers;
  GList     *thumbnails;
  GList     *tp;

  g_return_val_if_fail (TUMBLER_IS_FILE_INFO (info), FALSE);
  g_return_val_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  /* create a GFile for the URI */
  file = g_file_new_for_uri (info->priv->uri);

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
  info->priv->mtime = g_file_info_get_attribute_uint64 (file_info,
                                                        G_FILE_ATTRIBUTE_TIME_MODIFIED);

  /* we no longer need the file information */
  g_object_unref (file_info);

  /* make sure to clear the thumbnails list before we load the info, just in
   * case someone decides to load the info twice */
  g_list_foreach (info->priv->thumbnails, (GFunc) g_object_unref, NULL);
  g_list_free (info->priv->thumbnails);
  info->priv->thumbnails = NULL;

  /* get the provider factory */
  provider_factory = tumbler_provider_factory_get_default ();

  /* query a list of cache providers */
  providers = tumbler_provider_factory_get_providers (provider_factory, 
                                                      TUMBLER_TYPE_CACHE_PROVIDER);

  /* iterate over all available cache providers */
  for (lp = providers; err == NULL && lp != NULL; lp = lp->next)
    {
      /* query a list of cache implementations from the current provider */
      caches = tumbler_cache_provider_get_caches (lp->data);

      /* iterate over all available cache implementations */
      for (cp = caches; err == NULL && cp != NULL; cp = cp->next)
        {
          /* check if the file itself is a thumbnail */
          if (!tumbler_cache_is_thumbnail (cp->data, info->priv->uri))
            {
              /* query thumbnail infos for this URI from the current cache */
              thumbnails = tumbler_cache_get_thumbnails (cp->data, info->priv->uri);

              /* try to load thumbnail infos. the loop will terminate if 
               * one of them fails */
              for (tp = thumbnails; err == NULL && tp != NULL; tp = tp->next)
                tumbler_thumbnail_load (tp->data, cancellable, &err);

              /* add all queried thumbnails to the list */
              info->priv->thumbnails = g_list_concat (info->priv->thumbnails, 
                                                      thumbnails);
            }
          else
            {
              /* we don't allow the generation of thumbnails for thumbnails */
              g_set_error (&err, TUMBLER_ERROR, TUMBLER_ERROR_IS_THUMBNAIL,
                           _("The file \"%s\" is a thumbnail itself"), info->priv->uri);
            }
        }

      /* release cache references */
      g_list_foreach (caches, (GFunc) g_object_unref, NULL);
      g_list_free (caches);
    }

  /* release provider references */
  g_list_foreach (providers, (GFunc) g_object_unref, NULL);
  g_list_free (providers);

  /* release the provider factory */
  g_object_unref (provider_factory);

  if (err != NULL)
    {
      /* propagate errors */
      g_propagate_error (error, err);

      /* release thumbnails as we assume not to have any on errors */
      g_list_foreach (info->priv->thumbnails, (GFunc) g_object_unref, NULL);
      g_list_free (info->priv->thumbnails);
      info->priv->thumbnails = NULL;

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
  return info->priv->uri;
}



guint64
tumbler_file_info_get_mtime (TumblerFileInfo *info)
{
  g_return_val_if_fail (TUMBLER_IS_FILE_INFO (info), 0);
  return info->priv->mtime;
}



gboolean
tumbler_file_info_needs_update (TumblerFileInfo *info)
{
  gboolean needs_update = FALSE;
  GList   *lp;

  g_return_val_if_fail (TUMBLER_IS_FILE_INFO (info), FALSE);

  /* iterate over all thumbnails and check if at least one of them needs an update */
  for (lp = info->priv->thumbnails; !needs_update && lp != NULL; lp = lp->next)
    {
      needs_update = needs_update || tumbler_thumbnail_needs_update (lp->data, 
                                                                     info->priv->uri,
                                                                     info->priv->mtime);
    }

  return needs_update;
}



GList *
tumbler_file_info_get_thumbnails (TumblerFileInfo *info)
{
  g_return_val_if_fail (TUMBLER_IS_FILE_INFO (info), NULL);
  return info->priv->thumbnails;
}
