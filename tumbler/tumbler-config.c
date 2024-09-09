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
#include "config.h"
#endif

#include "tumbler-config.h"



/**
 * SECTION:tumbler-config
 * @title: Version Information and Compatibility Checks
 * @include: tumbler/tumbler.h
 */



const guint tumbler_major_version = TUMBLER_MAJOR_VERSION;
const guint tumbler_minor_version = TUMBLER_MINOR_VERSION;
const guint tumbler_micro_version = TUMBLER_MICRO_VERSION;



/**
 * tumbler_check_version:
 * @required_major : the required major version.
 * @required_minor : the required minor version.
 * @required_micro : the required micro version.
 *
 * Checks that the <systemitem class="library">tumbler</systemitem> library
 * in use is compatible with the given version. Generally you would pass in
 * the constants #TUMBLER_MAJOR_VERSION, #TUMBLER_MINOR_VERSION and
 * #TUMBLER_MICRO_VERSION as the three arguments to this function; that
 * produces a check that the library in use is compatible with the version
 * of <systemitem class="library">tumbler</systemitem> the application was
 * compiled against.
 *
 * <example>
 * <title>Checking the runtime version of the tumbler library</title>
 * <programlisting>
 * const gchar *mismatch;
 * mismatch = tumbler_check_version (TUMBLER_VERSION_MAJOR,
 *                                   TUMBLER_VERSION_MINOR,
 *                                   TUMBLER_VERSION_MICRO);
 * if (G_UNLIKELY (mismatch != NULL))
 *   g_error ("Version mismatch: %<!---->s", mismatch);
 * </programlisting>
 * </example>
 *
 * Return value: %NULL if the library is compatible with the given version,
 *               or a string describing the version mismatch. The returned
 *               string is owned by the library and must not be freed or
 *               modified by the caller.
 **/
const gchar *
tumbler_check_version (guint required_major,
                       guint required_minor,
                       guint required_micro)
{
  return NULL;
}
