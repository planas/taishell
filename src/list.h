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

#ifndef __LIST_H__
#define __LIST_H__

#include <psp2kern/types.h>

struct tsh_list_node_s {
  struct tsh_list_node_s *prev;
  struct tsh_list_node_s *next;

  int key;
  union {
    void *data;
    int value;
  };
  SceUID mem_uid;
};

typedef struct tsh_list_node_s tsh_list_node_t;

typedef struct {
  tsh_list_node_t *head;
  tsh_list_node_t *tail;
  size_t data_size;
  int n;
} tsh_list_t;

tsh_list_node_t * list_find(tsh_list_t *list, int key);
tsh_list_node_t * list_push(tsh_list_t *list, int key);
tsh_list_node_t * list_remove(tsh_list_t *list, int key);
void list_clear(tsh_list_t *list);

#endif