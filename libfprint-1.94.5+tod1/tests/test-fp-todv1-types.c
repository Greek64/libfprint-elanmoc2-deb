/*
 * FpDevice Unit tests
 * Copyright (C) 2021 Marco Trevisan <marco.trevisan@canonical.com>
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

#include "drivers_api.h"
#include "fp-todv1-enums.h"

#include "tod-drivers/base-fp-device.h"
#include "tod-drivers/base-fp-print.h"
#include "tod-drivers/base-fpi-device.h"
#include "tod-drivers/base-fpi-image.h"
#include "tod-drivers/base-fpi-image-device.h"
#include "tod-drivers/base-fpi-spi.h"
#include "tod-drivers/base-fpi-usb.h"

static void
check_enum_compatibility (GType old_type, GType current_type)
{
  g_autoptr(GEnumClass) old_class = g_type_class_ref (old_type);
  g_autoptr(GEnumClass) current_class = g_type_class_ref (current_type);
  int i;

  g_debug ("Checking Enum %s vs %s",
           g_type_name (current_type),
           g_type_name (old_type));

  g_assert_true (G_TYPE_IS_ENUM (old_type));
  g_assert_true (G_TYPE_IS_ENUM (current_type));
  g_assert_cmpuint (old_class->n_values, <=, current_class->n_values);

  for (i = old_class->minimum; i <= old_class->maximum; i++)
    {
      GEnumValue *old_value = g_enum_get_value (old_class, i);
      GEnumValue *current_value;

      if (!old_value)
        continue;

      current_value = g_enum_get_value_by_nick (current_class,
                                                old_value->value_nick);

      g_debug (" .. %s (%d)", old_value->value_nick, old_value->value);
      g_assert_nonnull (current_value);
      g_assert_cmpuint (old_value->value, ==, current_value->value);
    }
}

static void
check_flags_compatibility (GType old_type, GType current_type)
{
  g_autoptr(GFlagsClass) old_class = g_type_class_ref (old_type);
  g_autoptr(GFlagsClass) current_class = g_type_class_ref (current_type);
  int i;

  g_debug ("Checking Flags %s vs %s",
           g_type_name (current_type),
           g_type_name (old_type));

  g_assert_true (G_TYPE_IS_FLAGS (old_type));
  g_assert_true (G_TYPE_IS_FLAGS (current_type));
  g_assert_cmpuint (old_class->n_values, <=, current_class->n_values);

  for (i = 0; i < old_class->n_values; ++i)
    {
      GFlagsValue *old_value = &old_class->values[i];
      GFlagsValue *current_value = g_flags_get_value_by_nick (current_class,
                                                              old_value->value_nick);

      g_debug (" .. %s (%d)", old_value->value_nick, old_value->value);
      g_assert_nonnull (current_value);
      g_assert_cmpuint (old_value->value, ==, current_value->value);
    }
}

static void
check_compatiblity_auto (GType old_type, GType current_type)
{
  if (G_TYPE_IS_ENUM (old_type))
    return check_enum_compatibility (old_type, current_type);

  if (G_TYPE_IS_FLAGS (old_type))
    return check_flags_compatibility (old_type, current_type);

  g_assert_not_reached ();
}

#define check_type_compatibility(type, major, minor, micro) \
  g_debug ("Checking " # type " @ " G_STRLOC); \
  check_compatiblity_auto (type ## _TOD_V ## major ## _ ## minor ## _ ## micro, type);

#define tod_versioned_type(type, major, minor, micro) \
  type ## TODV ## major ## _ ## minor ## _ ## micro

#define check_struct_size(type, major, minor, micro) \
  g_debug ("Checking " # type " v" #major "." #minor "." #micro " size  @ " G_STRLOC); \
  g_assert_cmpuint (sizeof (tod_versioned_type (type, major, minor, micro)), \
                    ==, \
                    sizeof (type))

#define check_struct_member(type, major, minor, micro, member) \
  g_debug ("Checking " # type " v" #major "." #minor "." #micro "'s " # member " offset @ " G_STRLOC); \
  g_assert_cmpuint (G_STRUCT_OFFSET (tod_versioned_type (type, major, minor, micro), member), \
                    ==, \
                    G_STRUCT_OFFSET (type, member))

static void
test_device_type (void)
{
  check_struct_size (FpIdEntry, 1, 90, 1);
  check_struct_size (FpIdEntry, 1, 92, 0);
  check_struct_size (FpDeviceClass, 1, 90, 1);
  check_struct_size (FpDeviceClass, 1, 92, 0);
  check_struct_size (FpDeviceClass, 1, 94, 0);

  check_struct_member (FpIdEntry, 1, 90, 1, virtual_envvar);
  check_struct_member (FpIdEntry, 1, 90, 1, driver_data);

  check_struct_member (FpDeviceClass, 1, 90, 1, id);
  check_struct_member (FpDeviceClass, 1, 90, 1, full_name);
  check_struct_member (FpDeviceClass, 1, 90, 1, type);
  check_struct_member (FpDeviceClass, 1, 90, 1, id_table);

  check_struct_member (FpDeviceClass, 1, 90, 1, nr_enroll_stages);
  check_struct_member (FpDeviceClass, 1, 90, 1, scan_type);

  check_struct_member (FpDeviceClass, 1, 90, 1, usb_discover);
  check_struct_member (FpDeviceClass, 1, 90, 1, probe);
  check_struct_member (FpDeviceClass, 1, 90, 1, open);
  check_struct_member (FpDeviceClass, 1, 90, 1, close);
  check_struct_member (FpDeviceClass, 1, 90, 1, enroll);
  check_struct_member (FpDeviceClass, 1, 90, 1, verify);
  check_struct_member (FpDeviceClass, 1, 90, 1, identify);
  check_struct_member (FpDeviceClass, 1, 90, 1, capture);
  check_struct_member (FpDeviceClass, 1, 90, 1, list);
  check_struct_member (FpDeviceClass, 1, 90, 1, delete);
  check_struct_member (FpDeviceClass, 1, 90, 1, cancel);

  /* Version 1.92 */
  check_struct_member (FpIdEntry, 1, 92, 0, virtual_envvar);
  check_struct_member (FpIdEntry, 1, 92, 0, driver_data);
  check_struct_member (FpIdEntry, 1, 92, 0, udev_types);
  check_struct_member (FpIdEntry, 1, 92, 0, spi_acpi_id);
  check_struct_member (FpIdEntry, 1, 92, 0, hid_id);

  check_struct_member (FpDeviceClass, 1, 92, 0, usb_discover);
  check_struct_member (FpDeviceClass, 1, 92, 0, probe);
  check_struct_member (FpDeviceClass, 1, 92, 0, open);
  check_struct_member (FpDeviceClass, 1, 92, 0, close);
  check_struct_member (FpDeviceClass, 1, 92, 0, enroll);
  check_struct_member (FpDeviceClass, 1, 92, 0, verify);
  check_struct_member (FpDeviceClass, 1, 92, 0, identify);
  check_struct_member (FpDeviceClass, 1, 92, 0, capture);
  check_struct_member (FpDeviceClass, 1, 92, 0, list);
  check_struct_member (FpDeviceClass, 1, 92, 0, delete);
  check_struct_member (FpDeviceClass, 1, 92, 0, cancel);

  check_struct_member (FpDeviceClass, 1, 92, 0, id);
  check_struct_member (FpDeviceClass, 1, 92, 0, full_name);
  check_struct_member (FpDeviceClass, 1, 92, 0, type);
  check_struct_member (FpDeviceClass, 1, 92, 0, id_table);

  check_struct_member (FpDeviceClass, 1, 92, 0, nr_enroll_stages);
  check_struct_member (FpDeviceClass, 1, 92, 0, scan_type);

  check_struct_member (FpDeviceClass, 1, 92, 0, features);

  /* Version 1.94 */
  check_struct_member (FpDeviceClass, 1, 94, 0, usb_discover);
  check_struct_member (FpDeviceClass, 1, 94, 0, probe);
  check_struct_member (FpDeviceClass, 1, 94, 0, open);
  check_struct_member (FpDeviceClass, 1, 94, 0, close);
  check_struct_member (FpDeviceClass, 1, 94, 0, enroll);
  check_struct_member (FpDeviceClass, 1, 94, 0, verify);
  check_struct_member (FpDeviceClass, 1, 94, 0, identify);
  check_struct_member (FpDeviceClass, 1, 94, 0, capture);
  check_struct_member (FpDeviceClass, 1, 94, 0, list);
  check_struct_member (FpDeviceClass, 1, 94, 0, delete);
  check_struct_member (FpDeviceClass, 1, 94, 0, cancel);
  check_struct_member (FpDeviceClass, 1, 94, 0, clear_storage);
  check_struct_member (FpDeviceClass, 1, 94, 0, suspend);
  check_struct_member (FpDeviceClass, 1, 94, 0, resume);

  check_struct_member (FpDeviceClass, 1, 94, 0, id);
  check_struct_member (FpDeviceClass, 1, 94, 0, full_name);
  check_struct_member (FpDeviceClass, 1, 94, 0, type);
  check_struct_member (FpDeviceClass, 1, 94, 0, id_table);

  check_struct_member (FpDeviceClass, 1, 94, 0, nr_enroll_stages);
  check_struct_member (FpDeviceClass, 1, 94, 0, scan_type);

  check_struct_member (FpDeviceClass, 1, 94, 0, features);

  check_struct_member (FpDeviceClass, 1, 94, 0, temp_hot_seconds);
  check_struct_member (FpDeviceClass, 1, 94, 0, temp_cold_seconds);
}

