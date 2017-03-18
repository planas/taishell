#ifndef __TAISHELL_TAI_H__
#define __TAISHELL_TAI_H__

#include "taishell/plugin.h"

#define TAI_MAGIC_NUMBER "TAI\0"
#define TAI_FILE_VERSION 0x0001

typedef struct {
  char magic[4];
  uint16_t version;
  struct plugin_s {
    uint32_t id;
    uint16_t version;
    uint16_t flags;
    char name[MAX_PLUGIN_NAME];
  } plugin;
} tai_file_t;

#endif
