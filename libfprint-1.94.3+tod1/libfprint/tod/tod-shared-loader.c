/*
 * Shared library loader for libfprint
 * Copyright (C) 2019 Marco Trevisan <marco.trevisan@canonical.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#define FP_COMPONENT "tod"

#include <gmodule.h>

#include "tod-shared-loader.h"
#include "fpi-device.h"
#include "fpi-log.h"
#include "tod-config.h"

#define FPI_TOD_ENTRY_GTYPE_GETTER "fpi_tod_shared_driver_get_type"

static GArray *shared_drivers = NULL;
static GList *shared_modules = NULL;

typedef GModule FpiTodModule;
typedef GType (*FpiTodShardDriverTypeGetter) (void);

G_DEFINE_AUTOPTR_CLEANUP_FUNC (FpiTodModule, g_module_close);

static const char *
get_tod_drivers_dir (void)
{
  const char *tod_env_path = g_getenv ("FP_TOD_DRIVERS_DIR");

  if (!tod_env_path || *tod_env_path == '\0')
    return TOD_DRIVERS_DIR;

  return tod_env_path;
}

void
fpi_tod_shared_drivers_register (void)
{
  const char *dirname;
  const char *basename;

  g_autoptr(GError) error = NULL;
  g_autoptr(GDir) dir = NULL;
  gpointer symbol;

  g_assert_null (shared_drivers);

  dirname = get_tod_drivers_dir ();
  dir = g_dir_open (dirname, 0, &error);

  shared_drivers = g_array_new (TRUE, FALSE, sizeof (GType));

  if (error)
    {
      fp_dbg ("Impossible to load the shared drivers dir %s", error->message);
      return;
    }

  while ((basename = g_dir_read_name (dir)) != NULL)
    {
      g_autoptr(FpiTodModule) module = NULL;
      g_autoptr(GTypeClass) type_class = NULL;
      g_autofree char *module_path = NULL;
      FpiTodShardDriverTypeGetter type_getter;
      FpDeviceClass *cls;
      GType driver;

      if (!g_str_has_prefix (basename, "lib"))
        continue;

      if (!g_str_has_suffix (basename, ".so"))
        continue;

      module_path = g_build_filename (dirname, basename, NULL);

      if (!g_file_test (module_path, G_FILE_TEST_IS_REGULAR))
        continue;

      fp_dbg ("Opening driver %s", module_path);

      module = g_module_open (module_path, G_MODULE_BIND_LAZY | G_MODULE_BIND_LOCAL);

      if (!module)
        {
          fp_err ("Impossible to load module %s: %s", module_path,
                  g_module_error ());
          continue;
        }

      if (!g_module_symbol (module, FPI_TOD_ENTRY_GTYPE_GETTER, &symbol))
        {
          fp_err ("Library %s doesn't expose the required entry point symbol",
                  module_path);
          continue;
        }

      type_getter = symbol;
      driver = type_getter ();
      fp_dbg ("Found TOD entry point symbol %p, GType is %lu", symbol, driver);

      if (!G_TYPE_IS_OBJECT (driver) || !g_type_is_a (driver, FP_TYPE_DEVICE))
        {
          fp_err ("Library %s returned GType (%lu) doesn't represent a device",
                  module_path, driver);
          continue;
        }

      type_class = g_type_class_ref (driver);
      g_assert_true (g_type_check_class_is_a (type_class, FP_TYPE_DEVICE));

      cls = FP_DEVICE_CLASS (type_class);

      fp_dbg ("Loading driver %s (%s)", cls->id, cls->full_name);
      g_array_append_val (shared_drivers, driver);

      if (cls->features == FP_DEVICE_FEATURE_NONE)
        {
          g_debug ("Initializing features for driver %s", cls->id);
          fpi_device_class_auto_initialize_features (cls);
        }

      shared_modules = g_list_prepend (shared_modules,
                                       g_steal_pointer (&module));
    }
}

void
fpi_tod_shared_drivers_unregister (void)
{
  g_clear_pointer (&shared_drivers, g_array_unref);

  if (g_strcmp0 (g_getenv ("FP_TOD_KEEP_MODULES_OPEN"), "TRUE") != 0)
    {
      g_list_free_full (shared_modules, (GDestroyNotify) g_module_close);
      shared_modules = NULL;
    }
}

GArray *
fpi_tod_shared_drivers_get (void)
{
  return g_array_ref (shared_drivers);
}
