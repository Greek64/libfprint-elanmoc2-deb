/*
 * FpContext Unit tests
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

#include <libfprint/fprint.h>

static void
test_context_new (void)
{
  g_autoptr(FpContext) context = fp_context_new ();
  g_assert_true (FP_CONTEXT (context));
}

static void
test_context_has_no_devices (void)
{
  g_autoptr(FpContext) context = NULL;
  GPtrArray *devices;
  const char *old_drivers_dir = g_getenv ("FP_TOD_DRIVERS_DIR");

  g_setenv ("FP_TOD_DRIVERS_DIR", "__HOPEFULLY_AN_INVALID_PATH", TRUE);
  context = fp_context_new ();
  devices = fp_context_get_devices (context);
  g_setenv ("FP_TOD_DRIVERS_DIR", old_drivers_dir, TRUE);

  g_assert_nonnull (devices);
  g_assert_cmpuint (devices->len, ==, 0);
}

static void
test_context_has_fake_device (void)
{
  g_autoptr(FpContext) context = NULL;
  FpDevice *fake_device = NULL;
  GPtrArray *devices;
  unsigned int i;

  context = fp_context_new ();
  devices = fp_context_get_devices (context);

  g_assert_nonnull (devices);
  g_assert_cmpuint (devices->len, ==, 1);

  for (i = 0; i < devices->len; ++i)
    {
      FpDevice *device = devices->pdata[i];

      if (g_strcmp0 (fp_device_get_driver (device), "fake_test_dev") == 0)
        {
          fake_device = device;
          break;
        }
    }

  g_assert_true (FP_IS_DEVICE (fake_device));
}

static void
test_context_enumerates_new_devices (void)
{
  g_autoptr(FpContext) context = NULL;
  GPtrArray *devices;

  context = fp_context_new ();

  fp_context_enumerate (context);
  devices = fp_context_get_devices (context);

  g_assert_nonnull (devices);
  g_assert_cmpuint (devices->len, ==, 1);
}

int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_assert_nonnull (g_getenv ("FP_TOD_DRIVERS_DIR"));

  g_test_add_func ("/context/new", test_context_new);
  g_test_add_func ("/context/no-devices", test_context_has_no_devices);
  g_test_add_func ("/context/has-virtual-device", test_context_has_fake_device);
  g_test_add_func ("/context/enumerates-new-devices", test_context_enumerates_new_devices);

  return g_test_run ();
}
