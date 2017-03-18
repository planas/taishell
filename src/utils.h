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

#ifndef __UTILS_H__
#define __UTILS_H__

#define PATH(SUB) (TAISHELL_PATH "/" SUB)
#define ALIGN(x, a) (((x) + ((a) - 1)) & ~((a) - 1))

char * build_path(const char *path, const char *sub, char *out);
void * memalloc(SceKernelMemBlockType type, int size);
void memfree(void *ptr);

#endif
