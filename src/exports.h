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

#ifndef __EXPORTS_H__
#define __EXPORTS_H__

#ifndef __exports_owner__
#define __extern__
#else
#define __extern__ extern
#endif

typedef struct {
  size_t length;
  int result;
  char *buffer;
} SceSysclibForDriver_E38E7605_s;

__extern__ int (*SceThreadmgrForDriver_C3E00919)(SceUID cb, int arg2);
__extern__ int (*SceSysclibForDriver_E38E7605)(void *, SceSysclibForDriver_E38E7605_s *, void *, void *);
__extern__ int (*ksceIoDclose)(SceUID);

#endif