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

#include <stdio.h>

#include <psp2kern/kernel/sysmem.h>
#include <psp2kern/kernel/cpu.h>

#include "utils.h"
#include "taishell/logger.h"

SceUID ksceKernelFindMemBlockByAddr(const void *addr, SceSize size);

char * build_path(const char *path, const char *sub, char *out) {
  snprintf(out, PATH_MAX, "%s/%s", path ? path : TAISHELL_PATH, sub);
  return out;
}

void ktshMemAlloc() __attribute__ ((weak, alias ("memalloc")));
void * memalloc(SceKernelMemBlockType type, int size) {
  void *addr;
  int ret;

  LOG_D("Trying to allocate a %d bytes memory block", size);
  SceUID block = ksceKernelAllocMemBlock("taishell", type, size, NULL);

  if(block < 0) {
    LOG_E("(0x%08X) Unable to allocate memory block", block);
    return NULL;
  }

  if((ret = ksceKernelGetMemBlockBase(block, &addr)) < 0) {
    LOG_E("(0x%08X) Unable to get memory block address", ret);
    ksceKernelFreeMemBlock(block);
    return NULL;
  }

  return addr;
}

void * tshMemAlloc(SceKernelMemBlockType type, int size) {
  uint32_t state;
  void *ptr;

  ENTER_SYSCALL(state);
  ptr = memalloc(type, size);
  EXIT_SYSCALL(state);

  return ptr;
}

void ktshMemFree() __attribute__ ((weak, alias ("memfree")));
void memfree(void *ptr) {
  SceUID block;

  if((block = ksceKernelFindMemBlockByAddr(ptr, 0)) > 0)
    ksceKernelFreeMemBlock(block);
  else
    LOG_E("Memory block for address %p not found", ptr);
}

void tshMemFree(void *ptr) {
  uint32_t state;
  ENTER_SYSCALL(state);
  memfree(ptr);
  EXIT_SYSCALL(state);
}