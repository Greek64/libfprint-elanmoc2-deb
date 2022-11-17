/*
 * Upek TouchChip Fingerprint Coprocessor definitions
 * Copyright (c) 2013 Vasily Khoruzhick <anarsoul@gmail.com>
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

static const unsigned char upek2020_init_1[] = {
  'C', 'i', 'a', 'o',
  0x04,
  0x00, 0x0d,
  0x01, 0x00, 0x46, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
  0x01, 0x00, 0x00, 0x00,
  0xda, 0xc1
};

static const unsigned char upek2020_init_2[] = {
  0x43, 0x69, 0x61, 0x6f,
  0x07,
  0x00, 0x01,
  0x01,
  0x3d, 0x72
};

static const unsigned char upek2020_init_3[] = {
  'C', 'i', 'a', 'o',
  0x04,
  0x00, 0x0d,
  0x01, 0x00, 0xbc, 0x02, 0x00, 0x00, 0x00, 0x00, 0x01,
  0x01, 0x00, 0x00, 0x00,
  0x55, 0x2f
};

static const unsigned char upek2020_init_4[] = {
  'C', 'i', 'a', 'o',
  0x00,
  0x00, 0x07,
  0x28, 0x04, 0x00, 0x00, 0x00, 0x06, 0x04,
  0xc0, 0xd6
};

static const unsigned char upek2020_deinit[] = {
  'C', 'i', 'a', 'o',
  0x07,
  0x00, 0x01,
  0x01,
  0x3d,
  0x72
};

static const unsigned char upek2020_init_capture[] = {
  'C', 'i', 'a', 'o',
  0x00,
  0x00, 0x0e, /* Seq = 7, len = 0x00e */
  0x28, /* CMD = 0x28 */
  0x0b, 0x00, /* Inner len = 0x000b */
  0x00, 0x00,
  0x0e, /* SUBCMD = 0x0e */
  0x02,
  0xfe, 0xff, 0xff, 0xff, /* timeout = -2 = 0xfffffffe = infinite time */
  0x02,
  0x00, /* Wait for acceptable finger */
  0x02,
  0x14, 0x9a /* CRC */
};

#if 0
static const unsigned char finger_status[] = {
  'C', 'i', 'a', 'o',
  0x00,
  0x70, 0x14, /* Seq = 7, len = 0x014 */
  0x28, /* CMD = 0x28 */
  0x11, 0x00, /* Inner len = 0x0011 */
  0x00, 0x00, 0x00, 0x20, 0x01, 0x00, 0x00, 0x00,
  0x0c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00,
  0x26, 0x03, /* CRC */
  0x00, 0x00, 0x00, /* Rest is garbage */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};
#endif

static const unsigned char upek2020_ack_00_28[] = {
  'C', 'i', 'a', 'o',
  0x00,
  0x80, 0x08, /* Seq = 8, len = 0x008 */
  0x28, /* CMD = 0x28 */
  0x05, 0x00, /* Inner len = 0x0005 */
  0x00, 0x00, 0x00, 0x30, 0x01,
  0x6a, 0xc4 /* CRC */
};

#if 0
/* No seq should be tracked here */
static const unsigned char got_finger[] = {
  'C', 'i', 'a', 'o',
  0x08,
  0x00, 0x00, /* Seq = 0, len = 0x000 */
  0xa1, 0xa9, /* CRC */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* Rest is garbage */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};
#endif

/* No seq should be put in there */
static const unsigned char upek2020_ack_08[] = {
  'C', 'i', 'a', 'o',
  0x09,
  0x00, 0x00, /* Seq = 0, len = 0x0 */
  0x91, 0x9e /* CRC */
};

static const unsigned char upek2020_ack_frame[] = {
  'C', 'i', 'a', 'o',
  0x00,
  0x50, 0x01, /* Seq = 5, len = 0x001 */
  0x30,
  0xac, 0x5b /* CRC */
};
