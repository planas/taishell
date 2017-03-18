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

#ifndef __MODULE_H__
#define __MODULE_H__

// taihenModuleUtils exports not present on taihen.h
int module_get_by_name_nid(SceUID pid, const char *name, uint32_t nid, tai_module_info_t *info);
int module_get_offset(SceUID pid, SceUID modid, int segidx, size_t offset, uintptr_t *addr);
int module_get_export_func(SceUID pid, const char *modname, uint32_t libnid, uint32_t funcnid, uintptr_t *func);
int module_get_import_func(SceUID pid, const char *modname, uint32_t target_libnid, uint32_t funcnid, uintptr_t *stub);

#endif