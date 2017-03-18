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

#include <string.h>
#include <stdbool.h>
#include <psp2kern/io/fcntl.h>

#include "registry.h"
#include "list.h"

#include "taishell/logger.h"

tsh_reg_header_t header = {
  .magic   = TSR_MAGIC_NUMBER,
  .version = TSR_FILE_VERSION
};

static inline int read_header(SceUID fd, tsh_reg_header_t *hdr) {
  return ksceIoRead(fd, &hdr, sizeof(tsh_reg_header_t));
}

static inline int write_header(SceUID fd) {
  return ksceIoWrite(fd, &header, sizeof(tsh_reg_header_t));
}

static inline int read_entry(SceUID fd, tsh_reg_entry_t *entry) {
  return ksceIoRead(fd, entry, sizeof(tsh_reg_entry_t));
}

static inline int write_entry(SceUID fd, tsh_reg_entry_t *entry) {
  return ksceIoWrite(fd, entry, sizeof(tsh_reg_entry_t));
}

int registry_get_list(const char *fp, tsh_list_t *list) {
  SceUID fd;
  tsh_reg_header_t hdr;
  int rb;

  list->data_size = sizeof(tsh_reg_entry_t);

  if((fd = ksceIoOpen(fp, SCE_O_RDONLY, 0)) < 0) {
    LOG_E("(0x%08X) Unable to open registry file (RO): %s", fd, fp);
    return fd;
  }

  if(read_header(fd, &hdr) != sizeof(tsh_reg_header_t)) {
    LOG_E("Unable to read registry header. Invalid size");
    goto err;
  }

  tsh_list_node_t *node;
  tsh_reg_entry_t entry;

  while((rb = read_entry(fd, &entry)) > 0) {
    if((node = list_push(list, list->n)) == NULL) {
      LOG_E("Unable to allocate list node for registry");
      goto err2;
    }
    memcpy(node->data, &entry, sizeof(tsh_reg_entry_t));
  }

  ksceIoClose(fd);
  return 0;

  err:
    ksceIoClose(fd);
    return -1;
  err2:
    list_clear(list);
    ksceIoClose(fd);
    return -1;
}

int registry_set(const char *fp, const char *key, int value) {
  SceUID fd;
  int rb;

  tsh_reg_header_t hdr;

  if((fd = ksceIoOpen(fp, SCE_O_RDWR|SCE_O_CREAT, 0777)) < 0) {
    LOG_E("(0x%08X) Unable to open registry file (RW): %s", fd, fp);
    return -1;
  }

  if((rb = read_header(fd, &hdr)) != sizeof(tsh_reg_header_t)) {
    if(rb == 0) { // empty file
      write_header(fd);
    } else { // corrupt file?
      LOG_E("Unable to read registry header. Invalid size");
      goto err;
    }
  }

  SceOff off = sizeof(tsh_reg_header_t);

  tsh_reg_entry_t entry;
  bool found = false;

  // find existing key
  while((rb = read_entry(fd, &entry)) > 0) {
    if(strncmp(key, entry.key, REGISTRY_MAX_KEY) == 0) {
      ksceIoLseek(fd, off, SCE_SEEK_SET);
      found = true;
      break;
    }
    off += rb;
  }

  if(!found) {
    strncpy(entry.key, key, REGISTRY_MAX_KEY-1);
  }
  entry.value = value;

  write_entry(fd, &entry);
  ksceIoClose(fd);

  return 0;

  err:
    ksceIoClose(fd);
    return -1;
}

int registry_store_list(const char *fp, tsh_list_t *list) {
  SceUID fd;

  if((fd = ksceIoOpen(fp, SCE_O_WRONLY|SCE_O_CREAT|SCE_O_TRUNC, 0777)) < 0) {
    LOG_E("(0x%08X) Unable to open registry file (RW): %s", fd, fp);
    return fd;
  }

  write_header(fd);
  //ksceIoLseek(fd, sizeof(tsh_reg_header_t), SCE_SEEK_SET);

  tsh_list_node_t *node = list->head;

  if(node == NULL) {
    LOG_W("Got an empty list! ¬¬'");
    goto err;
  }

  do {
    write_entry(fd, (tsh_reg_entry_t *)node->data);
    //ksceIoLseek(fd, sizeof(tsh_reg_entry_t), SCE_SEEK_CUR);
  } while((node = node->next) != NULL);

  ksceIoClose(fd);

  return 0;

  err:
    ksceIoClose(fd);
    return -1;
}