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

#ifndef __MENU_H__
#define __MENU_H__

#include <psp2/types.h>

#define MAX_MENU_NAME 32
#define MAX_ITEM_NAME 32

#define STRINGIZE(v) #v

#define DEF_SUBTITLE(_name_) \
  { \
    .name = STRINGIZE(_name_), \
    .type = MENU_SUBTITLE \
  } \

#define DEF_SUBMENU(_name_, _submenu_) \
  { \
    .name    = STRINGIZE(_name_), \
    .type    = MENU_SUBMENU, \
    .submenu = _submenu_ \
  } \

#define DEF_ACTION(_name_, _func_) \
  { \
    .name   = STRINGIZE(_name_), \
    .type   = MENU_ACTION, \
    .action = _func_ \
  }

typedef enum {
  MENU_STATIC,
  MENU_DYNAMIC
} menu_type_t;

typedef enum {
  MENU_SUBTITLE,
  MENU_SUBMENU,
  MENU_ACTION
} menu_item_type_t;

struct menu_s;
struct menu_item_s;

typedef void (*menu_action_t)(int param);
typedef int (*menu_gen_t)(int param, struct menu_item_s **items);

typedef struct menu_s {
  char name[MAX_MENU_NAME];
  menu_type_t type;
  struct menu_s *parent;

  union {
    struct menu_item_s *items;
    menu_gen_t generator;
  };
  union {
    uint16_t n_items;
    int generator_param;
  };
} menu_t;

typedef struct menu_item_s {
  char name[MAX_ITEM_NAME];
  menu_item_type_t type;

  union {
    menu_action_t action;
    menu_t *submenu;
  };
  int action_param;
} menu_item_t;

void menu_add_plugin(int plugin_id, const char *name);
menu_item_t* menu_current_item();
void menu_draw();
void menu_next_item();
void menu_previous_item();
void menu_reset();
void menu_open(menu_t *m);
void menu_back();
void menu_clear_cache();


#endif