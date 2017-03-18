#ifndef __TAISHELL_PLUGIN_H__
#define __TAISHELL_PLUGIN_H__

#include <stdbool.h>

#define MAX_PLUGIN_NAME 32

#ifndef MAX_PATH
#define MAX_PATH 128
#endif

#define TSH_KERNEL 0x1000 /* Plugin running on kernel space */
#define TSH_USER   0x2000 /* Plugin running on userland */

#define TSH_SHELL   0x100 /* User mode plugin loaded on the shell */
#define TSH_APP     0x200 /* User mode plugin loaded on every app that is launched */
#define TSH_SYSTEM  0x300 /* Kernel plugin or user plugin that loaded on the shell and every app that is launched */

typedef struct {
  uint32_t      id;                    /* Plugin unique ID. For now it's the crc hash of the name */
  char          name[MAX_PLUGIN_NAME]; /* Plugin name */
  uint16_t      version;               /* Plugin version. Hi byte being major, low byte being minor */
  uint16_t      flags;                 /* Plugin flags */
  char          path[MAX_PATH];        /* Plugin module path */
  bool          enabled;               /* */
} tsh_plugin_t;

int tshGetNumPlugins();
int tshGetPluginList(tsh_plugin_t *plugins);
int tshSetPluginState(uint32_t plugin_id, bool state);
int tshGetPlugin(uint32_t plugin_id, tsh_plugin_t *ptr);

#endif