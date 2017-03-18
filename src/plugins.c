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

#include <psp2kern/io/fcntl.h>
#include <psp2kern/kernel/sysmem.h>
#include <psp2kern/kernel/modulemgr.h>
#include <psp2kern/kernel/threadmgr.h>
#include <psp2kern/kernel/sysmem.h>
#include <psp2kern/kernel/cpu.h>

#include <taihen.h>

#include "psp2kern/io/dirent.h"

#include "taishell/tai_file.h"
#include "taishell/logger.h"

#include "plugins.h"
#include "utils.h"
#include "exports.h"
#include "list.h"
#include "registry.h"

#define DIR_MAX_DEPTH 2
#define PLUGINS_REG_FP PATH("settings/plugins.tsr")

extern SceUID shell_pid;

tsh_list_t plugins;
tsh_list_t instances;

void create_instance(tsh_plugin_t *plugin, SceUID pid, SceUID mod_uid) {
  tsh_list_node_t *pid_node;

  if((pid_node = list_find(&instances, pid)) == NULL) {
    LOG_D("No instance for PID 0x%04X exists. Creating PID list", pid);

    if((pid_node = list_push(&instances, pid)) == NULL) {
      LOG_E("Unable to allocate node for PID");
      return;
    }
  }

  tsh_list_node_t *instance_node = list_push((tsh_list_t *)pid_node->data, (int)plugin->id);
  if(instance_node == NULL) {
    LOG_E("Unable to allocate node for instance");
    return;
  }
  instance_node->value = mod_uid;
}

int load_plugin(tsh_plugin_t *plugin, SceUID pid, bool queue) {
  SceUID uid;

  if(queue)
    uid = ksceKernelLoadModuleForPid(pid, plugin->path, 0x8000, NULL);
  else
    uid = ksceKernelLoadStartModuleForPid(pid, plugin->path, 0, NULL, 0, NULL, NULL);

  if(uid < 0) {
    LOG_E("(0x%08X) Failed to load module for plugin 0x%08lX. PID: 0x%08X", uid, plugin->id, pid);
    return uid;
  }
  create_instance(plugin, pid, uid);

  return 0;
}

void queue_plugins_for_pid(SceUID pid) {
  tsh_plugin_t *plugin;
  tsh_list_node_t *node = plugins.head;

  if(node == NULL) {
    LOG_D("Nothing to load. Plugins list is empty");
    return;
  }

  do {
    plugin = (tsh_plugin_t *)node->data;

    if((plugin->flags & TSH_USER) &&
       (plugin->flags & TSH_APP) &&
       plugin->enabled) {
      LOG_D("Loading plugin 0x%lX", plugin->id);
      load_plugin(plugin, pid, true);
    }
  } while((node = node->next) != NULL);
}

void load_initial_plugins() {
  tsh_plugin_t *plugin;
  tsh_list_node_t *node = plugins.head;

  if(node == NULL) {
    LOG_D("Nothing to load. Plugins list is empty");
    return;
  }

  do {
    plugin = (tsh_plugin_t *)node->data;

    if(!plugin->enabled)
      continue;

    if(plugin->flags & TSH_KERNEL) {
      load_plugin(plugin, KERNEL_PID, false);
    } else if(plugin->flags & TSH_SHELL) {
      load_plugin(plugin, shell_pid, false);
    }
  } while((node = node->next) != NULL);
}