static void
test_image_device_private (void)
{
  check_struct_size (FpImage, 1, 90, 1);
  check_struct_size (FpImageDeviceClass, 1, 90, 1);

  check_struct_member (FpImageDeviceClass, 1, 90, 1, bz3_threshold);
  check_struct_member (FpImageDeviceClass, 1, 90, 1, img_width);
  check_struct_member (FpImageDeviceClass, 1, 90, 1, img_height);
  check_struct_member (FpImageDeviceClass, 1, 90, 1, img_open);
  check_struct_member (FpImageDeviceClass, 1, 90, 1, img_close);
  check_struct_member (FpImageDeviceClass, 1, 90, 1, activate);
  check_struct_member (FpImageDeviceClass, 1, 90, 1, change_state);
  check_struct_member (FpImageDeviceClass, 1, 90, 1, deactivate);
}

static void
test_usb_private (void)
{
  check_struct_size (FpiUsbTransfer, 1, 90, 1);

  check_struct_member (FpiUsbTransfer, 1, 90, 1, device);
  check_struct_member (FpiUsbTransfer, 1, 90, 1, ssm);
  check_struct_member (FpiUsbTransfer, 1, 90, 1, length);
  check_struct_member (FpiUsbTransfer, 1, 90, 1, actual_length);
  check_struct_member (FpiUsbTransfer, 1, 90, 1, buffer);
  check_struct_member (FpiUsbTransfer, 1, 90, 1, ref_count);
  check_struct_member (FpiUsbTransfer, 1, 90, 1, type);
  check_struct_member (FpiUsbTransfer, 1, 90, 1, endpoint);
  check_struct_member (FpiUsbTransfer, 1, 90, 1, direction);
  check_struct_member (FpiUsbTransfer, 1, 90, 1, request_type);
  check_struct_member (FpiUsbTransfer, 1, 90, 1, recipient);
  check_struct_member (FpiUsbTransfer, 1, 90, 1, request);
  check_struct_member (FpiUsbTransfer, 1, 90, 1, value);
  check_struct_member (FpiUsbTransfer, 1, 90, 1, idx);
  check_struct_member (FpiUsbTransfer, 1, 90, 1, short_is_error);
  check_struct_member (FpiUsbTransfer, 1, 90, 1, user_data);
  check_struct_member (FpiUsbTransfer, 1, 90, 1, callback);
  check_struct_member (FpiUsbTransfer, 1, 90, 1, free_buffer);
}

