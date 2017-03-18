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

#include <psp2kern/io/fcntl.h>
#include <psp2kern/kernel/sysmem.h>
#include <taihen.h>

#include <stdio.h>
#include <string.h>

#include "logger.h"
#include "module.h"
#include "utils.h"
#include "exports.h"

#define LOGFILE_PATH PATH("taishell.log")
#define LOGFILE_MODE SCE_O_WRONLY|SCE_O_CREAT|SCE_O_APPEND
#define LOGFILE_PERM 0777

#define MAX_LINE_LENGTH 256

// temporal function, we don't want to write stuff on the kernel
void write_to_file(char *msg, size_t len) {
  SceUID fd;

  fd = ksceIoOpen(LOGFILE_PATH, LOGFILE_MODE , LOGFILE_PERM);
  ksceIoWrite(fd, msg, len);
  ksceIoClose(fd);
}

void _log_(const char level, const char *msg, void *args) {
  char str[MAX_LINE_LENGTH];
  size_t len = 0;

  len = snprintf(str, MAX_LINE_LENGTH, "[%c:] ", level);

  if(args != NULL) {
    SceSysclibForDriver_E38E7605_s print_s = {
      .buffer = &str[len],
      .length = MAX_LINE_LENGTH - len,
      .result = 0
    };

    SceSysclibForDriver_E38E7605(
      (void *)(SceSysclibForDriver_E38E7605 + 0x84C),
      &print_s,
      (void *)msg,
      args
    );
  } else {
    strncpy(&str[len], msg, MAX_LINE_LENGTH - len);
  }

  len = strnlen(str, MAX_LINE_LENGTH-2);
  if(str[len-1] != '\n') {
    strncat(&str[len], "\n", 1);
    len++;
  }

  // disabled for early release
  write_to_file(str, len);
}

void ktshLog(const char level, const char *msg, ...) {
  _log_(level, msg, __builtin_next_arg(msg));
}

void tshLog(const char level, const char *msg) {
  char str[MAX_LINE_LENGTH];

  ksceKernelStrncpyUserToKernel(str, (uintptr_t)msg, MAX_LINE_LENGTH-1);
  _log_(level, str, NULL);
}
