/*
  TaiShell
  Copyright (C) 2017, Adrià Planas

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef __REGISTRY_H__
#define __REGISTRY_H__

#include "list.h"

#define TSR_MAGIC_NUMBER "TSR\0"
#define TSR_FILE_VERSION 0x0001

#define REGISTRY_MAX_KEY 32

typedef struct {
  char magic[4];
  uint16_t version;
} tsh_reg_header_t;

typedef struct {
  char key[REGISTRY_MAX_KEY];
  int value;
} tsh_reg_entry_t;

int registry_get_list(const char *fp, tsh_list_t *list);
int registry_store_list(const char *fp, tsh_list_t *list);
int registry_set(const char *fp, const char *key, int value);

#endif