/*
 * FpImageDevice - An image based fingerprint reader device
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

#pragma once

#include <glib.h>
#include <glib-object.h>

#include "tod/tod-macros.h"

typedef struct _FpImage           FpImage;
typedef struct _FpImageTODV1_90_1 FpImageTODV1_90_1;

typedef enum {
  FPI_IMAGE_TODV1_90_1_V_FLIPPED       = 1 << 0,
  FPI_IMAGE_TODV1_90_1_H_FLIPPED       = 1 << 1,
  FPI_IMAGE_TODV1_90_1_COLORS_INVERTED = 1 << 2,
} FpiImageFlagsTODV1_90_1;

typedef enum {
  FPI_IMAGE_TODV1_90_2_V_FLIPPED       = 1 << 0,
  FPI_IMAGE_TODV1_90_2_H_FLIPPED       = 1 << 1,
  FPI_IMAGE_TODV1_90_2_COLORS_INVERTED = 1 << 2,
  FPI_IMAGE_TODV1_90_2_PARTIAL         = 1 << 3,
} FpiImageFlagsTODV1_90_2;

struct _FpImageTODV1_90_1
{
  /*< private >*/
  GObject parent;

  /*< public >*/
  guint                   width;
  guint                   height;

  gdouble                 ppmm;

  FpiImageFlagsTODV1_90_1 flags;

  /*< private >*/
  guint8    *data;
  guint8    *binarized;

  GPtrArray *minutiae;
  guint      ref_count;

  TOD_PADDING (32, 0);
};
