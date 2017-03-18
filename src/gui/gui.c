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

#include <psp2/kernel/modulemgr.h>
#include <psp2/kernel/threadmgr.h>
#include <psp2/kernel/sysmem.h>
#include <psp2kern/kernel/cpu.h>
#include <psp2/gxm.h>
#include <psp2/display.h>
#include <psp2/ctrl.h>
#include <psp2/kernel/clib.h>

#include <taihen.h>

#include "blit.h"
#include "menu.h"

#include "taishell/plugin.h"
#include "taishell/logger.h"
#include "taishell/utils.h"

static SceUID g_hooks[5];

static SceGxmDisplayQueueCallback **displayQCB;
static SceGxmDisplayQueueCallback *displayQCB_shell;

static uint32_t old_buttons, pressed_buttons;
static int visible;

static int is_shell;

typedef void (*tsh_fb_cb_t)(const SceDisplayFrameBuf *fb);

static tsh_fb_cb_t fb_callbacks[16];
int n_fb_callbacks;

// messy :3
void tshRegisterFbCallback(tsh_fb_cb_t cb) {
  if(n_fb_callbacks+1 >= 16)
    return;

  fb_callbacks[n_fb_callbacks++] = cb;
}

struct display_cb_data {
  void     *base;  // fb memory addr
  uint32_t pitch;  // 960
  uint32_t width;  // 960
  uint32_t height; // 544
  int      unk0;   // sync? always 1
  int      unk1;   // sizeof...? always 0x100000
  void     *unk2;  // function address
  void     *unk3;  // function address
  void     *unk4;  // function address
  void     *unk5;  // function address
};

void draw_gui(const SceDisplayFrameBuf *fb) {
  if(blit_set_frame_buf(fb) < 0)
    return;

  menu_draw();
}

void hide_gui() {
  visible = 0;
  menu_reset();
}

void display_callback(const void *callback_data) {
  const struct display_cb_data *data = callback_data;

  SceDisplayFrameBuf fb = {
    .base        = data->base,
    .pitch       = data->pitch,
    .width       = data->width,
    .height      = data->height,
    .pixelformat = SCE_DISPLAY_PIXELFORMAT_A8B8G8R8
  };

  if(visible) {
    if(fb.base) // when shell is on the bg base is null
      draw_gui(&fb);
    else
      visible = false;
  }

  if(fb.base != NULL) {
    for(int i = 0; i < n_fb_callbacks; i++) {
      fb_callbacks[i](&fb);
    }
  }

  displayQCB_shell(callback_data);
}

void add_display_hook_for_shell() {
  int ret;

  tai_module_info_t modinfo;
  modinfo.size = sizeof(modinfo);

  if((ret = taiGetModuleInfo("SceGxm", &modinfo)) < 0) {
    LOG_E("(0x%08X) Unable to obtain *SceGxm* module info", ret);
    return;
  }

  SceKernelModuleInfo sceinfo;
  sceinfo.size = sizeof(sceinfo);

  if((ret = sceKernelGetModuleInfo(modinfo.modid, &sceinfo)) < 0) {
    LOG_E("(0x%08X) Unable to obtain module info for modid %d", ret, modinfo.modid);
    return;
  }

  // data is @ segment 1
  void *ptr = *((void **)(sceinfo.segments[1].vaddr + 0x39c));
  displayQCB = ptr + 0x14;

  // swap callback functions
  displayQCB_shell = *displayQCB;
  *displayQCB = &display_callback;
}

void remove_display_hook_for_shell() {
  *displayQCB = displayQCB_shell;
}