tsh_plugin_t * load_tai(const char *path, const char *fn) {
  char fp[MAX_PATH];
  tai_file_t tai;
  SceUID fd;
  int rb;

  build_path(path, fn, fp);
  if((fd = ksceIoOpen(fp, SCE_O_RDONLY, 0)) < 0)
    return NULL;

  rb = ksceIoRead(fd, &tai, sizeof(tai_file_t));
  if(rb != sizeof(tai_file_t)) {
    LOG_E("Invalid TAI file");
    return NULL;
  }

  ksceIoClose(fd);

  if(strncmp(tai.magic, TAI_MAGIC_NUMBER, 3)) {
    LOG_E("Not a TAI file!");
    return NULL;
  }

  LOG_D("Tai file version: 0x%x", tai.version);
  LOG_D("Plugin name: %s", tai.plugin.name);
  LOG_D("Plugin version: %d", tai.plugin.version);
  LOG_D("Plugin flags: 0x%04X", tai.plugin.flags);

  // Build module expected path
  snprintf(fp, MAX_PATH, "%s/%.*s.%s",
    path,
    strchr(fn, '.')-fn,
    fn,
    tai.plugin.flags & TSH_KERNEL ? "skprx" : "suprx"
  );

  tsh_list_node_t *node = list_push(&plugins, tai.plugin.id);

  if(node == NULL) {
    LOG_D("Unable to allocate node for plugin");
    return NULL;
  }
  tsh_plugin_t *plugin = (tsh_plugin_t *)node->data;

  plugin->id      = tai.plugin.id;
  plugin->flags   = tai.plugin.flags;
  plugin->version = tai.plugin.version;
  strncpy(plugin->name, tai.plugin.name, MAX_PLUGIN_NAME-1);
  strncpy(plugin->path, fp, MAX_PATH-1);

  return plugin;
}

void find_plugins(const char *path, int level) {
  char *dotp, subdir[MAX_PATH];
  SceIoDirent dir;
  SceUID fd;

  if((fd = ksceIoDopen(path)) < 0)
    return;

  while(ksceIoDread(fd, &dir) > 0) {
    if((dir.d_stat.st_mode & 0x1000) && level < DIR_MAX_DEPTH) { // directory
      find_plugins(build_path(path, dir.d_name, subdir), level+1);
    }
    else if(dir.d_stat.st_mode & 0x2000) {  // regular file
      if(!(dotp = strrchr(dir.d_name, '.')))
        continue;

      if(strncmp(dotp+1, "tai", 3) == 0) {
        load_tai(path, dir.d_name);
      }
    }
  }
  ksceIoDclose(fd);
}

void unload_plugin(uint32_t plugin_id) {
  tsh_list_node_t *pid_list = instances.head;
  tsh_list_node_t *instance;
  int ret;

  if(pid_list == NULL) {
    return;
  }

  do {
    tsh_list_t *plugin_list = (tsh_list_t *)pid_list->data;
    instance = list_find(plugin_list, (int)plugin_id);

    if(instance != NULL) {
      ret = ksceKernelStopUnloadModuleForPid(pid_list->key, instance->value, 0, NULL, 0, NULL, NULL);
      LOG_D("Unloading modid 0x%X for PID 0x%X. Result: 0x%08X", instance->value, pid_list->key, ret);
      list_remove(plugin_list, instance->key);
    }
  } while((pid_list = pid_list->next) != NULL);
}

/*
Not useful right now.
Stopping a plugin explicitly on the current hook is problematic (at least)
*/
void unload_plugins_for_pid(SceUID pid) {
  tsh_list_node_t *pid_node;
  tsh_list_t *instance_list;
  tsh_list_node_t *instance;

  if((pid_node = list_find(&instances, pid)) == NULL) {
    LOG_D("No instance for PID 0x%04X exists. Nothing to unload", pid);
    return;
  }

  instance_list = (tsh_list_t *)pid_node->data;
  instance = instance_list->head;

  if(instance != NULL) {
    do {
      int ret = ksceKernelStopModuleForPid(pid, instance->value, 0, NULL, 0, NULL, NULL);
      LOG_D("Stopping modid 0x%X for PID 0x%X Result: 0x%08X", instance->value, pid, ret);
    } while((instance = list_remove(instance_list, instance->key)) != NULL);
  }
  list_remove(&instances, pid);
}

