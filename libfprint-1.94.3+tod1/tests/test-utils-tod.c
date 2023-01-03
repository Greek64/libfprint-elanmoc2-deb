/*
 * Unit tests for libfprint
 * Copyright (C) 2020 Marco Trevisan <marco.trevisan@canonical.com>
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

#include "test-utils-tod.h"

static FptContext *fake_context = NULL;

FptContext *
fpt_context_new_with_fake_dev (void)
{
  FptContext *tctx;
  GPtrArray *devices;
  const char *tod_name;
  unsigned int i;

  tctx = fpt_context_new ();
  devices = fp_context_get_devices (tctx->fp_context);
  tod_name = g_getenv ("FP_TOD_TEST_DRIVER_NAME");

  g_assert_nonnull (tod_name);
  g_assert_nonnull (devices);
  g_assert_cmpuint (devices->len, ==, 1);

  for (i = 0; i < devices->len; ++i)
    {
      FpDevice *device = devices->pdata[i];

      if (g_strcmp0 (fp_device_get_driver (device), tod_name) == 0)
        {
          tctx->device = device;
          break;
        }
    }

  g_assert_true (FP_IS_DEVICE (tctx->device));
  g_object_add_weak_pointer (G_OBJECT (tctx->device), (gpointer) & tctx->device);

  return tctx;
}

FptContext *
fpt_context_fake_dev_default (void)
{
  if (fake_context)
    return fake_context;

  fake_context = fpt_context_new_with_fake_dev ();
  return fake_context;
}
