/*
 * Shared library loader for libfprint
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

#define TOD_PADDING(original, wasted) \
  char _tod_expansion_padding[(GLIB_SIZEOF_VOID_P * (original)) - (wasted)];

#define TOD_PADDING_ALIGNED(original, wasted) \
  TOD_PADDING (original, (wasted) + GLIB_SIZEOF_VOID_P)

#define TOD_PADDING_ALIGNED4(original, wasted) \
  TOD_PADDING (original, (wasted) + (GLIB_SIZEOF_VOID_P == 4 ? GLIB_SIZEOF_VOID_P : 0))

#define TOD_PADDING_ALIGNED8(original, wasted) \
  TOD_PADDING (original, (wasted) + (GLIB_SIZEOF_VOID_P == 8 ? GLIB_SIZEOF_VOID_P : 0))
