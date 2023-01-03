/*
 * Shared library loader for libfprint
 * Copyright (C) 2022 Marco Trevisan <marco.trevisan@canonical.com>
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

#include "fp-device-private.h"
#include "fpi-device.h"
#define FP_COMPONENT "tod"

#include "tod-goodix-wrapper.h"

static void (*goodix_moc_identify)(FpDevice *) = NULL;

static void
goodix_tod_identify_wrapper (FpDevice *device)
{
  GPtrArray *prints;

  fpi_device_get_identify_data (device, &prints);

  if (prints->len)
    return goodix_moc_identify (device);

  g_warning ("%s does not support identify with empty gallery, let's skip it",
             fp_device_get_name (device));

  fpi_device_identify_report (device, NULL, NULL, NULL);
  fpi_device_identify_complete (device, NULL);
}

void
goodix_tod_wrapper_init (FpDeviceClass *device_class)
{
  g_assert (g_strcmp0 (device_class->id, "goodix-tod") == 0);
  g_assert (goodix_moc_identify == NULL);

  g_message ("Creating TOD wrapper for %s (%s) driver",
             device_class->id, device_class->full_name);

  goodix_moc_identify = device_class->identify;
  device_class->identify = goodix_tod_identify_wrapper;
}