void ctrl_callback(SceCtrlData *ctrl) {
  if(visible) {
    pressed_buttons = ctrl->buttons & ~old_buttons;

    if(pressed_buttons & SCE_CTRL_DOWN) {
      menu_next_item();
    }

    if(pressed_buttons & SCE_CTRL_UP) {
      menu_previous_item();
    }

    if(pressed_buttons & SCE_CTRL_RIGHT) {
      //menu_next_item_value();
    }

    if(pressed_buttons & SCE_CTRL_LEFT) {
      //menu_previous_item_value();
    }

    if(pressed_buttons & SCE_CTRL_CROSS) {
      menu_item_t *i = menu_current_item();

      if(i->type == MENU_SUBMENU && i->submenu != NULL)
        menu_open(i->submenu);
      else if(i->type == MENU_ACTION && i->action != NULL)
        i->action(i->action_param);
    }

    if(pressed_buttons & SCE_CTRL_CIRCLE) {
      menu_back();
    }

    if((ctrl->buttons & SCE_CTRL_START) && (ctrl->buttons & SCE_CTRL_DOWN)) {
      hide_gui();
    }

    old_buttons = ctrl->buttons;
    ctrl->buttons = 0;
  }
  else {
    if((ctrl->buttons & SCE_CTRL_START) && (ctrl->buttons & SCE_CTRL_UP)) {
      visible = 1;
      ctrl->buttons = 0;
    }
  }
}

int ctrl_hook_callback(int port, tai_hook_ref_t ref_hook, SceCtrlData *ctrl, int count) {
  int ret;

  if (ref_hook == 0)
    ret = 1;
  else
  {
    ret = TAI_CONTINUE(int, ref_hook, port, ctrl, count);
    ctrl_callback(ctrl);
  }

  return ret;
}

static tai_hook_ref_t g_display_fb_hook;
static int sceDisplaySetFrameBuf_patched(const SceDisplayFrameBuf *pParam, int sync) {
  if(visible)
    draw_gui(pParam);

  if(pParam->base != NULL) {
    for(int i = 0; i < n_fb_callbacks; i++) {
      fb_callbacks[i](pParam);
    }
  }

  return TAI_CONTINUE(int, g_display_fb_hook, pParam, sync);
}

static tai_hook_ref_t g_ctrl_hook1;
static int sceCtrlPeekBufferPositive_patched(int port, SceCtrlData *ctrl, int count) {
  return ctrl_hook_callback(port, g_ctrl_hook1, ctrl, count);
}

static tai_hook_ref_t g_ctrl_hook2;
static int sceCtrlPeekBufferPositive2_patched(int port, SceCtrlData *ctrl, int count) {
  return ctrl_hook_callback(port, g_ctrl_hook2, ctrl, count);
}

static tai_hook_ref_t g_ctrl_hook3;
static int sceCtrlReadBufferPositive_patched(int port, SceCtrlData *ctrl, int count) {
  return ctrl_hook_callback(port, g_ctrl_hook3, ctrl, count);
}

static tai_hook_ref_t g_ctrl_hook4;
static int sceCtrlReadBufferPositive2_patched(int port, SceCtrlData *ctrl, int count) {
  return ctrl_hook_callback(port, g_ctrl_hook4, ctrl, count);
}

void add_hooks_for_app() {
  g_hooks[0] = taiHookFunctionImport(&g_display_fb_hook,
                                    TAI_MAIN_MODULE,
                                    TAI_ANY_LIBRARY,
                                    0x7A410B64, // sceDisplaySetFrameBuf
                                    sceDisplaySetFrameBuf_patched);

  g_hooks[1] = taiHookFunctionImport(&g_ctrl_hook1,
                                    TAI_MAIN_MODULE,
                                    TAI_ANY_LIBRARY,
                                    0xA9C3CED6, // sceCtrlPeekBufferPositive
                                    sceCtrlPeekBufferPositive_patched);

  g_hooks[2] = taiHookFunctionImport(&g_ctrl_hook2,
                                    TAI_MAIN_MODULE,
                                    TAI_ANY_LIBRARY,
                                    0x15F81E8C, // sceCtrlPeekBufferPositive2
                                    sceCtrlPeekBufferPositive2_patched);

  g_hooks[3] = taiHookFunctionImport(&g_ctrl_hook3,
                                    TAI_MAIN_MODULE,
                                    TAI_ANY_LIBRARY,
                                    0x67E7AB83, // sceCtrlReadBufferPositive
                                    sceCtrlReadBufferPositive_patched);

  g_hooks[4] = taiHookFunctionImport(&g_ctrl_hook4,
                                    TAI_MAIN_MODULE,
                                    TAI_ANY_LIBRARY,
                                    0xC4226A3E, // sceCtrlReadBufferPositive2
                                    sceCtrlReadBufferPositive2_patched);
}