void init_plugins() {
  // Initialize lists
  plugins.data_size = sizeof(tsh_plugin_t);
  instances.data_size = sizeof(tsh_list_t);

  // Find tai files on plugins_dir
  find_plugins(PATH(PLUGINS_DIR), 1);

  // Load registry of plugins state
  tsh_list_t pstate;
  memset(&pstate, 0, sizeof(tsh_list_t));

  registry_get_list(PLUGINS_REG_FP, &pstate);

  tsh_plugin_t *plugin;
  tsh_list_node_t *node = plugins.head;
  bool registry_mod = false;

  // Traverse the plugin list matching
  // each plugin with its known state
  if(node != NULL) {
    do {
      plugin = (tsh_plugin_t *)node->data;

      bool entry_exists = false;
      tsh_list_node_t *snode = pstate.head;
      tsh_reg_entry_t *entry;

      char key[9];
      snprintf(key, 9, "%lX", plugin->id);

      if(snode != NULL) {
        do {
          entry = (tsh_reg_entry_t *)snode->data;
          if(strncmp(entry->key, key, 8) == 0) {
            entry_exists = true;
            break;
          }
        } while((snode = snode->next) != NULL);
      }

      if(entry_exists) {
        plugin->enabled = (bool)entry->value;
      } else {
        LOG_D("Entry for plugin 0x%08lX not found", plugin->id);

        if((snode = list_push(&pstate, pstate.n)) != NULL) {
          entry = (tsh_reg_entry_t *)snode->data;

          strncpy(entry->key, key, REGISTRY_MAX_KEY-1);
          entry->value = 0;

          registry_mod = true;
        }
      }
    } while((node = node->next) != NULL);
  }

  // store the changes if registry list was modified
  if(registry_mod) {
    LOG_D("Storing plugin registry changes");
    registry_store_list(PLUGINS_REG_FP, &pstate);
  }

  LOG_I("Plugin initialization done");
}

int tshGetPlugin(uint32_t plugin_id, tsh_plugin_t *ptr) {
  tsh_list_node_t *node = list_find(&plugins, plugin_id);

  if(node) {
    ksceKernelMemcpyKernelToUser((uintptr_t)ptr, node->data, sizeof(tsh_plugin_t));
    return 0;
  }

  return -1;
}

int tshGetNumPlugins() {
  return plugins.n;
}

int tshGetPluginList(tsh_plugin_t *buff) {
  tsh_list_node_t *node = plugins.head;
  int i = 0;

  if(!node) return -1;

  do {
    ksceKernelMemcpyKernelToUser((uintptr_t)&buff[i++], node->data, sizeof(tsh_plugin_t));
  } while((node = node->next) != NULL);

  return 0;
}

int ktshSetPluginState(uint32_t plugin_id, bool state) {
  tsh_list_node_t *node;
  tsh_plugin_t *plugin;
  int ret = 0;

  if((node = list_find(&plugins, (int)plugin_id)) == NULL) {
    LOG_D("Plugin 0x%lX not found", plugin_id);
    return -1;
  }

  plugin = (tsh_plugin_t *)node->data;

  if(state) {
    if(plugin->flags & TSH_KERNEL) {
      ret = load_plugin(plugin, KERNEL_PID, false);
    } else if(plugin->flags & TSH_SHELL) {
      ret = load_plugin(plugin, shell_pid, false);
    }
  } else {
    unload_plugin(plugin->id);
  }

  if(!state || (state && ret >= 0)) {
    char reg_key[9];
    snprintf(reg_key, 9, "%lX", plugin_id);

    registry_set(PLUGINS_REG_FP, reg_key, state);
    plugin->enabled = state;
  }

  return ret;
}

int tshSetPluginState(uint32_t plugin_id, bool state) {
  uint32_t s;
  int ret;

  ENTER_SYSCALL(s);
  ret = ktshSetPluginState(plugin_id, state);
  EXIT_SYSCALL(s);

  return ret;
}