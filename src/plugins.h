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

#ifndef __PLUGIN_H__
#define __PLUGIN_H__

#include <stdbool.h>
#include <psp2kern/types.h>

#include "taishell/plugin.h"

void init_plugins();
void queue_plugins_for_pid(SceUID pid);
void unload_plugins_for_pid(SceUID pid);
void load_initial_plugins();

#endif
