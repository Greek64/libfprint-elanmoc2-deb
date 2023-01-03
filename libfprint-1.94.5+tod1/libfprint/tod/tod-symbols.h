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

#define LIBFPRINT_2_SYMBOL_VERSION_2_0 "LIBFPRINT_2.0.0"
#define LIBFPRINT_2_SYMBOL_VERSION(major, minor) \
  LIBFPRINT_2_SYMBOL_VERSION_ ## major ## _ ## minor

#define TOD_1_SYMBOL_VERSION_1_90 "LIBFPRINT_TOD_1.0.0"
#define TOD_1_SYMBOL_VERSION_1_92 "LIBFPRINT_TOD_1_1.92"
#define TOD_1_SYMBOL_VERSION_1_94 "LIBFPRINT_TOD_1_1.94"
#define TOD_1_SYMBOL_VERSION(major, minor) \
  TOD_1_SYMBOL_VERSION_ ## major ## _ ## minor

#define TOD_DEFAULT_UPSTREAM_SYMBOL_VERSIONED(symbol, major, minor) \
  __asm__ (".symver " # symbol "," # symbol "@@@" \
           LIBFPRINT_2_SYMBOL_VERSION (major, minor));

#define TOD_DEFAULT_UPSTREAM_SYMBOL(symbol) \
  __asm__ (".symver " # symbol "," # symbol "@@@");

#define TOD_DEFAULT_VERSION_SYMBOL(symbol, major, minor) \
  __asm__ (".symver " # symbol "," # symbol "@@@" \
           TOD_1_SYMBOL_VERSION (major, minor));
#define TOD_VERSIONED_SYMBOL(symbol, major, minor) \
  __asm__ (".symver " # symbol "_" # major "_" #minor "," # symbol "@" \
           TOD_1_SYMBOL_VERSION (major, minor));

TOD_DEFAULT_VERSION_SYMBOL (fpi_ssm_new_full, 1, 92)
TOD_VERSIONED_SYMBOL (fpi_ssm_new_full, 1, 90)

TOD_DEFAULT_VERSION_SYMBOL (fpi_ssm_next_state_delayed, 1, 92)
TOD_VERSIONED_SYMBOL (fpi_ssm_next_state_delayed, 1, 90)

TOD_DEFAULT_VERSION_SYMBOL (fpi_ssm_jump_to_state_delayed, 1, 92)
TOD_VERSIONED_SYMBOL (fpi_ssm_jump_to_state_delayed, 1, 90)

TOD_DEFAULT_VERSION_SYMBOL (fpi_ssm_mark_completed_delayed, 1, 92)
TOD_VERSIONED_SYMBOL (fpi_ssm_mark_completed_delayed, 1, 90)