static void
test_spi_private (void)
{
  check_struct_size (FpiSpiTransfer, 1, 92, 0);

  check_struct_member (FpiSpiTransfer, 1, 92, 0, device);
  check_struct_member (FpiSpiTransfer, 1, 92, 0, ssm);
  check_struct_member (FpiSpiTransfer, 1, 92, 0, length_wr);
  check_struct_member (FpiSpiTransfer, 1, 92, 0, length_rd);
  check_struct_member (FpiSpiTransfer, 1, 92, 0, buffer_wr);
  check_struct_member (FpiSpiTransfer, 1, 92, 0, buffer_rd);
  check_struct_member (FpiSpiTransfer, 1, 92, 0, ref_count);
  check_struct_member (FpiSpiTransfer, 1, 92, 0, spidev_fd);
  check_struct_member (FpiSpiTransfer, 1, 92, 0, user_data);
  check_struct_member (FpiSpiTransfer, 1, 92, 0, callback);
  check_struct_member (FpiSpiTransfer, 1, 92, 0, free_buffer_wr);
  check_struct_member (FpiSpiTransfer, 1, 92, 0, free_buffer_rd);
}

static void
test_device_public_enums (void)
{
  check_type_compatibility (FP_TYPE_DEVICE_TYPE, 1, 90, 1);
  check_type_compatibility (FP_TYPE_SCAN_TYPE, 1, 90, 1);
  check_type_compatibility (FP_TYPE_DEVICE_RETRY, 1, 90, 1);
  check_type_compatibility (FP_TYPE_DEVICE_ERROR, 1, 90, 1);
  check_type_compatibility (FP_TYPE_DEVICE_ERROR, 1, 90, 3);
  check_type_compatibility (FP_TYPE_DEVICE_ERROR, 1, 90, 4);
  check_type_compatibility (FP_TYPE_DEVICE_ERROR, 1, 94, 0);
  check_type_compatibility (FP_TYPE_DEVICE_FEATURE, 1, 92, 0);
  check_type_compatibility (FP_TYPE_DEVICE_FEATURE, 1, 94, 0);
  check_type_compatibility (FP_TYPE_DEVICE_FEATURE, 1, 94, 3);
  check_type_compatibility (FP_TYPE_TEMPERATURE, 1, 94, 0);
  check_type_compatibility (FPI_TYPE_DEVICE_UDEV_SUBTYPE_FLAGS, 1, 92, 0);
}

