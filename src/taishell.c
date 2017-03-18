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
#include <string.h>

#include <psp2kern/kernel/cpu.h>
#include <psp2kern/kernel/threadmgr.h>
#include <psp2kern/kernel/modulemgr.h>
#include <psp2kern/kernel/sysmem.h>
#include <psp2kern/kernel/suspend.h>
#include <psp2/ctrl.h>

#include <taihen.h>

#include "taishell/logger.h"
#include "module.h"
#include "plugins.h"
#include "logger.h"
#include "utils.h"

#define __exports_owner__
#include "exports.h"

#define GUI_PATH PATH("gui.suprx")

SceUID shell_pid;

static SceUID g_hooks[3];

static bool intercept_ctrl;
static int lock_power;

void tshSetButtonIntercept(bool state) {
  intercept_ctrl = state;
}

void tshDisableAutoSuspend() {
  lock_power++;
}

void tshEnableAutoSuspend() {
  lock_power--;
  if(lock_power < 0)
    lock_power = 0;
}

bool tshIsShell() {
  SceUID pid;
  uint32_t state;

  ENTER_SYSCALL(state);
  pid = ksceKernelGetProcessId();
  EXIT_SYSCALL(state);

  return pid == shell_pid;
}

/* Stolen from vitashell */
static int power_tick_thread(SceSize args, void *argp) {
  while (1) {
    if (lock_power > 0) {
      ksceKernelPowerTick(SCE_KERNEL_POWER_TICK_DISABLE_AUTO_SUSPEND);
    }

    ksceKernelDelayThread(10 * 1000 * 1000);
  }
  return 0;
}

void create_power_tick_thread() {
  SceUID thid = ksceKernelCreateThread("power_tick_thread", power_tick_thread, 0x10000100, 0x40000, 0, 0, NULL);
  if (thid >= 0)
    ksceKernelStartThread(thid, 0, NULL);
}

int load_gui(SceUID pid) {
  SceUID uid;

  if(pid == shell_pid)
    uid = ksceKernelLoadStartModuleForPid(pid, GUI_PATH, 0, NULL, 0, NULL, NULL);
  else
    uid = ksceKernelLoadModuleForPid(pid, GUI_PATH, 0x8000, NULL);

  return uid;
}

static tai_hook_ref_t g_load_user_libs_hook;
static int load_user_libs_patched(SceUID pid, void *args, int flags) {
  char titleid[32];
  int ret;

  ret = TAI_CONTINUE(int, g_load_user_libs_hook, pid, args, flags);

  ksceKernelGetProcessTitleId(pid, titleid, 32);
  LOG_D("Title started: %s, PID: 0x%x", titleid, pid);

  load_gui(pid);
  queue_plugins_for_pid(pid);

  return ret;
}

static tai_hook_ref_t g_unload_process_hook;
static int unload_process_patched(SceUID pid) {
  LOG_D("Unloading PID: 0x%x", pid);
  // It doesn't seem to be possible
  // to stop modules explicitly here...
  //unload_plugins_for_pid(pid);

  return TAI_CONTINUE(int, g_unload_process_hook, pid);
}

static tai_hook_ref_t g_ctrl_hook;
static int sceCtrlPeekBufferPositive2_patched(int port, SceCtrlData *ctrl, int count) {
  int ret = TAI_CONTINUE(int, g_ctrl_hook, port, ctrl, count);

  if(ret > 0 && intercept_ctrl) {
    uint32_t buttons = 0;
    ksceKernelMemcpyKernelToUser((uintptr_t)&ctrl->buttons, (const void *)&buttons, sizeof(uint32_t));
  }

  return ret;
}

void add_hooks(void) {
  memset(g_hooks, 0, sizeof(g_hooks));

  g_hooks[0] = taiHookFunctionExportForKernel(KERNEL_PID,
                                              &g_load_user_libs_hook,
                                              "SceKernelModulemgr",
                                              0xC445FA63, // SceModulemgrForKernel
                                              0x3AD26B43,
                                              load_user_libs_patched);

  g_hooks[1] = taiHookFunctionImportForKernel(KERNEL_PID,
                                              &g_unload_process_hook,
                                              "SceProcessmgr",
                                              0xC445FA63, // SceModulemgrForKernel
                                              0x0E33258E,
                                              unload_process_patched);

  g_hooks[2] = taiHookFunctionExportForKernel(KERNEL_PID,
                                              &g_ctrl_hook,
                                              "SceCtrl",
                                              TAI_ANY_LIBRARY,
                                              0x15F81E8C, // sceCtrlPeekBufferPositive2
                                              sceCtrlPeekBufferPositive2_patched);
}

static void find_exports() {
  uintptr_t ptr;

  module_get_export_func(
    KERNEL_PID,
    "SceIofilemgr",
    0x40FD29C7, // SceIofilemgrForDriver
    0x19C81DD6, // ksceIoDclose
    &ptr
  );
  ksceIoDclose = (void *)ptr;

  module_get_export_func(
    KERNEL_PID,
    "SceSysmem",
    0x7EE45391, // SceSysclibForDriver
    0xE38E7605, // (sn)printf formater ??
    &ptr
  );
  SceSysclibForDriver_E38E7605 = (void *)ptr;

/*
  int ret = module_get_export_func(
    KERNEL_PID,
    "SceKernelThreadMgr",
    0xE2C40624, // SceKernelThreadMgrForDriver
    0xC3E00919, // ksceKernelNotifyCallback
    &ptr
  );
  SceThreadmgrForDriver_C3E00919 = (void *)ptr;
*/
}

static int find_shell_pid() {
  char titleid[16];
  int ret;

  for(int i = 70000; i < 80000; i++) {
    ret = ksceKernelGetProcessTitleId(i, titleid, 16);
    if(ret >= 0 && strncmp(titleid, "main", 4) == 0) {
      shell_pid = i;
      LOG_D("SceShell PID: %d", shell_pid);
      return 0;
    }
  }
  LOG_E("SceShell PID not found...");
  return -1;
}

void _start() __attribute__ ((weak, alias ("module_start")));
int module_start(SceSize argc, const void *args) {
  find_exports();

  LOG_I("TaiShell loaded");

  find_shell_pid();
  add_hooks();
  create_power_tick_thread();
  init_plugins();
  load_gui(shell_pid);
  load_initial_plugins();

  return SCE_KERNEL_START_SUCCESS;
}

int module_stop(SceSize argc, const void *args) {
  taiHookReleaseForKernel(g_hooks[0], g_load_user_libs_hook);
  taiHookReleaseForKernel(g_hooks[1], g_unload_process_hook);
  taiHookReleaseForKernel(g_hooks[2], g_ctrl_hook);

  return SCE_KERNEL_STOP_SUCCESS;
}
