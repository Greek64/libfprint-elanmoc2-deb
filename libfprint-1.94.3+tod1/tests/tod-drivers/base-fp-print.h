/*
 * FpDevice - A fingerprint reader device
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

typedef enum {
  FP_FINGER_TODV1_90_1_UNKNOWN = 0,
  FP_FINGER_TODV1_90_1_LEFT_THUMB,
  FP_FINGER_TODV1_90_1_LEFT_INDEX,
  FP_FINGER_TODV1_90_1_LEFT_MIDDLE,
  FP_FINGER_TODV1_90_1_LEFT_RING,
  FP_FINGER_TODV1_90_1_LEFT_LITTLE,
  FP_FINGER_TODV1_90_1_RIGHT_THUMB,
  FP_FINGER_TODV1_90_1_RIGHT_INDEX,
  FP_FINGER_TODV1_90_1_RIGHT_MIDDLE,
  FP_FINGER_TODV1_90_1_RIGHT_RING,
  FP_FINGER_TODV1_90_1_RIGHT_LITTLE,

  FP_FINGER_TODV1_90_1_FIRST = FP_FINGER_TODV1_90_1_LEFT_THUMB,
  FP_FINGER_TODV1_90_1_LAST = FP_FINGER_TODV1_90_1_RIGHT_LITTLE,
} FpFingerTODV1_90_1;

typedef enum {
  FP_FINGER_STATUS_TODV1_90_4_NONE    = 0,
  FP_FINGER_STATUS_TODV1_90_4_NEEDED  = 1 << 0,
  FP_FINGER_STATUS_TODV1_90_4_PRESENT = 1 << 1,
} FpFingerStatusFlagsTODV1_90_4;

/* Private flags */

typedef enum {
  FPI_PRINT_TODV1_90_1_UNDEFINED = 0,
  FPI_PRINT_TODV1_90_1_RAW,
  FPI_PRINT_TODV1_90_1_NBIS,
} FpiPrintTypeTODV1_90_1;

typedef enum {
  FPI_MATCH_TODV1_90_1_ERROR = -1,
  FPI_MATCH_TODV1_90_1_FAIL,
  FPI_MATCH_TODV1_90_1_SUCCESS,
} FpiMatchResultTODV1_90_1;
