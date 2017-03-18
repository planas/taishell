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

#include <stdbool.h>
#include <string.h>

#include <psp2kern/kernel/sysmem.h>

#include "taishell/logger.h"
#include "list.h"
#include "utils.h"

static tsh_list_node_t *alloc_node(int key, size_t data_size) {
  void *addr;
  int ret;

  size_t size = sizeof(tsh_list_node_t) + data_size;

  SceUID block = ksceKernelAllocMemBlock(
    "taishell",
    SCE_KERNEL_MEMBLOCK_TYPE_KERNEL_RW,
    ALIGN(size, 0xfff),
    NULL
  );

  if(block < 0) {
    LOG_E("(0x%08X) Unable to allocate memory block for list node", block);
    return NULL;
  }

  if((ret = ksceKernelGetMemBlockBase(block, &addr)) < 0) {
    LOG_E("(0x%08X) Unable to obtain memory block address for list node", ret);
    ksceKernelFreeMemBlock(block);
    return NULL;
  }
  memset(addr, 0, size);

  tsh_list_node_t *node = (tsh_list_node_t *)addr;
  node->key = key;
  node->mem_uid = block;

  // Node data is stored after the node struct
  if(data_size > 0)
    node->data = addr + sizeof(tsh_list_node_t);

  return node;
}

static inline void free_node(tsh_list_node_t *node) {
  ksceKernelFreeMemBlock(node->mem_uid);
}

void list_clear(tsh_list_t *list) {
  tsh_list_node_t *node = list->head;

  if(node != NULL) {
    do {
      free_node(node);
    } while((node = node->next) != NULL);

    list->head = NULL;
    list->tail = NULL;
  }
}

tsh_list_node_t *list_find(tsh_list_t *list, int key) {
  tsh_list_node_t *node = list->head;

  if(node != NULL) {
    do {
      if(node->key == key)
        return node;
    } while((node = node->next) != NULL);
  }
  return NULL;
}

tsh_list_node_t *list_push(tsh_list_t *list, int key) {
  tsh_list_node_t *node;

  if((node = alloc_node(key, list->data_size)) == NULL)
    return NULL;

  if(list->head == NULL) {
    list->head = node;
  } else {
    node->prev = list->tail;
    list->tail->next = node;
  }
  list->tail = node;

  (list->n)++;

  return node;
}

tsh_list_node_t * list_remove(tsh_list_t *list, int key) {
  tsh_list_node_t *node, *next;

  if((node = list_find(list, key)) == NULL)
    return NULL;

  next = node->next;

  if(node == list->head) {
    list->head = node->next;
  } else if(node->prev) {
    node->prev->next = node->next;
  }

  if(node == list->tail) {
    list->tail = node->prev;
  } else if(node->next) {
    node->next->prev = node->prev;
  }

  free_node(node);
  (list->n)--;

  return next;
}