int ctrl_peek_thread(SceSize args_size, void *args) {
  SceCtrlData ctrl;

  while (1) {
    sceCtrlPeekBufferPositive(0, &ctrl, 1);
    ctrl_callback(&ctrl);
    tshSetButtonIntercept(visible);
    sceKernelDelayThread(50);
  }
  return sceKernelDeleteThread(0);
}

static SceUID thid = -1;
SceUID create_ctrl_peek_thread() {
  thid = sceKernelCreateThread("ctrl_peek_thread", (SceKernelThreadEntry)ctrl_peek_thread, 0xBF, 0x4000, 0, 0, NULL);
  if (thid >= 0)
    sceKernelStartThread(thid, 0, NULL);

  return thid;
}

void enable_plugin(int plugin_id) {
  LOG_D("Enabling plugin 0x%X", plugin_id);
  tshSetPluginState(plugin_id, true);
  menu_clear_cache();
}

void disable_plugin(int plugin_id) {
  LOG_D("Disabling plugin 0x%X", plugin_id);
  tshSetPluginState(plugin_id, false);
  menu_clear_cache();
}

int generate_plugin_menu(int plugin_id, menu_item_t **i) {
  static menu_item_t items[16];
  static tsh_plugin_t plugin;

  if(tshGetPlugin(plugin_id, &plugin) < 0) {
    LOG_D("Plugin 0x%X not found", plugin_id);
    return 0;
  }

  items[0].type = MENU_ACTION;
  items[0].action_param = plugin_id;

  if(plugin.enabled) {
    items[0].action = &disable_plugin;
    sceClibStrncpy(items[0].name, "Disable", MAX_ITEM_NAME);
  } else {
    items[0].action = &enable_plugin;
    sceClibStrncpy(items[0].name, "Enable", MAX_ITEM_NAME);
  }

  *i = items;
  return 1;
}

void build_plugins_menu() {
  tsh_plugin_t *plugins;

  int n_plugins = tshGetNumPlugins();
  LOG_D("Num. plugins: %d", n_plugins);

  if(n_plugins <= 0)
    return;

  plugins = (tsh_plugin_t *)tshMemAlloc(
    SCE_KERNEL_MEMBLOCK_TYPE_USER_RW,
    ALIGN(n_plugins * sizeof(tsh_plugin_t), 0xfff)
  );
  if(!plugins)
    return;

  tshGetPluginList(plugins);

  for(int i = 0; i < n_plugins; i++) {
    menu_add_plugin(plugins[i].id, plugins[i].name);
  }
  tshMemFree(plugins);
}

void _start() __attribute__ ((weak, alias ("module_start")));
int module_start(SceSize argc, const void *args) {
  is_shell = tshIsShell();

  if(is_shell)
    LOG_D("GUI loaded (shell)");
  else
    LOG_D("GUI loaded");

  if(is_shell) {
    add_display_hook_for_shell();
    create_ctrl_peek_thread();
  }
  else {
    add_hooks_for_app();
  }
  build_plugins_menu();

  return SCE_KERNEL_START_SUCCESS;
}

int module_stop(SceSize argc, const void *args) {
  LOG_D("Unloading GUI");

  if(is_shell) {
    remove_display_hook_for_shell();
    // terminate thread, with a flag?
  }
  else {
    taiHookRelease(g_hooks[0], g_display_fb_hook);
    taiHookRelease(g_hooks[1], g_ctrl_hook1);
    taiHookRelease(g_hooks[2], g_ctrl_hook2);
    taiHookRelease(g_hooks[3], g_ctrl_hook3);
    taiHookRelease(g_hooks[4], g_ctrl_hook4);
  }
  return SCE_KERNEL_STOP_SUCCESS;
}