static void
test_device_private_enums (void)
{
  check_type_compatibility (FPI_TYPE_DEVICE_ACTION, 1, 90, 1);
  check_type_compatibility (FPI_TYPE_DEVICE_ACTION, 1, 92, 0);
}

static void
test_print_public_enums (void)
{
  check_type_compatibility (FP_TYPE_FINGER, 1, 90, 1);
  check_type_compatibility (FP_TYPE_FINGER_STATUS_FLAGS, 1, 90, 4);
}

static void
test_print_private_enums (void)
{
  check_type_compatibility (FPI_TYPE_PRINT_TYPE, 1, 90, 1);
  check_type_compatibility (FPI_TYPE_MATCH_RESULT, 1, 90, 1);
}

static void
test_image_device_enums (void)
{
  check_type_compatibility (FPI_TYPE_IMAGE_FLAGS, 1, 90, 1);
  check_type_compatibility (FPI_TYPE_IMAGE_FLAGS, 1, 90, 2);
  check_type_compatibility (FPI_TYPE_IMAGE_DEVICE_STATE, 1, 90, 1);
  check_type_compatibility (FPI_TYPE_IMAGE_DEVICE_STATE, 1, 90, 4);
}

static void
test_usb_enums (void)
{
  check_type_compatibility (FPI_TYPE_TRANSFER_TYPE, 1, 90, 3);
}

int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/type/device/private", test_device_type);
  g_test_add_func ("/type/device/enums", test_device_public_enums);
  g_test_add_func ("/type/device/private/enums", test_device_private_enums);
  g_test_add_func ("/type/print/enums", test_print_public_enums);
  g_test_add_func ("/type/print/private/enums", test_print_private_enums);
  g_test_add_func ("/type/image-device/private", test_image_device_private);
  g_test_add_func ("/type/image-device/enums", test_image_device_enums);
  g_test_add_func ("/type/usb/private", test_usb_private);
  g_test_add_func ("/type/usb/enums", test_usb_enums);
  g_test_add_func ("/type/spi/private", test_spi_private);

  return g_test_run ();
}
