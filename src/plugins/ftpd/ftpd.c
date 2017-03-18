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

#include <psp2/kernel/processmgr.h>
#include <psp2/kernel/modulemgr.h>

#include "taishell/logger.h"
#include "taishell/utils.h"

#include "./libftpvita/ftpvita.h"

void ftpvita_log_info(const char *msg) {
  LOG_I(msg);
}

void ftpvita_log_debug(const char *msg) {
  LOG_D(msg);
}

void _start() __attribute__ ((weak, alias ("module_start")));
int module_start(SceSize argc, const void *args) {
  unsigned short int vita_port;
  char vita_ip[16];

  LOG_D("FTPD loaded");

  tshDisableAutoSuspend();

  ftpvita_set_info_log_cb(ftpvita_log_info);
  ftpvita_set_debug_log_cb(ftpvita_log_debug);

  if (ftpvita_init(vita_ip, &vita_port) < 0) {
    LOG_D("You have to enable Wi-Fi.\n");
    return SCE_KERNEL_START_FAILED;
  }

  ftpvita_add_device("ux0:");
  ftpvita_add_device("ur0:");

  LOG_D("FTP initialized. Listening at %s@%d", vita_ip, vita_port);

  return SCE_KERNEL_START_SUCCESS;
}

int module_stop(SceSize argc, const void *args) {
  LOG_D("Stopping FTPd");
  tshEnableAutoSuspend();
  ftpvita_fini();
  return SCE_KERNEL_STOP_SUCCESS;
}
