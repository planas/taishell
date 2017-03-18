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

#include <psp2/kernel/clib.h>
#include <psp2/ctrl.h>

#include "taishell/logger.h"

#include "menu.h"
#include "actions.h"
#include "blit.h"

#define MENU_SY           100
#define TITLE_VPADDING    45
#define SUBTITLE_VPADDING 35
#define ITEM_VPADDING     25
#define BOX_PADDING       20

#define BOX_COLOR           0x44000000
#define TITLE_COLOR         COLOR_YELLOW
#define SUBTITLE_COLOR      COLOR_CYAN
#define ITEM_COLOR          COLOR_WHITE
#define ITEM_SELECTED_COLOR COLOR_MAGENTA

#define MAX_PLUGINS 16

static menu_t root_menu;

static menu_item_t plugins_menu_items[MAX_PLUGINS];
static menu_t plugins_submenus[MAX_PLUGINS];
static menu_t plugins_menu = {
  .name    = "Plugins",
  .type    = MENU_STATIC,
  .parent  = &root_menu,
  .items   = plugins_menu_items,
  .n_items = 0
};

static menu_item_t system_settings_items[] = {};
static menu_t system_settings_menu = {
  .name    = "System",
  .type    = MENU_STATIC,
  .parent  = &root_menu,
  .items   = system_settings_items,
  .n_items = sizeof(system_settings_items)/sizeof(menu_item_t)
};

static menu_item_t henkaku_settings_items[] = {};
static menu_t henkaku_settings_menu = {
  .name    = "Henkaku",
  .type    = MENU_STATIC,
  .parent  = &root_menu,
  .items   = henkaku_settings_items,
  .n_items = sizeof(henkaku_settings_items)/sizeof(menu_item_t)
};

static menu_item_t taishell_settings_items[] = {};
static menu_t taishell_settings_menu = {
  .name    = "TaiShell",
  .type    = MENU_STATIC,
  .parent  = &root_menu,
  .items   = taishell_settings_items,
  .n_items = sizeof(taishell_settings_items)/sizeof(menu_item_t)
};

static menu_item_t root_menu_items[] = {
  DEF_SUBMENU(Plugins, &plugins_menu),
  DEF_SUBTITLE(Settings),
  DEF_SUBMENU(System, &system_settings_menu),
  DEF_SUBMENU(Henkaku, &henkaku_settings_menu),
  DEF_SUBMENU(TaiShell, &taishell_settings_menu),
  DEF_SUBTITLE(Power),
  DEF_ACTION(Reboot, &vita_reboot),
  DEF_ACTION(Suspend, &vita_suspend),
  DEF_ACTION(Shutdown, &vita_shutdown)
};
static menu_t root_menu = {
  .name    = "TaiShell",
  .type    = MENU_STATIC,
  .items   = root_menu_items,
  .n_items = sizeof(root_menu_items)/sizeof(menu_item_t)
};

static uint16_t current_item_idx;
static menu_t *current_menu = &root_menu;

static menu_item_t *cached_items;
static int n_cached_items;

/* Defined @ gui.c */
int generate_plugin_menu(int attr, menu_item_t **i);

inline menu_item_t* menu_current_item() {
  if(current_menu->type == MENU_DYNAMIC)
    return &cached_items[current_item_idx];
  else
    return &current_menu->items[current_item_idx];

}

void menu_clear_cache() {
  n_cached_items = 0;
  cached_items = NULL;
}

void menu_add_plugin(int plugin_id, const char *name) {
  if(plugins_menu.n_items + 1 >= MAX_PLUGINS)
    return;

  menu_t *menu = &plugins_submenus[plugins_menu.n_items];
  menu_item_t *item = &plugins_menu_items[plugins_menu.n_items];

  menu->type = MENU_DYNAMIC;
  menu->parent = &plugins_menu;
  menu->generator = &generate_plugin_menu;
  menu->generator_param = plugin_id;
  sceClibStrncpy(menu->name, name, MAX_MENU_NAME);

  item->type = MENU_SUBMENU;
  item->submenu = menu;
  sceClibStrncpy(item->name, name, MAX_ITEM_NAME);

  plugins_menu.n_items++;
}

void menu_next_item() {
  int n_items;

  if(current_menu->type == MENU_DYNAMIC)
    n_items = n_cached_items;
  else
    n_items = current_menu->n_items;

  if(current_item_idx < n_items - 1)
    current_item_idx++;
  else
    current_item_idx = 0;

  if(menu_current_item()->type == MENU_SUBTITLE)
    menu_next_item();
}

void menu_previous_item() {
  int n_items;

  if(current_menu->type == MENU_DYNAMIC)
    n_items = n_cached_items;
  else
    n_items = current_menu->n_items;

  if(current_item_idx > 0)
    current_item_idx--;
  else
    current_item_idx = n_items - 1;

  if(menu_current_item()->type == MENU_SUBTITLE)
    menu_previous_item();
}

void menu_reset() {
  current_menu = &root_menu;
  current_item_idx = 0;
  menu_clear_cache();
}

void menu_open(menu_t *m) {
  current_menu = m;
  current_item_idx = 0;
  menu_clear_cache();
}

void menu_back() {
  if(current_menu->parent) {
    menu_open(current_menu->parent);
    menu_clear_cache();
  }
}

void menu_draw() {
  menu_t *m = current_menu;
  menu_item_t *items = NULL;
  int n_items = 0;

  int y = MENU_SY;

  if(m->type == MENU_STATIC) {
    items   = m->items;
    n_items = m->n_items;
  } else if(m->type == MENU_DYNAMIC) {
    if(cached_items != NULL) {
      items   = cached_items;
      n_items = n_cached_items;
    } else {
      n_items = m->generator(m->generator_param, &items);
      cached_items = items;
      n_cached_items = n_items;
    }
  }

  int box_height = (n_items*(16 + ITEM_VPADDING/2)) + (TITLE_VPADDING + 16) + (BOX_PADDING * 2);
  blit_square(BOX_COLOR, CENTER_SQ(250), y - BOX_PADDING, 250, box_height);

  // title
  blit_set_color(TITLE_COLOR, TRANSPARENT);
  blit_stringf(CENTER_TEXT(sceClibStrnlen(m->name, 32)), y, m->name);
  y += TITLE_VPADDING;

  if(n_items <= 0 || items == NULL) {
    return;
  }

  for(int i = 0; i < n_items; i++) {
    menu_item_t *item = &items[i];
    uint32_t color;

    if(item->type == MENU_SUBTITLE) {
      color = SUBTITLE_COLOR;
      y+= 10;
    }
    else {
      if(i == current_item_idx) {
        color = ITEM_SELECTED_COLOR;
      }
      else {
        color = ITEM_COLOR;
      }
    }

    blit_set_color(color, TRANSPARENT);
    blit_stringf(CENTER_TEXT(sceClibStrnlen(item->name, 32)), y, item->name);

    y += ITEM_VPADDING;
  }
}
