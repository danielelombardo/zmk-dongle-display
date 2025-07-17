/*
 * Copyright (c) 2024 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);
 
#include <dt-bindings/zmk/modifiers.h>
#include <zephyr/bluetooth/services/bas.h>
#include <zephyr/kernel.h>
#include <zmk/battery.h>
#include <zmk/ble.h>
#include <zmk/display.h>
#include <zmk/event_manager.h>
#include <zmk/events/battery_state_changed.h>
#include <zmk/events/endpoint_changed.h>
#include <zmk/events/layer_state_changed.h>
#include <zmk/keymap.h>
#include <zmk/events/keycode_state_changed.h>
#include <zmk/hid.h>

#include "custom_screen.h"
#include "assets/space_mono_14.c"
#include "assets/fonts.h"

static sys_slist_t widgets = SYS_SLIST_STATIC_INIT(&widgets);

struct status_state {
    char batteries[ZMK_SPLIT_BLE_PERIPHERAL_COUNT][9];
    char top_layer[12] = "\0";
    char active_mods[44] = "\0";
};

struct zmk_widget_screen {
  sys_snode_t node;
  lv_obj_t *obj;
  lv_color_t cbuf[CANVAS_HEIGHT * CANVAS_HEIGHT];
  struct status_state state;
};


// draw canvas

static void draw_canvas(lv_obj_t *widget, lv_color_t cbuf[], const struct status_state *state) {
    lv_obj_t *canvas = lv_obj_get_child(widget, 0);

    // draw background
    lv_draw_rect_dsc_t background_dsc;
    lv_draw_rect_dsc_init(&background_dsc);
    background_dsc.bg_color = LVGL_BACKGROUND;
    lv_canvas_draw_rect(canvas, 0, 0, CANVAS_WIDTH, CANVAS_HEIGHT, &background_dsc);
    
    // draw top layer


    // draw batteries
    lv_draw_label_dsc_t batteries_label;
    lv_draw_label_dsc_init(&batteries_label);
    batteries_label.color = LVGL_FOREGROUND;
    batteries_label.font = &space_mono_14;
    batteries_label.align = LV_TEXT_ALIGN_LEFT;
    
    char batteries_text[ZMK_SPLIT_BLE_PERIPHERAL_COUNT * 9] = {};
    for (int i = 0; i < ZMK_SPLIT_BLE_PERIPHERAL_COUNT; i++) {
        strcat(batteries_text, state->batteries[i]);
    }
    lv_canvas_draw_text(canvas, 0, 19, 25, &batteries_label, &batteries_text);

    // draw modifiers


    // Rotate for horizontal display
    rotate_canvas(canvas, cbuf);
}


// battery
struct battery {
    uint8_t source;
    uint8_t level;
};

static void set_battery(struct zmk_widget_screen *widget, struct battery state) {
    sprintf(widget->state.batteries[state.source], "%4u %% ", state.level);
    draw_canvas(widget->obj, widget->cbuf, &widget->state);
}

static void battery_update_cb(struct battery state) {
    struct zmk_widget_screen *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) { set_battery(widget, state); }
}

static struct battery battery_get_state(const zmk_event_t *eh) {
    const struct zmk_peripheral_battery_state_changed *ev = as_zmk_peripheral_battery_state_changed(eh);
    return (struct battery){
        .source = ev->source,
        .level = ev->state_of_charge,
    };
}

ZMK_DISPLAY_WIDGET_LISTENER(widget_battery, struct battery, battery_update_cb, battery_get_state)
ZMK_SUBSCRIPTION(widget_battery, zmk_peripheral_battery_state_changed);


// Layer
struct layer {
    uint8_t index;
    const char *label;
};

static void set_layer(struct zmk_widget_screen *widget, struct layer state) {
    if (state.label == NULL) {
        sprintf(widget->state.top_layer, "%i", state.index);
    } else {
        sprintf(widget->state.top_layer, "%s", state.label);
    }
    draw_canvas(widget->obj, widget->cbuf, &widget->state);
}

static void layer_update_cb(struct layer state) {
    struct zmk_widget_screen *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) { set_layer(widget, state); }
}

static struct layer layer_get_state(const zmk_event_t *eh) {
    uint8_t index = zmk_keymap_highest_layer_active();
    return (struct layer){
        .index = index, 
        .label = zmk_keymap_layer_name(index)
    };
}

ZMK_DISPLAY_WIDGET_LISTENER(widget_layer, struct layer, layer_update_cb, layer_get_state)
ZMK_SUBSCRIPTION(widget_layer, zmk_layer_state_changed);


// Modifiers
uint8_t modifiers

static void set_modifiers(lv_obj_t *widget, uint8_t modifiers mods) {
    if (mods & (MOD_LSFT | MOD_RSFT)) {
        widget->state.active_mods = "SHIFT\n";
    } else {
        widget->state.active_mods = " \n";
    }
    if (mods & (MOD_LCTL | MOD_RCTL)) {
        strcat(widget->state.active_mods, "CTRL\n");
    } else {
        strcat(widget->state.active_mods, " \n");
    }
    if (mods & (MOD_LGUI | MOD_RGUI)) {
        strcat(widget->state.active_mods, "GUI\n");
    } else {
        strcat(widget->state.active_mods, " \n");
    }
    if (mods & (MOD_LALT | MOD_RALT)) {
        strcat(widget->state.active_mods, "ALT");
    } else {
        strcat(widget->state.active_mods, " ");
    }
    draw_canvas(widget->obj, widget->cbuf, &widget->state);
}

void modifiers_update_cb(uint8_t modifiers state) {
    struct zmk_widget_screen *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) { set_modifiers(widget->obj, state); }
}

static uint8_t modifiers modifiers_get_state(const zmk_event_t *eh) {
    return (uint8_t modifiers = zmk_hid_get_explicit_mods())
}

ZMK_DISPLAY_WIDGET_LISTENER(widget_modifiers, uint8_t modifiers, modifiers_update_cb, modifiers_get_state)
ZMK_SUBSCRIPTION(widget_modifiers, zmk_keycode_state_changed);


// initialise

int zmk_widget_screen_init(struct zmk_widget_screen *widget, lv_obj_t *parent) {
    widget->obj = lv_obj_create(parent);
    lv_obj_set_size(widget->obj, CANVAS_HEIGHT, CANVAS_WIDTH);

    lv_obj_t *canvas = lv_canvas_create(widget->obj);
    lv_obj_align(canvas, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_canvas_set_buffer(canvas, widget->cbuf, CANVAS_HEIGHT, CANVAS_HEIGHT, LV_IMG_CF_TRUE_COLOR);

    sys_slist_append(&widgets, &widget->node);
    widget_battery_init();
    widget_layer_init();
    widget_modifiers_init();

    return 0;
}

lv_obj_t *zmk_widget_screen_obj(struct zmk_widget_screen *widget) { return widget->obj; }


lv_obj_t *zmk_display_screen() {
    lv_obj_t *screen;
    screen = lv_obj_create(NULL);

    zmk_widget_screen_init(&zmk_widget_screen, screen);
    lv_obj_align(zmk_widget_screen_obj(&zmk_widget_screen), LV_ALIGN_TOP_LEFT, 0, 0);

    return screen;
}